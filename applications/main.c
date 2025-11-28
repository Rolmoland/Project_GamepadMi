#include <rtthread.h>
#include "bsp_system.h"

int main(void)
{
    rt_kprintf("System Start\r\n");
    /* gamepad_app 通过 INIT_APP_EXPORT 自动启动 */
    return 0;
}
