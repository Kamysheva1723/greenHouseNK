#include "cloud.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>      // for std::strtof

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "FreeRTOS.h"
#include "task.h"

#include "thingspeak_config.h"   // Contains THINGSPEAK_WRITE_API_KEY, THINGSPEAK_TALKBACK_API_KEY

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
Cloud::Cloud(Controller* controller)
        : controller_(controller)
        , tls_config_(nullptr)
{
    // Initialize the TLS config once and store it in tls_config_
    //    extern const uint8_t ca_cert[];
    //    extern const size_t ca_cert_len;
    //    tls_config_ = altcp_tls_create_config_client(ca_cert, ca_cert_len);

    tls_config_ = altcp_tls_create_config_client(nullptr, 0);
    if (!tls_config_) {
        printf("Failed to create altcp_tls_config in Cloud constructor!\n");
    }
}

// ----------------------------------------------------------------------------
// Destructor
// ----------------------------------------------------------------------------
Cloud::~Cloud() {
    if (tls_config_) {
        // Clean up the TLS config. This will free the underlying mbedTLS structure
        altcp_tls_free_config(tls_config_);
        tls_config_ = nullptr;
    }
}

// ----------------------------------------------------------------------------
// Public function: performs the combined data update + TalkBack command
// ----------------------------------------------------------------------------
bool Cloud::updateSensorData() {
    if (!tls_config_) {
        printf("[Cloud] No TLS config available. Cannot update.\n");
        return false;
    }

    // Retrieve sensor data
    float field1 = controller_->getCurrentCO2();
    float field2 = controller_->getCurrentRH();
    float field3 = controller_->getCurrentTemp();
    float field4 = controller_->getCurrentFanSpeed();
    float field5 = controller_->getCO2Setpoint();

    // Build POST body
    char body[512];
    snprintf(body, sizeof(body),
             "api_key=%s"
             "&talkback_key=%s"
             "&field1=%.2f"
             "&field2=%.2f"
             "&field3=%.2f"
             "&field4=%.2f"
             "&field5=%.2f"
             "&lat=60.1699"
             "&long=24.9384"
             "&status=Update%%20from%%20Helsinki",
             THINGSPEAK_WRITE_API_KEY,
             THINGSPEAK_TALKBACK_API_KEY,
             field1, field2, field3, field4, field5);

    const char* serverName = "api.thingspeak.com";
    int contentLength = (int)strlen(body);

    // Create HTTP POST request
    char requestBuffer[1024];
    snprintf(requestBuffer, sizeof(requestBuffer),
             "POST /update.json HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Connection: close\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%s",
             serverName, contentLength, body);

    // Perform TLS request
    std::string response = performTLSRequest(serverName, requestBuffer, 15 /* timeout in seconds */);
    printf("ThingSpeak response:\n%s\n", response.c_str());

    if (!response.empty()) {
        printf("[Cloud] ThingSpeak update (and command execution) successful.\n");
        // Look for a new setpoint command in the response
        parseAndPrintSetpoint(response);
        return true;
    } else {
        printf("[Cloud] ThingSpeak update failed.\n");
        return false;
    }
}

// ----------------------------------------------------------------------------
// Helper to parse "SETPOINT=" from the ThingSpeak JSON response
// ----------------------------------------------------------------------------
void Cloud::parseAndPrintSetpoint(const std::string& response) {
    const std::string token = "SETPOINT=";
    size_t pos = response.find(token);
    if (pos == std::string::npos) {
        printf("[Cloud] No SETPOINT command found.\n");
        return;
    }

    size_t start = pos + token.size();
    size_t end   = response.find_first_of("\" \r\n", start);
    if (end == std::string::npos) {
        end = response.size();
    }

    // Extract the numeric portion
    std::string setpointValue = response.substr(start, end - start);
    printf("[Cloud] TalkBack command found: SETPOINT=%s\n", setpointValue.c_str());

    // Convert to float
    float newSetpoint = std::strtof(setpointValue.c_str(), nullptr);
    if (newSetpoint <= 0.0f or newSetpoint >1500.0f) {
        printf("[Cloud] Warning: invalid setpoint value received (%s), ignoring.\n", setpointValue.c_str());
        return;
    }

    // Update the controller's CO2 setpoint
    controller_->setCO2Setpoint(newSetpoint);
    printf("[Cloud] Controller setpoint updated to %.2f\n", newSetpoint);
}

// ----------------------------------------------------------------------------
// Private method: do a single TLS request, return entire HTTP response
// ----------------------------------------------------------------------------
std::string Cloud::performTLSRequest(const char *server, const char *request, int timeout) {
    // Create the TLS client state
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)calloc(1, sizeof(TLS_CLIENT_T));
    if (!state) {
        printf("[Cloud] Failed to allocate TLS_CLIENT_T.\n");
        return std::string();
    }
    state->pcb          = nullptr;
    state->complete     = false;
    state->error        = 0;
    state->http_request = request;
    state->timeout      = timeout;
    state->cloudInstance = this; // If needed by callbacks

    // Actually connect (DNS + TLS handshake + send request)
    if (!tlsClientOpen(server, state)) {
        free(state);
        return std::string();
    }

    // Wait until done or error
    while(!state->complete) {
#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        vTaskDelay(pdMS_TO_TICKS(1000));
#endif
    }

    std::string ret = state->response;
    int err = state->error;
    free(state);

    if (err == 0) {
        return ret;
    } else {
        return std::string();
    }
}

// =============================================================================
//                       TLS CLIENT CALLBACKS (STATIC)
// =============================================================================

/* Close the connection */
err_t Cloud::tlsClientClose(TLS_CLIENT_T *state) {
    if (!state) return ERR_OK;

    state->complete = true;
    err_t err = ERR_OK;

    if (state->pcb != NULL) {
        altcp_arg(state->pcb, NULL);
        altcp_poll(state->pcb, NULL, 0);
        altcp_recv(state->pcb, NULL);
        altcp_err(state->pcb, NULL);
        err = altcp_close(state->pcb);
        if (err != ERR_OK) {
            printf("[Cloud] altcp_close failed %d, calling abort.\n", err);
            altcp_abort(state->pcb);
            err = ERR_ABRT;
        }
        state->pcb = NULL;
    }
    return err;
}

/* When connection is established, send the HTTP request */
err_t Cloud::tlsClientConnected(void *arg, struct altcp_pcb *pcb, err_t err) {
    auto *state = (TLS_CLIENT_T*)arg;
    if (!state) return ERR_OK;

    if (err != ERR_OK) {
        printf("[Cloud] connect failed %d\n", err);
        tlsClientClose(state);
        return ERR_OK;
    }

    printf("[Cloud] Connected to server, sending request.\n");
    err_t werr = altcp_write(state->pcb, state->http_request, (u16_t)strlen(state->http_request), TCP_WRITE_FLAG_COPY);
    if (werr != ERR_OK) {
        printf("[Cloud] Error writing data, err=%d\n", werr);
        tlsClientClose(state);
        return werr;
    }
    return ERR_OK;
}

/* Poll callback (timeout) */
err_t Cloud::tlsClientPoll(void *arg, struct altcp_pcb *pcb) {
    auto *state = (TLS_CLIENT_T*)arg;
    if (!state) return ERR_OK;

    printf("[Cloud] TLS connection timed out.\n");
    state->error = PICO_ERROR_TIMEOUT;
    return tlsClientClose(state);
}

/* LWIP error callback */
void Cloud::tlsClientErr(void *arg, err_t err) {
    auto *state = (TLS_CLIENT_T*)arg;
    if (!state) return;

    printf("[Cloud] tlsClientErr: %d\n", err);
    state->error = PICO_ERROR_GENERIC;
    tlsClientClose(state);
}

/* Receive callback: accumulate into state->response */
err_t Cloud::tlsClientRecv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    auto *state = (TLS_CLIENT_T*)arg;
    if (!state) return ERR_OK;

    if (!p) {
        printf("[Cloud] Connection closed by remote.\n");
        return tlsClientClose(state);
    }

    if (p->tot_len > 0) {
        // Copy out the data
        char *buf = (char*)malloc(p->tot_len + 1);
        if (buf) {
            pbuf_copy_partial(p, buf, p->tot_len, 0);
            buf[p->tot_len] = '\0';
            // Append to response
            state->response.append(buf, p->tot_len);
            free(buf);
        }
        altcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

/* Connect to server IP (after DNS) */
void Cloud::tlsClientConnectToServerIp(const ip_addr_t *ipaddr, TLS_CLIENT_T *state) {
    if (!state || !ipaddr) return;

    err_t err;
    u16_t port = 443;
    printf("[Cloud] Connecting to %s port %d\n", ipaddr_ntoa(ipaddr), port);
    err = altcp_connect(state->pcb, ipaddr, port, tlsClientConnected);
    if (err != ERR_OK) {
        printf("[Cloud] Error in altcp_connect, err=%d\n", err);
        tlsClientClose(state);
    }
}

/* DNS resolution callback */
void Cloud::tlsClientDnsFound(const char* hostname, const ip_addr_t *ipaddr, void *arg) {
    auto *state = (TLS_CLIENT_T*)arg;
    if (!state) return;

    if (ipaddr) {
        printf("[Cloud] DNS resolved for %s\n", hostname);
        tlsClientConnectToServerIp(ipaddr, state);
    } else {
        printf("[Cloud] Error resolving hostname %s\n", hostname);
        tlsClientClose(state);
    }
}

/* Initiate a new TLS connection using our stored altcp_tls_config */
bool Cloud::tlsClientOpen(const char *hostname, TLS_CLIENT_T *state) {
    // Must have a valid state, a valid Cloud pointer, and a valid tls_config_
    if (!state || !state->cloudInstance || !state->cloudInstance->tls_config_) {
        printf("[Cloud] tlsClientOpen: invalid state or config\n");
        return false;
    }
    // Get the config from the instance
    struct altcp_tls_config* cfg = state->cloudInstance->tls_config_;
    // Create the PCB with our config
    state->pcb = altcp_tls_new(cfg, IPADDR_TYPE_ANY);
    if (!state->pcb) {
        printf("[Cloud] Failed to create TLS PCB\n");
        return false;
    }

    // Set up the callbacks
    altcp_arg(state->pcb, state);
    altcp_poll(state->pcb, tlsClientPoll, (u8_t)(state->timeout * 2));
    altcp_recv(state->pcb, tlsClientRecv);
    altcp_err(state->pcb, tlsClientErr);

    // Set SNI
    mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(state->pcb), hostname);

    // Start DNS resolution or connect if host in cache
    printf("[Cloud] Resolving hostname: %s\n", hostname);

    ip_addr_t server_ip;
    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(hostname, &server_ip, tlsClientDnsFound, state);
    if (err == ERR_OK) {
        // Host in DNS cache
        tlsClientConnectToServerIp(&server_ip, state);
    } else if (err != ERR_INPROGRESS) {
        printf("[Cloud] dns_gethostbyname failed, err=%d\n", err);
        tlsClientClose(state);
    }
    cyw43_arch_lwip_end();

    return (err == ERR_OK || err == ERR_INPROGRESS);
}

