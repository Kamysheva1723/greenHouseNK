//
// Created by natal on 26.1.2025.
//

#ifndef GREENHOUSE_UI_H
#define GREENHOUSE_UI_H

#include <cstdint>
#include <memory>
#include <string>

class ssd1306os;   // from your driver
class EEPROMStorage;
class Controller;

/**
 * @brief Manages local user interface:
 *  - Displays sensor readings & set-point
 *  - Handles rotary input to change set-point
 *  - Saves settings to EEPROM
 */
class UI {
public:
    UI(std::shared_ptr<ssd1306os> display,
       std::shared_ptr<EEPROMStorage> eeprom,
       std::shared_ptr<Controller> controller);

    /**
     * @brief Periodically update display & check for user input.
     */
    void updateUI();

    // Additional UI states or menu logic
    void showMainScreen();
    void showMenuScreen();

    // Called by an ISR or polled to handle encoder changes
    void onEncoderTurn(int delta);
    void onButtonPress();

private:
    std::shared_ptr<ssd1306os>    display_;
    std::shared_ptr<EEPROMStorage> eeprom_;
    std::shared_ptr<Controller>    controller_;

    // Track UI state
    float localCO2Setpoint_;
    bool  editingSetpoint_;
};

#endif //GREENHOUSE_UI_H
