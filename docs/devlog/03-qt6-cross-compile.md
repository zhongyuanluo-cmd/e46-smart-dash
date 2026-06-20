# Dev Log — Qt 6 Cross-Compilation

> 日期：2026-05-30
> Qt version: 6.8.2
> Toolchain: Arm GNU Toolchain 10.3-2021.07 (mingw-w64-i686, aarch64-none-linux-gnu)
> 结果：✅ 全部成功，Mali-G52 硬件加速运行

## 1. Toolchain

- Source: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain
- Version: GCC 10.3.1 (aarch64-none-linux-gnu)
- Install path: `C:\toolchains\gcc-arm-10.3-2021.07-mingw-w64-i686-aarch64-none-linux-gnu\`
- Test compile: `aarch64-none-linux-gnu-g++.exe --version` → 10.3.1
- 关键限制: toolchain 的 libstdc++ 需要 glibc 2.33+，但 Core3566 只有 glibc 2.28
  - 解决: `-static-libstdc++ -static-libgcc` 静态链接 C++ 运行时

## 2. Core3566 Sysroot

从板子同步系统库到 Windows sysroot:
```bash
rsync -avz linaro@192.168.1.161:/usr/lib/aarch64-linux-gnu/ C:\toolchains\core3566-sysroot\usr\lib\
rsync -avz linaro@192.168.1.161:/lib/aarch64-linux-gnu/ C:\toolchains\core3566-sysroot\lib\
rsync -avz linaro@192.168.1.161:/usr/include/ C:\toolchains\core3566-sysroot\usr\include\
```

板子安装的开发包: `build-essential cmake libfontconfig1-dev libfreetype6-dev libxkbcommon-dev libgbm-dev libdrm-dev libegl1-mesa-dev libgles2-mesa-dev`

关键修复:
1. **multiarch 目录扁平化**: `/usr/lib/aarch64-linux-gnu/` → `/usr/lib/`，`/lib/aarch64-linux-gnu/` → `/lib/`
2. **libc.so linker script**: 修复路径指向扁平化后的目录
3. **c++config.h**: 注释掉三个 glibc 2.30+ 的宏（板子 glibc 2.28 不支持）

## 3. Qt6 Configure & Build

### qtbase (1052 steps, 0 failures)
```bash
cmake "C:\toolchains\qt6-src\qt-everywhere-src-6.8.2\qtbase" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="D:\My Trunk\车机助手\cmake\toolchain-aarch64.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="C:/toolchains/qt6-aarch64" \
  -DQT_HOST_PATH="C:/toolchains/Qt6-host/6.8.2/mingw_64" \
  -DQT_BUILD_EXAMPLES=OFF -DQT_BUILD_TESTS=OFF \
  -DINPUT_opengl=es2 -DFEATURE_xcb=OFF -DFEATURE_wayland=OFF \
  -DFEATURE_linuxfb=OFF -DFEATURE_vulkan=OFF -DFEATURE_glib=OFF \
  -DFEATURE_dbus=OFF -DFEATURE_sql=OFF -DFEATURE_testlib=OFF \
  -DFEATURE_printsupport=OFF -DFEATURE_pkg_config=OFF \
  -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON
```

关键配置结果: EGL=yes, GLES2=yes, EGLFS=yes, EGLFS GBM=yes

### qtshadertools (80 steps, 0 failures)
```bash
cmake "C:\toolchains\qt6-src\qt-everywhere-src-6.8.2\qtshadertools" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="D:\My Trunk\车机助手\cmake\toolchain-aarch64.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="C:/toolchains/qt6-aarch64" \
  -DQT_HOST_PATH="C:/toolchains/Qt6-host/6.8.2/mingw_64" \
  -DQT_BUILD_EXAMPLES=OFF -DQT_BUILD_TESTS=OFF \
  -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON
```

### qtdeclarative/QML (3062 steps, 0 failures)
同 qtshadertools 配置，仅替换源码路径。

## 4. Deployment

部署策略: tar + scp + ldconfig
```bash
# 只打包版本号 .so（避免 Windows 符号链接问题）
tar -cf qt6-deploy-lib.tar libQt6*.so.6.8.2    # 147MB (50 files)
tar -cf qt6-deploy-plugins.tar -C plugins .      # 8.5MB
tar -cf qt6-deploy-qml.tar -C qml .             # 12MB

# 传输到板子
scp qt6-deploy-*.tar linaro@192.168.1.161:/tmp/

# 解压
sudo tar -xf /tmp/qt6-deploy-lib.tar -C /usr/local/lib/
sudo tar -xf /tmp/qt6-deploy-plugins.tar -C /usr/local/plugins/
sudo tar -xf /tmp/qt6-deploy-qml.tar -C /usr/local/qml/

# 更新库缓存（创建 .so.6 符号链接）
sudo /sbin/ldconfig
```

安装 Mali-G52 GPU 驱动:
```bash
sudo dpkg -i /packages/libmali/libmali-bifrost-g52-g2p0-gbm_1.9-1_arm64.deb
sudo /sbin/ldconfig
```

- 库: 50 个 .so 文件 → `/usr/local/lib/`
- 插件: 8 个目录 → `/usr/local/plugins/` (platforms, egldeviceintegrations, imageformats, etc.)
- QML: 7 个目录 → `/usr/local/qml/` (Qt, QtQml, QtQuick, QtCore, QtNetwork, QML, Assets)

## 5. QML Test

```bash
$ sudo QT_QPA_PLATFORM=eglfs QT_QPA_EGLFS_INTEGRATION=eglfs_kms QSG_INFO=1 /home/linaro/qt6-test/qt6-test
```

| Check | Expected | Actual | Notes |
|------|:---:|:---:|------|
| App starts without crash | yes | ✅ | |
| EGL vendor: ARM | yes | ✅ | |
| GL renderer: Mali-G52 | yes | ✅ | Mali-G52, OpenGL ES 3.2 |
| Hardware acceleration | yes | ✅ | Not LLVMpipe |
| QML Window renders | yes | ✅ | Threaded render loop, vsync 16.67ms |

输出关键行:
```
qt.rhi.general: OpenGL VENDOR: ARM RENDERER: Mali-G52 VERSION: OpenGL ES 3.2 v1.g2p0-01eac0
```

## 6. Issues & Workarounds

1. **PCH `__SSP_STRONG__` mismatch**: GCC cross-compiler 的 PCH 机制与 `-fstack-protector-strong` 冲突
   - 解决: `-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON`
   
2. **glibc 版本不匹配**: toolchain libstdc++ 需要 glibc 2.33+，板子只有 2.28
   - 解决: `-static-libstdc++ -static-libgcc` 静态链接

3. **CMake find_package 失败**: qtshadertools 找不到 Qt6Core
   - 解决: 在 toolchain 中添加 `CMAKE_FIND_ROOT_PATH` 包含 qt6-aarch64

4. **PowerShell 吞噬 g++ stderr**: PowerShell 把 stderr 当错误流，管道时丢失
   - 解决: 用 `cmd /c` 执行编译命令，或 `Out-File` 重定向

5. **Windows tar 不保留 Unix 符号链接**: tar 打包时符号链接变成常规文件
   - 解决: 只打包 `.so.6.8.2` 版本文件，在板子上用 ldconfig 生成符号链接

6. **光标警告**: "Failed to move cursor on screen DSI1: -14" / "Could not set cursor: -6"
   - 状态: 可忽略，嵌入式 DRM/KMS 常见，不影响渲染

7. **Mesa 18.3.6 不含 Panfrost**: Debian 10 的 Mesa 太旧，没有 RK3566 开源驱动
   - 解决: 使用预装的 Mali 闭源驱动 (`libmali-bifrost-g52-g2p0-gbm`)

---

## 7. 踩坑详细记录

> 以下按时间顺序记录，每个坑都包括：现象 → 排查过程 → 根因 → 解决方案 → 教训

### 坑 1: Sysroot multiarch 路径找不到库

- **现象**: CMake configure 阶段报 `Could NOT find EGL`、`Could NOT find GLESv2`，尽管板上确实装了
- **排查**: 检查 `CMAKE_SYSROOT` 下目录结构，发现 Debian 的 `.so` 在 `/usr/lib/aarch64-linux-gnu/` 而非 `/usr/lib/`
- **根因**: CMake 的 `find_library` 在 sysroot 中搜索 `/usr/lib/`，但 Debian multiarch 把库放在 `/usr/lib/aarch64-linux-gnu/`
- **解决**: rsync 时扁平化——`/usr/lib/aarch64-linux-gnu/` → `/usr/lib/`，`/lib/aarch64-linux-gnu/` → `/lib/`；同时修复 `libc.so` linker script 里的路径
- **教训**: 交叉编译 sysroot 遇到 Debian/Ubuntu multiarch 时，必须处理 `aarch64-linux-gnu` 子目录，要么扁平化要么在 CMake 里加搜索路径

### 坑 2: c++config.h 中 glibc 2.30+ 宏

- **现象**: configure 阶段 `HAVE_STDATOMIC` 检测失败，CMake 报 "Could NOT find WrapAtomic"
- **排查**: 手动编译 CMake 的检测程序，发现 undefined reference to `pthread_mutex_clocklock` 等 glibc 2.30+ 函数
- **根因**: sysroot 的 `c++config.h`（来自 toolchain）引用了 glibc 2.30+ 的新 API（`_GLIBCXX_USE_PTHREAD_MUTEX_CLOCKLOCK` 等），但板子 glibc 2.28 没有这些符号
- **解决**: 在 sysroot 的 `c++config.h` 中 `#undef` 三个宏
- **教训**: 交叉编译时 toolchain 的 libstdc++ headers 可能比目标 glibc 更新，需要手动检查兼容性

### 坑 3: PCH `__SSP_STRONG__` 不匹配（最难排查，耗时最长）

- **现象**: qtbase 编译到 QtOpenGL 时，8 个 .cpp 文件全部编译失败。但 PowerShell 不显示任何错误
- **排查**:
  1. 用 `cmd /c` 运行 g++，仍然看不到错误（stderr 被吞）
  2. 手动复制完整编译命令运行，**去掉 PCH 参数**后编译成功
  3. 加上 `-include cmake_pch.hxx` 后编译，终于看到 `cc1plus.exe: warning: cmake_pch.hxx.gch: not used because '__SSP_STRONG__' not defined`
- **根因**: GCC PCH 机制要求生成 PCH 和使用 PCH 时的预处理器宏必须完全一致。`-fstack-protector-strong` 在内部定义 `__SSP_STRONG__`，但 Windows 托管交叉编译器的 PCH 验证逻辑有 bug，无法正确匹配
- **解决**: `-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON` 禁用 PCH，删除所有 `.gch` 文件重新编译
- **教训**:
  - Windows 托管的 GCC 交叉编译器 PCH 不可靠，直接禁用
  - 编译失败时 PowerShell 的 stderr 处理是严重干扰，必须用 `cmd /c` 或重定向到文件
  - 逐步剥离编译选项（二分法）是定位问题的有效手段

### 坑 4: CMake find_package 找不到已安装的 Qt6 模块

- **现象**: qtshadertools configure 阶段报 `Could NOT find Qt6BuildInternals`，`Could NOT find Qt6Core`
- **排查**: 确认 `C:/toolchains/qt6-aarch64/lib/cmake/` 下确实有 Qt6Config.cmake 等文件
- **根因**: `CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY` 导致 CMake 只在 `CMAKE_FIND_ROOT_PATH` 中搜索，而 qt6-aarch64 不在其中
- **解决**: `set(CMAKE_FIND_ROOT_PATH "C:/toolchains/core3566-sysroot" "C:/toolchains/qt6-aarch64")` 将两个路径都加入
- **教训**: 交叉编译时 CMake 的 `FIND_ROOT_PATH` 机制需要仔细配置，确保所有依赖路径都在搜索范围内

### 坑 5: PowerShell 吞噬 g++ stderr

- **现象**: ninja 构建失败但 PowerShell 不显示任何编译错误信息
- **根因**: PowerShell 把外部命令的 stderr 当作 error stream，管道 `|` 只传递 stdout，stderr 被丢弃
- **解决**: 三种方案——
  1. `2>&1 | Out-File build-log.txt` 重定向到文件后查看
  2. `cmd /c "cmake --build . 2>&1"` 用 cmd 代替 PowerShell
  3. `2>&1 | Select-String "FAILED"` 搜索失败关键词
- **教训**: 在 Windows PowerShell 下做交叉编译，永远先配置日志输出策略

### 坑 6: Windows tar 不保留 Unix 符号链接

- **现象**: 打包 442MB tar（应该只有 ~150MB），`tar -tvf` 显示所有文件都是 regular file
- **根因**: Windows tar 在 NTFS 上无法正确创建/读取 Unix 符号链接
- **解决**: 只打包 `.so.6.8.2` 版本号文件（147MB），在板子上 `ldconfig` 自动创建 `.so.6` 符号链接
- **教训**: Windows→Linux 部署共享库时，不要试图保留符号链接，让目标系统 ldconfig 生成

### 坑 7: Mali-G52 硬件驱动缺失

- **现象**: 首次测试 QML app 时输出 "Running on a software rasterizer (LLVMpipe)"，无硬件加速
- **排查**: `ls /usr/lib/aarch64-linux-gnu/dri/` 无 rockchip/panfrost 驱动；`dpkg -l | grep mesa` 显示 Mesa 18.3.6（太旧，没有 Panfrost）
- **根因**: Debian 10 的 Mesa 18.3.6 不包含 Panfrost 开源驱动（需要 Mesa 19.2+）
- **解决**: 板子 `/packages/libmali/` 下有预装的 ARM Mali 闭源驱动 deb 包，安装 `libmali-bifrost-g52-g2p0-gbm_1.9-1_arm64.deb`
- **教训**: RK3566 在 Debian 10 上需要闭源 Mali 驱动；如果升级到 Debian 11+（Mesa 20.3+）可以用 Panfrost 开源驱动

---

## 8. 时间线

| 阶段 | 耗时 | 备注 |
|------|:---:|------|
| 工具链+源码下载解压 | ~2h | 含网络下载等待 |
| Sysroot 搭建+修复 | ~3h | multiarch 扁平化、linker script、c++config.h |
| qtbase configure | ~5min | |
| qtbase 首次构建→失败→排查 PCH | ~4h | 最大坑：PCH 问题排查 |
| qtbase 重构建 (PCH disabled) | ~25min | 1052 steps |
| qtshadertools 构建 | ~5min | 80 steps |
| qtdeclarative 构建 | ~40min | 3062 steps |
| 部署+驱动安装+测试 | ~30min | |
| **总计** | **~10h** | |

## 9. 如果重来一次的优化路径

1. **一开始就加 `-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON`**（省 4h 排查时间）
2. **一开始就把 `CMAKE_FIND_ROOT_PATH` 设好**（省 30min）
3. **用 `Out-File` 日志方式构建**，不依赖 PowerShell 实时输出
4. **先装 Mali 驱动再测试**，避免走软件渲染器弯路
