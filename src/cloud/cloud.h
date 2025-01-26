//
// Created by natal on 26.1.2025.
//

#ifndef GREENHOUSE_CLOUD_H
#define GREENHOUSE_CLOUD_H

#include <string>
#include <memory>

class Controller;

/**
 * @brief Handles HTTP(S) requests to ThingSpeak using Wi-Fi + TLS.
 */
class CloudClient {
public:
    CloudClient(const std::string &apiKey,
                const std::string &talkbackKey,
                const std::string &channelID);
    ~CloudClient() = default;

    void setController(std::shared_ptr<Controller> controller);

    /**
     * @brief Periodically send data to ThingSpeak fields:
     *  field1=CO2, field2=RH, field3=T, field4=FanSpeed, field5=CO2Setpoint
     */
    void uploadData();

    /**
     * @brief (Optional) check for commands from ThingSpeak talkback.
     *  E.g. new set-point from the cloud.
     */
    void checkCommands();

private:
    std::string apiKey_;
    std::string talkbackKey_;
    std::string channelID_;
    std::shared_ptr<Controller> controller_;

    // Possibly store Wi-Fi credentials or pass them to a separate wifi manager
    // In a real system, we'd have methods to handle the actual TLS/HTTP library
};

#endif //GREENHOUSE_CLOUD_H
