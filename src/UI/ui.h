#ifndef UI_H
#define UI_H

#include <memory>
#include <cstdint>

// Forward declarations
class ssd1306os;
class Controller;

/**
 * @brief UI class handles displaying sensor values and adjusting the CO₂ setpoint
 *        via a rotary encoder (turn + press).
 */
class UI {
public:
    UI(std::shared_ptr<ssd1306os> display,
       std::shared_ptr<Controller> controller);

    /**
     * @brief Called periodically (e.g. every 200ms) to refresh the display.
     */
    void updateUI();

    /**
     * @brief Called when the rotary encoder is turned.
     * @param delta +1 for clockwise, -1 for counterclockwise
     */
    void onEncoderTurn(int delta);

    /**
     * @brief Called when the rotary encoder button is pressed.
     */
    void onButtonPress();

    /**
     * @brief Optionally set a starting local setpoint from elsewhere
     */
    void setLocalSetpoint(float sp);


private:
    std::shared_ptr<ssd1306os> display_;
    std::shared_ptr<Controller> controller_;

    float localCO2Setpoint_;  // UI's working copy of the setpoint
    bool editingSetpoint_;    // Are we currently editing the setpoint?
    int  savedMessageTimer_;  // Countdown to show "Saved" message after user saves
};

#endif // UI_H

