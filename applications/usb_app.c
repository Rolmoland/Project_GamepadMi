/**
 * @file usb_app.c
 * @brief USB HID游戏手柄应用程序实现
 * @details 基于CherryUSB协议栈和RT-Thread RTOS实现的USB游戏手柄
 */

#include "usb_app.h"
#include <string.h>

/* ================ USB描述符定义 ================ */

/**
 * @brief USB完整描述符数组
 * @note 包含: 设备描述符、配置描述符、接口描述符、HID描述符、端点描述符、字符串描述符
 */
static const uint8_t hid_descriptor[] = {
    /* 设备描述符 (18字节) */
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00,
                               USBD_VID, USBD_PID,
                               0x0100, 0x01),

    /* 配置描述符 (9字节) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_DESC_SIZ,
                               0x01, 0x01,
                               USB_CONFIG_BUS_POWERED,
                               USBD_MAX_POWER),

    /* 接口描述符 (9字节) */
    0x09,                          /* bLength: 接口描述符大小 */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType: 接口描述符 */
    0x00,                          /* bInterfaceNumber: 接口编号 */
    0x00,                          /* bAlternateSetting: 备用设置 */
    0x01,                          /* bNumEndpoints: 端点数量 */
    0x03,                          /* bInterfaceClass: HID类 */
    0x00,                          /* bInterfaceSubClass: 无子类(非boot) */
    0x00,                          /* nInterfaceProtocol: 无协议(游戏手柄) */
    0x00,                          /* iInterface: 接口字符串索引 */

    /* HID描述符 (9字节) */
    0x09,                    /* bLength: HID描述符大小 */
    HID_DESCRIPTOR_TYPE_HID, /* bDescriptorType: HID */
    0x11,                    /* bcdHID: HID Class Spec release number */
    0x01,
    0x00,                          /* bCountryCode: 硬件目标国家 */
    0x01,                          /* bNumDescriptors: HID类描述符数量 */
    0x22,                          /* bDescriptorType: 报告描述符 */
    HID_GAMEPAD_REPORT_DESC_SIZE,  /* wItemLength: 报告描述符长度 低字节 */
    0x00,                          /* wItemLength: 报告描述符长度 高字节 */

    /* 端点描述符 (7字节) */
    0x07,                         /* bLength: 端点描述符大小 */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: 端点描述符 */
    HID_INT_EP,                   /* bEndpointAddress: 端点地址(IN) */
    0x03,                         /* bmAttributes: 中断传输 */
    HID_INT_EP_SIZE,              /* wMaxPacketSize: 最大包大小 低字节 */
    0x00,                         /* wMaxPacketSize: 最大包大小 高字节 */
    HID_INT_EP_INTERVAL,          /* bInterval: 轮询间隔 */

    /* 字符串描述符0 - 语言ID (4字节) */
    USB_LANGID_INIT(USBD_LANGID_STRING),

    /* 字符串描述符1 - 制造商 */
    0x22,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'M', 0x00,                  /* M */
    'i', 0x00,                  /* i */
    'c', 0x00,                  /* c */
    'h', 0x00,                  /* h */
    'u', 0x00,                  /* u */
    ' ', 0x00,                  /* (space) */
    'E', 0x00,                  /* E */
    'l', 0x00,                  /* l */
    'e', 0x00,                  /* e */
    'c', 0x00,                  /* c */
    't', 0x00,                  /* t */
    'r', 0x00,                  /* r */
    'o', 0x00,                  /* o */
    'n', 0x00,                  /* n */
    'i', 0x00,                  /* i */
    'c', 0x00,                  /* c */

    /* 字符串描述符2 - 产品名称 (15字符: 2 + 15*2 = 32 = 0x20) */
    0x20,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'U', 0x00,
    'S', 0x00,
    'B', 0x00,
    ' ', 0x00,
    'G', 0x00,
    'a', 0x00,
    'm', 0x00,
    'e', 0x00,
    'p', 0x00,
    'a', 0x00,
    'd', 0x00,
    ' ', 0x00,
    'H', 0x00,
    'I', 0x00,
    'D', 0x00,

    /* 字符串描述符3 - 序列号 */
    0x18,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'M', 0x00,                  /* M */
    'C', 0x00,                  /* C */
    'X', 0x00,                  /* X */
    'A', 0x00,                  /* A */
    '1', 0x00,                  /* 1 */
    '5', 0x00,                  /* 5 */
    '6', 0x00,                  /* 6 */
    '-', 0x00,                  /* - */
    '0', 0x00,                  /* 0 */
    '0', 0x00,                  /* 0 */
    '1', 0x00,                  /* 1 */

#ifdef CONFIG_USB_HS
    /* 设备限定符描述符 (仅用于高速USB) */
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x00,
    0x00,
#endif
    0x00
};

/**
 * @brief HID游戏手柄报告描述符
 * @details 定义游戏手柄的输入报告格式:
 *          - 16个按钮 (2字节)
 *          - 4个轴: 左右摇杆X/Y (4字节)
 *          - 2个扳机: 左右扳机 (2字节)
 *          - 1个Hat Switch: 方向键 (1字节)
 *          总计: 9字节
 */
static const uint8_t hid_gamepad_report_desc[HID_GAMEPAD_REPORT_DESC_SIZE] = {
    0x05, 0x01,        /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x05,        /* USAGE (Game Pad) */
    0xA1, 0x01,        /* COLLECTION (Application) */

    /* 按钮定义 - 16个按钮 */
    0x05, 0x09,        /*   USAGE_PAGE (Button) */
    0x19, 0x01,        /*   USAGE_MINIMUM (Button 1) */
    0x29, 0x10,        /*   USAGE_MAXIMUM (Button 16) */
    0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
    0x25, 0x01,        /*   LOGICAL_MAXIMUM (1) */
    0x75, 0x01,        /*   REPORT_SIZE (1) */
    0x95, 0x10,        /*   REPORT_COUNT (16) */
    0x81, 0x02,        /*   INPUT (Data,Var,Abs) */

    /* 摇杆轴定义 - 左摇杆X/Y, 右摇杆X/Y */
    0x05, 0x01,        /*   USAGE_PAGE (Generic Desktop) */
    0x09, 0x30,        /*   USAGE (X) */
    0x09, 0x31,        /*   USAGE (Y) */
    0x09, 0x32,        /*   USAGE (Z) */
    0x09, 0x35,        /*   USAGE (Rz) */
    0x15, 0x81,        /*   LOGICAL_MINIMUM (-127) */
    0x25, 0x7F,        /*   LOGICAL_MAXIMUM (127) */
    0x75, 0x08,        /*   REPORT_SIZE (8) */
    0x95, 0x04,        /*   REPORT_COUNT (4) */
    0x81, 0x02,        /*   INPUT (Data,Var,Abs) */

    /* 扳机定义 - 左扳机(LT), 右扳机(RT) */
    0x05, 0x02,        /*   USAGE_PAGE (Simulation Controls) */
    0x09, 0xC4,        /*   USAGE (Accelerator) */
    0x09, 0xC5,        /*   USAGE (Brake) */
    0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
    0x26, 0xFF, 0x00,  /*   LOGICAL_MAXIMUM (255) */
    0x75, 0x08,        /*   REPORT_SIZE (8) */
    0x95, 0x02,        /*   REPORT_COUNT (2) */
    0x81, 0x02,        /*   INPUT (Data,Var,Abs) */

    /* Hat Switch - 8位，0=居中(Null)，1-8=方向 */
    0x05, 0x01,        /*   USAGE_PAGE (Generic Desktop) */
    0x09, 0x39,        /*   USAGE (Hat switch) */
    0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
    0x25, 0x07,        /*   LOGICAL_MAXIMUM (7) */
    0x35, 0x00,        /*   PHYSICAL_MINIMUM (0) */
    0x46, 0x3B, 0x01,  /*   PHYSICAL_MAXIMUM (315) */
    0x65, 0x14,        /*   UNIT (Eng Rot:Angular Pos) */
    0x75, 0x08,        /*   REPORT_SIZE (8) */
    0x95, 0x01,        /*   REPORT_COUNT (1) */
    0x81, 0x42,        /*   INPUT (Data,Var,Abs,Null) */

    /* 重置UNIT */
    0x65, 0x00,        /*   UNIT (None) */

    0xC0               /* END_COLLECTION */
};

/* ================ 全局变量 ================ */

/* HID状态标志 */
#define HID_STATE_IDLE 0
#define HID_STATE_BUSY 1
static volatile uint8_t hid_state = HID_STATE_IDLE;

/* 游戏手柄报告数据缓冲区 */
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX usb_gamepad_report_t gamepad_report;

/* USB接口对象 */
static struct usbd_interface intf0;

/* ================ 内部函数实现 ================ */

/**
 * @brief USB设备事件处理回调函数
 */
static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
        case USBD_EVENT_RESET:
            rt_kprintf("[USB] Device Reset\n");
            break;

        case USBD_EVENT_CONNECTED:
            rt_kprintf("[USB] Device Connected\n");
            break;

        case USBD_EVENT_DISCONNECTED:
            rt_kprintf("[USB] Device Disconnected\n");
            hid_state = HID_STATE_IDLE;
            break;

        case USBD_EVENT_RESUME:
            rt_kprintf("[USB] Device Resume\n");
            break;

        case USBD_EVENT_SUSPEND:
            rt_kprintf("[USB] Device Suspend\n");
            break;

        case USBD_EVENT_CONFIGURED:
            rt_kprintf("[USB] Device Configured - Gamepad Ready!\n");
            hid_state = HID_STATE_IDLE;
            break;

        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;

        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

/* HID中断端点发送完成回调函数 */
void usbd_hid_int_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    (void)nbytes;

    /* 数据发送完成，恢复空闲状态 */
    hid_state = HID_STATE_IDLE;
}

/* HID IN端点定义 */
static struct usbd_endpoint hid_in_ep = {
    .ep_cb = usbd_hid_int_callback,
    .ep_addr = HID_INT_EP
};

/* ================ 公共API实现 ================ */

/* 初始化USB HID游戏手柄 */
void hid_gamepad_init(uint8_t busid, uintptr_t reg_base)
{
    rt_kprintf("[USB] Initializing HID Gamepad...\n");

    /* 注册USB描述符 */
    usbd_desc_register(busid, hid_descriptor);

    /* 添加HID接口 */
    usbd_add_interface(busid, usbd_hid_init_intf(busid, &intf0,
                                                  hid_gamepad_report_desc,
                                                  HID_GAMEPAD_REPORT_DESC_SIZE));

    /* 添加HID中断IN端点 */
    usbd_add_endpoint(busid, &hid_in_ep);

    /* 初始化USB设备 */
    usbd_initialize(busid, reg_base, usbd_event_handler);

    /* 初始化游戏手柄报告数据为中立状态 */
    memset(&gamepad_report, 0, sizeof(gamepad_report));
    gamepad_report.hat = GAMEPAD_HAT_CENTER;  /* 方向键居中 */

    rt_kprintf("[USB] HID Gamepad initialized successfully\n");
    rt_kprintf("[USB] VID:0x%04X PID:0x%04X\n", USBD_VID, USBD_PID);
}

/* 发送游戏手柄报告数据 */
int hid_gamepad_send_report(uint8_t busid, const usb_gamepad_report_t *report)
{
    /* 检查设备是否已配置 */
    if (!usb_device_is_configured(busid)) {
        return -1;  /* 设备未配置 */
    }

    /* 检查HID是否忙碌 */
    if (hid_state == HID_STATE_BUSY) {
        return -2;  /* 设备忙碌 */
    }

    /* 复制报告数据到缓冲区 */
    if (report != NULL && report != &gamepad_report) {
        memcpy(&gamepad_report, report, sizeof(usb_gamepad_report_t));
    }

    /* 设置忙碌状态 */
    hid_state = HID_STATE_BUSY;

    /* 通过中断端点发送数据 */
    int ret = usbd_ep_start_write(busid, HID_INT_EP,
                                   (uint8_t *)&gamepad_report,
                                   sizeof(usb_gamepad_report_t));

    if (ret < 0) {
        hid_state = HID_STATE_IDLE;
        return -3;  /* 发送失败 */
    }

    return 0;  /* 成功 */
}

/* 获取游戏手柄报告缓冲区 */
usb_gamepad_report_t* hid_gamepad_get_report(void)
{
    return &gamepad_report;
}

/* 检查USB设备是否已配置 */
bool hid_gamepad_is_configured(uint8_t busid)
{
    return usb_device_is_configured(busid);
}

/* 测试函数 - 模拟摇杆旋转和按钮按下 */
void hid_gamepad_test(uint8_t busid)
{
    if (!usb_device_is_configured(busid)) {
        rt_kprintf("[USB] Device not configured, waiting...\n");
        return;
    }

    rt_kprintf("[USB] Starting gamepad test...\n");

    /* 测试参数 */
    #define TEST_STEPS  100

    for (int i = 0; i < TEST_STEPS; i++) {
        /* 模拟左摇杆运动 */
        gamepad_report.left_x = (int8_t)((i * 127) / TEST_STEPS);
        gamepad_report.left_y = (int8_t)((i * 127) / TEST_STEPS);

        /* 右摇杆反向运动 */
        gamepad_report.right_x = -gamepad_report.left_x;
        gamepad_report.right_y = -gamepad_report.left_y;

        /* 模拟扳机按压(周期性变化) */
        gamepad_report.left_trigger = (uint8_t)((i * 255) / TEST_STEPS);
        gamepad_report.right_trigger = (uint8_t)(255 - ((i * 255) / TEST_STEPS));

        /* 循环按下不同按钮 */
        gamepad_report.buttons = (uint16_t)(1 << (i % 16));

        /* 循环切换方向键 */
        gamepad_report.hat = i % 9;

        /* 发送报告 */
        hid_gamepad_send_report(busid, NULL);

        /* 等待发送完成 */
        while (hid_state == HID_STATE_BUSY) {
            rt_thread_mdelay(1);
        }

        /* 控制测试速度 */
        rt_thread_mdelay(50);

        /* 每完成20%打印提示 */
        if (i % 20 == 0) {
            rt_kprintf("[USB] Test progress: %d%%\n", i * 100 / TEST_STEPS);
        }
    }

    /* 测试完成，复位为中立状态 */
    memset(&gamepad_report, 0, sizeof(gamepad_report));
    gamepad_report.hat = GAMEPAD_HAT_CENTER;
    hid_gamepad_send_report(busid, NULL);

    rt_kprintf("[USB] Gamepad test completed!\n");
}
