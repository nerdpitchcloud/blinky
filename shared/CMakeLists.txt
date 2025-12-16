add_library(blinky_shared STATIC
    src/protocol.cpp
    src/metrics.cpp
    src/version.cpp
)

target_include_directories(blinky_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
