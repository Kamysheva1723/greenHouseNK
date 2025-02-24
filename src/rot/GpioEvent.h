//
// Created by natal on 23.2.2025.
//

#ifndef GPIO_EVENT_H
#define GPIO_EVENT_H

#include <cstdint>

enum class EventType { PRESS, TURN };

class GpioEvent {
public:
    EventType type;       // PRESS or TURN
    bool      clockwise;  // Only meaningful if type == TURN
    uint32_t  timestamp;  // For debouncing/logging

    GpioEvent()
            : type(EventType::PRESS),
              clockwise(false),
              timestamp(0)
    {}

    GpioEvent(EventType t, bool cw, uint32_t ts)
            : type(t),
              clockwise(cw),
              timestamp(ts)
    {}
};

#endif // GPIO_EVENT_H

