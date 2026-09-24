# STM32 传感器数据 → Luckfox → Wi-Fi → 电脑（方案文档）

> 目标：把 DHT11（温湿度）和光敏传感器的数据，通过 USART 串口发给 Luckfox Pico Ultra W，再由 Luckfox 经 Wi-Fi（TCP）转发到电脑上查看。
> 链路是**单向**的：`STM32 → 串口 → Luckfox → Wi-Fi(TCP) → 电脑`。

## 0. 总体架构

```
┌──────────────┐   USART2(PA2 TX)   ┌────────────────────┐   Wi-Fi(TCP:9000)   ┌──────────┐
│  STM32F407   │ ─────────────────▶ │  Luckfox Pico Ultra W│ ─────────────────▶ │   电脑    │
│  (FreeRTOS)  │   3.3V + 共地      │  (Linux + Python)    │                     │  (nc/终端)│
└──────────────┘                    └────────────────────┘                     └──────────┘
```

三个选型（已定）：
- 转发方式：**TCP 服务器**（Luckfox 开端口，电脑主动连）
- Luckfox 程序：**Python 脚本**
- 电脑端：**终端打印原始文本**

---

## 1. 数据格式（约定）

STM32 每 1 秒发一行，每行以 `\r\n` 结尾：

```
T=25,H=60,L=75,S=1
```

| 字段 | 含义 |
|---|---|
| `T` | 温度(°C) |
| `H` | 湿度(%) |
| `L` | 光照强度(0~100) |
| `S` | DHT11 状态（1=正常，0=读失败） |

> DHT11 读失败时，`T/H` 保留上次成功值，`S=0`（与 LCD 任务行为一致）。

---

## 2. STM32 侧

### 2.1 CubeMX 增加 USART2

1. 打开 `RemoteMonitoringSystem.ioc` → **Connectivity → USART2** → Mode = **Asynchronous**
2. 波特率 **115200**、8N1（与 USART1 相同）
3. 引脚自动分配：**PA2 = USART2_TX**、**PA3 = USART2_RX**（本方案只用 TX）
4. **GENERATE CODE** 重新生成 → 得到 `huart2` / `MX_USART2_UART_Init()`

> 保留 USART1（PA9/PA10）继续做 `printf` 调试口，不影响。

### 2.2 `freertos.c` 新增发送任务（共 5 处改动）

**① 顶部 Includes（`USER CODE BEGIN Includes`）加入：**
```c
#include <stdio.h>
#include "usart.h"
```

**② 任务属性（放在 `keyTask_attributes` 之后）：**
```c
osThreadId_t uartTxTaskHandle;
const osThreadAttr_t uartTxTask_attributes = {
	.name = "uartTxTask",
	.stack_size = 256 * 4,
	.priority = (osPriority_t) osPriorityNormal,
};
```

**③ 函数原型（`USER CODE BEGIN FunctionPrototypes`）：**
```c
void StartUartTxTask(void *argument);
```

**④ 创建任务（`USER CODE BEGIN RTOS_THREADS`）：**
```c
uartTxTaskHandle = osThreadNew(StartUartTxTask, NULL, &uartTxTask_attributes);
```

**⑤ 任务体（`USER CODE BEGIN Application`）：**
```c
void StartUartTxTask(void *argument)
{
  char buf[64];
  monitor_data_t d;
  int n;

  for(;;)
  {
    /* 照抄 lcdTask 的互斥锁快照写法：拿笔→复制→放笔 */
    osMutexAcquire(g_monitor_mutex, osWaitForever);
    d = g_monitor;
    osMutexRelease(g_monitor_mutex);

    n = snprintf(buf, sizeof(buf), "T=%d,H=%d,L=%d,S=%d\r\n",
                 d.temperature, d.humidity, d.light, d.dht11_ok);
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, 100);

    osDelay(1000);   /* 每秒发一次 */
  }
}
```

**说明：**
- 单向发送，`HAL_UART_Transmit` 阻塞式即可（约 20 字节 ≈ 1.7ms，每秒一次，可接受），无需中断/DMA。
- 复用现有 `g_monitor` + `g_monitor_mutex`，直接抄 LCD 任务"互斥锁下复制快照"的写法。

---

## 3. Luckfox 侧（Linux + Python）

### 3.1 前置准备

```bash
# 1) 确认串口设备存在（应是 /dev/ttyS3）
ls /dev/ttyS*

# 2) 确认有 python3 + pyserial
python3 -c "import serial"          # 报错则: python3 -m pip install pyserial

# 3) 连上和电脑同一个 Wi-Fi，记下 IP
ip addr show wlan0                  # 或 hostname -I
```

> UART3 需在设备树/overlay 里开启。具体针脚位置用 `luckfox-config` 确认，或参考官方文档：<https://wiki.luckfox.com/Luckfox-Pico-Ultra/UART/>

### 3.2 转发脚本 `serial2tcp.py`

```python
#!/usr/bin/env python3
"""STM32 -> Luckfox -> 电脑(Wi-Fi/TCP) 串口转TCP转发脚本"""
import sys
import socket
import serial

SERIAL_DEV = "/dev/ttyS3"   # Luckfox Pico Ultra W 的 UART3
BAUD       = 115200          # 要和 STM32 端一致
PORT       = 9000            # 电脑连接的 TCP 端口

def main():
    dev  = sys.argv[1] if len(sys.argv) > 1 else SERIAL_DEV
    port = int(sys.argv[2]) if len(sys.argv) > 2 else PORT

    try:
        ser = serial.Serial(dev, BAUD, timeout=1)
    except serial.SerialException as e:
        print(f"[!] 打开串口失败: {dev} ({e})")
        print("    请检查: 1) 设备是否存在 (ls /dev/ttyS*)  2) UART3 是否已开启")
        sys.exit(1)

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("0.0.0.0", port))
    srv.listen(1)
    print(f"[*] 串口 {dev} @ {BAUD} -> TCP :{port} 就绪，等待电脑连接...")

    while True:
        conn, addr = srv.accept()
        print(f"[+] 客户端已连接: {addr}")
        try:
            while True:
                line = ser.readline()   # 读到 \n 结尾的一整行
                if line:
                    conn.sendall(line)
        except (BrokenPipeError, ConnectionResetError, OSError) as e:
            print(f"[-] 客户端断开: {e}")
        finally:
            conn.close()
            print("[*] 回到等待连接状态...")

if __name__ == "__main__":
    main()
```

### 3.3 运行

```bash
python3 serial2tcp.py            # 前台跑
# 或后台: nohup python3 serial2tcp.py &
```

---

## 4. 电脑端查看

```bash
nc <luckfox_ip> 9000
```

应每 1 秒看到一行 `T=xx,H=xx,L=xx,S=x`。

---

## 5. 接线（重点：共地！）

| STM32（探索者 F407） | Luckfox Pico Ultra W |
|---|---|
| **PA2** (USART2_TX) | **UART3_RX_M0** |
| **GND** | **GND** |

- 两侧都是 **3.3V 逻辑**，直接连，**无需电平转换**
- **GND 必须共地**，否则收到全是乱码（新手最常漏）
- 单向发送，RX 不用接

---

## 6. 分阶段验证（从后往前定位问题）

1. **STM32 串口单测**：PA2 接 USB-TTL 的 RX，电脑串口助手 115200 应看到 `T=...,...` → 证明 STM32 侧 OK
2. **Luckfox 串口单测**：`stty -F /dev/ttyS3 115200 && cat /dev/ttyS3` 应看到同样数据 → 查接线/GND/UART3 是否开启
3. **Luckfox TCP 单测**：跑脚本后，在 Luckfox 本机 `nc 127.0.0.1 9000` 自测
4. **端到端**：电脑 `nc <luckfox_ip> 9000`

---

## 7. 风险与备选

- **Luckfox 镜像可能没有 python3/pyserial**：已列为前置检查项。若无，零代码备选（板上有 socat 时）：
  ```bash
  socat TCP-LISTEN:9000,fork /dev/ttyS3,b115200
  ```
- **UART3 针脚位置**：以 Luckfox 官方 pinout 为准，用 `luckfox-config` 确认。
- **安全**：数据为明文、局域网内传输、无鉴权，仅作本地学习用。
