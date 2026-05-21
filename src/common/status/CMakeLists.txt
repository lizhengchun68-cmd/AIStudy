# common/status：错误码与 StatusOr（对外目标名 common_status）

set(COMMON_STATUS_SOURCES
    api_response.cpp
    exception/error_category.cpp
    exception/error_code_registry.cpp
    exception/error_code_wrapper.cpp
    exception/error_info.cpp
    exception/exception.cpp
    exception/error_handler.cpp
    exception/error_messages.cpp
)

add_library(common_status STATIC ${COMMON_STATUS_SOURCES})

target_include_directories(common_status
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(common_status
    PUBLIC
        PocoFoundation::PocoFoundation
)

if(TARGET PocoJSON::PocoJSON)
    target_link_libraries(common_status PUBLIC PocoJSON::PocoJSON)
endif()
