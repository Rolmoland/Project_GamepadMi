/**
 * @file gamepad_app.c
 * @brief 游戏手柄应用层实现
 * @details 整合矩阵按键和双摇杆，映射到USB HID手柄报告并发送
 */

#include "gamepad_app.h"
#include "key_app.h"
#include "joystick_app.h"
#include "usb_app.h"
#include <rtthread.h>

/* ================ 死区配置 ================ */

#define JOYSTICK_DEADZONE     2000   /* 死区阈值 (原始值，约6%) */
#define AXIS_CHANGE_THRESHOLD 500    /* 轴变化阈值，低于此值不触发更新 */

/* ================ 内部函数 ================ */

/* 应用死区，消除中心抖动 */
static int16_t apply_deadzone(int16_t value)
{
    if (value > -JOYSTICK_DEADZONE && value < JOYSTICK_DEADZONE)
        return 0;
    return value;
}

/* 将int16_t (-32768~32767) 转换为 int8_t (-127~127) */
static int8_t scale_axis(int16_t value)
{
    int32_t scaled = ((int32_t)value * 127) / 32768;
    if (scaled > 127) scaled = 127;
    if (scaled < -127) scaled = -127;
    return (int8_t)scaled;
}

/* 检测轴值是否有显著变化 */
static bool axis_changed(int16_t new_val, int16_t old_val)
{
    int32_t diff = (int32_t)new_val - (int32_t)old_val;
    if (diff < 0) diff = -diff;
    return diff > AXIS_CHANGE_THRESHOLD;
}

/* ================ 全局变量 ================ */

static rt_thread_t gamepad_thread = RT_NULL;
static uint16_t current_buttons = 0;
static bool pending_send = false;  /* 有待发送的报告 */

/* 上一次状态，用于检测变化 */
static uint8_t last_key = 0xFF;
static joystick_data_t last_left = {0};
static joystick_data_t last_right = {0};

/* ================ 线程入口 ================ */

static void gamepad_thread_entry(void *parameter)
{
    usb_gamepad_report_t *report;
    uint8_t key_index;
    joystick_data_t left, right;
    int16_t left_x, left_y, right_x, right_y;
    bool state_changed;
    int ret;

    rt_kprintf("[GAMEPAD] Thread started\n");

    while (1)
    {
        /* 读取矩阵按键 */
        key_index = key_read();

        /* 读取双摇杆并应用死区 */
        joystick_left_read(&left);
        joystick_right_read(&right);
        left_x = apply_deadzone(left.x);
        left_y = apply_deadzone(left.y);
        right_x = apply_deadzone(right.x);
        right_y = apply_deadzone(right.y);

        /* 检测是否有显著变化 */
        state_changed = (key_index != last_key) ||
                        (left.btn != last_left.btn) || (right.btn != last_right.btn) ||
                        axis_changed(left_x, apply_deadzone(last_left.x)) ||
                        axis_changed(left_y, apply_deadzone(last_left.y)) ||
                        axis_changed(right_x, apply_deadzone(last_right.x)) ||
                        axis_changed(right_y, apply_deadzone(last_right.y));

        if (state_changed || pending_send)
        {
            /* 获取USB报告缓冲区 */
            report = hid_gamepad_get_report();

            /* 矩阵按键映射到 bit0-13 (14/15保留给摇杆按键) */
            if (key_index != 0xFF && key_index < 14)
                current_buttons = (1 << key_index);
            else
                current_buttons = 0;

            /* 摇杆按键映射到 bit14(LS) 和 bit15(RS) */
            if (left.btn)
                current_buttons |= GAMEPAD_BUTTON_LS;
            if (right.btn)
                current_buttons |= GAMEPAD_BUTTON_RS;

            /* 更新报告 */
            report->buttons = current_buttons;
            report->left_x = scale_axis(left_x);
            report->left_y = scale_axis(left_y);
            report->right_x = scale_axis(right_x);
            report->right_y = scale_axis(right_y);
            report->left_trigger = 0;
            report->right_trigger = 0;
            report->hat = GAMEPAD_HAT_CENTER;

            /* 发送USB报告 */
            if (hid_gamepad_is_configured(GAMEPAD_USB_BUS_ID))
            {
                ret = hid_gamepad_send_report(GAMEPAD_USB_BUS_ID, NULL);
                if (ret == 0)
                {
                    /* 发送成功，保存状态 */
                    pending_send = false;
                    last_key = key_index;
                    last_left = left;
                    last_right = right;
                }
                else if (ret == -2)
                {
                    /* 设备忙，下次重试 */
                    pending_send = true;
                }
            }
        }

        rt_thread_mdelay(GAMEPAD_SCAN_INTERVAL_MS);
    }
}

/* ================ 公共API ================ */

/* 启动游戏手柄应用 */
int gamepad_app_start(void)
{
    gamepad_thread = rt_thread_create(
        "gamepad",
        gamepad_thread_entry,
        RT_NULL,
        2048,
        RT_THREAD_PRIORITY_MAX / 2,
        10
    );

    if (gamepad_thread == RT_NULL)
    {
        rt_kprintf("[GAMEPAD] Failed to create thread\n");
        return -1;
    }

    rt_thread_startup(gamepad_thread);
    rt_kprintf("[GAMEPAD] Started (interval: %dms)\n", GAMEPAD_SCAN_INTERVAL_MS);

    return 0;
}
INIT_APP_EXPORT(gamepad_app_start);

/* 检查游戏手柄是否准备好 */
bool gamepad_is_ready(void)
{
    return hid_gamepad_is_configured(GAMEPAD_USB_BUS_ID);
}

/* 获取当前按钮状态 */
uint16_t gamepad_get_buttons(void)
{
    return current_buttons;
}
