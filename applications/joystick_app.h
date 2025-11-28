/**
 * @file joystick_app.h
 * @brief 双摇杆数据读取
 */

#ifndef __JOYSTICK_APP_H__
#define __JOYSTICK_APP_H__

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 摇杆数据结构 */
typedef struct {
    int16_t x;      /* X轴: -32768 ~ 32767 */
    int16_t y;      /* Y轴: -32768 ~ 32767 */
    bool btn;       /* 按键: true=按下 */
} joystick_data_t;

/**
 * @brief 读取左摇杆数据
 * @param data 输出数据
 * @return RT_EOK成功
 */
rt_err_t joystick_left_read(joystick_data_t *data);

/**
 * @brief 读取右摇杆数据
 * @param data 输出数据
 * @return RT_EOK成功
 */
rt_err_t joystick_right_read(joystick_data_t *data);

/**
 * @brief 读取原始ADC值(调试用)
 * @param left_x  左摇杆X轴原始值
 * @param left_y  左摇杆Y轴原始值
 * @param right_x 右摇杆X轴原始值
 * @param right_y 右摇杆Y轴原始值
 */
void joystick_read_raw(uint32_t *left_x, uint32_t *left_y,
                       uint32_t *right_x, uint32_t *right_y);

#ifdef __cplusplus
}
#endif

#endif /* __JOYSTICK_APP_H__ */
