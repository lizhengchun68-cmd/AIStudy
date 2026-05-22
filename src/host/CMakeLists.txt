add_library(host STATIC
    skill_host.cpp
    skill_host.h
    host_exit_codes.h
    http_server.cpp
    http_server.h
)

target_include_directories(host
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(host
    PUBLIC
        scheduler
)

if(TARGET PocoJSON::PocoJSON)
    target_link_libraries(host PUBLIC PocoJSON::PocoJSON)
endif()

if(WIN32)
    target_link_libraries(host PUBLIC ws2_32)
endif()
