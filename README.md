# LVGL Desktop for RK356X

[![Platform](https://img.shields.io/badge/platform-RK356X%20%7C%20Linux%20Framebuffer-blue)]()
[![Language](https://img.shields.io/badge/language-C99-green)]()
[![GUI](https://img.shields.io/badge/GUI-LVGL%209-orange)]()
[![License](https://img.shields.io/badge/license-MIT-lightgrey)]()

一个基于 **LVGL 9** 的嵌入式 Linux 桌面系统，运行在 **RK356X + Buildroot** 平台上，直接输出到 `/dev/fb0`，不依赖 X11 / Wayland。包含 7 个可交互应用，覆盖**文件系统操作、图像处理、GPIO 硬件控制、多进程视频播放、TCP 网络通信与文件传输**等典型嵌入式场景。

> 项目定位：个人嵌入式 Linux 学习实践，系统性地练习从 framebuffer 显示、input 子系统输入、GPIO sysfs 控制，到 socket 编程、多线程、进程管理与自定义二进制协议。

---

## ✨ 项目亮点

- **无桌面环境依赖**：直接操作 `/dev/fb0` 与 `/dev/input/eventX`，适用于 Buildroot 等精简系统
- **硬件直控**：通过 sysfs 操作 GPIO / PWM / 背光，实现 LED 呼吸灯与蜂鸣器调频
- **多进程架构**：视频播放采用 `fork + execlp` 拉起 mplayer，通过命名管道 FIFO 实现 IPC
- **自定义二进制文件格式**：BMP → ARGB8888 raw 转换器，自行解析 BMP 头、行对齐、行序
- **网络文件传输**：TCP socket + `mmap` 零拷贝落盘，支持聊天与文件收发
- **软键盘系统**：滑动呼出式双侧软键盘，适配无物理键盘的触摸屏设备

---

## 📦 功能列表

| # | 应用 | 核心功能 | 涉及技术 |
|---|------|---------|---------|
| 1 | 文件管理器 | 目录浏览、创建/删除文件与目录 | `opendir`/`readdir`、`open`、`mkdir`、`remove` |
| 2 | 图片查看器 | 查看 `.raw`(ARGB8888) / `.bmp`，幻灯播放 | LVGL canvas、BMP 解析、framebuffer 直写 |
| 3 | BMP→RAW 转换器 | BMP 裁剪缩放为任意尺寸 ARGB8888 raw | BMP 头解析、行对齐、最近邻缩放 |
| 4 | 双人对战游戏 | 双人实时对战，血量/子弹碰撞检测 | LVGL timer、实时碰撞检测、状态机 |
| 5 | GPIO 控制 | LED/蜂鸣器 PWM 调光、背光亮度 | sysfs GPIO、软件 PWM、定时器 |
| 6 | 视频播放器 | 指定位置/尺寸播放视频，前进/后退/暂停 | `fork`/`execlp`、FIFO、`kill` 信号 |
| 7 | 局域网聊天 | TCP 聊天 + 文件传输 | socket、`poll`、`pthread`、`mmap` |

---

## 🛠️ 技术栈

### 语言与构建
- **C99**，单文件模块化组织（约 1500 行）
- **Makefile** 手写构建，`-Wall -Wextra` 零警告

### GUI
- **LVGL 9.x**：fbdev display driver、evdev input driver、canvas、timer、event system、CJK 字体

### 嵌入式 Linux 系统编程
- **Framebuffer**：`/dev/fb0` 直接读写，ARGB8888 像素格式
- **Input 子系统**：`/dev/input/eventX` + `poll` 非阻塞读取触摸事件
- **文件 I/O**：`open`/`read`/`write`/`lseek`/`mmap`，二进制文件解析
- **进程管理**：`fork` + `execlp` 拉起外部程序，`kill` 发送 `SIGSTOP`/`SIGCONT`/`SIGKILL`
- **IPC**：命名管道 FIFO 与 mplayer 通信
- **多线程**：`pthread_create` + `pthread_detach` 后台接收文件
- **信号**：进程间暂停/恢复控制

### 硬件交互
- **GPIO sysfs**：`/sys/class/gpio/export`、`direction`、`value`
- **软件 PWM**：5ms 定时器模拟占空比，驱动 LED 与蜂鸣器
- **背光控制**：`/sys/class/backlight/*/brightness`

### 网络
- **TCP Socket**：`socket`/`connect`/`send`/`recv`
- **I/O 多路复用**：`poll` 实现非阻塞收发
- **零拷贝落盘**：`mmap` + `ftruncate` 将接收数据直接映射到文件

### 图像处理
- **BMP 格式解析**：文件头 + 信息头、行 4 字节对齐、高度正负判断行序
- **像素格式转换**：BGR24 → BGRA32、最近邻缩放
- **ARGB8888 raw**：直接构造 LVGL canvas 缓冲区

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────┐
│                    LVGL 9 主循环                     │
│              lv_timer_handler() 每 5ms               │
└──────────┬──────────────────────────────┬───────────┘
           │                              │
    ┌──────▼──────┐                ┌──────▼──────┐
    │  display    │                │   input     │
    │  fbdev      │                │   evdev     │
    │  /dev/fb0   │                │ event6      │
    └─────────────┘                └─────────────┘
           │
    ┌──────▼──────────────────────────────────────┐
    │              7 个应用模块                    │
    ├─────────┬─────────┬─────────┬───────────────┤
    │ file    │ photo   │ bmp2raw │ game          │
    │ gpio    │ studio  │ chat    │ keyboard      │
    └────┬────┴────┬────┴────┬────┴───────┬───────┘
         │         │         │            │
    ┌────▼───┐ ┌───▼────┐ ┌──▼─────┐ ┌────▼─────┐
    │ sysfs  │ │ 文件IO │ │ fork   │ │ socket   │
    │ GPIO   │ │ mmap   │ │ FIFO   │ │ pthread  │
    └────────┘ └────────┘ └────────┘ └──────────┘
```

**分层设计**：

- **驱动层**：fbdev + evdev，屏蔽底层显示与输入差异
- **框架层**：LVGL 提供 widget、canvas、timer、event
- **应用层**：7 个 app 各自独立，通过统一事件回调接入
- **系统层**：文件 I/O、进程、线程、socket、GPIO

---

## 🔨 编译

```bash
make
```

编译产物为 `main`。

**依赖**：

- LVGL 9.x（需开启 `lv_font_simsun_16_cjk` 字体）
- gcc、make
- mplayer（仅视频播放功能需要）

---

## 🚀 运行

```bash
# 把游戏素材拷到运行目录
cp assets/*.raw .

# 需要 root 权限访问 /dev/fb0 和 GPIO
sudo ./main
```

**硬件要求**：

| 项目 | 配置 |
|------|------|
| 平台 | RK356X（或任意 Linux + `/dev/fb0`） |
| 分辨率 | 1024 × 600 |
| 触摸屏 | `/dev/input/event6` |
| GPIO | LED: 120/121/123/124，蜂鸣器: 111 |
| 背光 | `/sys/class/backlight/backlight/brightness` |

> 换板子需修改 `main.c` 中硬编码的设备节点、分辨率与 GPIO 编号。

---

## 📖 使用说明

### 首次登录

首次运行检测 `/exe.log/pass.log`，不存在则进入**首次设置**界面，输入用户名密码后创建。后续运行进入**登录**界面，校验通过进入桌面。

```
pass.log 格式：
第 1 行：用户名
第 2 行：密码
第 3 行：壁纸路径（可选）
```

忘记密码：`rm /exe.log/pass.log` 重新运行。

### 软键盘

屏幕左右边缘各有一个**隐藏滑块**，滑动呼出：

- **左侧**：a~z 字母键
- **右侧**：数字、符号、`back`/`ok`/`qk` 功能键，以及 `/my_photo`、`/my_studio`、`0 0`、`512 300` 快捷输入

先点击文本框激活光标，再滑出键盘，按 `ok` 提交。

### 壁纸设置

壁纸必须是 **1024×600 ARGB8888 raw**（2,457,600 字节）。

**方式一**：app2 中 `input photo path` 输入目录 → `input bz name` 输入 raw 完整路径 → 回车，自动写入 `pass.log` 第三行。

**方式二**：手动 `echo "/my_photo/bg.raw" >> /exe.log/pass.log`。

### 各应用要点

**app1 文件管理器**：输入路径浏览，下方三个输入框分别创建文件、创建目录、删除。删除时**留空回车 = 删除当前目录**（仅空目录生效）。

**app2 图片查看**：支持 `.raw` 和 `.bmp`，`photo tv` 按钮进入幻灯播放，任意点击退出。

**app3 BMP 转换**：输入 `x y`、`w l`、BMP 路径、raw 输出路径，回车转换。**支持任意尺寸**（只要 `x+w≤1024 && y+l≤600`），但用作壁纸/幻灯播放时必须为 1024×600。

**app4 双人游戏**：左侧玩家 1、右侧玩家 2，`W` 跳、`A` 左移、`S` 发射、`D` 右移。各 3 血 4 弹，血量归零判负。依赖 `assets/` 下 6 个 raw 素材。

**app5 GPIO**：6 个滑块控制 4 路 LED、蜂鸣器 PWM 占空比和背光亮度。

**app6 视频播放**：输入目录、位置、尺寸，点击列表播放。右侧 `<<` / `||` / `>>` 控制前进、暂停、后退。**尺寸宽度必须 > 50**。

**app7 聊天**：输入 `IP_端口` 连接服务端，支持文本聊天与文件接收（存到 `save_file/`）。

---

## 🎯 技术难点与实现细节

### 1. Framebuffer 直写与 LVGL 集成

LVGL 9 通过 `lv_linux_fbdev_create()` 直接映射 `/dev/fb0`，无需 X11。关键点：

- `fb_var_screeninfo` 获取分辨率与位深
- `mmap` 将显存映射到用户空间
- 双缓冲与 `FBIOPAN_DISPLAY` 避免撕裂

### 2. BMP 解析与行对齐

BMP 每行按 **4 字节对齐**，且 `height` 正负决定行序（自底向上 / 自顶向下）。解析时：

```c
int row_stride = (width * 3 + 3) / 4 * 4;   // 行对齐
int top_down = (height < 0);
// 按 row_stride 跳行，按 top_down 决定读序
```

忽略对齐或行序会导致图像错位、上下颠倒。

### 3. 软件 PWM 调光

sysfs GPIO 只支持 0/1，通过 **5ms 定时器 + 计数器取模**模拟 PWM：

```c
cnt = (cnt + 1) % 20;
gpio_set(led, cnt < duty ? 1 : 0);
```

20 档占空比，人眼视觉上是连续调光。

### 4. 多进程视频播放 + FIFO 控制

mplayer 作为子进程运行，通过命名管道接收控制命令：

```c
mkfifo("1.fifo", 0666);
pid = fork();
if (pid == 0)
    execlp("mplayer64", "-slave", "-input", "file=1.fifo", ...);
// 父进程写 FIFO 控制播放
write(fifo_fd, "seek 15\n", 8);
```

暂停/恢复通过 `kill(pid, SIGSTOP/SIGCONT)` 实现。

### 5. TCP 文件传输 + mmap 零拷贝

接收文件时，先用 `ftruncate` 设定大小，再 `mmap` 映射到用户空间，收到的数据直接 `memcpy` 到映射区，**避免 write 系统调用**：

```c
fd = open(path, O_RDWR | O_CREAT, 0644);
ftruncate(fd, file_size);
buf = mmap(NULL, file_size, PROT_WRITE, MAP_SHARED, fd, 0);
// recv 的数据直接 memcpy 到 buf
```

### 6. 事件驱动的 UI 架构

所有交互通过 LVGL 事件回调解耦：

```c
lv_obj_add_event_cb(btn, app_mode, LV_EVENT_CLICKED, canvas);
```

7 个 app 通过 `app_mode` 统一分发，避免硬编码跳转。

---

## 📁 目录结构

```
lvgl-desktop-rk356x/
├── README.md
├── LICENSE
├── .gitignore
├── Makefile
├── main.c                 # 单文件源码
├── assets/                # 游戏素材
│   ├── 1.raw              # 子弹 50x80
│   ├── 2.raw              # 背景 1024x600
│   ├── 3.raw ~ 6.raw      # 玩家 100x100
└── docs/
    └── screenshot.png
```

---

## ⚠️ 已知限制与改进方向

作为一个学习项目，以下问题**已识别但尚未修复**，也是后续迭代方向：

| 类别 | 问题 | 改进方案 |
|------|------|---------|
| 内存安全 | 路径拼接用 `strcat` 到固定 128 字节缓冲区 | 改用 `snprintf` + 长度检查 |
| 内存安全 | 网络接收的 `memcpy` 未严格校验边界 | 加入 `fget + n <= file_size` 校验 |
| 资源管理 | 游戏与 Studio 共用 `del_timer`，同时使用可能 UAF | 拆分为独立清理函数 |
| 可移植性 | `/dev/fb0`、`event6`、GPIO 编号、分辨率硬编码 | 抽取到配置文件或命令行参数 |
| 代码结构 | 单文件 1500 行 | 按 app 拆分为多模块，头文件声明接口 |
| 构建 | 手写 Makefile | 迁移 CMake，支持交叉编译工具链配置 |
| 测试 | 无自动化测试 | 引入单元测试与 sanitizer（ASan/UBSan） |

---

## 📄 License

MIT

---

## 👤 关于

本项目为个人嵌入式 Linux 学习实践，重点在**系统编程能力**的综合性练习：从底层 framebuffer / input / GPIO，到应用层 GUI、多进程、多线程、网络通信，完整覆盖嵌入式应用开发的典型技术栈。

如有问题或建议，欢迎提 Issue 或 PR。
