# CAN Gateway Daemon (Phase 2)
# SocketCAN interface + PT-CAN decoder + K-Bus listener + DBus broadcaster

add_executable(e46-can-gateway
    src/main.cpp
    src/GatewayDaemon.cpp
    src/CanDecoder.cpp
    src/CanInterface.cpp
    src/KBusInterface.cpp
    src/KBusDecoder.cpp
    src/AdcSampler.cpp
)

target_include_directories(e46-can-gateway PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(e46-can-gateway PRIVATE
    e46-common
    pthread
)
