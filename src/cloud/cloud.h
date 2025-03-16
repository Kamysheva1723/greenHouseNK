#ifndef CLOUD_H
#define CLOUD_H

#include <string>
#include <memory>
#include "pico/stdlib.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "Controller/Controller.h"

/*
   Cloud Module Header

   This module handles secure TLS communications with a remote server (e.g., ThingSpeak)
   for both uploading sensor data and receiving remote commands. It leverages LWIP’s
   altcp_tls APIs to perform a TLS handshake over a TCP connection and integrates with
   FreeRTOS for real-time operation.

   Key responsibilities include:
     - Retrieving sensor data from the Controller instance.
     - Building an HTTP POST request with sensor and location data.
     - Transmitting the request over a TLS-secured channel.
     - Receiving and parsing the HTTP response for any remote commands (e.g., new CO₂ setpoint).
*/
class Cloud {
public:
    // ------------------------------------------------------------------------
    // Constructor / Destructor
    // ------------------------------------------------------------------------
    // The constructor receives a pointer to the Controller for accessing sensor data.
    explicit Cloud(Controller* controller);

    // Destructor frees TLS configuration resources.
    ~Cloud();

    // ------------------------------------------------------------------------
    // Public method to update sensor data (and process TalkBack commands)
    // ------------------------------------------------------------------------
    // This method:
    //   - Retrieves current sensor values from the controller.
    //   - Builds an HTTP POST request (including sensor data, API keys, and location).
    //   - Sends the data to the ThingSpeak server over a TLS connection.
    //   - Parses the response to check for a new setpoint command.
    // Returns true if successful, otherwise false.
    bool updateSensorData();

private:
    Controller* controller_; // Pointer to central Controller for sensor data and setpoint updates.

    // Global TLS configuration used for all TLS connections created by this class.
    struct altcp_tls_config* tls_config_;

    // ------------------------------------------------------------------------
    // Helper to perform a complete TLS request
    // ------------------------------------------------------------------------
    // This method handles:
    //   - Initiating the TLS connection (including DNS resolution and handshake).
    //   - Sending the provided HTTP request.
    //   - Waiting for a complete response.
    // Returns the HTTP response as a std::string.
    std::string performTLSRequest(const char *server, const char *request, int timeout);

    // ------------------------------------------------------------------------
    // Internals for TLS connection state and callback functions
    // ------------------------------------------------------------------------
    // TLS_CLIENT_T holds the state of an ongoing TLS request.
    struct TLS_CLIENT_T {
        struct altcp_pcb *pcb;       // Protocol Control Block for the TLS connection.
        bool complete;               // Flag indicating completion of the request.
        int error;                   // Error code (if any) from the TLS operations.
        const char *http_request;    // Pointer to the HTTP request string to be sent.
        int timeout;                 // Timeout for the request in seconds.
        std::string response;        // Buffer to accumulate the HTTP response.
        Cloud *cloudInstance;        // Back-reference to the Cloud instance, if needed in callbacks.
    };

    // ------------------------------------------------------------------------
    // Static TLS callbacks used by LWIP's altcp_tls APIs.
    // These functions manage connection closure, data sending, receiving, polling, and error handling.
    // ------------------------------------------------------------------------
    // Closes the TLS connection gracefully.
    static err_t tlsClientClose(TLS_CLIENT_T *state);
    
    // Called when a connection is successfully established; sends the HTTP request.
    static err_t tlsClientConnected(void *arg, struct altcp_pcb *pcb, err_t err);
    
    // Poll callback triggered periodically for timeouts.
    static err_t tlsClientPoll(void *arg, struct altcp_pcb *pcb);
    
    // Error callback invoked by LWIP on a connection error.
    static void tlsClientErr(void *arg, err_t err);
    
    // Receive callback that accumulates incoming data and appends it to the response.
    static err_t tlsClientRecv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err);
    
    // Attempts to connect to the server using its resolved IP address.
    static void tlsClientConnectToServerIp(const ip_addr_t *ipaddr, TLS_CLIENT_T *state);
    
    // DNS resolution callback; called when hostname resolution is complete.
    static void tlsClientDnsFound(const char* hostname, const ip_addr_t *ipaddr, void *arg);
    
    // Initiates a new TLS connection; creates the PCB and sets up callbacks.
    static bool tlsClientOpen(const char *hostname, TLS_CLIENT_T *state);

    // ------------------------------------------------------------------------
    // Helper to parse remote command from the HTTP response.
    // ------------------------------------------------------------------------
    // This function extracts the "SETPOINT=" command from the response (if present)
    // and updates the controller's CO₂ setpoint accordingly.
    void parseAndPrintSetpoint(const std::string& response);
};

#endif // CLOUD_H
