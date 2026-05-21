# HDF5 I/O：thirdparty/hdf5（Debug/Release 分目录）
option(AISTUDY_ENABLE_HDF5 "Build common_io_hdf5 (HDF5 I/O module)" ON)

if(NOT AISTUDY_ENABLE_HDF5)
    return()
endif()

set(AISTUDY_HDF5_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/hdf5")

if(NOT EXISTS "${AISTUDY_HDF5_ROOT}/include/H5Cpp.h")
    message(STATUS "HDF5 not found at ${AISTUDY_HDF5_ROOT}; common_io_hdf5 skipped.")
    return()
endif()

set(_aistudy_hdf5_required
    "${AISTUDY_HDF5_ROOT}/lib/Release/hdf5.lib"
    "${AISTUDY_HDF5_ROOT}/lib/Release/hdf5_cpp.lib"
    "${AISTUDY_HDF5_ROOT}/lib/Debug/hdf5.lib"
    "${AISTUDY_HDF5_ROOT}/lib/Debug/hdf5_cpp.lib"
    "${AISTUDY_HDF5_ROOT}/bin/Release/hdf5.dll"
    "${AISTUDY_HDF5_ROOT}/bin/Release/hdf5_cpp.dll"
    "${AISTUDY_HDF5_ROOT}/bin/Debug/hdf5.dll"
    "${AISTUDY_HDF5_ROOT}/bin/Debug/hdf5_cpp.dll"
)

foreach(_aistudy_hdf5_file IN LISTS _aistudy_hdf5_required)
    if(NOT EXISTS "${_aistudy_hdf5_file}")
        message(STATUS "HDF5 incomplete at ${AISTUDY_HDF5_ROOT} (missing ${_aistudy_hdf5_file}); common_io_hdf5 skipped.")
        return()
    endif()
endforeach()

set(AISTUDY_HDF5_INCLUDE_DIR "${AISTUDY_HDF5_ROOT}/include")

if(NOT TARGET AISTudy::HDF5)
    add_library(AISTudy::HDF5 SHARED IMPORTED GLOBAL)
    set_target_properties(AISTudy::HDF5 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${AISTUDY_HDF5_INCLUDE_DIR}"
        IMPORTED_IMPLIB_RELEASE "${AISTUDY_HDF5_ROOT}/lib/Release/hdf5.lib"
        IMPORTED_LOCATION_RELEASE "${AISTUDY_HDF5_ROOT}/bin/Release/hdf5.dll"
        IMPORTED_IMPLIB_DEBUG "${AISTUDY_HDF5_ROOT}/lib/Debug/hdf5.lib"
        IMPORTED_LOCATION_DEBUG "${AISTUDY_HDF5_ROOT}/bin/Debug/hdf5.dll"
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        MAP_IMPORTED_CONFIG_MINSIZEREL Release
    )
endif()

if(NOT TARGET AISTudy::HDF5_CPP)
    add_library(AISTudy::HDF5_CPP SHARED IMPORTED GLOBAL)
    set_target_properties(AISTudy::HDF5_CPP PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${AISTUDY_HDF5_INCLUDE_DIR}"
        IMPORTED_IMPLIB_RELEASE "${AISTUDY_HDF5_ROOT}/lib/Release/hdf5_cpp.lib"
        IMPORTED_LOCATION_RELEASE "${AISTUDY_HDF5_ROOT}/bin/Release/hdf5_cpp.dll"
        IMPORTED_IMPLIB_DEBUG "${AISTUDY_HDF5_ROOT}/lib/Debug/hdf5_cpp.lib"
        IMPORTED_LOCATION_DEBUG "${AISTUDY_HDF5_ROOT}/bin/Debug/hdf5_cpp.dll"
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        MAP_IMPORTED_CONFIG_MINSIZEREL Release
    )
    target_link_libraries(AISTudy::HDF5_CPP INTERFACE AISTudy::HDF5)
endif()

set(COMMON_IO_HDF5_SOURCES
    hdf5_handles.cpp
    hdf5_reader.cpp
    hdf5_writer.cpp
    hdf5_factory.cpp
    hdf5_file_lock.cpp
    hdf5_disk.cpp
    hdf5_checksum.cpp
    hdf5_logging.cpp
    hdf5_async.cpp
)

add_library(common_io_hdf5 STATIC ${COMMON_IO_HDF5_SOURCES})

target_include_directories(common_io_hdf5
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${AISTUDY_HDF5_INCLUDE_DIR}
)

target_compile_definitions(common_io_hdf5 PUBLIC AISTUDY_HDF5_ENABLED)

target_link_libraries(common_io_hdf5
    PUBLIC
        common_status
        PocoFoundation::PocoFoundation
        AISTudy::HDF5_CPP
        AISTudy::HDF5
)

message(STATUS "HDF5 enabled: common_io_hdf5 -> ${AISTUDY_HDF5_ROOT}")
