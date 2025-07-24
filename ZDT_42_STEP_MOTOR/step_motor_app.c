#include "step_motor_app.h"

void step_motor_app_init(void)
{
    // 1. 初始化步进电机硬件接口和参数
    Step_Motor_Init();

    // 2. (可选) 复位电机驱动器的当前位置计数，将其设置为零点
    //    这对于需要精确相对位置控制的场景非常有用。
    //    第一个参数是UART句柄，第二个是驱动器地址（在step_motor_bsp.h中定义）
    Emm_V5_Reset_CurPos_To_Zero(&MOTOR_X_UART, STEP_MOTOR_X_ADDR); // 复位X轴电机
    Emm_V5_Reset_CurPos_To_Zero(&MOTOR_Y_UART, STEP_MOTOR_Y_ADDR); // 复位Y轴电机
}

// 让X轴电机以50%的最大速度正转
Step_Motor_Set_Speed(STEP_MOTOR_X, 50);

// 让Y轴电机以30%的最大速度反转
Step_Motor_Set_Speed(STEP_MOTOR_Y, -30);

// 让电机停止
Step_Motor_Set_Speed(STEP_MOTOR_X, 0);