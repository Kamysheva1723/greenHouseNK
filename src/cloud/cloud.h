#ifndef CLOUD_H
#define CLOUD_H

#include <string>
#include <memory>
#include "pico/stdlib.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "Controller/Controller.h"

class Cloud {
public:
    // Constructor / Destructor
    explicit Cloud(Controller* controller);
    ~Cloud();

    // Public method to update sensor data (and TalkBack) on ThingSpeak
    bool updateSensorData();

private:
    Controller* controller_;

    // The global TLS configuration held for the lifetime of this class
    struct altcp_tls_config* tls_config_;

    // Helper to do the actual TLS request (formerly run_tls_client_test)
    std::string performTLSRequest(const char *server, const char *request, int timeout);

    // --- Internals for the TLS connection: a state struct and static callbacks
    struct TLS_CLIENT_T {
        struct altcp_pcb *pcb;
        bool complete;
        int error;
        const char *http_request;
        int timeout;
        std::string response;
        Cloud *cloudInstance; // pointer back to Cloud if needed
    };

    static err_t     tlsClientClose(TLS_CLIENT_T *state);
    static err_t     tlsClientConnected(void *arg, struct altcp_pcb *pcb, err_t err);
    static err_t     tlsClientPoll(void *arg, struct altcp_pcb *pcb);
    static void      tlsClientErr(void *arg, err_t err);
    static err_t     tlsClientRecv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err);
    static void      tlsClientConnectToServerIp(const ip_addr_t *ipaddr, TLS_CLIENT_T *state);
    static void      tlsClientDnsFound(const char* hostname, const ip_addr_t *ipaddr, void *arg);
    static bool      tlsClientOpen(const char *hostname, TLS_CLIENT_T *state);

    // Helper to parse the "SETPOINT=" command in the ThingSpeak JSON reply
    void parseAndPrintSetpoint(const std::string& response);
};

#endif // CLOUD_H
