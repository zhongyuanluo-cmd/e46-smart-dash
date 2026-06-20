# 审查报告：Build / Config 文件

> 审查日期: 2026-05-31 | 审查人: Copilot | 模块: cmake/, src/CMakeLists.txt, can-gateway.json

---

## 整体评价：⭐⭐⭐⭐ (4/5)

构建体系设计合理：Monorepo + Phase-staging + 跨平台 toolchain 的架构清晰。没有致命的编译错误或配置遗漏。主要改进空间在于 toolchain flag 的整理、缺少默认 Build Type、JSON 中存在未被代码使用的字段。

---

## 1. cmake/toolchain-aarch64.cmake ⭐⭐⭐⭐

### 1.1 设计概览

- Arm GNU Toolchain 10.3, Windows host → aarch64 Linux target
- Sysroot 从 Core3566 板子 scp 提取
- Qt6 交叉编译 prefix 集成
- glibc 版本兼容处理（toolchain 2.33 vs board 2.28）

### 1.2 🟡 TC-1 — `-static-libstdc++` 在 CMAKE_CXX_FLAGS 中冗余

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ... -static-libstdc++ -static-libgcc")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS}  -static-libstdc++ -static-libgcc ...")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++ -static-libgcc ...")
set(CMAKE_REQUIRED_LINK_OPTIONS "-static-libstdc++;-static-libgcc")
```

**考量**:
- `CMAKE_CXX_FLAGS` 常用于编译+链接步骤，包含 linker flag 虽能工作但不规范
- 四个地方重复声明同一组 flag，修改时容易遗漏
- `CMAKE_REQUIRED_LINK_OPTIONS` 只影响 `check_cxx_source_compiles` 等 CMake 探测命令，不影响实际构建

**建议**: 将公共 linker flags 提取为变量，在三个 linker 变量中引用：
```cmake
set(E46_STATIC_LINK_FLAGS "-static-libstdc++ -static-libgcc")
set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} ${E46_STATIC_LINK_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${E46_STATIC_LINK_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${E46_STATIC_LINK_FLAGS}")
# CMAKE_CXX_FLAGS 移除 -static-libstdc++ -static-libgcc
```

---

### 1.3 🟡 TC-2 — 绝对路径硬编码

```cmake
set(TOOLCHAIN_PREFIX "C:/toolchains/gcc-arm-10.3-.../bin/aarch64-none-linux-gnu-")
set(CMAKE_SYSROOT "C:/toolchains/core3566-sysroot")
```

**考量**:
- 当前只有一台开发机，硬编码不是问题
- 但文档中说"交叉编译用 cmake/toolchain-aarch64.cmake"，如果未来有合作者或在 CI 中构建，路径会断裂
- toolchain 路径在 user memory 中有记录（`qt6-cross-compile.md`），但 toolchain file 本身也应该可配置

**建议**（低优先级）: 用 `$ENV{TOOLCHAIN_ROOT}` 作为备选：
```cmake
if(NOT DEFINED TOOLCHAIN_ROOT)
    set(TOOLCHAIN_ROOT "C:/toolchains/gcc-arm-10.3-2021.07-mingw-w64-i686-aarch64-none-linux-gnu")
endif()
set(TOOLCHAIN_PREFIX "${TOOLCHAIN_ROOT}/bin/aarch64-none-linux-gnu-")
```

---

### 1.4 🟢 TC-3 — c++config.h 手动 patch 的文档化

```cmake
# NOTE: glibc 2.30+ pthread features (pthread_mutex_clocklock...) are disabled
# by commenting out their #define in c++config.h (using #undef).
```

**考量**: 已经详细注释了为什么需要手动 patch。但手动编辑 toolchain 的系统头文件是脆弱的——重新安装 toolchain 后 patch 丢失。建议将 patch 步骤写成脚本（如 `scripts/patch-sysroot.sh`）。

**当前可接受**（Phase 2，单人开发）。

---

### 1.5 🟢 TC-4 — `-lrt` 冗余（glibc ≥ 2.17 后已不需要）

```cmake
set(CMAKE_EXE_LINKER_FLAGS "... -lpthread -ldl -lrt")
```

**考量**: `librt`（`clock_gettime` 等）在 glibc 2.17 起已合并入 `libc`。Core3566 使用 Debian 10 (glibc 2.28)，`-lrt` 是空操作。保留无害但多余。

**建议**: 如果将来升级到 musl libc（如 Alpine），`-lrt` 仍然是必需的。先保留，加注释。

---

### 1.6 ✅ TC-5 — `CMAKE_FIND_ROOT_PATH_MODE_*` 设置正确

```cmake
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # 使用 host 工具（moc, rcc 等）
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)    # 只用 sysroot 的库
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)    # 只用 sysroot 的头文件
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)    # 只用 sysroot 的 cmake 包
```

✅ 完美。`NEVER` for PROGRAM 确保 Qt 的 host 工具（moc/rcc）不会被 sysroot 中的交叉编译版本覆盖。

---

## 2. src/CMakeLists.txt (根) ⭐⭐⭐⭐

```cmake
cmake_minimum_required(VERSION 3.22)
project(e46-smart-dash VERSION 0.1.0 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -Wextra -Wpedantic)
```

### 2.1 🟡 RC-1 — 未设置默认 `CMAKE_BUILD_TYPE`

**现象**: 如果用户运行 `cmake -B build -DCMAKE_TOOLCHAIN_FILE=...` 忘记加 `-DCMAKE_BUILD_TYPE=Release`，构建将没有 `-O2`/`-O3`，在嵌入式板子上性能显著下降。

**建议**:
```cmake
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to Release")
endif()
```

---

### 2.2 🟡 RC-2 — Phase-staging 注释可能过期

```cmake
add_subdirectory(src/common)
add_subdirectory(src/can-gateway)      # Phase 2
# add_subdirectory(src/sensor-collector)  # Phase 3
# ...
```

**考量**: `ui-app/` 目录内已有 `main.cpp`/`main.qml`/`resources.qrc`（Qt eglfs Hello World），但 CMakeLists.txt 只是占位注释，源码无法编译。Phase-staging 的注释与实际目录内容不同步。

**建议**: 要么给 `ui-app/CMakeLists.txt` 写一个能编译 `main.cpp` 的临时目标（方便测试 Qt 交叉编译），要么在注释中标注"源码已就绪但未构建"。

---

### 2.3 🟢 RC-3 — `-Wpedantic` 可能产生噪音

**考量**: GCC 的 `-Wpedantic` 会对某些合法 C++17 代码产生警告（如 `extra ;` after member function）。在 Phase 2 快速迭代阶段可能造成不必要的编译噪音。但作为代码质量标准是有价值的。

**建议**: 保留，但如果编译时出现大量 false positive 可以降级为 `-Wpedantic -Wno-pedantic-ms-format`。

---

## 3. src/common/CMakeLists.txt ⭐⭐⭐⭐⭐

```cmake
add_library(e46-common STATIC
    src/logging.cpp
    src/config.cpp
    src/types.cpp
)
target_include_directories(e46-common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
```

完美。STATIC 库选择正确（嵌入式无动态库加载开销），PUBLIC include 让依赖方自动获得头文件路径。唯一的小问题是 `target_link_libraries(e46-common PUBLIC)` 有空注释，但无害。

---

## 4. src/can-gateway/CMakeLists.txt ⭐⭐⭐⭐

```cmake
add_executable(e46-can-gateway
    src/main.cpp src/GatewayDaemon.cpp src/CanDecoder.cpp
    src/CanInterface.cpp src/KBusInterface.cpp src/KBusDecoder.cpp
    src/AdcSampler.cpp
)
target_link_libraries(e46-can-gateway PRIVATE e46-common)
```

### 4.1 🟡 CGW-1 — 缺少显式系统库依赖

**现象**: `can-gateway` 使用了 `pthread`（通过 e46-common 间接）、`epoll`、`timerfd`、`socket` 等系统调用，但没有显式 link `pthread`。

**考量**: `CMakeLists.txt` 只链接 `e46-common`。系统库通过 toolchain 的 `CMAKE_EXE_LINKER_FLAGS` (`-lpthread -ldl -lrt`) 隐式链接。能工作，但依赖关系不透明——如果换一个不设 `CMAKE_EXE_LINKER_FLAGS` 的 toolchain，链接会失败。

**建议**:
```cmake
target_link_libraries(e46-can-gateway PRIVATE e46-common pthread)
```

CMake 的 `FindThreads` 会正确处理跨平台差异。

---

### 4.2 🟢 CGW-2 — 源码列表清晰

7 个源文件，一一列出，可读性好。

---

## 5. can-gateway.json ⭐⭐⭐⭐

```json
{
    "can":  { "interface": "can0", "bitrate": 500000 },
    "kbus": { "uart": "/dev/ttyS4", "baud": 9600 },
    "adc":  { "i2c_bus": 1, "i2c_addr": 72, "interval_sec": 1 },
    "dbus": { "bus": "session", "service": "com.e46.can1" },
    "logging": { "level": "INFO", "file": "/var/log/e46-cangw.log" }
}
```

### 5.1 🟡 JSON-1 — `can.bitrate` 字段未被代码使用

**现象**: `GatewayDaemon::init()` 中 `bringUp(500000)` 硬编码了 500000，Config 的 `can.bitrate` 没有被读取。

```cpp
// GatewayDaemon.cpp init():
canIf_->bringUp(500000);  // ← 硬编码，忽略 JSON 的 bitrate 字段
```

**考量**: JSON 中定义了 `"bitrate": 500000`，但如果改为 250000 或 1000000，代码不会使用。与 `kbus.baud` 的处理形成对比（`kbus.baud` 同样未被使用，因为 `open(kbusUart_, 9600)` 也硬编码了）。

**建议**: 从 Config 读取 bitrate/baud：
```cpp
int bitrate = cfg.get<int>("can.bitrate", 500000);
canIf_->bringUp(bitrate);

int kbusBaud = cfg.get<int>("kbus.baud", 9600);
kbusIf_->open(kbusUart_, kbusBaud);
```

---

### 5.2 🟡 JSON-2 — `adc.interval_sec` 字段未被使用

**现象**: JSON 定义了 `"interval_sec": 1`，但 `setupEpoll()` 中 timerfd 间隔硬编码为 1 秒：

```cpp
its.it_interval.tv_sec = 1;  // ← 硬编码，忽略 JSON
```

**建议**: 与 JSON-1 类似，从 Config 读取或删除 JSON 字段。

---

### 5.3 🟡 JSON-3 — `dbus` 配置段未实现

Phase 2 没有 DBus 功能，`publishVehicleData()` 只是 log 占位。保留 JSON 配置没问题，但建议加注释 `"// Phase 3: DBus publishing"`（JSON 不支持注释，可改为 `"_comment": "..."` 或在使用文档中说明）。

---

### 5.4 🟡 JSON-4 — `logging.level` 字段未被使用

**现象**: `Logger::init()` 没有从 Config 读取 log level。`GatewayDaemon::init()` 第一行就是硬编码的 `Logger::instance().init("e46-cangw", true, "/var/log/e46-cangw.log")`。

**建议**: 从 Config 读取 level 并传递给 Logger：
```cpp
std::string logLevel = cfg.get<std::string>("logging.level", "INFO");
// Logger::setLevel(parseLevel(logLevel));
```

---

## 6. 未来 Phase CMakeLists.txt ⭐⭐⭐

### 6.1 🟡 UI-1 — `ui-app/` 有源码但 CMakeLists.txt 是占位符

**现象**: `src/ui-app/` 包含 `main.cpp`、`main.qml`、`resources.qrc`（Qt6 eglfs Hello World），但 CMakeLists.txt 只写了一行注释，无法编译。

**考量**: 这是一个 Qt6 交叉编译的 Hello World 验证程序——之前已经验证过 Qt6 能 cross-compile 和在板子上运行。但现在它不在构建树中，意味着验证状态可能过期。

**建议**: 在 `ui-app/CMakeLists.txt` 中添加一个 `e46-ui-hello` target（被根 CMakeLists 注释掉），方便随时验证 Qt6 交叉编译链：

```cmake
# Phase 6: Qt 6 QML Main UI App
# Uncomment below in src/CMakeLists.txt when ready for Phase 6
# add_executable(e46-ui-hello
#     main.cpp
#     resources.qrc
# )
# target_link_libraries(e46-ui-hello PRIVATE Qt6::Quick Qt6::Gui Qt6::Core)
```

---

## 📊 汇总

| 模块 | 🔴 必须修复 | 🟡 建议修复 | 🟢 可选 |
|------|-----------|-----------|--------|
| toolchain.cmake | 0 | 2 (TC-1 flag去重, TC-2 硬编码路径) | 3 (TC-3/4/5) |
| src/CMakeLists.txt | 0 | 2 (RC-1 BUILD_TYPE, RC-2 phase注释) | 1 (RC-3) |
| common/CMakeLists | 0 | 0 | 0 |
| can-gateway/CMakeLists | 0 | 1 (CGW-1 显式pthread) | 1 (CGW-2) |
| can-gateway.json | 0 | 4 (JSON-1~4 字段未使用) | 0 |
| ui-app/CMakeLists | 0 | 1 (UI-1 源码未构建) | 0 |

### 修复优先级

1. **本 Phase (.json 对齐)**:
   - **JSON-1/JSON-2**: `can.bitrate` 和 `adc.interval_sec` 从 Config 读取，消除硬编码
   - **JSON-4**: `logging.level` 从 Config 读取

2. **低优先级 (🟡)**:
   - RC-1: 设置默认 CMAKE_BUILD_TYPE
   - CGW-1: 显式链接 pthread
   - UI-1: 给 ui-app 写一个可编译的临时 CMakeLists
   - TC-1: flag 提取为变量

3. **Phase 3+ (🟢)**:
   - TC-2: 环境变量替代硬编码路径
   - RC-2: Phase-staging 注释维护规则

---

## ✅ 做得好的地方

1. **Monorepo Phase-staging**: 注释驱动的子目录管理，清晰划分各阶段
2. **STATIC library**: e46-common 选型正确，避免嵌入式场景的动态库开销
3. **CMAKE_FIND_ROOT_PATH_MODE**: PROGRAM=NEVER + LIBRARY/INCLUDE=ONLY 的组合完美处理交叉编译
4. **CMAKE_EXPORT_COMPILE_COMMANDS**: ON 使 IDE 能提供准确的代码补全
5. **sysroot scp 注释**: 记录了 sysroot 的获取方法，可复现
