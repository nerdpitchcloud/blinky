add_executable(blinky-agent
    src/main.cpp
    src/collector.cpp
    src/cpu_monitor.cpp
    src/memory_monitor.cpp
    src/disk_monitor.cpp
    src/smart_monitor.cpp
    src/network_monitor.cpp
    src/systemd_monitor.cpp
    src/container_monitor.cpp
    src/kubernetes_monitor.cpp
    src/websocket_client.cpp
)

target_include_directories(blinky-agent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(OpenSSL REQUIRED)

target_link_libraries(blinky-agent PRIVATE
    blinky_shared
    pthread
    OpenSSL::SSL
    OpenSSL::Crypto
)
