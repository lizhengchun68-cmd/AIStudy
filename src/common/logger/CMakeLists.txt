# common/logger：Poco 日志封装与错误码联动

set(COMMON_LOGGER_SOURCES
    logger.cpp
    logger_config.cpp
    logger_error.cpp
    log_record.cpp
)

add_library(common_logger STATIC ${COMMON_LOGGER_SOURCES})

target_include_directories(common_logger
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(common_logger
    PUBLIC
        common_status
        PocoFoundation::PocoFoundation
    PRIVATE
        common_io_json
)
