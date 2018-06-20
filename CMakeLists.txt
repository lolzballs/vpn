cmake_minimum_required(VERSION 3.6)
project(vpn C)

set(CMAKE_C_STANDARD 99)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(LibUV REQUIRED)

IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(
        PLATFORM_SRC 
        src/tun_linux.c
        src/tun_linux.h
        src/util_linux.c
    )
endif()

add_executable(
    vpn
    src/main.c
    src/util.c
    src/util.h
    src/trie.c
    src/trie.h
    ${PLATFORM_SRC}
)

target_link_libraries(
        vpn
        ${LIBUV_LIBRARIES}
)
target_include_directories(
        vpn
        PUBLIC
        ${LIBUV_INCLUDE_DIRS}
)

