add_library(common STATIC
    status_or.h
    exception/error_category.cpp
    exception/error_code_registry.cpp
    exception/error_code_wrapper.cpp
    exception/error_info.cpp
    exception/exception.cpp
    exception/error_handler.cpp
    exception/error_messages.cpp
)

target_include_directories(common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(common
    PUBLIC
        PocoFoundation::PocoFoundation
)

if(TARGET PocoJSON::PocoJSON)
    target_link_libraries(common PUBLIC PocoJSON::PocoJSON)
endif()
