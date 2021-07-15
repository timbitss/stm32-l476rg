/**
 * @file uart.c
 * @author Timothy Nguyen
 * @brief Interface for UART peripheral with CMSIS-RTOSv2
 * @version 0.1
 * @date 2021-07-12
 */

#include "uart.h"
#include "printf.h"
#include "common.h"
#include "stm32l4xx_ll_usart.h"
#include "stm32l4xx_hal.h"

////////////////////////////////////////////////////////////////////////////////
// Common macros
////////////////////////////////////////////////////////////////////////////////

/* Maximum number of menu options */
#define MAX_MENU_OPTIONS 9

////////////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Structure to view state of UART peripheral.
 */
typedef struct
{
    USART_TypeDef *uart_reg_base; // Pointer to base UART register address.
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
} ttys_u16_pms;

////////////////////////////////////////////////////////////////////////////////
// Private (static) variables
////////////////////////////////////////////////////////////////////////////////

/* UART_t Instance */
static UART_t uart;

/* Queue for transmitting messages over UART */
static osMessageQueueId_t tx_msg_queue_id;

/* Performance measurements */
static uint16_t cnts_u16[NUM_U16_PMS];

////////////////////////////////////////////////////////////////////////////////
// Private (static) function prototypes
////////////////////////////////////////////////////////////////////////////////

static char _getchar(void);  // Get character from receive data register in non-blocking mode.
static char _getcharb(void); // Get character from receive data register in blocking mode.

static void UART_ISR(void) // UART interrupt service routine.

////////////////////////////////////////////////////////////////////////////////
// RTOS threads
////////////////////////////////////////////////////////////////////////////////

    void UART_RX_Thread(void *args)
{
    Command_display_menu();

    while (1)
    {
        Command_menu_read();
        osDelay(1);
    }
}

void UART_TX_Thread(void *args)
{
    /* Wait forever for message from queue, then transmit over UART */
    while (1)
    {
        char *msg_to_tx = NULL; // Buffer to store char *.
        osMessageQueueGet(tx_msg_queue_id, (void *)&msg_to_tx, NULL, osWaitForever);
        printf("%s", msg_to_tx);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Public (global) functions
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Place character in transmit data register (blocking).
 *
 *        Requirement for tiny printf() function.
 *
 * @param character Character to place in transmit data register.
 */
void _putchar(char character)
{
    /* Wait for TXE bit to be set */
    while (!(uart_handle_ptr->Instance->ISR & USART_ISR_TXE_Msk))
    {
    }
    uart_handle_ptr->Instance->TDR = (uint8_t)character;
}

mod_err_t uart_init(USART_TypeDef *_uart_base_reg)
{
    if (_uart_base_reg == NULL)
    {
        return MOD_ERR_ARG;
    }
    else
    {
        memset(&uart, 0, sizeof(uart));
        uart.uart_reg_base = _uart_base_reg;
    }

    /* Define task attributes */
    const osThreadAttr_t rx_task_attributes = {
        .name = "RxTask",
        .stack_size = 1024 * 2,
        .priority = (osPriority_t)osPriorityNormal,
    };
    const osThreadAttr_t tx_task_attributes = {
        .name = "TxTask",
        .stack_size = 1024 * 2,
        .priority = (osPriority_t)osPriorityNormal,
    };

    /* Create threads */
    osThreadId_t rx_thread_handle = (osThreadId_t)osThreadNew(UART_RX_Thread, NULL, &rx_task_attributes);
    osThreadId_t tx_thread_handle = (osThreadId_t)osThreadNew(UART_TX_Thread, NULL, &tx_task_attributes);

    /* Create UART transmit message queue */
    tx_msg_queue_id = osMessageQueueNew(MAX_MSG_COUNT, sizeof(char *), NULL);

    if (rx_thread_handle == NULL || tx_thread_handle == NULL || tx_msg_queue_id == NULL)
    {
        return MOD_ERR_RESOURCE; // Insufficient memory.
    }
    else
    {
        return MOD_OK;
    }
}

osStatus_t Command_Transmit(const char *msg, uint32_t timeout_period)
{
    /* Transmit character pointer. */
    osStatus_t status = osMessageQueuePut(tx_msg_queue_id, (void *)&msg, 0, timeout_period);
    return status;
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
 * @brief Start UART peripheral by enabling interrupts.
 * 
 * @return mod_err_t 
 */
static mod_err_t uart_start(void)
{
    if (st->uart_reg_base == NULL)
    {
        return MOD_ERR_NOT_INIT;
    }

    LL_USART_EnableIT_RXNE(st->uart_reg_base); // Generate interrupt whenever RXNE flag is set.
    LL_USART_EnableIT_TXE(st->uart_reg_base);  // Generate interrupt whenever TXE flag is set.

    /* Acquire interrupt request number */
    IRQn_Type irq_num = 0;
    switch (st->uart_reg_base)
    {
    case USART1:
        irq_num = USART1_IRQn;
        break;
    case USART2:
        irq_num = USART2_IRQn;
        break;
    case USART3:
        irq_num = USART3_IRQn;
        break;
    case UART4:
        irq_num = UART4_IRQn;
        break;
    case UART5:
        irq_num = UART5_IRQn;
        break;
    default:
        return MOD_ERR_BAD_INSTANCE;
    }

    /* Set group and sub priority to highest priority (0). */
    __NVIC_SetPriority(irq_num, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));

    /* Enable UARTx interrupt channel */
    __NVIC_EnableIRQ(irq_num);

    return MOD_OK;
}

/**
 * @brief Read character from receive data register (RDR) and place into receive buffer.
 */
static void read_rdr(void)
{
    uint16_t next_rx_put_idx = (uart->rx_buf_put_idx + 1) % UART_RX_BUF_SIZE;
   
    if (next_rx_put_idx == uart->rx_buf_get_idx)
    {
        INC_SAT_U16(cnts_u16[CNT_RX_BUF_OVERRUN]);
        LL_USART_RequestRxDataFlush(uart->uart_reg_base); // Flush RDR to avoid overrun error.
    }
    else
    {
        uart->rx_buf[uart->rx_buf_put_idx] = uart->uart_reg_base->RDR & 0xFFU; // Clears RXNE flag.
        uart->rx_buf_put_idx = next_rx_put_idx;
    }
}

/**
 * @brief Write character from transmit buffer to transmit data register (TDR).
 */
static void write_tdr(void)
{
	if (uart->tx_buf_get_idx == uart->tx_buf_put_idx)
	{
		// Nothing to transmit, disable TXE flag from generating an interrupt.
		LL_USART_DisableIT_TXE(uart->uart_reg_base);
	}
	else
	{
		uart->uart_reg_base->TDR = uart->tx_buf[uart->tx_buf_get_idx]; // Clears TXE flag.
		uart->tx_buf_get_idx = (uart->tx_buf_get_idx + 1) % UART_TX_BUF_SIZE;
	}
}


static void UART_ISR(void)
{
    /* Read interrupt status register. */
    uint32_t status_reg = uart->uart_reg_base->ISR;

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
          if (status_reg & LL_USART_SR_ORE)
          {   // An overrun error occurs if a character is received and RXNE has not been reset.
        	  // The RDR register content is not lost but the shift register is overwritten by incoming data.
              INC_SAT_U16(cnts_u16[CNT_RX_UART_ORE]);
              LL_USART_ClearFlag_ORE(uart->uart_reg_base);
          }
          if (status_reg & LL_USART_SR_NE)
          {
              INC_SAT_U16(cnts_u16[CNT_RX_UART_NE]);
              LL_USART_ClearFlag_NE(uart->uart_reg_base);
          }
          if (status_reg & LL_USART_SR_FE)
          {
              INC_SAT_U16(cnts_u16[CNT_RX_UART_FE]);
              LL_USART_ClearFlag_FE(uart->uart_reg_base);
          }
          if (status_reg & LL_USART_SR_PE)
          {
              INC_SAT_U16(cnts_u16[CNT_RX_UART_PE]);
              LL_USART_ClearFlag_PE(uart->uart_reg_base);
          }
    }
}
