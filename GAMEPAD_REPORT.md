# USB HID 游戏手柄技术报告

## 1. 项目概述

### 1.1 实现功能

本项目基于 NXP FRDM-MCXA156 开发板，实现了一个标准的 USB HID 游戏手柄设备，主要功能包括：

- **16 个数字按钮**：通过 4x4 矩阵键盘实现 14 个按钮 + 2 个摇杆按键
- **双摇杆输入**：左右两个模拟摇杆，各提供 X/Y 轴数据
- **USB HID 协议**：标准 HID Gamepad 设备，即插即用，无需驱动
- **实时响应**：10ms 扫描间隔，低延迟输入

### 1.2 技术特点

| 特性 | 描述 |
|------|------|
| MCU | NXP MCXA156 (ARM Cortex-M33, 96MHz) |
| RTOS | RT-Thread v5.x |
| USB 协议栈 | CherryUSB |
| HID 报告大小 | 9 字节 |
| 按键扫描频率 | 100Hz (10ms) |
| ADC 分辨率 | 16-bit |

---

## 2. RT-Thread 使用情况概述

### 2.1 内核配置

```c
#define RT_THREAD_PRIORITY_MAX  32      // 32 级优先级
#define RT_TICK_PER_SECOND      1000    // 1ms 系统节拍
#define RT_USING_TIMER_SOFT             // 软件定时器
#define RT_USING_SEMAPHORE              // 信号量
#define RT_USING_MUTEX                  // 互斥锁
#define RT_USING_MAILBOX                // 邮箱
```

### 2.2 使用的组件

| 组件 | 用途 |
|------|------|
| PIN 设备框架 | GPIO 控制（矩阵键盘、摇杆按键） |
| ADC 设备框架 | 摇杆模拟量读取 |
| CherryUSB | USB HID 设备实现 |
| FinSH | 调试命令行 |

### 2.3 线程设计

| 线程名 | 优先级 | 栈大小 | 功能 |
|--------|--------|--------|------|
| gamepad | 16 | 2048B | 主控制线程，扫描输入并发送 USB 报告 |
| tshell | 20 | 4096B | FinSH 命令行 |
| main | 10 | 2048B | 主线程 |

### 2.4 自动初始化

项目使用 RT-Thread 自动初始化机制：

```c
INIT_BOARD_EXPORT(rt_hw_adc_init);      // ADC 驱动初始化
INIT_DEVICE_EXPORT(key_init);            // 矩阵键盘初始化
INIT_DEVICE_EXPORT(joystick_init);       // 摇杆初始化
INIT_COMPONENT_EXPORT(cherryusb_init);   // USB 初始化
INIT_APP_EXPORT(gamepad_app_start);      // 应用层启动
```

---

## 3. 硬件框架

### 3.1 系统框图

```
┌─────────────────────────────────────────────────────────────┐
│                      FRDM-MCXA156                           │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │  4x4 矩阵   │  │  双摇杆模块  │  │     USB Device      │ │
│  │   键盘      │  │  (带按键)    │  │    (Full Speed)     │ │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘ │
│         │                │                     │            │
│    GPIO P2/P3       ADC0 CH0/1/8/13           USB0         │
│         │                │                     │            │
│  ┌──────┴────────────────┴─────────────────────┴──────────┐│
│  │                    MCXA156 MCU                          ││
│  │              (Cortex-M33 @ 96MHz)                       ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

### 3.2 引脚分配

#### 3.2.1 矩阵键盘 (4x4)

| 功能 | 引脚 | GPIO 编号 | 方向 |
|------|------|-----------|------|
| 列 C1 | P2_3 | 67 | 输出 |
| 列 C2 | P2_4 | 68 | 输出 |
| 列 C3 | P2_5 | 69 | 输出 |
| 列 C4 | P2_6 | 70 | 输出 |
| 行 R1 | P3_17 | 113 | 输入上拉 |
| 行 R2 | P3_16 | 112 | 输入上拉 |
| 行 R3 | P3_15 | 111 | 输入上拉 |
| 行 R4 | P3_14 | 110 | 输入上拉 |

#### 3.2.2 摇杆 ADC

| 功能 | 引脚 | ADC 通道 |
|------|------|----------|
| 左摇杆 X 轴 | P2_0 | ADC0_CH0 |
| 左摇杆 Y 轴 | P2_1 | ADC0_CH1 |
| 右摇杆 X 轴 | P0_18 | ADC0_CH8 |
| 右摇杆 Y 轴 | P0_23 | ADC0_CH13 |

#### 3.2.3 摇杆按键

| 功能 | 引脚 | GPIO 编号 |
|------|------|-----------|
| 左摇杆按键 (LS) | P3_7 | 103 |
| 右摇杆按键 (RS) | P3_6 | 102 |

---

## 4. 软件框架说明

### 4.1 软件架构

```
┌─────────────────────────────────────────────────────────────┐
│                      应用层 (Application)                    │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                   gamepad_app.c                         ││
│  │         (整合输入设备，映射到 USB HID 报告)              ││
│  └─────────────────────────────────────────────────────────┘│
├─────────────────────────────────────────────────────────────┤
│                      功能层 (Function)                       │
│  ┌───────────────┐  ┌───────────────┐  ┌─────────────────┐ │
│  │  key_app.c    │  │ joystick_app.c│  │   usb_app.c     │ │
│  │  (矩阵键盘)   │  │   (双摇杆)    │  │  (USB HID)      │ │
│  └───────────────┘  └───────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                      驱动层 (Driver)                         │
│  ┌───────────────┐  ┌───────────────┐  ┌─────────────────┐ │
│  │   drv_pin.c   │  │   drv_adc.c   │  │   CherryUSB     │ │
│  │   (GPIO)      │  │   (ADC)       │  │   (USB Stack)   │ │
│  └───────────────┘  └───────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                      RT-Thread 内核                          │
│         (线程调度、IPC、设备框架、自动初始化)                │
├─────────────────────────────────────────────────────────────┤
│                      硬件抽象层 (HAL)                        │
│                   NXP MCX SDK / CMSIS                        │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 数据流

```
矩阵键盘 ──► key_read() ──────────────────────────────────┐
                                                           │
左摇杆 ADC ──► joystick_left_read() ──► apply_deadzone() ──┤
                                                           ├──► gamepad_thread
右摇杆 ADC ──► joystick_right_read() ──► apply_deadzone() ──┤     │
                                                           │     ▼
摇杆按键 ──► rt_pin_read() ────────────────────────────────┘  scale_axis()
                                                                  │
                                                                  ▼
                                                           USB HID Report
                                                                  │
                                                                  ▼
                                                        hid_gamepad_send_report()
                                                                  │
                                                                  ▼
                                                              USB Host (PC)
```

---

## 5. 软件模块说明

### 5.1 key_app 模块（矩阵键盘）

**文件**: `applications/key_app.c`, `applications/key_app.h`

**功能**: 4x4 矩阵键盘扫描

**核心函数**:
```c
rt_uint8_t key_read(void);  // 返回 0-15 表示按键索引，0xFF 表示无按键
```

**扫描原理**:
1. 逐列输出低电平
2. 读取所有行引脚状态
3. 检测到低电平表示该交叉点按键被按下

### 5.2 joystick_app 模块（摇杆）

**文件**: `applications/joystick_app.c`, `applications/joystick_app.h`

**功能**: 双摇杆 ADC 读取与按键检测

**数据结构**:
```c
typedef struct {
    int16_t x;      // X轴: -32768 ~ 32767
    int16_t y;      // Y轴: -32768 ~ 32767
    bool btn;       // 按键: true=按下
} joystick_data_t;
```

**核心函数**:
```c
rt_err_t joystick_left_read(joystick_data_t *data);
rt_err_t joystick_right_read(joystick_data_t *data);
```

### 5.3 usb_app 模块（USB HID）

**文件**: `applications/usb_app.c`, `applications/usb_app.h`

**功能**: USB HID 游戏手柄设备实现

**HID 报告结构** (9 字节):
```c
typedef struct __attribute__((packed)) {
    uint16_t buttons;      // 16 个按钮
    int8_t left_x;         // 左摇杆 X (-127 ~ 127)
    int8_t left_y;         // 左摇杆 Y
    int8_t right_x;        // 右摇杆 X
    int8_t right_y;        // 右摇杆 Y
    uint8_t left_trigger;  // 左扳机 (0-255)
    uint8_t right_trigger; // 右扳机 (0-255)
    uint8_t hat;           // 方向键 (0-8)
} usb_gamepad_report_t;
```

**USB 描述符配置**:
- VID: 0x045E (Microsoft)
- PID: 0x02FF (Generic Gamepad)
- 端点: 0x81 (IN), 中断传输
- 轮询间隔: 1ms

### 5.4 gamepad_app 模块（应用层）

**文件**: `applications/gamepad_app.c`, `applications/gamepad_app.h`

**功能**: 整合所有输入设备，映射到 USB HID 报告

**核心特性**:
- **死区处理**: 消除摇杆中心位置的抖动
- **变化检测**: 只有状态变化时才发送报告
- **发送重试**: USB 忙碌时保留报告，下次重试

**按键映射**:
| 矩阵按键 | HID 按钮 |
|----------|----------|
| 0-13 | Button 1-14 |
| 左摇杆按键 | Button 15 (LS) |
| 右摇杆按键 | Button 16 (RS) |

### 5.5 drv_adc 模块（ADC 驱动）

**文件**: `Libraries/drivers/drv_adc.c`

**功能**: LPADC 驱动，支持多通道 ADC 读取

**关键修改**:
- 修复了多通道初始化覆盖问题
- 添加了超时保护，防止系统死锁
- 优化了命令槽分配（4 个通道使用 4 个独立命令槽）

---

## 6. 演示效果

### 6.1 启动日志

```
KEY OK
joystick: init OK
[USB] Initializing HID Gamepad...
[USB] HID Gamepad initialized successfully
[USB] VID:0x045E PID:0x02FF
[GAMEPAD] Started (interval: 10ms)
System Start
[GAMEPAD] Thread started
[USB] Device Configured - Gamepad Ready!
```

### 6.2 Windows 测试

1. 设备连接后，在"设备管理器"中显示为 "USB Gamepad HID"
2. 使用 `joy.cpl`（游戏控制器）可测试所有按钮和摇杆
3. 支持所有标准 DirectInput 游戏

### 6.3 功能演示

- [x] 16 个按钮正常响应
- [x] 左右摇杆 X/Y 轴正常
- [x] 摇杆按键正常
- [x] 低延迟响应

---

## 7. 代码地址

**Git 仓库**: [请填写您的代码仓库地址]

**项目路径**: `rt-thread/bsp/nxp/mcx/mcxa/frdm-mcxa156/`

**主要文件**:
```
applications/
├── main.c              # 主入口
├── gamepad_app.c/h     # 游戏手柄应用层
├── key_app.c/h         # 矩阵键盘模块
├── joystick_app.c/h    # 摇杆模块
└── usb_app.c/h         # USB HID 模块

board/
├── MCUX_Config/board/pin_mux.c  # 引脚配置
└── ports/cherryusb/             # CherryUSB 适配

Libraries/drivers/
└── drv_adc.c           # ADC 驱动（已修改）
```

---

## 8. 总结

本项目成功实现了基于 RT-Thread 的 USB HID 游戏手柄，具有以下特点：

1. **模块化设计**: 硬件层、功能层、应用层分离，易于维护
2. **实时性好**: 基于 RT-Thread 实时内核，10ms 扫描周期
3. **兼容性强**: 标准 HID 协议，Windows/Linux/macOS 免驱
4. **可扩展**: 可方便添加震动反馈、LED 指示等功能
