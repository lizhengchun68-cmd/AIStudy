# JSON I/O（Poco JSON + common_status）
set(COMMON_IO_JSON_SOURCES
    json_reader.cpp
    json_writer.cpp
    json_stream_parser.cpp
    json_advanced.cpp
)

add_library(common_io_json STATIC ${COMMON_IO_JSON_SOURCES})

target_include_directories(common_io_json
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(common_io_json
    PUBLIC
        common_status
        PocoFoundation::PocoFoundation
)

if(TARGET PocoJSON::PocoJSON)
    target_link_libraries(common_io_json PUBLIC PocoJSON::PocoJSON)
endif()

# JSON <-> HDF5 桥接（依赖 common_io_hdf5）
if(TARGET common_io_hdf5)
    target_sources(common_io_json PRIVATE json_hdf5_bridge.cpp)
    target_link_libraries(common_io_json PUBLIC common_io_hdf5)
    target_compile_definitions(common_io_json PRIVATE AISTUDY_HDF5_ENABLED)
endif()
