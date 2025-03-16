# Greenhouse Fertilization System

**Version:** 1.0.0  
**Last Updated:** February 23, 2025  


---

## Overview

The Greenhouse Fertilization System is a modular embedded project designed to automate and monitor greenhouse operations. This system includes sensors (CO₂, pressure, temperature/humidity), actuator drivers (valve, fan), and various communication interfaces (I2C, Modbus, wireless networking) to efficiently manage fertilization and environmental control. The project implements both hardware-level interfaces and higher-level application logic, making it a comprehensive solution for greenhouse management.

---

## Features

- **Sensor Integration:**  
  - **CO₂ Sensor:** Measures CO₂ concentration in parts per million (ppm).  
  - **Pressure Sensor:** Monitors environmental pressure levels.
  - **Temperature & Humidity Sensor:** Provides ambient environmental readings.
  
- **Actuator Control:**  
  - **Valve Driver:** Controls water/chemical delivery valves.
  - **Fan Driver:** Manages ventilation systems.
  
- **Non-volatile Storage:**  
  - EEPROM support for storing critical parameters.
  
- **Communication Protocols:**  
  - **I2C:** For communication with peripherals like EEPROM and displays.
  - **Modbus:** For robust industrial sensor interfacing.
  - **Wireless (Wi-Fi):** For remote monitoring and control.
  
- **Real-Time Operation:**  
  - Uses FreeRTOS for task scheduling and timing functions.

---

## Usage
Sensor output can be seen on the display in a human readable format.

The system tries to keep the carbon dioxide levels at a certain level, this target concentration can be modified.
To set the target CO2 level, press the rotary encoder down. Then rotate it until the display shows the desired level.
To save this new target, press the rotary encoder down again.

---

## Hardware Requirements

- **Microcontroller:** A device compatible with FreeRTOS (e.g., an ARM Cortex-M series microcontroller).
- **Sensors:** CO₂ sensor, pressure sensor, temperature/humidity sensor.
- **Actuators:** Fan and valve hardware.
- **Communication Interfaces:** I2C, Modbus, and Wi-Fi module.
- **EEPROM:** External EEPROM module for non-volatile storage.

---

## Software Requirements

- **Compiler:** A C/C++ compiler that supports C++11 (or later).
- **RTOS:** FreeRTOS for real-time task management.
- **Libraries:**
  - Standard C++ libraries
  - [PicoI2C](https://github.com/raspberrypi/pico-sdk) (if using Raspberry Pi Pico)
  - Custom or third-party libraries for Modbus and sensor interfacing.
- **Git:** For version control and collaboration.

---

## Build and Deployment

### Building the Project

1. **Clone the Repository:**

   ```sh
   git clone https://github.com/Kamysheva1723/greenHouseNK.git
   cd greenHouseNK


