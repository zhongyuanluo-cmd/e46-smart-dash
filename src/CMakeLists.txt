cmake_minimum_required(VERSION 3.22)
project(e46-smart-dash VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Common compile options
add_compile_options(-Wall -Wextra -Wpedantic)

# Default to Release if not set (critical for embedded performance)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to Release")
endif()

# Subdirectories (populated in later phases)
add_subdirectory(src/common)
add_subdirectory(src/can-gateway)      # Phase 2
# add_subdirectory(src/sensor-collector)  # Phase 3
# add_subdirectory(src/audio-manager)     # Phase 4
# add_subdirectory(src/voice-engine)      # Phase 5
# add_subdirectory(src/ui-app)            # Phase 6
# add_subdirectory(src/track-logger)      # Phase 7
# add_subdirectory(src/carplay)           # Phase 8
