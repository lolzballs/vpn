cmake_minimum_required(VERSION 3.6)
project(vpn C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)

set(CMAKE_C_STANDARD 99)

IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(
        PLATFORM_SRC 
        tun_linux.c
        util_linux.c
    )
endif()

add_executable(
    vpn
    main.c
    util.c
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
