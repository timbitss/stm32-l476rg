/**
 * @file uart.c
 * @author Timothy Nguyen
 * @brief Interface for UART peripheral using STM32L4 LL library.
 * @version 0.1
 * @date 2021-07-12
 */

#include <stdio.h>

#include "uart.h"
#include "printf.h"
#include "common.h"
#include "stm32l4xx_ll_usart.h"
#include "stm32l4xx_hal.h"
#include "string.h"

////////////////////////////////////////////////////////////////////////////////
// Common macros
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief UART peripheral structure.
 */
typedef struct
{
	/* Configuration parameters */
    USART_TypeDef *uart_reg_base; // Pointer to UART's base register address.
    IRQn_Type irq_num;

    /* Data */
    uint16_t rx_buf_get_idx;      // Receive buffer get index.
    uint16_t rx_buf_put_idx;      // Receive buffer put index.
    uint16_t tx_buf_get_idx;      // Transmit buffer get index.
    uint16_t tx_buf_put_idx;      // Transmit buffer put index.
    char tx_buf[UART_TX_BUF_SIZE];     // Transmit circular buffer.
    char rx_buf[UART_RX_BUF_SIZE];     // Receive circular buffer.
} UART_t;

/**
 * @brief List of UART performance measurements.
 */
typedef enum{
    CNT_RX_UART_ORE,    // Overrun error count.
    CNT_RX_UART_NE,     // Noise detection error count.
    CNT_RX_UART_FE,     // Frame error count.   
    CNT_RX_UART_PE,     // Parity error count.
    CNT_TX_BUF_OVERRUN, // Tx buffer overrun count.
    CNT_RX_BUF_OVERRUN, // Rx buffer overrun count.

    NUM_U16_PMS // Number of performance measurements
} UART_pms_t;

////////////////////////////////////////////////////////////////////////////////
// Private (static) variables
////////////////////////////////////////////////////////////////////////////////

/* UART_t Instance */
static UART_t uart;

/* Performance measurement counters */
static uint16_t uart_pms[NUM_U16_PMS];

////////////////////////////////////////////////////////////////////////////////
// Private (static) function prototypes
////////////////////////////////////////////////////////////////////////////////

/* UART interrupt service routine. */
static void UART_ISR(void);

////////////////////////////////////////////////////////////////////////////////
// Public (global) functions
////////////////////////////////////////////////////////////////////////////////

mod_err_t uart_init(uart_config_t* uart_cfg)
{
    if (uart_cfg->uart_reg_base == NULL)
    {
        return MOD_ERR_ARG;
    }
    else if (!LL_USART_IsEnabled(uart_cfg->uart_reg_base))
    {
    	return MOD_ERR_PERIPH;
    }
    else
    {
		switch(uart_cfg->irq_num)
		{
			case USART1_IRQn:
			case USART2_IRQn:
			case USART3_IRQn:
			case UART4_IRQn:
			case UART5_IRQn:
				memset(&uart, 0, sizeof(uart));
				uart.irq_num = uart_cfg->irq_num;
				uart.uart_reg_base = uart_cfg->uart_reg_base;
				return MOD_OK;
			default:
				return MOD_ERR_ARG;
		}
    }

}

mod_err_t uart_start(void)
{
    if (uart.uart_reg_base == NULL)
    {
        return MOD_ERR_NOT_INIT;
    }

    LL_USART_EnableIT_TXE(uart.uart_reg_base);  // Generate interrupt whenever TXE flag is set.
    LL_USART_EnableIT_RXNE(uart.uart_reg_base); // Generate interrupt whenever RXNE flag is set.

    /* Set group and sub priority to highest priority (0). */
    __NVIC_SetPriority(uart.irq_num, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));

    /* Enable UARTx interrupt channel. */
    __NVIC_EnableIRQ(uart.irq_num);

    return MOD_OK;
}

mod_err_t uart_putc(char c)
{

    uint16_t next_put_idx = (uart.tx_buf_put_idx + 1) % UART_TX_BUF_SIZE;

    /* Tx circular buffer is full. */
    if (next_put_idx == uart.tx_buf_get_idx) {
        INC_SAT_U16(uart_pms[CNT_TX_BUF_OVERRUN]);
        return MOD_ERR_BUF_OVERRUN;
    }

    /* Place char in buffer */
    uart.tx_buf[uart.tx_buf_put_idx] = c;
    uart.tx_buf_put_idx = next_put_idx;

    // Ensure TXE interrupt is enabled.
    if (uart.uart_reg_base != NULL && !LL_USART_IsEnabledIT_TXE(uart.uart_reg_base)) {
        __disable_irq();
        LL_USART_EnableIT_TXE(uart.uart_reg_base);
        __enable_irq();
    }

    return MOD_OK;
}

uint8_t uart_getc(char* c)
{
    /* Check if rx buffer is empty. */
    if (uart.rx_buf_get_idx == uart.rx_buf_put_idx)
    {
        return 0;
    }
    else
    {
    	/* Get character and increment get index */
    	*c = uart.rx_buf[uart.rx_buf_get_idx];
    	uart.rx_buf_get_idx = (uart.rx_buf_get_idx + 1) % UART_RX_BUF_SIZE;
    	return 1;
    }
}


////////////////////////////////////////////////////////////////////////////////
// Interrupt handlers
////////////////////////////////////////////////////////////////////////////////

void USART1_IRQHandler(void)
{
    UART_ISR();
}

void USART2_IRQHandler(void)
{
    UART_ISR();
}

void USART3_IRQHandler(void)
{
    UART_ISR();
}

void UART4_IRQHandler(void)
{
    UART_ISR();
}

void UART5_IRQHandler(void)
{
    UART_ISR();
}

////////////////////////////////////////////////////////////////////////////////
// Private (static) function definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Read character from receive data register (RDR) and place into receive buffer.
 */
static void read_rdr(void)
{
    uint16_t next_rx_put_idx = (uart.rx_buf_put_idx + 1) % UART_RX_BUF_SIZE;
   
    if (next_rx_put_idx == uart.rx_buf_get_idx)
    {
        INC_SAT_U16(uart_pms[CNT_RX_BUF_OVERRUN]);
        LL_USART_RequestRxDataFlush(uart.uart_reg_base); // Drop character.
    }
    else
    {
        uart.rx_buf[uart.rx_buf_put_idx] = uart.uart_reg_base->RDR & 0xFFU; // Clears RXNE flag.
        uart.rx_buf_put_idx = next_rx_put_idx;
    }
}

/**
 * @brief Write character from transmit buffer to transmit data register (TDR).
 */
static void write_tdr(void)
{
	if (uart.tx_buf_get_idx == uart.tx_buf_put_idx)
	{
		/* Nothing to transmit, disable TXE flag from generating an interrupt. */
		LL_USART_DisableIT_TXE(uart.uart_reg_base);
	}
	else
	{
		uart.uart_reg_base->TDR = uart.tx_buf[uart.tx_buf_get_idx]; // Clears TXE flag.
		uart.tx_buf_get_idx = (uart.tx_buf_get_idx + 1) % UART_TX_BUF_SIZE;
	}
}


static void UART_ISR(void)
{
    /* Read interrupt status register. */
    uint32_t status_reg = uart.uart_reg_base->ISR;

    /* Service interrupt flags. */
    if (status_reg & USART_ISR_RXNE_Msk)
    {
        read_rdr();
    }
    if (status_reg & USART_ISR_TXE_Msk)
    {
    	write_tdr();
    }

    /* Check error flags. */
    if (status_reg & (USART_ISR_ORE | USART_ISR_NE | USART_ISR_FE | USART_ISR_PE))
    {
          if (status_reg & LL_USART_ISR_ORE)
          {   // An overrun error occurs if a character is received and RXNE has not been reset.
        	  // The RDR register content is not lost but the shift register is overwritten by incoming data.
              INC_SAT_U16(uart_pms[CNT_RX_UART_ORE]);
              LL_USART_ClearFlag_ORE(uart.uart_reg_base);
          }
          if (status_reg & LL_USART_ISR_NE)
          {
              INC_SAT_U16(uart_pms[CNT_RX_UART_NE]);
              LL_USART_ClearFlag_NE(uart.uart_reg_base);
          }
          if (status_reg & LL_USART_ISR_FE)
          {
              INC_SAT_U16(uart_pms[CNT_RX_UART_FE]);
              LL_USART_ClearFlag_FE(uart.uart_reg_base);
          }
          if (status_reg & LL_USART_ISR_PE)
          {
              INC_SAT_U16(uart_pms[CNT_RX_UART_PE]);
              LL_USART_ClearFlag_PE(uart.uart_reg_base);
          }
    }

}

