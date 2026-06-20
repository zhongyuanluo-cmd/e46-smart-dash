# Shared utilities library for all daemons

add_library(e46-common STATIC
    src/logging.cpp
    src/config.cpp
    src/types.cpp
)

target_include_directories(e46-common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# nlohmann/json is included via <nlohmann/json.hpp> (prefix path),
# so only the parent include/ dir is needed above.

target_link_libraries(e46-common PUBLIC
    # syslog, file I/O — system libraries
)
