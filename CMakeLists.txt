cmake_minimum_required(VERSION 3.6)
project(vpn C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)

set(CMAKE_C_STANDARD 99)

add_executable(
    vpn
    main.c
    tun_linux.c
    util.c
    util_linux.c
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
