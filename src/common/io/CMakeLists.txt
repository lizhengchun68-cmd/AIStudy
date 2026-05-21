# common/io：I/O 子模块（HDF5 须在 json 之前，以便 json 桥接目标已存在）
add_subdirectory(HDF5)
add_subdirectory(json)

# 将 JSON（及 HDF5 桥接）并入 common_status，供 adapter / scheduler / 主程序使用
if(TARGET common_io_json AND TARGET common_status)
    target_link_libraries(common_status PUBLIC common_io_json)
endif()
