cmake_minimum_required(VERSION 3.16)
project(AIstudy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
    # 与 thirdparty Poco 一致：Debug 配置也使用 /MD（MultiThreadedDLL）
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
endif()

# 禁止 Poco 头文件 #pragma comment 自动链接 *d.lib（使用显式 IMPORTED 目标）
add_compile_definitions(POCO_NO_AUTOMATIC_LIBS)

include(${CMAKE_SOURCE_DIR}/cmake/AistudyPoco.cmake)
# 预编译头文件：thirdparty/poco/include/Poco/...
aistudy_import_poco_shared(PocoFoundation::PocoFoundation PocoFoundation)
aistudy_import_poco_shared(PocoJSON::PocoJSON PocoJSON)
aistudy_import_poco_shared(PocoUtil::PocoUtil PocoUtil)
target_link_libraries(PocoJSON::PocoJSON INTERFACE PocoFoundation::PocoFoundation)
target_link_libraries(PocoUtil::PocoUtil INTERFACE PocoJSON::PocoJSON)

include_directories(${CMAKE_SOURCE_DIR}/src)

option(AISTUDY_BUILD_TESTS "Build unit tests (Google Test)" OFF)

if(AISTUDY_BUILD_TESTS)
    set(GTest_DIR "${CMAKE_SOURCE_DIR}/thirdparty/gtest/lib/cmake/GTest")
    find_package(GTest CONFIG REQUIRED)
    enable_testing()
endif()

add_subdirectory(src)

if(AISTUDY_BUILD_TESTS)
    add_subdirectory(tests)
endif()
