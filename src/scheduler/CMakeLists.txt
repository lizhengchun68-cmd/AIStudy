include(${CMAKE_SOURCE_DIR}/cmake/AistudySkills.cmake)
aistudy_verify_skill_registry(${CMAKE_SOURCE_DIR})

add_library(scheduler STATIC
    Dispatcher.cpp
    Dispatcher.h
    skill_manifest.cpp
    skill_manifest.h
    skill_protocol.cpp
    skill_protocol.h
    skill_payload_validator.cpp
    skill_payload_validator.h
    skill_registry.cpp
    skill_registry.h
    skill_context_types.h
    context_handle_rules.cpp
    context_handle_rules.h
    skill_context_store.cpp
    skill_context_store.h
)

target_include_directories(scheduler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(scheduler
    PUBLIC
        AISTUDY_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(scheduler
    PUBLIC
        adapter
        common_status
        common_logger
)

if(TARGET PocoJSON::PocoJSON)
    target_link_libraries(scheduler PUBLIC PocoJSON::PocoJSON)
endif()
if(TARGET PocoFoundation::PocoFoundation)
    target_link_libraries(scheduler PUBLIC PocoFoundation::PocoFoundation)
endif()

if(TARGET common_io_hdf5)
    target_compile_definitions(scheduler PRIVATE AISTUDY_HDF5_ENABLED)
endif()
