/**
 * @file MAX31855K.c
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

#include "MAX31855K.h"

// Temperature resolution:
#define HJ_RES 0.25   // Hot junction temperature resolution in degrees Celsius.
#define CJ_RES 0.0625 // Cold junction temperature resolution in degrees Celsius. 

static void error_check(MAX31855K_t *max, uint32_t data32);
static void parse_data(MAX31855K_t *max, uint32_t data32);

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
MAX31855K_t *max31855k_read(SPI_HandleTypeDef *spi_handle, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    static MAX31855K_t max_obj = {0};

    // Acquire data from MAX31855K
    uint8_t raw_data[4] = {0};
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);              // Assert CS line.
    HAL_SPI_Receive(spi_handle, raw_data, sizeof(raw_data), HAL_MAX_DELAY); // Sample 4 bytes off MISO line.
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);                // Deassert CS line.
    uint32_t raw_data32 = raw_data[0] << 24 | (raw_data[1] << 16) | (raw_data[2] << 8) | raw_data[3];

    // Check for faults.
    error_check(&max_obj, raw_data32);
    
    // Parse data for HJ and CJ temperature.
    parse_data(&max_obj, raw_data32);

    return &max_obj;
}

/**
 * @brief Check data for device faults or SPI read error.
 * 
 * @param[in] data32 32-bit data received from MAX31855K.
 * @param[out] max MAX31855K_t object to hold error value. 
 */
static void error_check(MAX31855K_t *max, uint32_t data32)
{
    if (data32 == 0)
    {
        max->err = THERMO_ZEROS;
    }
    else if (data32 & ((uint32_t)1 << 16))
    {
        uint8_t fault = data32 & 0x7;
        switch (fault)
        {
        case 0x4:
            max->err = THERMO_SHORT_VCC;
        case 0x2:
            max->err = THERMO_SHORT_GND;
        case 0x1:
            max->err = THERMO_OPEN;
        default:
            break;
        }
    }
    else
    {
        max->err = THERMO_OK;
    }
}

/**
 * @brief Parse data to acquire signed CJ and HJ temperature readings.
 * 
 * @param max MAX31855K_t object to store temperature readings.
 * @param data32 Raw data from MAX31885K device.
 */
static void parse_data(MAX31855K_t *max, uint32_t data32)
{
    // Extract cold-junction compensated hot junction temperature.
    int16_t val = 0;
    max->hj_temp = 0;
    if (data32 & ((uint32_t)1 << 31)) // Perform sign-extension.
    {
        val = 0xC000 | ((data32 >> 18) & 0x3FFF);
    }
    else
    {
        val = data32 >> 18;
    }
    max->hj_temp = val * HJ_RES;

    // Extract cold junction temperature.
    val = 0;
    max->cj_temp = 0;
    if (data32 & ((uint32_t)1 << 15))
    {
        val = 0xF000 | ((data32 >> 4) & 0xFFF);
    }
    else
    {
        val = (data32 >> 4) & 0xFFF;
    }
    max->cj_temp = val * CJ_RES;
}
