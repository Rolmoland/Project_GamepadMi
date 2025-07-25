#include "step_motor_app.h"

// 创建X轴和Y轴电机的UART句柄
Emm_V5_Response_t resp_x;
Emm_V5_Response_t resp_y;

// 创建X轴和Y轴的环形缓冲区数据结构
struct rt_ringbuffer x_ringbuffer;
struct rt_ringbuffer y_ringbuffer;

// X轴和Y轴串口部分所需的内存
// 串口寄存器到DMA缓冲区
uint8_t x_uart_rx_dma_buffer[STEP_MOTOR_BUFFER_SIZE] = {0};
uint8_t y_uart_rx_dma_buffer[STEP_MOTOR_BUFFER_SIZE] = {0};

// DMA缓冲区到环形缓冲区的内存池
uint8_t x_dma_buffer_ringbuffer_pool[STEP_MOTOR_BUFFER_SIZE] = {0};
uint8_t y_dma_buffer_ringbuffer_pool[STEP_MOTOR_BUFFER_SIZE] = {0};

// 环形缓冲区内存池到待处理数据的内存
uint8_t x_ringbuffer_pool_parse_buffer[STEP_MOTOR_BUFFER_SIZE] = {0};
uint8_t y_ringbuffer_pool_parse_buffer[STEP_MOTOR_BUFFER_SIZE] = {0};

// 步进电机初始化函数
void step_motor_app_init(void)
{
    // 1. 初始化步进电机硬件接口和参数
    Step_Motor_Init();

    // 2. (可选) 复位电机驱动器的当前位置计数，将其设置为零点
    //    这对于需要精确相对位置控制的场景非常有用。
    //    第一个参数是UART句柄，第二个是驱动器地址（在step_motor_bsp.h中定义）
    Emm_V5_Reset_CurPos_To_Zero(&MOTOR_X_UART, MOTOR_X_ADDR); // 复位X轴电机
    Emm_V5_Reset_CurPos_To_Zero(&MOTOR_Y_UART, MOTOR_Y_ADDR); // 复位Y轴电机
}

// 放在uart_app_init中，用于串口部分的初始化工作
void step_motor_app_uart_init(void)
{
    // 环形缓冲区初始化
    rt_ringbuffer_init(&x_ringbuffer, x_dma_buffer_ringbuffer_pool, STEP_MOTOR_BUFFER_SIZE);
    rt_ringbuffer_init(&y_ringbuffer, y_dma_buffer_ringbuffer_pool, STEP_MOTOR_BUFFER_SIZE);

    // 开启中断（串口使用配置好的，这里只是举例）
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, x_uart_rx_dma_buffer, STEP_MOTOR_BUFFER_SIZE); // 启动读取中断
	__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); // 关闭 DMA 的"半满中断"功能

    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, y_uart_rx_dma_buffer, STEP_MOTOR_BUFFER_SIZE); // 启动读取中断
    __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT); // 关闭 DMA 的"半满中断"功能
}

// 放在uart的回调函数中
void step_motor_callback_x(void)
{
    // 停止x轴的DMA接收
    HAL_UART_DMAStop(&huart1);

    // 从DMA缓冲区中取出数据
    rt_ringbuffer_put(&x_ringbuffer, x_uart_rx_dma_buffer, STEP_MOTOR_BUFFER_SIZE);

    // 清空DMA接收区
    memset(x_uart_rx_dma_buffer, 0, STEP_MOTOR_BUFFER_SIZE);

    // 重新启动DMA接收
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, x_uart_rx_dma_buffer, STEP_MOTOR_BUFFER_SIZE); // 启动读取中断
	__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); // 关闭 DMA 的"半满中断"功能
}

// 放在uart的回调函数中
void step_motor_callback_y(void)
{
    // 停止y轴的DMA接收
    HAL_UART_DMAStop(&huart2);

    // 从DMA缓冲区中取出数据
    rt_ringbuffer_put(&y_ringbuffer, y_uart_rx_dma_buffer, STEP_MOTOR_BUFFER_SIZE);

    // 清空DMA接收区
    memset(y_uart_rx_dma_buffer, 0, STEP_MOTOR_BUFFER_SIZE);

    // 重新启动DMA接收
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, y_uart_rx_dma_buffer, STEP_MOTOR_BUFFER_SIZE); // 启动读取中断
    __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT); // 关闭 DMA 的"半满中断"功能
}

// 步进电机任务函数(因为使用串口，实际需要放入uart_task中)
void step_motor_task(void)
{
    uint16_t length_x, length_y;

    // 处理X轴电机的UART接收数据(x轴的ringbuffer结构体指针)
    length_x = rt_ringbuffer_data_len(&x_ringbuffer);
    if (length_x > 0)
    {
        rt_ringbuffer_get(&x_ringbuffer, x_ringbuffer_pool_parse_buffer, length_x);
        x_ringbuffer_pool_parse_buffer[length_x] = '\0'; // 确保字符串结束

        // 解析X轴数据
        if (Emm_V5_Parse_Response(x_ringbuffer_pool_parse_buffer, length_x, &resp_x))
        {
            // 处理解析后的X轴数据
            Emm_V5_Handle_Response(&resp_x);
            // 串口打印X轴电机的地址，此处使用通用调试串口：串口1处理
            my_printf(&huart1, "id:%d\r\n",resp_x.addr);
        }
        else
        {
            my_printf(&huart1, "X轴数据解析错误\r\n");
        }

        // 清空解析缓冲区
        memset(x_ringbuffer_pool_parse_buffer, 0, length_x);
    }

    // 处理Y轴电机的UART接收数据(y轴的ringbuffer结构体指针)
    length_y = rt_ringbuffer_data_len(&y_ringbuffer);
    if (length_y > 0)
    {
        rt_ringbuffer_get(&y_ringbuffer, y_ringbuffer_pool_parse_buffer, length_y);
        y_ringbuffer_pool_parse_buffer[length_y] = '\0'; // 确保字符串结束

        // 解析Y轴数据
        if (Emm_V5_Parse_Response(y_ringbuffer_pool_parse_buffer, length_y, &resp_y))
        {
            // 处理解析后的Y轴数据
            Emm_V5_Handle_Response(&resp_y);
            // 串口打印Y轴电机的地址，此处使用通用调试串口：串口1处理
            my_printf(&huart1, "id:%d\r\n",resp_y.addr);
        }
        else
        {
            my_printf(&huart1, "Y轴数据解析错误\r\n");
        }

        // 清空解析缓冲区
        memset(y_ringbuffer_pool_parse_buffer, 0, length_y);
    }
}


