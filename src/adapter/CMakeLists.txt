add_library(adapter STATIC
    rainflow_adapter.cpp
    rainflow_adapter.h
)

target_include_directories(adapter
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(adapter
    PUBLIC
        common_status
        common_logger
        kernel
)

if(TARGET PocoJSON::PocoJSON)
    target_link_libraries(adapter PUBLIC PocoJSON::PocoJSON)
endif()
