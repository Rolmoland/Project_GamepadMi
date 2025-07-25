#ifndef __STEP_MOTOR_APP_H__
#define __STEP_MOTOR_APP_H__

#include "bsp_system.h"
#include "ringbuffer.h"

#define STEP_MOTOR_BUFFER_SIZE 64

extern Emm_V5_Response_t resp_x;
extern Emm_V5_Response_t resp_y;
extern struct rt_ringbuffer x_ringbuffer;
extern struct rt_ringbuffer y_ringbuffer;

extern uint8_t x_uart_rx_dma_buffer[STEP_MOTOR_BUFFER_SIZE];
extern uint8_t y_uart_rx_dma_buffer[STEP_MOTOR_BUFFER_SIZE];
extern uint8_t x_dma_buffer_ringbuffer_pool[STEP_MOTOR_BUFFER_SIZE];
extern uint8_t y_dma_buffer_ringbuffer_pool[STEP_MOTOR_BUFFER_SIZE];
extern uint8_t x_ringbuffer_pool_parse_buffer[STEP_MOTOR_BUFFER_SIZE];
extern uint8_t y_ringbuffer_pool_parse_buffer[STEP_MOTOR_BUFFER_SIZE];

void step_motor_app_init(void);
void step_motor_app_uart_init(void);
void step_motor_callback_x(void);
void step_motor_callback_y(void);
void step_motor_task(void);

#endif

