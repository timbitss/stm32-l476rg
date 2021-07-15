/**
 * @file command.c
 * @author Timothy Nguyen
 * @brief Delegate commands received from serial and transmit information to host.
 * @version 0.1
 * @date 2021-07-12
 */

#include "command.h"
#include "printf.h"

/* Maximum number of menu options */
#define MAX_MENU_OPTIONS 9

/* Queue for transmitting messages over UART */
static osMessageQueueId_t tx_msg_queue_id;

/* Pointer to UART handler */
static UART_HandleTypeDef *uart_handle_ptr;

/* Array of menu options */
static const Command_menu_option *menu_options;

/* Number of menu options */
static uint8_t num_of_options;

/* Function prototypes */
static void Command_display_menu(void); // Display menu to user.
static void Command_menu_read(void);    // Save input character and execute option if enter key is pressed.
static char _getchar(void);             // Get character from receive data register in non-blocking mode.
static char _getcharb(void);            // Get character from receive data register in blocking mode.

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

/********************* RTOS Threads *********************/

void Menu_Thread(void *args)
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

/**********************************************************/

uint8_t Command_Init(UART_HandleTypeDef *huart, const Command_menu_option *_menu_options, uint8_t _num_of_options)
{
    uart_handle_ptr = huart;

    /* Define task attributes */
    const osThreadAttr_t rx_task_attributes = {
        .name = "MenuTask",
        .stack_size = 1024 * 2,
        .priority = (osPriority_t)osPriorityNormal,
    };
    const osThreadAttr_t tx_task_attributes = {
        .name = "TxTask",
        .stack_size = 1024 * 2,
        .priority = (osPriority_t)osPriorityNormal,
    };

    /* Create threads */
    osThreadId_t rx_thread_handle = (osThreadId_t)osThreadNew(Menu_Thread, NULL, &rx_task_attributes);
    osThreadId_t tx_thread_handle = (osThreadId_t)osThreadNew(UART_TX_Thread, NULL, &tx_task_attributes);

    /* Create UART transmit message queue */
    tx_msg_queue_id = osMessageQueueNew(MAX_MSG_COUNT, sizeof(char *), NULL);

    /* Store menu options */
    menu_options = _menu_options;
    if (_num_of_options > MAX_MENU_OPTIONS)
    {
        num_of_options = MAX_MENU_OPTIONS;
    }
    else
    {
        num_of_options = _num_of_options;
    }

    if (rx_thread_handle == NULL || tx_thread_handle == NULL || tx_msg_queue_id == NULL)
    {
        return 1; // Insufficient memory.
    }
    else
    {
        return 0;
    }
}

osStatus_t Command_Transmit(const char *msg, uint32_t timeout_period)
{
    /* Transmit character pointer. */
    osStatus_t status = osMessageQueuePut(tx_msg_queue_id, (void *)&msg, 0, timeout_period);
    return status;
}

uint32_t Command_get_uint32(const char *prompt)
{
    uint32_t val = 0;

    Command_Transmit(prompt, 50);
    Command_Transmit("\r\n>> ", 50);

    while (1)
    {
        char c = _getcharb();
        _putchar(c);
        if (c >= '0' && c <= '9')
        {
            val *= 10; // Handle multiple digits.
            val += c - '0';
        }
        else if (c == 127) // User pressed backspace key.
        {
            val /= 10; // Truncate
        }
        else if (c == '\r') // User pressed Enter key (PuTTY).
        {
            _putchar('\n');
            break;
        }
        else
        {
            /* Ignore other characters */
        }
    }

    return val;
}

/**
 * @brief Display menu options to user.
 */
static void Command_display_menu(void)
{
    if (num_of_options > 0)
    {
        Command_Transmit("\r\nMenu (enter # to select):", 50);
        static char option[] = "\r\nx) ";

        for (uint8_t i = 0; i < num_of_options; i++)
        {
            option[2] = '1' + i;
            Command_Transmit(option, 50);
            Command_Transmit(menu_options[i].name, 50);
            osDelay(5); // TODO: More efficient solution??
        };

        /* Print prompt */
        Command_Transmit("\r\n>> ", 50);
    }
    else
    {
        Command_Transmit("\r\nNo menu options to display\r\n", 50);
    }
}

/**
 * @brief Save input character and execute option if enter key is pressed. Non-blocking.
 * 
 * @note Recommended to call Command_display_menu() prior to calling Command_run_menu().
 */
static void Command_menu_read(void)
{
    /* Receive and echo back */
    char c = _getchar();
    _putchar(c);

    static uint8_t option_num = 0;
    if (c >= '0' && c <= '9')
    {
        option_num *= 10; // Handle multiple digits.
        option_num += c - '0';
    }
    else if (c == 127) // User pressed backspace key.
    {
        option_num /= 10; // Truncate
    }
    else if (c == '\r') // User pressed Enter key (PuTTY).
    {
        _putchar('\n');

        /* Make sure that valid callback function exists. */
        if (menu_options[option_num - 1].cb != NULL)
        {
            uint8_t err = menu_options[option_num - 1].cb();
            if (err)
            {
                Command_Transmit("Error calling option\r\n", 50);
            }
        }
        else
        {
            Command_Transmit("Invalid option\r\n", 50);
        }

        Command_display_menu();
        option_num = 0;
    }
    else
    {
        /* Ignore all other characters */
    }
}

/**
 * @brief Get character from receive data register (non-blocking).
 * 
 * @return char Received character or 0 if nothing is placed in receive register.
 */
static char _getchar(void)
{
    if (uart_handle_ptr->Instance->ISR & USART_ISR_RXNE_Msk)
    {
        return (char)(uart_handle_ptr->Instance->RDR & 0xFF);
    }
    else
    {
        return 0;
    }
}

/**
 * @brief Get character from receive data register (blocking).
 * 
 * @return char Received character or 0 if nothing is placed in receive register.
 */
static char _getcharb(void)
{
    while (!(uart_handle_ptr->Instance->ISR & USART_ISR_RXNE_Msk))
    {
    }
    return (char)(uart_handle_ptr->Instance->RDR & 0xFF);
}
