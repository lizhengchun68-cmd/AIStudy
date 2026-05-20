cmake_minimum_required(VERSION 3.16)
project(AIstudy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# 禁止 Poco 头文件 #pragma comment 自动链接 *d.lib（thirdparty 仅含 Release 库）
add_compile_definitions(POCO_NO_AUTOMATIC_LIBS)

set(POCO_INSTALL_DIR ${CMAKE_SOURCE_DIR}/thirdparty/poco)

# thirdparty 仅提供 Release 版 Poco；Debug 构建也链接同一 .lib，避免 LNK1104 (PocoFoundationd.lib)
set(POCO_LIB "${POCO_INSTALL_DIR}/lib/PocoFoundation.lib")
set(POCO_DLL "${POCO_INSTALL_DIR}/bin/PocoFoundation.dll")

if(NOT EXISTS "${POCO_LIB}")
    message(FATAL_ERROR "PocoFoundation.lib not found under ${POCO_INSTALL_DIR}")
endif()

add_library(PocoFoundation::PocoFoundation SHARED IMPORTED GLOBAL)
set_target_properties(PocoFoundation::PocoFoundation PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${POCO_INSTALL_DIR}/include"
    IMPORTED_IMPLIB "${POCO_LIB}"
    IMPORTED_IMPLIB_RELEASE "${POCO_LIB}"
    IMPORTED_IMPLIB_DEBUG "${POCO_LIB}"
    IMPORTED_IMPLIB_RELWITHDEBINFO "${POCO_LIB}"
    IMPORTED_IMPLIB_MINSIZEREL "${POCO_LIB}"
)
if(EXISTS "${POCO_DLL}")
    set_target_properties(PocoFoundation::PocoFoundation PROPERTIES
        IMPORTED_LOCATION "${POCO_DLL}"
        IMPORTED_LOCATION_RELEASE "${POCO_DLL}"
        IMPORTED_LOCATION_DEBUG "${POCO_DLL}"
        IMPORTED_LOCATION_RELWITHDEBINFO "${POCO_DLL}"
        IMPORTED_LOCATION_MINSIZEREL "${POCO_DLL}"
    )
endif()

set(POCO_JSON_LIB "${POCO_INSTALL_DIR}/lib/PocoJSON.lib")
set(POCO_JSON_DLL "${POCO_INSTALL_DIR}/bin/PocoJSON.dll")

if(EXISTS "${POCO_JSON_LIB}")
    add_library(PocoJSON::PocoJSON SHARED IMPORTED GLOBAL)
    set_target_properties(PocoJSON::PocoJSON PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${POCO_INSTALL_DIR}/include"
        IMPORTED_IMPLIB "${POCO_JSON_LIB}"
        IMPORTED_IMPLIB_RELEASE "${POCO_JSON_LIB}"
        IMPORTED_IMPLIB_DEBUG "${POCO_JSON_LIB}"
        IMPORTED_IMPLIB_RELWITHDEBINFO "${POCO_JSON_LIB}"
        IMPORTED_IMPLIB_MINSIZEREL "${POCO_JSON_LIB}"
    )
    if(EXISTS "${POCO_JSON_DLL}")
        set_target_properties(PocoJSON::PocoJSON PROPERTIES
            IMPORTED_LOCATION "${POCO_JSON_DLL}"
            IMPORTED_LOCATION_RELEASE "${POCO_JSON_DLL}"
            IMPORTED_LOCATION_DEBUG "${POCO_JSON_DLL}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${POCO_JSON_DLL}"
            IMPORTED_LOCATION_MINSIZEREL "${POCO_JSON_DLL}"
        )
    endif()
    target_link_libraries(PocoJSON::PocoJSON INTERFACE PocoFoundation::PocoFoundation)
endif()

include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)
