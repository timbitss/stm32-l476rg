#ifndef _MAX31855K_H_
#define _MAX31855K_H_

#include "stm32l4xx_hal.h"

// MAX31855K thermocouple device error definitions.
typedef enum
{
    THERMO_OK,        // Successful temperature reading.
    THERMO_SHORT_VCC, // Thermocouple shorted to VCC.
    THERMO_SHORT_GND, // Thermocouple shorted to GND.
    THERMO_OPEN,      // Thermocouple connection is open.
    THERMO_ZEROS      // SPI read only 0s.
} Thermo_err_t;

// MAX31885K thermocouple device structure definition.
typedef struct
{
    float hj_temp;    // Hot junction temperature.
    float cj_temp;    // Cold junction temperature.
    Thermo_err_t err; // Thermocouple error value.
} MAX31855K_t;

/**
 * @brief Read cold and hot junction temperature from MAX31855K device.
 * 
 *        SPI instance must be initialized prior to function call.
 * 
 * @param spi_handle Handle to SPI instance for reading data.
 * @param cs_port Chip-select port letter.
 * @param cs_pin  Chip-select pin number.
 * @return Pointer to MAX31855K_t object with updated temperature and error values.
 */
MAX31855K_t* max31855k_read(SPI_HandleTypeDef *spi_handle, GPIO_TypeDef *cs_port, uint16_t cs_pin);

#endif