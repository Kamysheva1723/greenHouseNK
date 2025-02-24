//
// Created by natal on 16.2.2025.
//
/*
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "FreeRTOS.h"
#include "task.h"

// -----------------------------------------------------------------------------
// TLS Client State
// -----------------------------------------------------------------------------
typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;
    bool complete;
    int error;
    const char *http_request;
    int timeout;
    std::string response;   // Accumulate the HTTP response here
} TLS_CLIENT_T;

static struct altcp_tls_config *tls_config = NULL;

// -----------------------------------------------------------------------------
// Close the TLS connection
// -----------------------------------------------------------------------------
static err_t tls_client_close(void *arg) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    err_t err = ERR_OK;

    state->complete = true;
    if (state->pcb != NULL) {
        altcp_arg(state->pcb, NULL);
        altcp_poll(state->pcb, NULL, 0);
        altcp_recv(state->pcb, NULL);
        altcp_err(state->pcb, NULL);
        err = altcp_close(state->pcb);
        if (err != ERR_OK) {
            printf("close failed %d, calling abort\n", err);
            altcp_abort(state->pcb);
            err = ERR_ABRT;
        }
        state->pcb = NULL;
    }
    return err;
}

// -----------------------------------------------------------------------------
// Called when connection is established
// -----------------------------------------------------------------------------
static err_t tls_client_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        return tls_client_close(state);
    }

    printf("Connected to server, sending request\n");
    err = altcp_write(state->pcb, state->http_request, strlen(state->http_request), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Error writing data, err=%d\n", err);
        return tls_client_close(state);
    }

    return ERR_OK;
}

// -----------------------------------------------------------------------------
// Poll callback (used for timeouts)
// -----------------------------------------------------------------------------
static err_t tls_client_poll(void *arg, struct altcp_pcb *pcb) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    printf("Timed out\n");
    state->error = PICO_ERROR_TIMEOUT;
    return tls_client_close(arg);
}

// -----------------------------------------------------------------------------
// Error callback
// -----------------------------------------------------------------------------
static void tls_client_err(void *arg, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    printf("tls_client_err: %d\n", err);
    tls_client_close(state);
    state->error = PICO_ERROR_GENERIC;
}

// -----------------------------------------------------------------------------
// Receive callback: accumulate received data into the response string
// -----------------------------------------------------------------------------
static err_t tls_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    if (!p) {
        printf("Connection closed\n");
        return tls_client_close(state);
    }

    if (p->tot_len > 0) {
        // Allocate a temporary buffer and copy the data.
        char *buf = (char *) malloc(p->tot_len + 1);
        if (!buf) {
            pbuf_free(p);
            return ERR_MEM;
        }
        pbuf_copy_partial(p, buf, p->tot_len, 0);
        buf[p->tot_len] = '\0';

        // Append the received data to our response string.
        state->response.append(buf, p->tot_len);
        printf("***\nNew data received (appended):\n***\n%s\n", buf);
        free(buf);

        altcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

// -----------------------------------------------------------------------------
// Connect to server IP (after DNS resolution)
// -----------------------------------------------------------------------------
static void tls_client_connect_to_server_ip(const ip_addr_t *ipaddr, TLS_CLIENT_T *state)
{
    err_t err;
    u16_t port = 443;
    printf("Connecting to server IP %s port %d\n", ipaddr_ntoa(ipaddr), port);
    err = altcp_connect(state->pcb, ipaddr, port, tls_client_connected);
    if (err != ERR_OK)
    {
        fprintf(stderr, "Error initiating connect, err=%d\n", err);
        tls_client_close(state);
    }
}

// -----------------------------------------------------------------------------
// DNS resolution callback
// -----------------------------------------------------------------------------
static void tls_client_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg)
{
    if (ipaddr)
    {
        printf("DNS resolving complete\n");
        tls_client_connect_to_server_ip(ipaddr, (TLS_CLIENT_T *) arg);
    }
    else
    {
        printf("Error resolving hostname %s\n", hostname);
        tls_client_close(arg);
    }
}

// -----------------------------------------------------------------------------
// Open a TLS connection to the given hostname
// -----------------------------------------------------------------------------
static bool tls_client_open(const char *hostname, void *arg) {
    err_t err;
    ip_addr_t server_ip;
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;

    state->pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    if (!state->pcb) {
        printf("Failed to create pcb\n");
        return false;
    }

    altcp_arg(state->pcb, state);
    altcp_poll(state->pcb, tls_client_poll, state->timeout * 2);
    altcp_recv(state->pcb, tls_client_recv);
    altcp_err(state->pcb, tls_client_err);

    /* Set SNI */
    mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(state->pcb), hostname);


    printf("Resolving %s\n", hostname);

    cyw43_arch_lwip_begin();
    err = dns_gethostbyname(hostname, &server_ip, tls_client_dns_found, state);
    if (err == ERR_OK)
    {
        /* Host is in DNS cache */
        tls_client_connect_to_server_ip(&server_ip, state);
    }
    else if (err != ERR_INPROGRESS)
    {
        printf("Error initiating DNS resolving, err=%d\n", err);
        tls_client_close(state->pcb);
    }
    cyw43_arch_lwip_end();

    return err == ERR_OK || err == ERR_INPROGRESS;
}

// -----------------------------------------------------------------------------
// Allocate and initialize TLS client state
// -----------------------------------------------------------------------------
static TLS_CLIENT_T* tls_client_init(void) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*) calloc(1, sizeof(TLS_CLIENT_T));
    if (!state) {
        printf("Failed to allocate state\n");
        return NULL;
    }
    return state;
}

// -----------------------------------------------------------------------------
// Debug callback (optional)
// -----------------------------------------------------------------------------
static void tlsdebug(void *ctx, int level, const char *file, int line, const char *message){
    fputs(message, stdout);
}

// -----------------------------------------------------------------------------
// Modified run_tls_client_test():
// Now returns the full HTTP response as a std::string.
// An empty string indicates an error occurred.
// -----------------------------------------------------------------------------
std::string run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout) {

    // Use a static TLS configuration so it is created only once.
    static struct altcp_tls_config *global_tls_config = NULL;
    if (global_tls_config == NULL) {
        global_tls_config = altcp_tls_create_config_client(cert, cert_len);
        if (global_tls_config == NULL) {
            printf("Failed to create global TLS config\n");
            return std::string();
        }
    }
    TLS_CLIENT_T *state = tls_client_init();
    if (!state) {
        return std::string();
    }
    state->http_request = request;
    state->timeout = timeout;

    if (!tls_client_open(server, state)) {
        free(state);
        return std::string();
    }

    // Wait until the connection is complete.
    while(!state->complete) {
#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        vTaskDelay(1000);
#endif
    }

    std::string ret = state->response;
    int err = state->error;
    free(state);
    //altcp_tls_free_config(tls_config);

    if (err == 0) {
        return ret;
    } else {
        return std::string();
    }
}
