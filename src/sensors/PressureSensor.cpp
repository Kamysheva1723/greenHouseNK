#include "PressureSensor.h"               // Include header file for PressureSensor class definition
#include "PicoI2C.h"                      // Include PicoI2C header to access I²C functions specific to the Raspberry Pi Pico platform
#include <cstdio>                         // Provides printf for debugging output
#include <cmath>                          // Provides math functions if any advanced math is needed

/*
 * Constructor for PressureSensor class.
 *
 * Parameters:
 *   my_i2c     - Shared pointer to a PicoI2C object that manages I²C communications.
 *   i2cAddress - I²C address of the pressure sensor.
 *   altitude_m - Altitude of the sensor in meters (could be used for altitude compensation if implemented).
 *   useCrc    - Boolean flag indicating whether to enable CRC checking for I²C data.
 *
 * The initializer list:
 *   - Initializes the PicoI2C communication object.
 *   - Sets the sensor's I²C address.
 *   - Initializes the pressure value to 0.0 Pa.
 *   - Stores the altitude for potential compensation calculations.
 *   - Sets the flag for enabling/disabling CRC error checking.
 */
PressureSensor::PressureSensor(std::shared_ptr<PicoI2C> my_i2c,
                               uint8_t i2cAddress,
                               float altitude_m,
                               bool useCrc)
        : my_i2c(my_i2c)
        , address(i2cAddress)
        , pressurePa(0.0f)
        , altitudeMeters(altitude_m)
        , crcEnabled(useCrc)
{
    // No additional initialization required; constructor primarily uses initializer list.
}

/*
 * readSensor - Reads the pressure data from the sensor over I²C.
 *
 * This method performs the following steps:
 *   1. Checks if the I²C bus (PicoI2C) is available.
 *   2. Writes a command (0xF1) to the sensor to request a reading.
 *   3. Reads 2 bytes of raw data from the sensor.
 *   4. Converts the raw 16-bit value into a pressure reading in Pascals
 *      by dividing by a fixed conversion factor (60 in this implementation).
 *
 * Returns:
 *   true if the sensor read was successful and valid data was obtained,
 *   false if any error occurred during I²C communication.
 */
bool PressureSensor::readSensor() {
    // Verify that the I²C bus pointer is valid
    if (!my_i2c) {
        printf("\n[PressureSensor]: No I2C bus\n");
        return false;
    }

    // Define the command byte to request pressure data.
    // Command 0xF1 is used as per sensor datasheet specifications.
    uint8_t command = 0xF1;

    // Write the command to the sensor over I²C.
    // The address specifies the sensor's I²C address; send one byte command.
    int written = my_i2c->write(address, &command, 1);
    if (written != 1) {
        // If the number of bytes written is not 1, an error occurred during I²C write.
        printf("PressureSensor: I2C write error\n");
        return false;
    }

    // Buffer to hold the 2 bytes of data read from the sensor.
    uint8_t buf[2];

    // Read 2 bytes from the sensor.
    int readBytes = my_i2c->read(address, buf, 2);
    if (readBytes != 2) {
        // If the number of bytes read is not equal to 2, report an I²C read error.
        printf("PressureSensor: I2C read error\n");
        return false;
    }

    // Combine the two received bytes into a 16-bit raw value.
    // The first byte is the high order byte and the second is the low order byte.
    int16_t rawVal = (buf[0] << 8) | buf[1];

    // Convert the raw sensor value to pressure in Pascals.
    // The conversion factor 60.0f is defined by sensor characteristics (as specified in datasheet).
    pressurePa = rawVal / 60.0f;

    // Output the raw and converted pressure values for debugging.
    printf("\n[PressureSensor] rawVal=%d => %.2f Pa\n", rawVal, pressurePa);

    return true;  // Successful sensor read.
}

/*
 * getValue - Returns the current pressure reading.
 *
 * This method implements the ISensor interface function to provide the sensor value.
 * It simply returns the pressure in Pascals as calculated by readSensor().
 *
 * Returns:
 *   The current pressure reading in Pascals.
 */
float PressureSensor::getValue() {
    return pressurePa;
}

/*
 * getPressurePa - Getter function for pressure in Pascals.
 *
 * Provides read-only access to the internally stored pressure value.
 *
 * Returns:
 *   The current pressure value in Pascals.
 */
float PressureSensor::getPressurePa() const {
    return pressurePa;
}