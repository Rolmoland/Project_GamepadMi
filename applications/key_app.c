#include "key_app.h"

// C（column）：列  主动驱动低电平进行扫描
// R（row）   ：行  默认高电平，按键按下时被拉低
#define KEY_C1   ((2*32)+3)
#define KEY_C2   ((2*32)+4)
#define KEY_C3   ((2*32)+5)
#define KEY_C4   ((2*32)+6)
#define KEY_R1   ((3*32)+17)
#define KEY_R2   ((3*32)+16)
#define KEY_R3   ((3*32)+15)
#define KEY_R4   ((3*32)+14)

/* 初始化函数 */
static int key_init(void)
{
	/* 列引脚配置为输出模式 */
	rt_pin_mode(KEY_C1, PIN_MODE_OUTPUT);
	rt_pin_mode(KEY_C2, PIN_MODE_OUTPUT);
	rt_pin_mode(KEY_C3, PIN_MODE_OUTPUT);
	rt_pin_mode(KEY_C4, PIN_MODE_OUTPUT);

	/* 行引脚配置为输入上拉模式 */
	rt_pin_mode(KEY_R1, PIN_MODE_INPUT_PULLUP);
	rt_pin_mode(KEY_R2, PIN_MODE_INPUT_PULLUP);
	rt_pin_mode(KEY_R3, PIN_MODE_INPUT_PULLUP);
	rt_pin_mode(KEY_R4, PIN_MODE_INPUT_PULLUP);

	/* 初始化列引脚为高电平 */
	rt_pin_write(KEY_C1, PIN_HIGH);
	rt_pin_write(KEY_C2, PIN_HIGH);
	rt_pin_write(KEY_C3, PIN_HIGH);
	rt_pin_write(KEY_C4, PIN_HIGH);

	rt_kprintf("KEY OK\r\n");

	return 0;
}
INIT_DEVICE_EXPORT(key_init);

/* 引脚数组 */
static const rt_base_t col_pins[4] = {KEY_C1, KEY_C2, KEY_C3, KEY_C4};
static const rt_base_t row_pins[4] = {KEY_R1, KEY_R2, KEY_R3, KEY_R4};

/* 读取按键状态 */
rt_uint8_t key_read(void)
{
	rt_uint8_t temp = 0xFF;  /* 0xFF表示无按键，0-15表示按键索引 */

	/* 逐列扫描 */
	for (rt_uint8_t col = 0; col < 4; col++)
	{
		/* 设置当前列为低电平，其他列为高电平 */
		for (rt_uint8_t i = 0; i < 4; i++)
		{
			rt_pin_write(col_pins[i], (i == col) ? PIN_LOW : PIN_HIGH);
		}

		/* 短暂延时，等待信号稳定 */
		rt_hw_us_delay(5);

		/* 检测所有行 */
		for (rt_uint8_t row = 0; row < 4; row++)
		{
			if (rt_pin_read(row_pins[row]) == PIN_LOW)
			{
				temp = col * 4 + row;
				goto scan_done;  /* 找到按键，立即退出扫描 */
			}
		}
	}

scan_done:
	/* 恢复所有列为高电平 */
	for (rt_uint8_t i = 0; i < 4; i++)
	{
		rt_pin_write(col_pins[i], PIN_HIGH);
	}

	return temp;
}