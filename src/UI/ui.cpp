#include "UI.h"
#include <cstdio>
#include <utility>
#include "ssd1306os.h"
#include "Controller/Controller.h"
#include "pico/stdlib.h" // for sleep_ms()

UI::UI(std::shared_ptr<ssd1306os> display,
       std::shared_ptr<Controller> controller)
        : display_(std::move(display))
        , controller_(std::move(controller))
        , localCO2Setpoint_(1500.0f)  // default value; can be overridden
        , editingSetpoint_(false)
        , savedMessageTimer_(0)
{
    // If the Controller already has a setpoint, sync it here:
    if (controller_) {
        localCO2Setpoint_ = controller_->getCO2Setpoint();
    }
    // Simple splash screen
    display_->fill(0);
    display_->text("Greenhouse UI", 0, 0, 1);
    display_->text("Starting...", 0, 10, 1);
    display_->show();
    sleep_ms(1500);
    display_->fill(0);
    display_->show();
}

void UI::updateUI() {
    // When not editing, synchronize the setpoint from the controller in case it was changed remotely.
    if (!editingSetpoint_ && controller_) {
        localCO2Setpoint_ = controller_->getCO2Setpoint();
    }

    display_->fill(0);
    char buffer[32];

    if (editingSetpoint_) {
        // *** Edit Mode: Dedicated page for setting the new setpoint ***
        snprintf(buffer, sizeof(buffer), "Edit Setpoint:");
        display_->text(buffer, 0, 0, 2);
        snprintf(buffer, sizeof(buffer), "%.1f ppm", localCO2Setpoint_);
        display_->text(buffer, 0, 20, 2);
        display_->text("Turn to adjust", 0, 45, 1);
        display_->text("Press to save", 0, 55, 1);
    } else {
        // *** Normal Mode: Display sensor values and current controller status ***
        float currentCO2  = controller_ ? controller_->getCurrentCO2()  : 0.0f;
        float currentTemp = controller_ ? controller_->getCurrentTemp()   : 0.0f;
        float currentRH   = controller_ ? controller_->getCurrentRH()     : 0.0f;
        float currentFan  = controller_ ? controller_->getCurrentFanSpeed() : 0.0f;
        bool valveOpen    = controller_ ? controller_->isValveOpen()        : false;

        // Layout the display. Adjust the Y positions as needed for your 128x64 display.
        snprintf(buffer, sizeof(buffer), "CO2: %.1f ppm", currentCO2);
        display_->text(buffer, 0, 0, 2);
        snprintf(buffer, sizeof(buffer), "T:%.1fC  RH:%.1f%%", currentTemp, currentRH);
        display_->text(buffer, 0, 16, 1);
        snprintf(buffer, sizeof(buffer), "Fan: %.0f%%", currentFan);
        display_->text(buffer, 0, 26, 1);
        snprintf(buffer, sizeof(buffer), "Valve: %s", valveOpen ? "OPEN" : "CLOSED");
        display_->text(buffer, 0, 36, 1);
        snprintf(buffer, sizeof(buffer), "Set: %.1f ppm", localCO2Setpoint_);
        display_->text(buffer, 0, 46, 2);

        // Show a temporary "Saved!" message if recently saved.
        if (savedMessageTimer_ > 0) {
            display_->text("Saved!", 90, 58, 1);
            savedMessageTimer_--;
        }
    }
    display_->show();
}



void UI::onEncoderTurn(int delta) {
    // Only update the setpoint if we're in edit mode.
    if (editingSetpoint_) {
        const float stepSize = 10.0f;  // Adjust step size as needed
        localCO2Setpoint_ += delta * stepSize;
        if (localCO2Setpoint_ < 0.0f) localCO2Setpoint_ = 0.0f;
        if (localCO2Setpoint_ > 1500.0f) localCO2Setpoint_ = 1500.0f;
    }
}

#include "pico/stdlib.h" // for to_ms_since_boot()

void UI::onButtonPress() {
    static uint32_t lastPressTime = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    printf("Button pressed at %u ms, delta = %u\n", now, now - lastPressTime);
    if (now - lastPressTime < 150) {  // Adjust debounce delay if needed
        printf("Ignoring press due to debounce\n");
        return;
    }
    lastPressTime = now;

    // Toggle between edit and normal modes
    editingSetpoint_ = !editingSetpoint_;
    printf("Editing mode now: %s\n", editingSetpoint_ ? "ON" : "OFF");
    if (!editingSetpoint_) {
        // When exiting edit mode, update the controller and persist the new setpoint.
        if (controller_) {
            controller_->setCO2Setpoint(localCO2Setpoint_);
            printf("Setpoint updated in controller to: %.1f\n", localCO2Setpoint_);
        }
        savedMessageTimer_ = 20;  // Display a "Saved!" message briefly.
    }
}



void UI::setLocalSetpoint(float sp) {
    if (sp < 0.0f) sp = 0.0f;
    if (sp > 3000.0f) sp = 3000.0f;
    localCO2Setpoint_ = sp;
}
