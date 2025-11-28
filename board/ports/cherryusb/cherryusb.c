/*
 * Copyright (c) 2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-01-17     Supperthomas first version
 * 2025-02-25     hydevcode
 */
#include <rtthread.h>
#include <board.h>

#ifdef RT_CHERRYUSB_DEVICE_HID
extern void hid_gamepad_init(uint8_t busid, uintptr_t reg_base);
extern void hid_gamepad_test(uint8_t busid);

static int rt_hw_mcxa156_cherryusb_hid_init(void)
{
	hid_gamepad_init(0, 0x400A4000u);
	return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_mcxa156_cherryusb_hid_init);

static int hid_example(int argc, char **argv)
{
	hid_gamepad_test(0);
	return 0;
}
MSH_CMD_EXPORT(hid_example, USB hid example);

#endif