add_library(scheduler STATIC
    Dispatcher.cpp
    Dispatcher.h
)

target_include_directories(scheduler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scheduler
    PUBLIC
        adapter
)

if(TARGET PocoJSON::PocoJSON)
    target_link_libraries(scheduler PUBLIC PocoJSON::PocoJSON)
endif()
