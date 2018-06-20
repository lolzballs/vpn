cmake_minimum_required(VERSION 3.6)
project(vpn C)

set(CMAKE_C_STANDARD 99)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(LibUV REQUIRED)

IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(
        PLATFORM_SRC 
        tun_linux.c
        tun_linux.h
        util_linux.c
    )
endif()

add_executable(
    vpn
    main.c
    util.c
    util.h
    trie.c
    trie.h
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
