# Shared utilities library for all daemons
# Populated in Phase 2+ with logging, config, IPC helpers

add_library(e46-common INTERFACE)

target_include_directories(e46-common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Future contents:
# - logging.h / logging.cpp       (syslog + file logger)
# - config.h / config.cpp         (JSON config reader)
# - ipc.h / ipc.cpp               (DBus helper wrappers)
# - can-dbc.h / can-dbc.cpp       (DBC file parser)
# - ring-buffer.h / ring-buffer.c (lock-free ring buffer)
