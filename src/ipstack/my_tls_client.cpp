//
// Created by natal on 16.2.2025.
//
// This file implements a TLS client test for the Greenhouse Fertilization System's cloud connectivity.
// It demonstrates how to establish a secure TLS connection using the LWIP altcp_tls APIs on the RP2040 platform.
// The code performs the following steps:
// 1. Initializes a TLS client state structure.
// 2. Resolves the server hostname via DNS.
// 3. Establishes a TLS connection over TCP and sends an HTTP request.
// 4. Receives and accumulates the HTTP response into a string.
// 5. Returns the full HTTP response, which is used by the cloud module to update sensor data remotely.

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
// TLS Client State Structure
// -----------------------------------------------------------------------------

/*
   TLS_CLIENT_T holds the state for a single TLS client session.
   It includes:
     - A pointer to the Protocol Control Block (PCB) for the TLS connection.
     - A completion flag to indicate when the connection has finished.
     - An error code to capture any errors encountered.
     - A pointer to the HTTP request string to be sent.
     - A timeout (in seconds) for the connection.
     - A response string used to accumulate chunks of the HTTP response.
*/
typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;
    bool complete;
    int error;
    const char *http_request;
    int timeout;
    std::string response;   // Accumulate the HTTP response here
} TLS_CLIENT_T;

// -----------------------------------------------------------------------------
// Global TLS configuration pointer.
// This is reused for all TLS connections made by this client test.
static struct altcp_tls_config *tls_config = NULL;

// -----------------------------------------------------------------------------
// TLS Client Connection Closure Function
// -----------------------------------------------------------------------------

/*
   tls_client_close:
   Gracefully closes the TLS connection by clearing all associated callbacks,
   closing the PCB, and marking the connection as complete.
   Returns an error code if the close operation fails.
*/
static err_t tls_client_close(void *arg) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    err_t err = ERR_OK;

    state->complete = true;
    if (state->pcb != NULL) {
        // Clear callbacks
        altcp_arg(state->pcb, NULL);
        altcp_poll(state->pcb, NULL, 0);
        altcp_recv(state->pcb, NULL);
        altcp_err(state->pcb, NULL);
        // Attempt to close the connection
        err = altcp_close(state->pcb);
        if (err != ERR_OK) {
            printf("close failed %d, calling abort\n", err);
            // Abort connection if close fails
            altcp_abort(state->pcb);
            err = ERR_ABRT;
        }
        state->pcb = NULL;
    }
    return err;
}

// -----------------------------------------------------------------------------
// TLS Client Connected Callback Function
// -----------------------------------------------------------------------------

/*
   tls_client_connected:
   This callback is invoked when the TLS connection has been successfully established.
   It sends the prepared HTTP request over the TLS connection.
   If writing fails, it closes the connection.
*/
static err_t tls_client_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        return tls_client_close(state);
    }

    printf("Connected to server, sending request\n");
    // Write the HTTP request to the connection
    err = altcp_write(state->pcb, state->http_request, strlen(state->http_request), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Error writing data, err=%d\n", err);
        return tls_client_close(state);
    }

    return ERR_OK;
}

// -----------------------------------------------------------------------------
// TLS Client Poll Callback Function (for timeouts)
// -----------------------------------------------------------------------------

/*
   tls_client_poll:
   Called periodically by LWIP for connection monitoring (polling).
   If the connection times out, it sets a timeout error and closes the connection.
*/
static err_t tls_client_poll(void *arg, struct altcp_pcb *pcb) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    printf("Timed out\n");
    state->error = PICO_ERROR_TIMEOUT;
    return tls_client_close(arg);
}

// -----------------------------------------------------------------------------
// TLS Client Error Callback Function
// -----------------------------------------------------------------------------

/*
   tls_client_err:
   This callback is invoked when an error occurs on the TLS connection.
   It logs the error, closes the connection, and sets a generic error code.
*/
static void tls_client_err(void *arg, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    printf("tls_client_err: %d\n", err);
    tls_client_close(state);
    state->error = PICO_ERROR_GENERIC;
}

// -----------------------------------------------------------------------------
// TLS Client Receive Callback Function
// -----------------------------------------------------------------------------

/*
   tls_client_recv:
   Called when data is received over the TLS connection.
   It copies the received data into a temporary buffer, appends it to the response string,
   and acknowledges the reception so that LWIP can manage the flow.
*/
static err_t tls_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    if (!p) {
        printf("Connection closed\n");
        return tls_client_close(state);
    }

    if (p->tot_len > 0) {
        // Allocate temporary buffer to copy received data
        char *buf = (char *) malloc(p->tot_len + 1);
        if (!buf) {
            pbuf_free(p);
            return ERR_MEM;
        }
        pbuf_copy_partial(p, buf, p->tot_len, 0);
        buf[p->tot_len] = '\0';

        // Append the new data to the accumulated response string
        state->response.append(buf, p->tot_len);
        printf("***\nNew data received (appended):\n***\n%s\n", buf);
        free(buf);

        // Inform the LWIP stack that the data has been processed
        altcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

// -----------------------------------------------------------------------------
// Connect to Server IP Callback Function
// -----------------------------------------------------------------------------

/*
   tls_client_connect_to_server_ip:
   Once the DNS resolution returns an IP address, this function attempts to establish a TCP connection
   on port 443 (HTTPS). If the connection fails, the TLS client is closed.
*/
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
// DNS Resolution Callback Function
// -----------------------------------------------------------------------------

/*
   tls_client_dns_found:
   Callback that is invoked when the DNS resolution for the given hostname completes.
   If an IP address is found, it initiates a TCP connection to the server IP.
   Otherwise, it logs the error and closes the connection.
*/
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
// Open TLS Connection Function
// -----------------------------------------------------------------------------

/*
   tls_client_open:
   Creates a new TLS client Protocol Control Block (PCB) using the global TLS configuration,
   sets up all the necessary callbacks, and initiates DNS resolution and connection to the server.
   Returns true if connection initiation was successful.
*/
static bool tls_client_open(const char *hostname, void *arg) {
    err_t err;
    ip_addr_t server_ip;
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;

    // Create a new TLS PCB using the global TLS configuration.
    state->pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    if (!state->pcb) {
        printf("Failed to create pcb\n");
        return false;
    }

    // Set the state as the argument for callbacks.
    altcp_arg(state->pcb, state);
    // Set up polling callback to check for timeouts.
    altcp_poll(state->pcb, tls_client_poll, state->timeout * 2);
    // Set receive and error callbacks.
    altcp_recv(state->pcb, tls_client_recv);
    altcp_err(state->pcb, tls_client_err);

    /* Set the Server Name Indication (SNI) so the server returns the proper certificate. */
    mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(state->pcb), hostname);

    printf("Resolving %s\n", hostname);

    // Begin DNS resolution of the hostname.
    cyw43_arch_lwip_begin();
    err = dns_gethostbyname(hostname, &server_ip, tls_client_dns_found, state);
    if (err == ERR_OK)
    {
        // Host is already in DNS cache.
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
// Allocate and Initialize TLS Client State
// -----------------------------------------------------------------------------

/*
   tls_client_init:
   Allocates and initializes a TLS_CLIENT_T structure with all fields zeroed.
   This state structure will hold all the information for a TLS session.
*/
static TLS_CLIENT_T* tls_client_init(void) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*) calloc(1, sizeof(TLS_CLIENT_T));
    if (!state) {
        printf("Failed to allocate state\n");
        return NULL;
    }
    return state;
}

// -----------------------------------------------------------------------------
// Optional Debug Callback Function
// -----------------------------------------------------------------------------

/*
   tlsdebug:
   A debug callback function used during TLS handshake.
   It prints debug messages to stdout.
*/
static void tlsdebug(void *ctx, int level, const char *file, int line, const char *message){
    fputs(message, stdout);
}

// -----------------------------------------------------------------------------
// run_tls_client_test Function
// -----------------------------------------------------------------------------

/*
   run_tls_client_test:
   This function performs the entire test of a TLS client connection.
   It:
     - Uses or creates a global TLS configuration.
     - Initializes a TLS client state.
     - Opens a TLS connection to the specified server.
     - Waits for the connection to complete and for the HTTP response to be fully received.
     - Returns the HTTP response as a std::string.
     - Returns an empty string if an error occurred during the process.
     
   Parameters:
     - cert: Pointer to a buffer holding a CA certificate (optional, can be NULL).
     - cert_len: Length of the certificate in bytes.
     - server: The hostname of the server (e.g., "api.thingspeak.com").
     - request: The full HTTP request string to send.
     - timeout: Timeout in seconds for the TLS connection.
*/
std::string run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout) {

    // Use a static global TLS config to avoid re-creating it each time.
    static struct altcp_tls_config *global_tls_config = NULL;
    if (global_tls_config == NULL) {
        global_tls_config = altcp_tls_create_config_client(cert, cert_len);
        if (global_tls_config == NULL) {
            printf("Failed to create global TLS config\n");
            return std::string();
        }
    }
    // Set the global tls_config pointer for use in tls_client_open.
    tls_config = global_tls_config;

    // Allocate and initialize the TLS client state.
    TLS_CLIENT_T *state = tls_client_init();
    if (!state) {
        return std::string();
    }
    state->http_request = request;
    state->timeout = timeout;

    // Initiate the TLS connection to the server.
    if (!tls_client_open(server, state)) {
        free(state);
        return std::string();
    }

    // Wait for the TLS connection and HTTP response to complete.
    while(!state->complete) {
#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        vTaskDelay(1000);
#endif
    }

    // Capture the response and error code.
    std::string ret = state->response;
    int err = state->error;
    free(state);

    // If no error occurred, return the response; otherwise, return an empty string.
    if (err == 0) {
        return ret;
    } else {
        return std::string();
    }
}