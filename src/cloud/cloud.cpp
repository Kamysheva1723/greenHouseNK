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

#include "thingspeak_config.h"   // Defines THINGSPEAK_WRITE_API_KEY and THINGSPEAK_TALKBACK_API_KEY

// =============================================================================
//                           Cloud Module Implementation
// =============================================================================

/*
   The Cloud module establishes a secure TLS connection to a remote server (ThingSpeak)
   to transmit sensor data and receive remote commands. It uses the LWIP altcp_tls APIs
   integrated with the FreeRTOS scheduling system.
   
   This module retrieves sensor data via the Controller instance, constructs an HTTP POST
   request, and dispatches it securely. It then parses the HTTP response for any commands
   (e.g., a new CO₂ setpoint) to update the system.
*/

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
Cloud::Cloud(Controller* controller)
        : controller_(controller)   // Save pointer to the Controller for sensor data access
        , tls_config_(nullptr)        // TLS config will be created below
{
    // Initialize the TLS configuration for the client. Optionally, a CA certificate
    // can be used if available. Here, we pass a null pointer to use a minimal config.
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
        // Free the TLS configuration, which in turn frees the underlying mbedTLS structure.
        altcp_tls_free_config(tls_config_);
        tls_config_ = nullptr;
    }
}

// ----------------------------------------------------------------------------
// Public method: updateSensorData()
// ----------------------------------------------------------------------------- 
/*
    This function handles transmitting sensor data to ThingSpeak by:
    1. Retrieving current sensor values from the Controller.
    2. Constructing the HTTP POST body with sensor fields and geolocation data.
    3. Building a full HTTP POST request.
    4. Sending the request over a TLS-secured connection.
    5. Parsing the response from the server for any new setpoint command.
*/
bool Cloud::updateSensorData() {
    // If TLS configuration is not available, log an error and return failure.
    if (!tls_config_) {
        printf("[Cloud] No TLS config available. Cannot update.\n");
        return false;
    }

    // Retrieve sensor data from the controller.
    float field1 = controller_->getCurrentCO2();
    float field2 = controller_->getCurrentRH();
    float field3 = controller_->getCurrentTemp();
    float field4 = controller_->getCurrentFanSpeed();
    float field5 = controller_->getCO2Setpoint();

    // Build the POST body with sensor data and additional static parameters.
    // The body uses URL-encoded parameters and includes API keys, sensor fields,
    // latitude/longitude data, and a status message.
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

    // Define server information.
    const char* serverName = "api.thingspeak.com";
    int contentLength = (int)strlen(body);

    // Create the complete HTTP POST request including headers.
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

    // Perform the TLS request using our helper function.
    // The timeout parameter is set to 15 seconds.
    std::string response = performTLSRequest(serverName, requestBuffer, 15 /* timeout in seconds */);
    printf("ThingSpeak response:\n%s\n", response.c_str());

    if (!response.empty()) {
        printf("[Cloud] ThingSpeak update (and command execution) successful.\n");
        // Parse the HTTP response to check for any new setpoint commands.
        parseAndPrintSetpoint(response);
        return true;
    } else {
        printf("[Cloud] ThingSpeak update failed.\n");
        return false;
    }
}

// ----------------------------------------------------------------------------
// Helper method: parseAndPrintSetpoint()
// ----------------------------------------------------------------------------
/*
    This function scans the HTTP response for a token "SETPOINT=".
    If found, it extracts the numeric value, validates it, and updates
    the Controller's CO₂ setpoint accordingly.
*/
void Cloud::parseAndPrintSetpoint(const std::string& response) {
    const std::string token = "SETPOINT=";
    size_t pos = response.find(token);
    if (pos == std::string::npos) {
        // Token not found; no command to adjust the setpoint.
        printf("[Cloud] No SETPOINT command found.\n");
        return;
    }
    // Short delay, which may be necessary due to asynchronous FreeRTOS timing.
    vTaskDelay(100);
    size_t start = pos + token.size();
    size_t end   = response.find_first_of("\" \r\n", start);
    if (end == std::string::npos) {
        end = response.size();
    }

    // Extract the portion corresponding to the new setpoint value.
    std::string setpointValue = response.substr(start, end - start);
    printf("[Cloud] TalkBack command found: SETPOINT=%s\n", setpointValue.c_str());

    // Convert extracted string to a float.
    float newSetpoint = std::strtof(setpointValue.c_str(), nullptr);
    // Validate the setpoint value; must be in a safe range.
    if (newSetpoint <= 0.0f or newSetpoint > 1500.0f) {
        printf("[Cloud] Warning: invalid setpoint value received (%s), ignoring.\n", setpointValue.c_str());
        return;
    }

    // Update the controller's CO₂ setpoint with the new valid value.
    controller_->setCO2Setpoint(newSetpoint);
    printf("[Cloud] Controller setpoint updated to %.2f\n", newSetpoint);
}

// ----------------------------------------------------------------------------
// Private helper: performTLSRequest()
// ----------------------------------------------------------------------------
/*
    This function performs a complete TLS request cycle:
      1. Create a TLS client state.
      2. Initiate the connection and TLS handshake.
      3. Send the HTTP request.
      4. Wait until the connection is complete or an error occurs.
      5. Return the full HTTP response as a std::string.
*/
std::string Cloud::performTLSRequest(const char *server, const char *request, int timeout) {
    // Allocate memory for the TLS client state structure.
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)calloc(1, sizeof(TLS_CLIENT_T));
    if (!state) {
        printf("[Cloud] Failed to allocate TLS_CLIENT_T.\n");
        return std::string();
    }
    // Initialize state members.
    state->pcb          = nullptr;
    state->complete     = false;
    state->error        = 0;
    state->http_request = request;
    state->timeout      = timeout;
    state->cloudInstance = this; // Store pointer to Cloud instance for callbacks.

    // Initiate the TLS connection. This will handle DNS resolution and TLS handshake.
    if (!tlsClientOpen(server, state)) {
        free(state);
        return std::string();
    }

    // Wait until the TLS request is complete or an error occurs.
    while(!state->complete) {
#if PICO_CYW43_ARCH_POLL
        // If using the Pico's cyw43 architecture polling mode.
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        // Use FreeRTOS delay if polling is not enabled.
        vTaskDelay(pdMS_TO_TICKS(1000));
#endif
    }

    std::string ret = state->response; // Capture the response.
    int err = state->error;
    free(state);  // Free the allocated state.

    if (err == 0) {
        return ret;
    } else {
        return std::string();
    }
}

// =============================================================================
//                       TLS CLIENT CALLBACKS (STATIC)
// =============================================================================

/*
   The following static functions serve as callback handlers for TLS events
   on the LWIP stack. They manage connection establishment, data transmission,
   reception, errors, and connection closure.
*/

// ----------------------------------------------------------------------------
// tlsClientClose: Closes the TLS connection gracefully.
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// tlsClientConnected: Callback when TLS connection is successfully established.
// ----------------------------------------------------------------------------
err_t Cloud::tlsClientConnected(void *arg, struct altcp_pcb *pcb, err_t err) {
    auto *state = (TLS_CLIENT_T*)arg;
    if (!state) return ERR_OK;

    if (err != ERR_OK) {
        printf("[Cloud] connect failed %d\n", err);
        tlsClientClose(state);
        return ERR_OK;
    }

    printf("[Cloud] Connected to server, sending request.\n");
    // Write the HTTP request over the established TLS connection.
    err_t werr = altcp_write(state->pcb, state->http_request, (u16_t)strlen(state->http_request), TCP_WRITE_FLAG_COPY);
    if (werr != ERR_OK) {
        printf("[Cloud] Error writing data, err=%d\n", werr);
        tlsClientClose(state);
        return werr;
    }
    return ERR_OK;
}

// ----------------------------------------------------------------------------
// tlsClientPoll: Callback for polling the TLS connection (e.g., on timeout).
// ----------------------------------------------------------------------------
err_t Cloud::tlsClientPoll(void *arg, struct altcp_pcb *pcb) {
    auto *state = (TLS_CLIENT_T*)arg;
    if (!state) return ERR_OK;

    printf("[Cloud] TLS connection timed out.\n");
    state->error = PICO_ERROR_TIMEOUT;
    return tlsClientClose(state);
}

// ----------------------------------------------------------------------------
// tlsClientErr: Callback for reporting errors on the TLS connection.
// ----------------------------------------------------------------------------
void Cloud::tlsClientErr(void *arg, err_t err) {
    auto *state = (TLS_CLIENT_T*)arg;
    if (!state) return;

    printf("[Cloud] tlsClientErr: %d\n", err);
    state->error = PICO_ERROR_GENERIC;
    tlsClientClose(state);
}

// ----------------------------------------------------------------------------
// tlsClientRecv: Callback when data is received from the TLS connection.
// ----------------------------------------------------------------------------
/*
   This function is invoked by the LWIP stack when data is received. It appends the
   received data from the pbuf to the state's response string and acknowledges reception.
*/
err_t Cloud::tlsClientRecv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    auto *state = (TLS_CLIENT_T*)arg;
    if (!state) return ERR_OK;

    if (!p) {
        printf("[Cloud] Connection closed by remote.\n");
        return tlsClientClose(state);
    }

    if (p->tot_len > 0) {
        // Allocate a temporary buffer to copy the data.
        char *buf = (char*)malloc(p->tot_len + 1);
        if (buf) {
            pbuf_copy_partial(p, buf, p->tot_len, 0);
            buf[p->tot_len] = '\0';
            // Append received data to the response.
            state->response.append(buf, p->tot_len);
            free(buf);
        }
        // Inform LWIP that the data has been received.
        altcp_recved(pcb, p->tot_len);
    }
    // Free the received pbuf.
    pbuf_free(p);
    return ERR_OK;
}

// ----------------------------------------------------------------------------
// tlsClientConnectToServerIp: Initiates connection to the server using its IP address.
// ----------------------------------------------------------------------------
void Cloud::tlsClientConnectToServerIp(const ip_addr_t *ipaddr, TLS_CLIENT_T *state) {
    if (!state || !ipaddr) return;

    err_t err;
    u16_t port = 443;  // Default HTTPS port.
    printf("[Cloud] Connecting to %s port %d\n", ipaddr_ntoa(ipaddr), port);
    err = altcp_connect(state->pcb, ipaddr, port, tlsClientConnected);
    if (err != ERR_OK) {
        printf("[Cloud] Error in altcp_connect, err=%d\n", err);
        tlsClientClose(state);
    }
}

// ----------------------------------------------------------------------------
// tlsClientDnsFound: DNS callback invoked when hostname is resolved.
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// tlsClientOpen: Establishes a new TLS connection to the specified hostname.
// ----------------------------------------------------------------------------
/*
   This function creates a new TLS client Protocol Control Block (PCB), sets up required
   callbacks, and initiates DNS resolution and connection to the server.
*/
bool Cloud::tlsClientOpen(const char *hostname, TLS_CLIENT_T *state) {
    // Ensure we have valid state and a valid TLS configuration.
    if (!state || !state->cloudInstance || !state->cloudInstance->tls_config_) {
        printf("[Cloud] tlsClientOpen: invalid state or config\n");
        return false;
    }
    // Retrieve TLS config from the Cloud instance.
    struct altcp_tls_config* cfg = state->cloudInstance->tls_config_;
    // Create a new TLS PCB using the provided configuration.
    state->pcb = altcp_tls_new(cfg, IPADDR_TYPE_ANY);
    if (!state->pcb) {
        printf("[Cloud] Failed to create TLS PCB\n");
        return false;
    }

    // Set the argument for callbacks to use our TLS client state.
    altcp_arg(state->pcb, state);
    // Set polling callback with an interval based on the timeout.
    altcp_poll(state->pcb, tlsClientPoll, (u8_t)(state->timeout * 2));
    // Set receive callback to handle incoming data.
    altcp_recv(state->pcb, tlsClientRecv);
    // Set error callback to handle connection errors.
    altcp_err(state->pcb, tlsClientErr);

    // Set Server Name Indication (SNI) for secure TLS handshake.
    mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(state->pcb), hostname);

    // Begin DNS resolution. If result is cached, connect immediately.
    printf("[Cloud] Resolving hostname: %s\n", hostname);
    ip_addr_t server_ip;
    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(hostname, &server_ip, tlsClientDnsFound, state);
    if (err == ERR_OK) {
        // Host IP found in DNS cache.
        tlsClientConnectToServerIp(&server_ip, state);
    } else if (err != ERR_INPROGRESS) {
        printf("[Cloud] dns_gethostbyname failed, err=%d\n", err);
        tlsClientClose(state);
    }
    cyw43_arch_lwip_end();

    return (err == ERR_OK || err == ERR_INPROGRESS);
}