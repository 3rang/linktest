cmake_minimum_required(VERSION 3.10)
project(linktest C)

set(CMAKE_C_STANDARD 11)

add_executable(linktest
    src/main.c
    src/platform.c
    src/discover.c
    src/tput.c
)

target_include_directories(linktest PRIVATE inc)

if(MSVC)
    target_compile_options(linktest PRIVATE /W4 /O2)
else()
    target_compile_options(linktest PRIVATE -Wall -Wextra -O2)
endif()

if(WIN32)
    target_link_libraries(linktest ws2_32 iphlpapi)
endif()
