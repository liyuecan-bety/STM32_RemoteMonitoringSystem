# 我的 FreeRTOS 监测项目 —— 代码逐行讲解

> 目标：不用再"云里雾里"。这份文档按照**程序真正执行的顺序**，把你 `RemoteMonitoringSystem2` 里和 FreeRTOS 相关的代码讲一遍。
> 原则：只讲"为什么"，不讲套话。每一段都能对应到你项目里的真实文件。

---

## 0. 先建立心智模型（读代码前必看）

裸机时代，你的程序是这样的：

```
main() {
    while(1) {
        读光敏();      // 干完一件
        读温湿度();    // 再干下一件
        刷LCD();       // 一件接一件
        闪灯();        // 顺序执行，一眼望到底
    }
}
```

FreeRTOS 时代，变成了**四个并排的 `while(1)`**，像四个工人同时在你桌旁干活：

```
工人A（光敏任务）:  while(1){ 读光敏; 写白板; 睡200ms; }
工人B（温湿度任务）: while(1){ 读温湿度; 写白板; 睡2秒; }
工人C（LCD任务）:    while(1){ 读白板; 刷屏; 睡200ms; }
工人D（LED任务）:    while(1){ 闪灯; 睡200ms; }
```

**操作系统（调度器）每秒在四个工人之间快速切换几百上千次**，让你感觉他们"同时在干活"。

你以前的思维是："下一步执行哪一行代码？"
FreeRTOS 要你换成："**现在是谁在跑？它在碰哪块数据？**"

这就是那道坎。下面按执行顺序，一步步走。

---

## 1. 程序从 `main.c` 启动（main.c:77）

`main()` 做的事可以分成三段，理解这三段，你就理解了整个框架。

### 1.1 第一段：初始化硬件（main.c:87-106）

```c
HAL_Init();                  // 复位所有外设、初始化Flash、配置Systick
SystemClock_Config();        // 配置主频168MHz（PLL）
MX_GPIO_Init();              // 初始化GPIO
MX_DMA_Init();               // 初始化DMA
MX_USART1_UART_Init();       // 初始化串口
MX_FSMC_Init();              // 初始化FSMC（LCD用的总线）
MX_ADC3_Init();              // 初始化ADC3（光敏传感器用）
```

这一段和裸机**完全一样**，就是"上电，把各个外设准备好"。看不懂的部分（FSMC、DMA、时钟树）跟 FreeRTOS 无关，是 STM32 本身的知识，先放着。

### 1.2 第二段：用户初始化（main.c:106-119）

```c
lcd_init();      // 初始化LCD
delay_init();    // 算好 fac_us（每微秒多少个时钟周期），供 delay_us 用
```

注意 `lcd_init()` 的注释（main.c:107）：**"uses HAL_Delay (TIM7) - works before scheduler"**。

这句话很重要：LCD 初始化里用了 `HAL_Delay`，而 `HAL_Delay` 靠的是 TIM7 产生的中断（不是 FreeRTOS）。所以**在调度器启动之前，`HAL_Delay` 还能正常工作**。等会会讲为什么这个"能/不能"的边界这么关键。

### 1.3 第三段：启动 FreeRTOS 调度器（main.c:122-126）

```c
osKernelInitialize();   // 初始化 FreeRTOS 内核
MX_FREERTOS_Init();     // 创建任务、互斥锁（在 freertos.c 里）
osKernelStart();        // ★ 从这里开始，控制权交给调度器
```

**`osKernelStart()` 永远不会返回。** 它一旦执行，`main` 函数就"冻结"在这里了。

所以 `main.c:132` 那个 `while(1)` 里的代码**永远执行不到**，它是死代码：

```c
while (1) {
    /* 这里永远不会跑，所有周期工作都在任务里做（freertos.c） */
}
```

**这是裸机到 FreeRTOS 最反直觉的一点**：以前你的逻辑写在 main 的 `while(1)` 里，现在 main 的 `while(1)` 是空的，真正的逻辑搬进了任务里。

---

## 2. `freertos.c`：四个工人 + 一块白板 + 一支笔

`osKernelStart()` 之后，程序就活在 `freertos.c` 里了。这个文件是整个项目的核心。

### 2.1 共享数据：那块"白板"（freertos.c:38-44）

```c
typedef struct {
    uint8_t light;        // 光照强度 0~100
    uint8_t temperature;  // 温度
    uint8_t humidity;     // 湿度
    uint8_t dht11_ok;     // 1=DHT11正常，0=出错
} monitor_data_t;
```

这就是四个工人共享的那块白板：光敏工人往上写 `light`，温湿度工人写 `temperature`/`humidity`/`dht11_ok`，LCD 工人读全部数据去刷屏。

### 2.2 那支"笔"：互斥锁（freertos.c:59-60）

```c
static monitor_data_t g_monitor = {0, 0, 0, 0};  // 共享的白板
static osMutexId_t    g_monitor_mutex;           // 保护白板的"笔"
```

**为什么需要这支笔？**

想象没有笔的情况：LCD 工人正在读白板，读到 `temperature = 25` 的那一刻，温湿度工人恰好更新了白板，把 `humidity` 从 `40` 改成了 `60`。

结果 LCD 工人读到的是一份**错位的快照**：温度是这一次采样的，湿度是上一次采样的，两个数字对不上，屏幕上就会闪过一帧错误数据。

互斥锁（mutex）就是"桌上唯一一支笔"：

- 谁要碰白板，先 `osMutexAcquire`（拿笔）；
- 用完 `osMutexRelease`（放笔）；
- 如果笔被别人拿着，就**等着**，直到对方放笔。

> 注意：这里的"错位"不是指单个字节读写不安全（ARM 上写一个 `uint8_t` 是原子的）。而是指**整份快照的一致性**——温度和湿度要属于同一次采样。互斥锁保证的是"这份数据是一套的"。

### 2.3 任务属性：每个工人的"工位大小"和"等级"（freertos.c:64-87）

```c
const osThreadAttr_t lightTask_attributes = {
  .name       = "lightTask",
  .stack_size = 256 * 4,                       // 栈大小
  .priority   = (osPriority_t) osPriorityNormal,  // 优先级
};
```

每个任务（工人）有两个关键属性：

**① `stack_size` —— 这个工人自己的"小桌面"有多大**

- 为什么要 `* 4`？因为 ARM 是 32 位机，一个"字" = 4 字节。FreeRTOS 的栈大小单位是"字"，所以 `256 * 4` 就是 256 个字 = 1024 字节 = 1KB。
- 为什么 LCD 任务要 `512 * 4`（2KB），LED 任务只要 `128 * 4`（512B）？因为 LCD 任务里要调用画字符的函数，用到的局部变量多，还复制了一份 `monitor_data_t data`（freertos.c:201），桌面得大；LED 任务就数个数、翻个灯，桌面小就够。
- **栈不够会怎样？** 程序会随机崩溃、死机（栈溢出，把别人的数据踩了）。这是新手最常见、最隐蔽的坑。

**② `priority` —— 这个工人的"等级"**

| 任务 | 优先级 | 为什么 |
|---|---|---|
| dht11Task | `osPriorityAboveNormal`（最高） | DHT11 的采样周期必须 ≥2 秒，而且读取有 10ms 超时窗口，最不能被拖延 |
| lightTask | `osPriorityNormal` | 普通 |
| lcdTask | `osPriorityNormal` | 普通 |
| ledTask | `osPriorityBelowNormal`（最低） | 闪灯不紧急，晚一点没影响 |

> **优先级的含义**：当一个高优先级任务醒来（比如 DHT11 睡了 2 秒醒来），它会**立刻**抢走 CPU，低优先级任务先让位。所以 DHT11 优先级最高，是为了保证它每次采样"准时"，不被 LCD 刷屏这种耗时操作拖慢。

---

## 3. `MX_FREERTOS_Init()`：开工前，先发笔、再雇人（freertos.c:104-138）

这个函数在 `main.c:123` 被调用，干两件事：

### 3.1 发笔（创建互斥锁）

```c
g_monitor_mutex = osMutexNew(NULL);
```

`NULL` 表示用默认属性。从现在起，工人们就可以用这支笔了。

### 3.2 雇人（创建四个任务）

```c
lightTaskHandle = osThreadNew(StartLightTask, NULL, &lightTask_attributes);
dht11TaskHandle = osThreadNew(StartDHT11Task, NULL, &dht11Task_attributes);
lcdTaskHandle   = osThreadNew(StartLcdTask,   NULL, &lcdTask_attributes);
ledTaskHandle   = osThreadNew(StartLedTask,   NULL, &ledTask_attributes);
```

`osThreadNew(要执行的函数, 传参, 属性)`：
- 第一个参数是**任务的入口函数**（工人入职后要去干活的函数）；
- 第二个参数 `NULL` 是不传参数（`StartXxxTask` 里的 `void *argument` 用不上）；
- 第三个参数就是上一节定义的"工位大小 + 等级"。

**注意**：这四个 `osThreadNew` 只是"登记入职"，任务**不会立刻开始跑**。它们要等 `main.c:126` 的 `osKernelStart()` 一声令下，调度器才开始轮流叫它们干活。

---

## 4. 逐个看四个工人（任务函数）

### 4.1 光敏任务 `StartLightTask`（freertos.c:148-160）

```c
void StartLightTask(void *argument) {
  for(;;) {
    uint8_t light = lsens_get_val();      // 读光照值（约50ms：采10次ADC取平均）

    osMutexAcquire(g_monitor_mutex, osWaitForever);  // 拿笔
    g_monitor.light = light;                          // 写白板
    osMutexRelease(g_monitor_mutex);                  // 放笔

    osDelay(200);                            // 睡200ms
  }
}
```

执行节奏：
1. 调 `lsens_get_val()` 读光敏（内部采 10 次 ADC 取平均，见 `bsp_lsens.c:4-13`）；
2. **拿笔 → 写 `light` → 放笔**（碰白板的标准三步）；
3. `osDelay(200)` 睡 200ms，让别的任务去跑。

> `osWaitForever` = 拿不到笔就一直等，绝不超时。因为这段很关键，宁可等也不能拿到旧数据。

### 4.2 温湿度任务 `StartDHT11Task`（freertos.c:171-192）

```c
void StartDHT11Task(void *argument) {
  uint8_t temp, humi, ok;

  dht11_init();   // 只初始化一次：配置EXTI + 创建信号量

  for(;;) {
    ok = (dht11_read_data(&temp, &humi) == 0);  // 读数据（会阻塞在信号量上）

    osMutexAcquire(g_monitor_mutex, osWaitForever);
    g_monitor.dht11_ok = ok;        // 写"是否成功"标志
    if (ok) {
      g_monitor.temperature = temp;  // 成功才更新温湿度
      g_monitor.humidity    = humi;
    }
    osMutexRelease(g_monitor_mutex);

    osDelay(2000);    // DHT11采样周期必须≥2秒
  }
}
```

要点：

- **`dht11_init()` 在 `for` 外面**（只跑一次）：它初始化 EXTI 中断、DWT 计数器、创建一个二进制信号量（`bsp_dht11.c:49-66`）。这就是"开工前先把工具备好"。
- **为什么 `osDelay(2000)` 是 2 秒**：DHT11 的规格书要求两次采样之间至少间隔 2 秒，否则读不到正确数据。这是个**硬件约束**，不是随便写的。
- 读成功才更新温度湿度；读失败就只把 `dht11_ok` 置 0，保留旧数据，LCD 会显示 "DHT11 ERROR"。

### 4.3 LCD 任务 `StartLcdTask`（freertos.c:199-230）

```c
void StartLcdTask(void *argument) {
  monitor_data_t data;
  uint8_t last_ok = 0xFF;   // 哨兵值，强制第一次刷状态

  for(;;) {
    osMutexAcquire(g_monitor_mutex, osWaitForever);
    data = g_monitor;        // 一次性复制整份白板
    osMutexRelease(g_monitor_mutex);

    lcd_show_num(70, 150, data.temperature, 2, 16, BLUE);   // 画温度
    lcd_show_num(70, 170, data.humidity,    2, 16, BLUE);   // 画湿度
    lcd_show_xnum(110, 110, data.light, 3, 16, 0, BLUE);    // 画光照

    if (last_ok != data.dht11_ok) {         // 只在状态"变化"时才重画
      last_ok = data.dht11_ok;
      // ... 画 "DHT11 OK" 或 "DHT11 ERROR"
    }

    osDelay(200);
  }
}
```

要点：

- **`data = g_monitor;` 是一整份复制**（`freertos.c:207`）：拿笔的瞬间把整块白板抄到自己的小本本 `data` 上，然后**立刻放笔**。之后画屏用的都是自己手里的副本，不再碰白板。
- **为什么要复制而不边读边画？** 因为画屏（LCD 操作）很慢，如果一直拿着笔，光敏和温湿度工人就得干等 200ms 不能写数据。**拿笔的时间越短越好**，这是 RTOS 编程的铁律。
- **`last_ok` 的优化**（`freertos.c:202,214`）：状态文字"OK/ERROR"只有在**变化时**才重画（用 `lcd_fill` 清掉旧字再写），平时只刷数字。因为画字符串比画数字慢，能省就省。

### 4.4 LED 任务 `StartLedTask`（freertos.c:237-251）

```c
void StartLedTask(void *argument) {
  uint32_t cnt = 0;
  for(;;) {
    cnt++;
    LED0_TOGGLE;          // 每200ms翻一次 -> 约2.5Hz
    if (cnt % 5 == 0) {
      LED1_TOGGLE;        // 每1秒翻一次 -> 1Hz
    }
    osDelay(200);
  }
}
```

这个任务最简单，但它演示了一个重要技巧：**用一个计数器 `cnt` 把两种频率的灯分开**。LED0 每 200ms 闪一次，LED1 每 `5 * 200ms = 1s` 闪一次。不用为两个灯开两个任务，一个任务就够。

---

## 5. 三个"延时"——你最晕的地方，一次讲清

你代码里出现了三种"等待"，它们**本质完全不同**：

| 名字 | 原理 | 谁在用 | 能不能在调度器启动前用 |
|---|---|---|---|
| `HAL_Delay(ms)` | 死等，靠 TIM7 中断把 `uwTick` 加 1，while 循环等到时间到 | LCD 初始化、HAL 库内部 | ✅ 能（还没启动调度器时也能用） |
| `osDelay(ms)` | **让出 CPU**，本任务去睡，调度器叫别的任务跑 | 所有任务里的周期延时 | ❌ 不能（必须在任务里，调度器已启动） |
| `delay_us(us)` | 死等，靠 DWT 硬件计数器数周期，不靠任何中断 | DHT11 驱动、精细时序 | ✅ 能（独立于调度器） |

### 5.1 关键区别：`HAL_Delay` 是"忙等"，`osDelay` 是"让位"

```c
// 忙等（CPU 空转数时间，但不会关中断、不会阻止调度器抢占）
HAL_Delay(1000);   // 这1秒里 CPU 在空转，每1ms仍会被 tick 打断、被高优先级任务抢占

// 主动让位（本任务进入阻塞态，完全不占 CPU）
osDelay(1000);     // 本任务睡1秒，CPU 去跑别的任务，1秒后回来继续
```

**关键结论：`HAL_Delay` 会不会"卡死别人"，取决于调用它的任务优先级高低。**

- 在**最低优先级**任务里用 → 每次 tick 都被高优先级任务抢占 → 几乎无影响（只是白耗电）；
- 在**最高优先级**任务里用 → 低优先级任务被"饿死"，一直抢不到 CPU → 明显卡顿/冻住。

所以严格的说法是：**任务里应该用 `osDelay`（让位），而不是 `HAL_Delay`（忙等）**。忙等的危害是——高优先级任务忙等会饿死低优先级任务；即使没饿死别人，也是在白白烧 CPU、耗电，没有任何好处。

> 你可以自己做实验验证：把 `ledTask`（最低优先级）里的 `osDelay` 换成 `HAL_Delay`，LCD 看不出差别；但把 `ledTask` 优先级改成 `osPriorityAboveNormal`（最高）再试，LCD 会直接冻住。

### 5.2 为什么会有"调度器启动前/后"这条分界线？

`HAL_Delay` 依赖 TIM7 中断，TIM7 在 `HAL_Init` 阶段就配好了，所以**一开始就能用**。

`osDelay` 依赖 FreeRTOS 调度器，而调度器要等 `osKernelStart()`（main.c:126）才启动。所以**在 main 的初始化阶段（调度器启动前）不能调 `osDelay`**。

于是你看到这样的分工：
- `lcd_init()` 在调度器启动前调用，里面用 `HAL_Delay`（能跑）；
- 任务里用 `osDelay`（让位）。

### 5.3 `delay_us` 是"第三世界"，完全独立

`bsp_delay.c:23-31` 的 `delay_us` 用 DWT（Cortex-M4 内核自带的一个计数器）数时钟周期，**既不靠 TIM7、也不靠调度器**，所以它在任何时刻都能用。

它在 DHT11 驱动里干嘛？DHT11 是单总线协议，靠"高电平持续多久"来区分 0 和 1（26μs 是 0，70μs 是 1）。这么精细的时间，`HAL_Delay`（毫秒级）根本不够，得用 `delay_us`（微秒级）。

> 不过你注意：你的 DHT11 现在改成了**中断驱动**版本，连 `delay_us` 都不用了，改由 EXTI 捕获边沿 + DWT 打时间戳（见第 7 节）。所以 `delay_us` 现在是"备用工具"，但原理你要懂。

---

## 6. 完整的时序图：一眼看懂"谁在同时干嘛"

```
时间轴 →
─────────────────────────────────────────────────────────────
main():
  init硬件 → lcd_init → osKernelStart()
                                   │
                                   ▼ （从这里起，4个任务并发）
lightTask:   [读光敏]──睡200ms──[读光敏]──睡200ms──[读光敏]──...
dht11Task:   [读温湿度]────────────睡2秒────────────────────[读温湿度]...
lcdTask:     [抄白板→刷屏]──睡200ms──[抄白板→刷屏]──睡200ms──...
ledTask:     [闪灯]──睡200ms──[闪灯]──睡200ms──[闪灯]──...

三个周期为200ms的任务（光敏、LCD、LED）大致同步刷新，
温湿度任务每2秒才"起床"一次，起床时优先级最高，先抢CPU。
```

---

## 7. 进阶：DHT11 的信号量——"任务睡觉 → 中断叫醒"

这一段是你代码里最"高阶"、也最有价值的部分：它用到了 FreeRTOS 的**第二种任务间通信方式——信号量**。搞懂它，你的 FreeRTOS 才算真正入门。

### 7.1 先分清：信号量 vs 互斥锁

| | 互斥锁（mutex） | 信号量（semaphore） |
|---|---|---|
| 类比 | 桌上唯一一支**笔** | 一个**铃铛**/通知牌 |
| 目的 | 保护共享数据，**互斥** | **通知**"事情发生了，快醒醒" |
| 谁拿谁放 | **同一个任务**拿、同一个任务放 | **两个不同的东西**：中断放，任务拿 |
| 状态 | 有没有被人占用 | 有几个"事件"待处理（二进制的就是 0 或 1） |

一句话记牢：

> **互斥锁是"别碰，我在用"；信号量是"醒醒，有活了"。**

### 7.2 为什么 DHT11 需要信号量？——因为它是"慢外设"

DHT11 读一次要拉低 20ms、再等它回 40 位数据，全程几十毫秒。裸机时代只能"死等"：

```c
拉低20ms;  while(等传感器回信号);  解码;
```

这几十毫秒 CPU 傻等，啥也干不了。FreeRTOS 里绝不让 4 个任务陪一个 DHT11 傻等，于是改成"任务睡觉 → 中断叫醒"：

- **任务**：发出读取请求，然后**去睡觉**（不占 CPU）；
- **中断**：数据线每个边沿触发 EXTI 中断，中断里默默采集，采够了就**摇铃叫醒任务**。

### 7.3 完整流程走一遍（对着 `bsp_dht11.c`）

#### 第一步：开工前备工具（`dht11_init`，bsp_dht11.c:49-66，只跑一次）

```c
dht11_sem = xSemaphoreCreateBinary();   // 造一个"二进制信号量"（铃铛）
dht11_pin_output();                     // 数据线先设成输出
HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);  // 配好边沿中断
HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
```

- `xSemaphoreCreateBinary()` 造一个只有两个状态的铃铛：`0 = 没事`，`1 = 有事`。
- 中间还开了 DWT 计数器（bsp_dht11.c:54-56），用来给边沿打时间戳（解码用，先不管）。

#### 第二步：任务发起读取（`dht11_read_data`，bsp_dht11.c:126-156）

```c
/* ① 先把上一次可能残留的"旧铃铛"清掉 */
xSemaphoreTake(dht11_sem, 0);          // 非阻塞地拿一次，拿不到就算了

/* ② 发启动信号：拉低20ms */
HAL_GPIO_WritePin(..., GPIO_PIN_RESET);
vTaskDelay(pdMS_TO_TICKS(20));         // ★ 睡20ms，不是忙等！

/* ③ 释放总线，切到输入+中断，然后去睡觉等结果 */
HAL_GPIO_WritePin(..., GPIO_PIN_SET);
g_edge_cnt = 0;
g_recording = 1;
dht11_pin_input();

if (xSemaphoreTake(dht11_sem, pdMS_TO_TICKS(10)) == pdTRUE) {   // ★ 睡觉，等铃铛
    result = dht11_decode(temp, humi);   // 被叫醒后才走到这里
}

/* ④ 收尾，恢复空闲状态 */
```

**第 ③ 步的 `xSemaphoreTake(dht11_sem, 10ms)` 是灵魂**：

- 铃铛没响 → 任务**进入阻塞态（睡觉），CPU 让给别人**；
- 最多睡 10ms；
- 10ms 内中断摇铃 → 任务被叫醒，返回 `pdTRUE`，去解码；
- 10ms 还没响 → 超时，`result` 保持 1（读失败），**不会永远卡死**。

#### 第三步：中断采集 + 摇铃（`HAL_GPIO_EXTI_Callback`，bsp_dht11.c:69-89）

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin != DHT11_DQ_PIN || g_recording == 0) return;

    if (g_edge_cnt < DHT11_MAX_EDGES)
        g_edges[g_edge_cnt++] = DWT->CYCCNT;   // 记下这个边沿发生的时间

    if (g_edge_cnt >= DHT11_EDGE_TARGET) {     // 83个边沿 = 40位数据齐了
        g_recording = 0;
        xSemaphoreGiveFromISR(dht11_sem, &xHigherPriorityTaskWoken);  // ★ 摇铃
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
```

数据线每跳变一次就进一次这个回调，把"跳变发生在第几个时钟周期"记进 `g_edges[]`。采够 83 个边沿就 `xSemaphoreGiveFromISR(...)` 摇铃，睡着的任务被唤醒。

### 7.4 三个关键细节（这才是"看懂"的地方）

**细节 1：为什么开头要 `xSemaphoreTake(dht11_sem, 0)`？**
防"旧铃铛"。假如上次读取超时了，但中断还是摇了一下铃，留下一个没被拿走的信号。不清掉的话，下次读取一上来就"立刻拿到旧信号"，误以为数据来了，去解码一堆垃圾。`0` 表示"不阻塞地拿一次"，有旧的就清掉。**这是老手才会注意的坑。**

**细节 2：为什么 20ms 用 `vTaskDelay` 而不是 `delay_ms`？**
DHT11 要求拉低 20ms，但这 20ms 没必要让 CPU 死等。`vTaskDelay` 让任务睡 20ms，期间其他任务照跑。这就是"绝不让 CPU 傻等"的具体体现。注：`vTaskDelay` 是 FreeRTOS 原生函数，`osDelay` 是 CMSIS 封装，两者是同一个东西（bsp 驱动里直接用了原生版）。

**细节 3：为什么是 `GiveFromISR`，还有 `portYIELD_FROM_ISR`？**
FreeRTOS 规定**中断里不能调普通 API**，必须用 `FromISR` 后缀的版本（中断里不能随便"睡觉"）。`xHigherPriorityTaskWoken` + `portYIELD_FROM_ISR` 是"叫醒高优先级任务"的标准动作——摇铃后被叫醒的任务可能比被打断的任务优先级高，要告诉系统"中断结束后先去跑那个更高优先级的任务"。这个动作先照抄，细节以后再深究。

### 7.5 一张图看懂整体

```
        ┌───────────── 任务（StartDHT11Task） ─────────────┐
        │  发启动信号(拉低20ms, 用vTaskDelay睡觉)            │
        │  切到输入+中断                                     │
        │  xSemaphoreTake(sem, 10ms)  ←── 睡在这里，等铃铛   │
        │        ▲                                          │
        │        │ 铃铛响了，被叫醒                          │
        │  解码 → 写白板 → osDelay(2s)                       │
        └────────┼──────────────────────────────────────────┘
                 │
        ┌────────┴──── 中断（HAL_GPIO_EXTI_Callback） ────┐
        │  数据线每次跳变 → 记时间戳                         │
        │  采够83个边沿 → xSemaphoreGiveFromISR 摇铃        │
        └──────────────────────────────────────────────────┘
```

---

到这里，你的 FreeRTOS 四件套就齐了：**任务、优先级/抢占、互斥锁、信号量**。这份代码里已经没有"云里雾里"的地方了。

---

## 8. 常见疑问（FAQ）

**Q1：`osKernelStart()` 之后 main 的 `while(1)` 为什么永远不执行？**
因为 `osKernelStart()` 不返回，它把 CPU 永久交给调度器了。main 的 `while(1)` 是 CubeMX 模板残留，实际是死代码。

**Q2：任务之间怎么"传数据"？**
你的项目用的是**共享全局变量 + 互斥锁**（`g_monitor` + `g_monitor_mutex`）。这是最简单的方式，适合"几个任务共用一个状态"。更复杂的方式还有队列（queue），以后用到再说。

**Q3：`stack_size` 设小了会怎样？怎么知道该设多大？**
设小了会栈溢出，症状是"随机死机/数据乱跳"，非常难查。经验值：简单任务 128~256 字，带 LCD/浮点/大数组的任务 512 字起步。不确定就先给大点，跑通了再用 FreeRTOS 的 `uxTaskGetStackHighWaterMark()` 看实际用了多少。

**Q4：优先级设错了会怎样？**
如果 ledTask 设得比 dht11Task 还高，DHT11 每次要读数据时都得等 LED 让位，可能错过 10ms 超时窗口，导致读失败。所以"谁的时间最紧，谁优先级最高"。

**Q5：为什么 `osMutexAcquire` 要用 `osWaitForever` 而不是设个超时？**
因为你必须拿到笔才能写白板，拿不到就等。设超时会导致"超时了还没写成"，产生更麻烦的边界情况。新手先一律 `osWaitForever`。

---

## 9. 建议的下一步（怎么真正"入门"）

1. **先把这份文档和代码对照读一遍**，读到每段都能自己复述"为什么"；
2. **做一个小实验**：把 `ledTask` 里的 `osDelay(200)` 改成 `HAL_Delay(200)`，看其他三个任务会不会被卡死。亲手看到差异，比看十遍文档都管用；
3. **再加一个任务练手**：比如新建一个按键任务，按一下让 LED0 灭 1 秒。体会"多一个工人"是怎么加的；
4. 三个核心概念吃透后，再回头看第 7 节 DHT11 的信号量。

---

*这份文档只覆盖 FreeRTOS 部分。FSMC(LCD总线)、ADC、DMA、时钟树这些是 STM32 外设知识，属于另一条学习线，不要和 RTOS 混在一起学。*
