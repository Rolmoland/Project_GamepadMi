/**
 * @file joystick_app.c
 * @brief 双摇杆数据读取
 */

#include "joystick_app.h"
#include <rtdevice.h>

/* ================ 硬件配置 ================ */

/* ADC设备名称 */
#define ADC_DEV_NAME    "adc0"

/* ADC通道分配 */
#define LEFT_X_CHANNEL   0   /* 左摇杆X轴 - ADC0_CH0 */
#define LEFT_Y_CHANNEL   1   /* 左摇杆Y轴 - ADC0_CH1 */
#define RIGHT_X_CHANNEL  8   /* 右摇杆X轴 - ADC0_CH8 */
#define RIGHT_Y_CHANNEL  13  /* 右摇杆Y轴 - ADC0_CH13 */

/* 摇杆按键GPIO (Port*32 + Pin) */
#define LEFT_BTN_PIN     ((3*32)+7)   /* 左摇杆按键 - P3_7 */
#define RIGHT_BTN_PIN    ((3*32)+6)   /* 右摇杆按键 - P3_6 */

/* ADC参数 */
#define ADC_RESOLUTION   16           /* 16位分辨率 */
#define ADC_MAX_VALUE    65535        /* 2^16 - 1 */
#define ADC_MID_VALUE    32768        /* 中心值 */

/* ================ 内部变量 ================ */

static rt_adc_device_t adc_dev = RT_NULL;

/* ================ 初始化 ================ */

static int joystick_init(void)
{
    /* 先配置按键引脚（即使ADC失败，按键也要能用） */
    rt_pin_mode(LEFT_BTN_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(RIGHT_BTN_PIN, PIN_MODE_INPUT_PULLUP);

    /* 查找ADC设备 */
    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("joystick: ADC device %s not found (buttons still work)\n", ADC_DEV_NAME);
        return RT_EOK;  /* 不返回错误，让系统继续运行 */
    }

    /* 使能ADC通道 */
    rt_adc_enable(adc_dev, LEFT_X_CHANNEL);
    rt_adc_enable(adc_dev, LEFT_Y_CHANNEL);
    rt_adc_enable(adc_dev, RIGHT_X_CHANNEL);
    rt_adc_enable(adc_dev, RIGHT_Y_CHANNEL);

    rt_kprintf("joystick: init OK\n");
    return RT_EOK;
}
INIT_DEVICE_EXPORT(joystick_init);

/* ================ ADC读取 ================ */

static uint32_t adc_read_channel(uint8_t channel)
{
    if (adc_dev == RT_NULL)
        return ADC_MID_VALUE;

    return rt_adc_read(adc_dev, channel);
}

/* 将ADC值转换为有符号轴值 (-32768 ~ 32767) */
static int16_t adc_to_axis(uint32_t adc_val)
{
    if (adc_val > ADC_MAX_VALUE)
        adc_val = ADC_MAX_VALUE;

    return (int16_t)(adc_val - ADC_MID_VALUE);
}

/* ================ 公共API ================ */

/* 读取左摇杆数据 */
rt_err_t joystick_left_read(joystick_data_t *data)
{
    if (data == RT_NULL)
        return -RT_EINVAL;

    data->x = adc_to_axis(adc_read_channel(LEFT_X_CHANNEL));
    data->y = adc_to_axis(adc_read_channel(LEFT_Y_CHANNEL));
    data->btn = (rt_pin_read(LEFT_BTN_PIN) == PIN_LOW);

    return RT_EOK;
}

/* 读取右摇杆数据 */
rt_err_t joystick_right_read(joystick_data_t *data)
{
    if (data == RT_NULL)
        return -RT_EINVAL;

    data->x = adc_to_axis(adc_read_channel(RIGHT_X_CHANNEL));
    data->y = adc_to_axis(adc_read_channel(RIGHT_Y_CHANNEL));
    data->btn = (rt_pin_read(RIGHT_BTN_PIN) == PIN_LOW);

    return RT_EOK;
}

/* 读取原始ADC值(调试用) */
void joystick_read_raw(uint32_t *left_x, uint32_t *left_y,
                       uint32_t *right_x, uint32_t *right_y)
{
    if (left_x)  *left_x  = adc_read_channel(LEFT_X_CHANNEL);
    if (left_y)  *left_y  = adc_read_channel(LEFT_Y_CHANNEL);
    if (right_x) *right_x = adc_read_channel(RIGHT_X_CHANNEL);
    if (right_y) *right_y = adc_read_channel(RIGHT_Y_CHANNEL);
}
