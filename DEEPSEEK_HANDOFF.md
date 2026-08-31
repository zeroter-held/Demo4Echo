# Echo-Mate 二次开发交接说明

更新时间：2026-08-28

## 1. 项目是什么

这是 No-Chicken `Demo4Echo` 的二次开发工程，目标硬件为 RV1106 开发板，桌面 UI 使用 LVGL，显示分辨率为 320×240。当前新增了一个番茄钟 App，同时保留原桌面布局和 AIChat、YOLO 等功能。

项目采用三端协作：

- Windows：保存 Git 工作区和交接文件。
- Ubuntu 虚拟机：使用 RV1106 SDK 进行 ARM/uClibc 交叉编译。
- RV1106 开发板：从 NAND Flash 的 `/root/bin` 运行程序。

## 2. 当前目录

Windows Git 工作区：

```text
E:\360MoveData\Users\22806\Desktop\Echo robot\Demo4Echo
```

Ubuntu 工程：

```text
/home/zxx360zxx/Projects/Echo-Mate/Demo/DeskBot_demo
```

Ubuntu SDK：

```text
/home/zxx360zxx/Projects/Echo-Mate/SDK/rv1106-sdk
```

开发板运行目录：

```text
/root/bin
```

## 3. 当前 Git 状态

交接分支：

```text
feature/focus-timer-handoff
```

当前修改尚未提交。先执行以下命令了解范围，不要直接使用 `git add .`：

```bash
git status --short
git diff --stat
git diff -- DeskBot_demo/gui_app
```

## 4. 番茄钟实现

主要文件：

```text
DeskBot_demo/gui_app/pages/ui_FocusPage/ui_FocusPage.c
DeskBot_demo/gui_app/pages/ui_FocusPage/ui_FocusPage.h
DeskBot_demo/gui_app/fonts/ui_font_focus22.c
DeskBot_demo/gui_app/pages/ui_HomePage/ui_HomePage.c
DeskBot_demo/gui_app/ui.c
DeskBot_demo/gui_app/ui.h
```

实现行为：

- 默认时长 25 分钟。
- 支持开始、暂停、重置和返回。
- 页面退出时删除 `lv_timer_t`，避免重复进入后残留定时器。
- 原第二页摄像机预留图标已恢复。
- 番茄钟放在独立第三页。
- 桌面分页数量由 2 改为 3。
- 使用仅包含番茄钟所需字符的专用字体，避免整套中文字库占用 NAND。

注意：原摄像机按钮会尝试打开 `CameraPage`，但仓库中没有该页面实现。运行日志会显示 `Page with name 'CameraPage' not found`。这是原项目的预留入口，不是番茄钟造成的回归。

## 5. 交叉编译环境

SDK 文件数约 107317，Windows 统计容量约 2.78GB。工具链：

```text
arm-rockchip830-linux-uclibcgnueabihf
```

目标 ABI：

```text
ARM 32-bit EABI5, hard-float, uClibc
```

已在工具链 sysroot 中补齐：

- json-c
- OpenSSL
- curl
- ALSA
- PortAudio
- Opus
- jsoncpp
- WebSocket++
- Boost.System
- OpenBLAS 0.3.34
- libdrm 2.4.134

OpenBLAS配置为 ARM32、CBLAS-only、无 Fortran、无 LAPACK、单线程、静态库，主要供 Snowboy 使用。

## 6. 编译方法

Ubuntu执行：

```bash
cd ~/Projects/Echo-Mate/Demo/DeskBot_demo
export RV1106_SDK_PATH=~/Projects/Echo-Mate/SDK/rv1106-sdk
cmake -S . -B build -DTARGET_ARM=ON
cmake --build build --parallel 4
file bin/main
readelf -d bin/main | grep -E 'RPATH|RUNPATH|NEEDED'
```

成功标准：

- `file bin/main` 显示 ARM 32-bit EABI5。
- 解释器为 `/lib/ld-uClibc.so.0`。
- RPATH 为 `$ORIGIN/lib`。
- 不出现 x86_64 或 `GLIBC_*` 目标库污染。

SDL2/SDL2_image 的 CMake 警告可忽略，因为 ARM 板端后端不使用 SDL 模拟器。

`RV1106_SDK_PATH` 的作用是让工具链文件不依赖固定用户名或固定家目录。若未设置，当前工程会回退到原 Ubuntu 路径。

LVGL 是 Git 子模块。父仓库不能直接记录子模块工作区中的未提交修改，因此额外保存了：

```text
patches/lvgl-findlibdrm.patch
```

在 `DeskBot_demo/lvgl` 中应用：

```bash
git apply ../../patches/lvgl-findlibdrm.patch
```

## 7. 运行库打包

`DeskBot_demo/CMakeLists.txt` 会把所需 ARM共享库复制到：

```text
bin/lib
```

程序运行时应能通过 `$ORIGIN/lib` 查找它们。当前开发板曾需要显式设置：

```sh
export LD_LIBRARY_PATH=/root/bin/lib
```

若直接启动提示 `can't load library 'libssl.so.3'`，先检查：

```sh
ls -l /root/bin/lib/libssl.so.3
readelf -d /root/bin/main | grep -E 'RPATH|RUNPATH'
```

## 8. 开发板部署约束

开发板 USB 网络：

```text
开发板：172.32.0.93
电脑：172.32.0.100
```

根分区约 181.3MB，部署时曾仅剩约 9.5MB。不要上传完整 SDK、完整备份或重复的 `bin` 目录到 NAND。

部署原则：

1. 先停止 `main`。
2. 只替换 `/root/bin/main` 和确实变更的运行库。
3. 不覆盖 `/root/bin/system_para.conf`。
4. 若 `main` 处于不可中断设备状态，`SIGKILL` 也可能无法停止；此时重启开发板后再替换。

开发板原始 `/root/bin` 已备份在 Windows：

```text
C:\Users\22806\Documents\Codex\2026-08-22\g-r\deploy-20260828-113515\backup\bin
```

## 9. Wi-Fi、DHCP、NTP与时区

启动 Wi-Fi：

```sh
ifconfig wlan0 up
mkdir -p /var/run/wpa_supplicant
wpa_supplicant -B -c /etc/wpa_supplicant.conf -i wlan0
wpa_cli -i wlan0 status
```

`wpa_state=COMPLETED` 表示已通过 Wi-Fi认证，但不代表已有 IP。继续执行 DHCP：

```sh
udhcpc -i wlan0
ifconfig wlan0
ping -c 3 www.baidu.com
```

NTP是 Network Time Protocol，用来联网校准系统时间。开发板重启后可能回到固件默认的 2021 年，联网后由 `ntpd` 校时。

系统采用 UTC 保存时间，时区文件已设置为：

```text
/etc/TZ = CST-8
```

POSIX时区中 `CST-8` 表示 UTC+8。项目使用 `localtime()`，因此界面显示北京时间。

## 10. AIChat 架构

AIChat是“开发板 C++ 客户端 + Windows Python WebSocket服务器”的结构：

```text
开发板麦克风/客户端
        ↓ WebSocket
Windows Python AIChat Server :8000
        ↓
VAD / SenseVoice ASR / 大模型 / TTS
```

电脑服务器监听：

```text
0.0.0.0:8000
```

开发板配置通常指向 USB电脑地址：

```text
AIChat_server_url=172.32.0.100
AIChat_server_port=8000
```

API Key不得提交到 Git。Python服务器从环境变量读取：

```powershell
$env:DASHSCOPE_API_KEY = Read-Host "请输入 DashScope API Key"
python .\main.py --access_token="你的私有令牌"
```

## 11. 密钥安全

以下内容禁止上传给任何模型或公共仓库：

- SSH私钥
- Wi-Fi密码
- 阿里云/DashScope API Key
- 高德 API Key
- 真实 AIChat访问令牌
- 开发板真实 `system_para.conf`

仓库中的示例配置已替换为占位符。开发板现有配置未被覆盖。

提交前执行：

```bash
git grep -n -I -E 'sk-[A-Za-z0-9._-]+|gaode_api_key=[^y]|AIChat_server_token=123456'
```

## 12. 已知问题

- `CameraPage` 没有实现，摄像机图标只是预留入口。
- 项目部分旧代码存在函数签名和 const 类型警告，当前未作为番茄钟任务处理。
- `main` 使用摄像头/显示设备时可能进入不可中断状态，导致普通信号无法结束。
- Wi-Fi和主程序尚未配置完整开机自启动流程。
- 开发板 NAND空间紧张，后续开发应优先增量部署。

## 13. DeepSeek 首轮任务

请先阅读本文件和 `git diff`，不要立即大范围重构。优先检查：

1. 番茄钟页面生命周期和 `lv_timer_t` 释放是否安全。
2. 第三页坐标、分页点和滑动边界是否正确。
3. `ui_font_focus22.c` 是否包含页面使用的全部中文字符。
4. 构建输出是否仍是 ARM/uClibc，不能混入 Ubuntu x86库。
5. 不修改、输出或提交任何真实密钥。
6. 不覆盖开发板 `/root/bin/system_para.conf`。
7. 每一步解释“做什么、为什么、命令含义、成功标准和失败含义”。
