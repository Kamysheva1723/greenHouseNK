#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "GpioEvent.h"      // Defines the GpioEvent structure and EventType enumeration for rotary encoder events
#include "main.h"           // Contains key definitions including pin assignments for the rotary encoder

// Global FreeRTOS queue used to forward GPIO events (from ISR) to the appropriate task for processing.
extern QueueHandle_t xGpioQueue;

// Static variables for debouncing rotary encoder events.
// These store the last timestamp (in milliseconds) when a turn or button press event was processed.
static uint32_t lastTurnEventTime = 0;
static uint32_t lastPressEventTime = 0;

// Definition of debounce delay in milliseconds. 
// Only events that occur at least this many milliseconds after a previous event will be accepted.
const uint32_t debounceDelayMs = 200;  // 200ms debounce delay

/*
   gpio_isr_callback:

   This function is the interrupt service routine (ISR) for handling events on the rotary encoder GPIO pins.
   It processes two types of events:
     1. Rotary turn events (detected on ROT_A_PIN rising edge) to determine the rotation direction by reading ROT_B_PIN.
     2. Button press events (detected on ROT_SW_PIN falling edge) for toggling UI modes.
   The ISR applies software debouncing by checking the elapsed time since the last event,
   and then sends a GpioEvent to the global FreeRTOS queue xGpioQueue for further processing by a dedicated task.
*/
static void gpio_isr_callback(uint gpio, uint32_t events)
{
    // Variable to indicate if a higher-priority task was woken by sending to the queue.
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Create an event structure to store details about the detected event.
    GpioEvent evt;
    // Get the current time in milliseconds since boot using Pico SDK's time functions.
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    // Process rotary turn events on the primary rotary signal (ROT_A_PIN).
    if ((gpio == ROT_A_PIN) && (events & GPIO_IRQ_EDGE_RISE)) {
        // Enforce debounce: ignore event if the time since the last turn event is less than debounceDelayMs.
        if ((now_ms - lastTurnEventTime) < debounceDelayMs) return;

        // Update the last event time.
        lastTurnEventTime = now_ms;

        // Determine rotation direction by reading the state of ROT_B_PIN:
        // If ROT_B_PIN is LOW, assume rotation is clockwise.
        bool b_state = gpio_get(ROT_B_PIN);
        evt.type = EventType::TURN;             // Set event type to TURN
        evt.clockwise = (b_state == 0);           // Set direction based on ROT_B_PIN state
        evt.timestamp = now_ms;                   // Record event timestamp

        // Send the event to the global event queue from ISR context.
        xQueueSendToBackFromISR(xGpioQueue, &evt, &xHigherPriorityTaskWoken);
    }

    // Process button press events on the rotary encoder push-button (ROT_SW_PIN).
    if ((gpio == ROT_SW_PIN) && (events & GPIO_IRQ_EDGE_FALL)) {
        // Enforce debounce: ignore event if the time since the last button press is less than debounceDelayMs.
        if ((now_ms - lastPressEventTime) < debounceDelayMs) return;

        // Update the last press event time.
        lastPressEventTime = now_ms;

        evt.type = EventType::PRESS;            // Set event type to PRESS
        evt.clockwise = false;                  // Direction is not applicable for button press; set to false
        evt.timestamp = now_ms;                 // Record event timestamp

        // Send the button press event into the global event queue.
        xQueueSendToBackFromISR(xGpioQueue, &evt, &xHigherPriorityTaskWoken);
    }

    // If sending the event unblocked a task that has higher priority than the currently running task,
    // yield from the ISR to allow that task to execute immediately.
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}