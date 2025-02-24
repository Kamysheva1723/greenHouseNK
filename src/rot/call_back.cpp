#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "GpioEvent.h"
#include "main.h"

extern QueueHandle_t xGpioQueue;

static uint32_t lastTurnEventTime = 0;
static uint32_t lastPressEventTime = 0;
const uint32_t debounceDelayMs = 200;  // 50ms debounce delay

static void gpio_isr_callback(uint gpio, uint32_t events)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    GpioEvent evt;
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    // Debounce for turn events
    if ((gpio == ROT_A_PIN) && (events & GPIO_IRQ_EDGE_RISE)) {
        if ((now_ms - lastTurnEventTime) < debounceDelayMs) return;
        lastTurnEventTime = now_ms;
        bool b_state = gpio_get(ROT_B_PIN);
        evt.type = EventType::TURN;
        evt.clockwise = (b_state == 0);
        evt.timestamp = now_ms;
        xQueueSendToBackFromISR(xGpioQueue, &evt, &xHigherPriorityTaskWoken);
    }

    // Debounce for button press events
    if ((gpio == ROT_SW_PIN) && (events & GPIO_IRQ_EDGE_FALL)) {
        if ((now_ms - lastPressEventTime) < debounceDelayMs) return;
        lastPressEventTime = now_ms;
        evt.type = EventType::PRESS;
        evt.clockwise = false;
        evt.timestamp = now_ms;
        xQueueSendToBackFromISR(xGpioQueue, &evt, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
