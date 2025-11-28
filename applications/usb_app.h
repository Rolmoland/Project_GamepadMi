/**
 * @file usb_app.h
 * @brief USB HID游戏手柄应用程序头文件
 * @details 基于CherryUSB协议栈和RT-Thread RTOS实现的USB游戏手柄
 */

#ifndef __USB_APP_H__
#define __USB_APP_H__

#include <usbd_core.h>
#include <usbd_hid.h>
#include <rtthread.h>
#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================ USB设备配置 ================ */

/* USB设备描述符配置 */
#define USBD_VID           0x045E  /* 厂商ID (Microsoft) */
#define USBD_PID           0x02FF  /* 产品ID (通用游戏手柄) */
#define USBD_MAX_POWER     100     /* 最大功耗 100mA */
#define USBD_LANGID_STRING 1033    /* 语言ID: 英语(美国) */

/* USB端点配置 */
#define HID_INT_EP          0x81   /* IN端点地址 */
#define HID_INT_EP_SIZE     9      /* 端点大小(匹配报告大小) */
#define HID_INT_EP_INTERVAL 1      /* 轮询间隔(1ms,适合游戏手柄) */

/* USB描述符大小 */
#define USB_HID_CONFIG_DESC_SIZ       34
#define HID_GAMEPAD_REPORT_DESC_SIZE  83

/* ================ 游戏手柄数据结构 ================ */

/**
 * @brief 游戏手柄报告数据结构
 * @note 总大小: 9字节
 */
typedef struct __attribute__((packed)) {
    uint16_t buttons;      /* 16个按钮 (bit0-bit15) */
    int8_t left_x;         /* 左摇杆 X轴 (-127 to 127) */
    int8_t left_y;         /* 左摇杆 Y轴 (-127 to 127) */
    int8_t right_x;        /* 右摇杆 X轴 (-127 to 127) */
    int8_t right_y;        /* 右摇杆 Y轴 (-127 to 127) */
    uint8_t left_trigger;  /* 左扳机 (0-255) */
    uint8_t right_trigger; /* 右扳机 (0-255) */
    uint8_t hat;           /* 方向键/Hat Switch (0-8, 8=center) */
} usb_gamepad_report_t;

/* ================ 按钮位定义 ================ */

#define GAMEPAD_BUTTON_A      (1 << 0)   /* A按钮 */
#define GAMEPAD_BUTTON_B      (1 << 1)   /* B按钮 */
#define GAMEPAD_BUTTON_X      (1 << 2)   /* X按钮 */
#define GAMEPAD_BUTTON_Y      (1 << 3)   /* Y按钮 */
#define GAMEPAD_BUTTON_LB     (1 << 4)   /* 左肩键 */
#define GAMEPAD_BUTTON_RB     (1 << 5)   /* 右肩键 */
#define GAMEPAD_BUTTON_BACK   (1 << 6)   /* Back/Select按钮 */
#define GAMEPAD_BUTTON_START  (1 << 7)   /* Start按钮 */
#define GAMEPAD_BUTTON_8      (1 << 8)   /* 矩阵按键8 */
#define GAMEPAD_BUTTON_9      (1 << 9)   /* 矩阵按键9 */
#define GAMEPAD_BUTTON_10     (1 << 10)  /* 矩阵按键10 */
#define GAMEPAD_BUTTON_11     (1 << 11)  /* 矩阵按键11 */
#define GAMEPAD_BUTTON_12     (1 << 12)  /* 矩阵按键12 */
#define GAMEPAD_BUTTON_13     (1 << 13)  /* 矩阵按键13 */
#define GAMEPAD_BUTTON_LS     (1 << 14)  /* 左摇杆按键 (bit14) */
#define GAMEPAD_BUTTON_RS     (1 << 15)  /* 右摇杆按键 (bit15) */

/* ================ Hat Switch (D-Pad) 方向定义 ================ */
/* 标准Hat Switch: 0-7为8个方向(45度间隔), 8=居中/释放(Null) */

#define GAMEPAD_HAT_UP         0
#define GAMEPAD_HAT_UP_RIGHT   1
#define GAMEPAD_HAT_RIGHT      2
#define GAMEPAD_HAT_DOWN_RIGHT 3
#define GAMEPAD_HAT_DOWN       4
#define GAMEPAD_HAT_DOWN_LEFT  5
#define GAMEPAD_HAT_LEFT       6
#define GAMEPAD_HAT_UP_LEFT    7
#define GAMEPAD_HAT_CENTER     8   /* Null state - 超出LOGICAL_MAX */

/* ================ 公共API ================ */

/**
 * @brief 初始化USB HID游戏手柄
 * @param busid USB总线ID (通常为0)
 * @param reg_base USB控制器寄存器基地址
 */
void hid_gamepad_init(uint8_t busid, uintptr_t reg_base);

/**
 * @brief 发送游戏手柄报告数据
 * @param busid USB总线ID
 * @param report 游戏手柄报告数据指针 (NULL则使用内部缓冲区)
 * @return 0表示成功，-1表示设备未配置，-2表示设备忙，-3表示发送失败
 */
int hid_gamepad_send_report(uint8_t busid, const usb_gamepad_report_t *report);

/**
 * @brief 获取游戏手柄报告缓冲区（可直接修改）
 * @return 游戏手柄报告数据指针
 */
usb_gamepad_report_t* hid_gamepad_get_report(void);

/**
 * @brief 检查USB设备是否已配置
 * @param busid USB总线ID
 * @return true表示已配置，false表示未配置
 */
bool hid_gamepad_is_configured(uint8_t busid);

/**
 * @brief 测试函数 - 模拟摇杆旋转和按钮按下
 * @param busid USB总线ID
 */
void hid_gamepad_test(uint8_t busid);

#ifdef __cplusplus
}
#endif

#endif /* __USB_APP_H__ */
