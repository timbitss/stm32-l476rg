/**
 * @file MAX31855K.h
 * @author Timothy Nguyen
 * @brief  MAX31855K SparkFun Breakout Board API.
 * @version 0.1
 * @date 2021-07-07
 * 
 * @note API inspired by SparkFun's MAX31855K Arduino library.
 * 
 * MAX31855K Memory Map:
 * D[31:18] Signed 14-bit cold-junction compensated thermocouple temperature (HJ temperature)
 * D17      Reserved: Always reads 0
 * D16      Fault: 1 when any of the SCV, SCG, or OC faults are active, else 0
 * D[15:4]  Signed 12-bit internal temperature
 * D3       Reserved: Always reads 0
 * D2       SCV fault: Reads 1 when thermocouple is shorted to V_CC, else 0
 * D1       SCG fault: Reads 1 when thermocouple is shorted to gnd, else 0
 * D0       OC  fault: Reads 1 when thermocouple is open-circuit, else 0
 */

#ifndef _MAX31855K_H_
#define _MAX31855K_H_

#include "stm32l4xx_hal.h"
#include "stm32l476xx.h"

// MAX31855K thermocouple device error definitions.
typedef enum
{
    MAX_OK,           // Successful temperature reading.
    MAX_SHORT_VCC,    // Thermocouple shorted to VCC.
    MAX_SHORT_GND,    // Thermocouple shorted to GND.
    MAX_OPEN,         // Thermocouple connection is open.
    MAX_ZEROS,        // SPI read only 0s.
    MAX_SPI_DMA_FAIL, // Error during SPI DMA RX transfer.
} MAX31855K_err_t;

// MAX31885K thermocouple device structure definition.
typedef struct
{
    /* SPI Configuration Parameters */
    SPI_HandleTypeDef *spi_handle; // SPI handler
    GPIO_TypeDef *cs_port;         // Chip-select GPIO port.
    uint16_t cs_pin;               // Chip-select pin number.

    /* Data */
    uint8_t tx_buf[4];   // SPI Transmit buffer.
    uint8_t rx_buf[4];   // SPI Receive buffer.
    uint32_t data32;     // Conversion of raw temperature reading to uint32.

    /* Error value */
    MAX31855K_err_t err; // Thermocouple error value of most recent reading.

} MAX31855K_t;

/**
 * @brief Initialize MAX31885K drive by configuring HAX31855K_t structrue. 
 * 
 *        Should be called once.
 * 
 * @param max           Pointer to MAX31855K_t object to store configuration parameters.
 * @param hspi          Pointer to SPI handler.
 * @param max_cs_port   GPIO port of MAX31855K chip-select.
 * @param max_cs_pin    GPIO pin number of MAX31855K chip-select.
 */
void MAX31855K_Init(MAX31855K_t *max, SPI_HandleTypeDef *hspi, GPIO_TypeDef *max_cs_port, uint16_t max_cs_pin);

/**
 * @brief Read data from MAX31855K in blocking mode and check for errors.
 * 
 *        SPI instance must be initialized prior to function call.
 * 
 * @param max Pointer to MAX321885K_t structure containing configuration parameters and data.
 * @return MAX31855K_err_t Error value.
 */
MAX31855K_err_t MAX31855K_RxBlocking(MAX31855K_t *max);



/**
 * @brief Read data from MAX31855K in non-blocking mode through DMA controller.
 * 
 * @param max Pointer to MAX321885K_t structure containing configuration parameters and data.
 */
void MAX31855K_RxDMA(MAX31855K_t *max);

/**
 * @brief Format data received and check for errors after DMA transfer.
 * 
 *        Function should be called from within SPI_RX_Cplt callback function.
 * 
 * @param max Pointer to MAX321885K_t structure containing configuration parameters and data.
 */
void MAX31885K_RxDMA_Complete(MAX31855K_t *max);

/**
 * @brief Parse HJ temperature from raw data.
 * 
 * @pre Check that max's error value equals MAX_OK.
 * @param max Pointer to MAX321885K_t structure containing configuration parameters and data.
 * @return float Hot junction temperature.
 */
float MAX31855K_Get_HJ(MAX31855K_t *max);

/**
 * @brief Parse CJ temperature from raw data.
 * 
 * @pre Check that max's error value equals MAX_OK.
 * @param max Pointer to MAX321885K_t structure containing configuration parameters and data.
 * @return float Cold junction temperature.
 */
float MAX31855K_Get_CJ(MAX31855K_t *max);

#endif
