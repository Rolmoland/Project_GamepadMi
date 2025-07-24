#include "servo_uart_driver.h"

static 

void servo_init(void)
{
    HAL_UART_Receive_DMA(&huart1, (uint8_t *)rxCmd, CMD_LEN); // 开启DMA接收模式
}