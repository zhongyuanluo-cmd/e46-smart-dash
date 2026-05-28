## ADDED Requirements

### Requirement: Armbian image boots on Core3566
The system SHALL boot a functional Armbian (Debian 12, Linux 6.1) image on the Core3566 module via SD card, with UART console accessible through the CH343 USB-UART debugger at 1500000 baud.

#### Scenario: First boot via SD card
- **WHEN** the user flashes the Luckfox Armbian image to a 32GB SD card and inserts it into the Core3566 SD slot
- **THEN** the system boots to a root shell prompt on the UART console within 30 seconds
- **AND** `uname -r` reports kernel version 6.1.x

#### Scenario: Ethernet connectivity
- **WHEN** the CM4-IO-BASE-B Ethernet port is connected to a LAN with DHCP
- **THEN** the system obtains an IP address via DHCP within 10 seconds of boot
- **AND** `apt update` completes successfully

### Requirement: Rockchip BSP kernel with required drivers
The Armbian kernel SHALL include the following drivers compiled in or as modules: Panfrost GPU (`CONFIG_DRM_PANFROST`), MIPI DSI (`CONFIG_DRM_ROCKCHIP` + `CONFIG_ROCKCHIP_DW_MIPI_DSI`), GPIO sysfs (`CONFIG_GPIO_SYSFS`), spidev, I2C dev, and standard UART (8250).

#### Scenario: Verify kernel config
- **WHEN** the user runs `zcat /proc/config.gz | grep -E "PANFROST|ROCKCHIP|MIPI_DSI|GPIO_SYSFS|SPI_SPIDEV|I2C_CHARDEV"`
- **THEN** all listed config options show `=y` or `=m`

### Requirement: Cross-compilation toolchain setup
The development environment SHALL provide an aarch64 cross-compilation toolchain that compiles C++17 code targeting the Core3566's Cortex-A55, with CMake integration via a `toolchain-aarch64.cmake` file.

#### Scenario: Compile C++ hello world
- **WHEN** the user runs `cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake && cmake --build build`
- **THEN** a statically-linked aarch64 ELF binary is produced
- **AND** `file build/hello` reports `ELF 64-bit LSB executable, ARM aarch64`

#### Scenario: Binary runs on target
- **WHEN** the binary is copied to Core3566 via `scp`
- **THEN** executing it on the target prints the expected output with exit code 0

### Requirement: Qt 6.8 cross-compilation for aarch64
The development environment SHALL cross-compile Qt 6.8 (Base, Quick, QuickControls2, EglFSDeviceIntegration) for aarch64, producing shared libraries that link against Mali-G52 EGL/GLESv2.

#### Scenario: Qt 6 eglfs smoke test
- **WHEN** a minimal Qt application using `QGuiApplication` with eglfs platform plugin is cross-compiled and deployed
- **THEN** running `./qt-smoke -platform eglfs` on the target renders a colored rectangle on the DSI display
- **AND** the application exits cleanly with exit code 0

### Requirement: Project repository skeleton
The project repository SHALL follow a monorepo structure with `src/`, `config/`, `scripts/`, and `docs/` directories, and a top-level `CMakeLists.txt` that builds all daemons.

#### Scenario: Empty build succeeds
- **WHEN** the user runs `cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake`
- **THEN** CMake configures successfully with no errors
- **AND** the build directory contains the configured build system
