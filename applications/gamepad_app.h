/**
 * @file gamepad_app.h
 * @brief 游戏手柄应用层 - 按键到USB HID映射
 * @details 协调key_app(硬件层)和usb_app(协议层),实现按键到手柄按钮的映射
 */

#ifndef __GAMEPAD_APP_H__
#define __GAMEPAD_APP_H__

#include <rtthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================ 配置参数 ================ */

#define GAMEPAD_SCAN_INTERVAL_MS  10   /* 按键扫描间隔(ms) */
#define GAMEPAD_USB_BUS_ID        0    /* USB总线ID */

/* ================ 按键映射定义 ================ */

/**
 * @brief 按键映射表
 * @note 4x4矩阵键盘(0-15)直接映射到手柄按钮(bit0-bit15)
 *
 * 按键布局示例:
 *     C1    C2    C3    C4
 * R1  [0]   [1]   [2]   [3]    -> Button0-3
 * R2  [4]   [5]   [6]   [7]    -> Button4-7
 * R3  [8]   [9]   [10]  [11]   -> Button8-11
 * R4  [12]  [13]  [14]  [15]   -> Button12-15
 */

/* 可根据实际手柄布局定义按键别名 */
/* 矩阵按键0-13可用，14和15保留给摇杆按键 */
#define KEY_A      0   /* 对应按键0 */
#define KEY_B      1   /* 对应按键1 */
#define KEY_X      2   /* 对应按键2 */
#define KEY_Y      3   /* 对应按键3 */
#define KEY_LB     4   /* 对应按键4 - 左肩键 */
#define KEY_RB     5   /* 对应按键5 - 右肩键 */
#define KEY_BACK   6   /* 对应按键6 */
#define KEY_START  7   /* 对应按键7 */
/* 矩阵按键8-13可自由分配 */
/* bit14 = 左摇杆按键(LS), bit15 = 右摇杆按键(RS) - 由摇杆硬件控制 */

/* ================ 公共API ================ */

/**
 * @brief 启动游戏手柄应用线程
 * @return 0表示成功,其他值表示失败
 * @note 此函数会自动被RT-Thread初始化框架调用
 */
int gamepad_app_start(void);

/**
 * @brief 检查游戏手柄是否就绪
 * @return true表示USB已配置,可以发送数据; false表示未就绪
 */
bool gamepad_is_ready(void);

/**
 * @brief 获取当前按键状态
 * @return 16位按钮状态,bit0-bit15对应按键0-15
 */
uint16_t gamepad_get_buttons(void);

#ifdef __cplusplus
}
#endif

#endif /* __GAMEPAD_APP_H__ */
