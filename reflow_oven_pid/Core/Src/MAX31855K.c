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
#include "string.h"

// Temperature resolutions:
#define HJ_RES 0.25   // Hot junction temperature resolution in degrees Celsius.
#define CJ_RES 0.0625 // Cold junction temperature resolution in degrees Celsius.

// Helper function declarations:
static void MAX31855K_error_check(MAX31855K_t *max); // Check data for device faults or SPI read error.

void MAX31855K_Init(MAX31855K_t *max, SPI_HandleTypeDef *hspi, GPIO_TypeDef *max_cs_port, uint16_t max_cs_pin)
{
    max->spi_handle = hspi;
    max->cs_port = max_cs_port;
    max->cs_pin = max_cs_pin;
    memset(max->tx_buf, 0, sizeof(max->tx_buf));
    memset(max->rx_buf, 0, sizeof(max->rx_buf));
    max->data32 = 0;
    max->err = MAX_OK;
}

MAX31855K_err_t MAX31855K_RxBlocking(MAX31855K_t *max)
{
    /* Acquire data from MAX31855K */
    HAL_GPIO_WritePin(max->cs_port, max->cs_pin, GPIO_PIN_RESET); // Assert CS line to start transaction.
    HAL_SPI_Receive(max->spi_handle,                              // Sample 4 bytes off MISO line.
                    max->rx_buf,
                    sizeof(max->rx_buf),
                    HAL_MAX_DELAY);
    HAL_GPIO_WritePin(max->cs_port, max->cs_pin, GPIO_PIN_SET); // Deassert CS line to end transaction.
    max->data32 = max->rx_buf[0] << 24 | (max->rx_buf[1] << 16) | (max->rx_buf[2] << 8) | max->rx_buf[3];

    /* Check for faults. */
    MAX31855K_error_check(max);

    return max->err;
}

void MAX31855K_RxDMA(MAX31855K_t *max)
{
    /* Pull CS line low */
    HAL_GPIO_WritePin(max->cs_port, max->cs_pin, GPIO_PIN_RESET);

    /* Execute DMA transfer */
    HAL_StatusTypeDef err = HAL_SPI_TransmitReceive_DMA(max->spi_handle, max->tx_buf, max->rx_buf, sizeof(max->rx_buf));
    if (err != HAL_OK)
    {
        HAL_GPIO_WritePin(max->cs_port, max->cs_pin, GPIO_PIN_SET);
        max->err = MAX_SPI_DMA_FAIL;
    }
}

void MAX31885K_RxDMA_Complete(MAX31855K_t *max)
{
    HAL_GPIO_WritePin(max->cs_port, max->cs_pin, GPIO_PIN_SET);
    max->data32 = max->rx_buf[0] << 24 | (max->rx_buf[1] << 16) | (max->rx_buf[2] << 8) | max->rx_buf[3];
    MAX31855K_error_check(max);
}

float MAX31855K_Get_HJ(MAX31855K_t *max)
{
    /* Extract HJ temperature. */
    uint32_t data = max->data32;    // Capture latest data reading.
    int16_t val = 0;                // Value prior to temperature conversion.
    if (data & ((uint32_t)1 << 31)) // Perform sign-extension.
    {
        val = 0xC000 | ((data >> 18) & 0x3FFF);
    }
    else
    {
        val = data >> 18;
    }
    return val * HJ_RES;
}

float MAX31855K_Get_CJ(MAX31855K_t *max)
{
    /* Extract CJ temperature. */
    uint32_t data = max->data32;    // Capture latest data reading.
    int16_t val = 0;                // Value prior to temperature conversion.
    if (data & ((uint32_t)1 << 15)) // Perform sign-extension.
    {
        val = 0xF000 | ((data >> 4) & 0xFFF);
    }
    else
    {
        val = (data >> 4) & 0xFFF;
    }
    return val * CJ_RES;
}


static void MAX31855K_error_check(MAX31855K_t *max)
{
    if (max->data32 == 0)
    {
        max->err = MAX_ZEROS;
    }
    else if (max->data32 & ((uint32_t)1 << 16))
    {
        uint8_t fault = max->data32 & 0x7;
        switch (fault)
        {
        case 0x4:
            max->err = MAX_SHORT_VCC;
        case 0x2:
            max->err = MAX_SHORT_GND;
        case 0x1:
            max->err = MAX_OPEN;
        default:
            break;
        }
    }
    else
    {
        max->err = MAX_OK;
    }
}