# Cross-compilation toolchain for Core3566 (RK3566, Cortex-A55)
# Usage: cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Toolchain binaries (adjust path to your Linaro/Arm toolchain)
set(TOOLCHAIN_PREFIX "C:/toolchains/aarch64-linux-gnu/bin/aarch64-linux-gnu-")

set(CMAKE_C_COMPILER   "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++")
set(CMAKE_AR           "${TOOLCHAIN_PREFIX}ar")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_PREFIX}objcopy")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_PREFIX}objdump")
set(CMAKE_STRIP        "${TOOLCHAIN_PREFIX}strip")
set(CMAKE_RANLIB       "${TOOLCHAIN_PREFIX}ranlib")

# Sysroot: copy from Core3566 via:
#   rsync -avz --copy-links core3566:/lib     /opt/core3566-sysroot/
#   rsync -avz --copy-links core3566:/usr/lib /opt/core3566-sysroot/usr/
#   rsync -avz --copy-links core3566:/usr/include /opt/core3566-sysroot/usr/
set(CMAKE_SYSROOT "/opt/core3566-sysroot")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Qt 6 cross-compiled installation path (on build host)
set(QT6_HOST_PREFIX "C:/toolchains/qt6-aarch64")
set(CMAKE_PREFIX_PATH "${QT6_HOST_PREFIX}")

# Target-specific flags
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mcpu=cortex-a55 -march=armv8.2-a")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a55 -march=armv8.2-a")
