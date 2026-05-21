# common/status：错误码与 StatusOr（对外目标名 common_status）

set(COMMON_STATUS_SOURCES
    exception/error_category.cpp
    exception/error_code_registry.cpp
    exception/error_code_wrapper.cpp
    exception/error_info.cpp
    exception/exception.cpp
    exception/error_handler.cpp
    exception/error_messages.cpp
)

add_library(common_status STATIC ${COMMON_STATUS_SOURCES})

# 兼容 #include "common/common_status/..."（构建目录镜像头文件，不移动源文件）
set(_common_status_legacy_inc "${CMAKE_CURRENT_BINARY_DIR}/legacy_include")
set(_common_status_legacy_root "${_common_status_legacy_inc}/common/common_status")
file(GLOB_RECURSE _common_status_legacy_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
foreach(_common_status_hdr IN LISTS _common_status_legacy_headers)
    file(RELATIVE_PATH _common_status_rel
        "${CMAKE_CURRENT_SOURCE_DIR}" "${_common_status_hdr}")
    get_filename_component(_common_status_dest_dir
        "${_common_status_legacy_root}/${_common_status_rel}" DIRECTORY)
    file(MAKE_DIRECTORY "${_common_status_dest_dir}")
    file(COPY_FILE "${_common_status_hdr}"
        "${_common_status_legacy_root}/${_common_status_rel}"
        ONLY_IF_DIFFERENT)
endforeach()

target_include_directories(common_status
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        $<BUILD_INTERFACE:${_common_status_legacy_inc}>
)

target_link_libraries(common_status
    PUBLIC
        PocoFoundation::PocoFoundation
)

if(TARGET PocoJSON::PocoJSON)
    target_link_libraries(common_status PUBLIC PocoJSON::PocoJSON)
endif()

# 兼容历史 target 名称
add_library(common ALIAS common_status)
add_library(status ALIAS common_status)
