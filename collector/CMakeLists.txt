add_executable(blinky-collector
    src/main.cpp
    src/websocket_server.cpp
    src/metrics_store.cpp
    src/http_server.cpp
)

target_include_directories(blinky-collector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(OpenSSL REQUIRED)

target_link_libraries(blinky-collector PRIVATE
    blinky_shared
    pthread
    OpenSSL::SSL
    OpenSSL::Crypto
)
