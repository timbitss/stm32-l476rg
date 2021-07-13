/**
 * @file command.c
 * @author Timothy Nguyen
 * @brief Delegate commands received from serial and transmit information to host.
 * @version 0.1
 * @date 2021-07-12
 */

#include "command.h"
#include "printf.h"

/* Configuration parameters */
#define MAX_MSG_COUNT 5UL // Maximum number of messages allowed in queue at a time to UART_TX_Thread.

/* Message queue object for UART_TX_Thread */
osMessageQueueId_t tx_msg_queue_id;

/* Pointer to UART handler */
static UART_HandleTypeDef *uart_handle_ptr;

/**
 * @brief Requirement for tiny printf() function.
 * 
 * @param character Character to place in UART TX buffer.
 */
void _putchar(char character)
{
    HAL_UART_Transmit(uart_handle_ptr, (uint8_t *)&character, 1, HAL_MAX_DELAY);
}

osStatus_t Command_Transmit(const char *msg, uint32_t timeout_period)
{
    osStatus_t status = osMessageQueuePut(tx_msg_queue_id, (void*)&msg, 0, timeout_period); // Transmit character pointer.
    return status;
}

/******************* RTOS Threads **************************/

void UART_RX_Thread(void *args)
{
    while (1)
    {
        osDelay(1);
    }
}

void UART_TX_Thread(void *args)
{   
    /* Wait forever for message from queue, then transmit over UART */
    while (1)
    {
        char* msg_to_tx = NULL; // Buffer to store char *.
        osMessageQueueGet(tx_msg_queue_id, (void *)&msg_to_tx, NULL, osWaitForever);
        if(msg_to_tx == NULL)
        {
            printf("Invalid message\r\n");
        }
        else
        {
            printf("%s", msg_to_tx);
        }
    }
}

uint8_t Command_Init(UART_HandleTypeDef *huart)
{
    uart_handle_ptr = huart;

    /* Define task attributes */
    const osThreadAttr_t rx_task_attributes = {
        .name = "RxTask",
        .stack_size = 128 * 4,
        .priority = (osPriority_t)osPriorityNormal,
    };
    const osThreadAttr_t tx_task_attributes = {
        .name = "TxTask",
        .stack_size = 4096,
        .priority = (osPriority_t)osPriorityNormal,
    };

    /* Create threads */
    osThreadId_t rx_thread_handle = (osThreadId_t)osThreadNew(UART_RX_Thread, NULL, &rx_task_attributes);
    osThreadId_t tx_thread_handle = (osThreadId_t)osThreadNew(UART_TX_Thread, NULL, &tx_task_attributes);

    /* Create UART transmit message queue */
    tx_msg_queue_id = osMessageQueueNew(MAX_MSG_COUNT, sizeof(char *), NULL);

    if (rx_thread_handle == NULL || tx_thread_handle == NULL || tx_msg_queue_id == NULL)
    {
        return 1; // Insufficient memory.
    }
    else
    {
        return 0;
    }
}

