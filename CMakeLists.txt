cmake_minimum_required(VERSION 3.10)
project(XenoHost LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    arduino_compat.cpp
    src/xeno/debug/xeno_debug_tools.cpp
    src/xeno/main/xeno_compiler.cpp
    src/xeno/main/xeno_vm.cpp
    src/xeno/security/xeno_security_config.cpp
    src/xeno/security/xeno_security.cpp
    src/xeno/xeno_common.cpp
    src/XenoLanguage.cpp
    xeno_host.cpp
    version.rc
)

add_executable(xeno_host ${SOURCES})

target_include_directories(xeno_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(xeno_host PRIVATE Ws2_32)
