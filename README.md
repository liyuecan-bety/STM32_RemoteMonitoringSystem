# STM32F407 远程环境监测系统

基于 **STM32F407 + FreeRTOS** 的环境监测终端：实时采集温湿度与光照强度，本地 LCD 显示，并经 **USART 串口 → 嵌入式 Linux 网关 → Wi-Fi/TCP** 实现远程监测。

```
STM32F407  ──USART2──▶  Luckfox Pico Ultra W  ──Wi-Fi/TCP:9000──▶  电脑 (nc)
 (FreeRTOS)               (嵌入式 Linux + Python)                  远程查看
```

---

## ✨ 功能特性

- **多传感器采集**：DHT11 温湿度 + 光敏传感器光照强度，实时采集
- **本地显示**：TFT-LCD（FSMC 总线）实时刷新温度 / 湿度 / 光照 / 传感器状态
- **FreeRTOS 多任务**：6 个任务并发运行，互斥锁保护共享数据
- **中断驱动 DHT11**：EXTI 双边沿中断 + DWT 时间戳解码单总线，非阻塞、无忙等
- **远程监测**：USART2 定时上报，经 Luckfox 网关 Python 脚本转发，电脑 `nc` 即可查看
- **分层 BSP 框架**：外设驱动模块化、可移植

---

## 🏗 系统架构

```
┌────────────────────── STM32F407ZGT6 (FreeRTOS) ──────────────────────┐
│                                                                      │
│  lightTask ──ADC3──▶ 光敏传感器（10 次均值滤波 → 0~100）              │
│  dht11Task ──EXTI/DWT──▶ DHT11 温湿度（中断驱动，信号量同步）          │
│                                                                      │
│        ┌─────────────── 共享数据 g_monitor ───────────────┐           │
│        │        (monitor_data_t + 互斥锁 g_monitor_mutex) │           │
│        └───────────────┬───────────────┬──────────────────┘           │
│                        │               │                              │
│                   lcdTask (LCD)   uartTxTask (USART2)                  │
│                        │               │                              │
└────────────────────────┼───────────────┼──────────────────────────────┘
                         │               │  T=25,H=60,L=75,S=1 (1s/帧)
                         ▼               ▼
                本地 LCD 显示      Luckfox Pico Ultra W
                                        │  Python 串口转 TCP
                                        ▼
                                   Wi-Fi / TCP:9000
                                        │
                                        ▼
                                    电脑 (nc)
```

---

## 🧰 硬件资源


| 资源       | 说明                                                    |
| ---------- | ------------------------------------------------------- |
| MCU        | STM32F407ZGT6（正点原子探索者，168MHz）                 |
| 传感器     | DHT11（温湿度，单总线）、光敏传感器                     |
| 采集       | ADC3（12 位，光敏电压采样）                             |
| 显示       | TFT-LCD（FSMC 总线驱动）                                |
| 按键 / LED | KEY0、LED0 / LED1                                       |
| 串口       | USART1（`printf` 调试）、USART2（数据上报，115200 8N1） |
| 网关       | Luckfox Pico Ultra W（Buildroot Linux）                 |

## 💻 软件环境

- **开发工具**：Keil MDK 5、STM32CubeMX 6.x
- **中间件**：FreeRTOS（CMSIS-OS v2）、STM32F4 HAL 库
- **网关侧**：Python3 + pyserial

---

## 🚀 快速开始

### 1. 编译烧录（STM32 端）

1. 用 **Keil MDK** 打开 `RemoteMonitoringSystem2/MDK-ARM/RemoteMonitoringSystem.uvprojx`
2. `Rebuild` 后下载到 STM32F407 开发板
3. 复位后，LCD 显示温湿度 / 光照；`PA2`（USART2_TX）每 1 秒发一帧 `T=...,H=...,L=...,S=...`

> 修改外设配置：用 CubeMX 打开 `RemoteMonitoringSystem.ioc` 重新生成即可（任务代码在 `USER CODE` 块内，不会被覆盖）。

### 2. 网关转发（Luckfox 端）

将 `Linux.py` 传到网关并运行：

```bash
scp Linux.py root@<luckfox_ip>:/root/
ssh root@<luckfox_ip>
python3 /root/Linux.py      # 串口 /dev/ttyS3 @115200 → TCP :9000
```

### 3. 远程查看（电脑端）

```bash
nc <luckfox_ip> 9000
```

每 1 秒看到一行数据，即端到端跑通。

### 接线


| STM32（探索者 F407） | Luckfox Pico Ultra W |
| -------------------- | -------------------- |
| PA2（USART2_TX）     | UART3_RX_M0          |
| GND                  | GND                  |

> 两侧均为 3.3V 逻辑，直接连接；**GND 必须共地**，否则乱码。

---

## 📂 目录结构

```
STM32_RemoteMonitoringSystem/
├── Linux.py                          # 网关：串口 → TCP 转发脚本
└── RemoteMonitoringSystem2/          # CubeMX / Keil 工程
    ├── RemoteMonitoringSystem.ioc    # CubeMX 配置
    ├── Core/
    │   ├── Inc/                      # 外设初始化头文件
    │   └── Src/
    │       ├── main.c                # 入口：硬件初始化 + 启动调度器
    │       ├── freertos.c            # ★ 6 个任务 + 共享数据 + 互斥锁
    │       ├── adc.c / usart.c / fsmc.c / dma.c / gpio.c   # 外设初始化
    │       └── bsp/                  # ★ BSP 驱动层（可移植）
    │           ├── dht11/            #   DHT11 中断驱动
    │           ├── lsens/            #   光敏传感器驱动
    │           ├── lcd/              #   LCD 显示驱动
    │           ├── LedKey/           #   LED + 按键
    │           ├── usart/            #   串口调试
    │           └── delay/            #   微秒延时
    ├── Drivers/                      # STM32F4 HAL 库 + CMSIS
    └── MDK-ARM/                      # Keil MDK 工程
```

---

## 🔧 技术细节

### 1. FreeRTOS 六任务设计


| 任务         | 周期  | 优先级                  | 职责                         |
| ------------ | ----- | ----------------------- | ---------------------------- |
| `lightTask`  | 200ms | Normal                  | 读取光敏，写入共享数据       |
| `dht11Task`  | 2s    | **AboveNormal（最高）** | 读取温湿度（阻塞在信号量上） |
| `lcdTask`    | 200ms | Normal                  | 复制快照，刷新 LCD           |
| `ledTask`    | 200ms | BelowNormal（最低）     | LED0 闪烁                    |
| `keyTask`    | 10ms  | Normal                  | 按键检测，控制 LED1          |
| `uartTxTask` | 1s    | Normal                  | 格式化数据帧，USART2 上报    |

- **共享数据保护**：所有任务通过 `g_monitor`（`monitor_data_t`）交换数据，用互斥锁 `g_monitor_mutex` 保证「快照一致性」——拿锁 → 复制 → 释放，持锁时间尽量短。
- **优先级设计**：DHT11 采样周期必须 ≥ 2s 且读取有 10ms 超时窗口，故优先级最高，保证采样准时不被 LCD 刷屏拖慢。

### 2. DHT11 中断驱动（单总线解码）

传统单总线驱动靠微秒级忙等。本方案改为**中断驱动、非阻塞**：

1. 任务发启动信号（拉低 20ms，用 `vTaskDelay` 让出 CPU）；
2. 数据线切换到**输入 + EXTI 双边沿中断**模式；
3. EXTI 中断回调里用 **DWT 周期计数器**给每个边沿打时间戳；
4. 采够 83 个边沿后，ISR 用 **`xSemaphoreGiveFromISR`** 释放二进制信号量唤醒任务；
5. 任务解码 40bit 数据并做**和校验**，校验失败保留上次数据并置 `S=0`。

全程无忙等、无需关闭调度器，CPU 空闲时间交给其他任务。

### 3. 光敏采集

`ADC3`（12 位）采集光敏电压 → **10 次采样取均值**滤波 → 线性映射为 0~100 光照强度百分比。

### 4. 数据上报链路

`uartTxTask` 每 1 秒从共享数据复制快照，`snprintf` 格式化为 `T=...,H=...,L=...,S=...`，经 `HAL_UART_Transmit` 由 USART2 发出；Luckfox 网关用 Python 脚本读串口、经 TCP 转发，实现 Wi-Fi 无线远程查看。

---

## 📡 数据协议


| 字段 | 含义                           | 示例 |
| ---- | ------------------------------ | ---- |
| `T`  | 温度（°C）                    | `25` |
| `H`  | 湿度（%）                      | `60` |
| `L`  | 光照强度（0~100）              | `75` |
| `S`  | DHT11 状态（1=正常，0=读失败） | `1`  |

每帧以 `\r\n` 结尾，例如：`T=25,H=60,L=75,S=1`

---

## 📄 参考

- Luckfox 官方 Wiki（登录 / 配置 UART）：[https://wiki.luckfox.com/](https://wiki.luckfox.com/)
- STM32F407 HAL 库文档：[https://www.st.com/](https://www.st.com/)

> 详细的设计思路（FreeRTOS 逐行讲解、串口 Wi-Fi 转发方案）见配套文档。
