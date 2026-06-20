# Cross-compilation toolchain for Core3566 (RK3566, Cortex-A55)
# Usage: cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Toolchain binaries (Arm GNU Toolchain 10.3 for Windows host)
set(TOOLCHAIN_PREFIX "C:/toolchains/gcc-arm-10.3-2021.07-mingw-w64-i686-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-")

set(CMAKE_C_COMPILER   "${TOOLCHAIN_PREFIX}gcc.exe")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++.exe")
set(CMAKE_AR           "${TOOLCHAIN_PREFIX}ar.exe")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_PREFIX}objcopy.exe")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_PREFIX}objdump.exe")
set(CMAKE_STRIP        "${TOOLCHAIN_PREFIX}strip.exe")
set(CMAKE_RANLIB       "${TOOLCHAIN_PREFIX}ranlib.exe")

# Sysroot: copied from Core3566 board via scp
#   scp -r linaro@192.168.1.161:/usr/include C:\toolchains\core3566-sysroot\usr\
#   scp -r linaro@192.168.1.161:/usr/lib     C:\toolchains\core3566-sysroot\usr\
#   scp -r linaro@192.168.1.161:/lib         C:\toolchains\core3566-sysroot\
set(CMAKE_SYSROOT "C:/toolchains/core3566-sysroot")
set(CMAKE_FIND_ROOT_PATH "C:/toolchains/core3566-sysroot" "C:/toolchains/qt6-aarch64")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Qt 6 cross-compiled installation path (on build host)
set(QT6_HOST_PREFIX "C:/toolchains/qt6-aarch64")
set(CMAKE_PREFIX_PATH "${QT6_HOST_PREFIX}")

# Linker flags repeated across EXE/SHARED/MODULE — extract to variable
set(E46_STATIC_LINK_FLAGS "-static-libstdc++ -static-libgcc")

# Target-specific flags
# -static-libstdc++ -static-libgcc: avoid glibc_2.33 mismatch (board has glibc 2.28, toolchain needs 2.33)
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mcpu=cortex-a55 -march=armv8.2-a -static-libgcc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a55 -march=armv8.2-a")
# NOTE: glibc 2.30+ pthread features (pthread_mutex_clocklock, pthread_cond_clockwait) are disabled
# by commenting out their #define in c++config.h (using #undef). We cannot use -D flags here because
# #ifdef checks for definition (not value), and -D defines the macro even if set to 0.
set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}    ${E46_STATIC_LINK_FLAGS} -lpthread -ldl -lrt")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${E46_STATIC_LINK_FLAGS} -lpthread -ldl -lrt")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${E46_STATIC_LINK_FLAGS} -lpthread -ldl -lrt")
# Tell CMake's FindThreads that pthread needs explicit linking
set(CMAKE_THREAD_LIBS_INIT "-lpthread;-ldl;-lrt")
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Ensure CMake's check_cxx_source_compiles and similar tests can link properly
# CMake lists use semicolons as separators
set(CMAKE_REQUIRED_LINK_OPTIONS "-static-libstdc++;-static-libgcc")
set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES};m;dl;pthread;rt")
