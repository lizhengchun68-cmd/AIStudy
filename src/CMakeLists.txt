add_subdirectory(common)
add_subdirectory(kernel)
add_subdirectory(adapter)
add_subdirectory(scheduler)

# 主程序
add_executable(AIstudy main.cpp)

# 链接所有子模块
# scheduler -> adapter -> kernel + common_status（分层依赖，main 只依赖调度层）
target_link_libraries(AIstudy
    PRIVATE
        scheduler
)

# 自动复制 Poco 运行时 DLL（仅使用预编译 thirdparty/poco/bin）
set(_poco_bin_dir "${CMAKE_SOURCE_DIR}/thirdparty/poco/bin")
foreach(_poco_dll PocoFoundation.dll PocoJSON.dll)
    if(EXISTS "${_poco_bin_dir}/${_poco_dll}")
        add_custom_command(TARGET AIstudy POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_poco_bin_dir}/${_poco_dll}"
                $<TARGET_FILE_DIR:AIstudy>
        )
    endif()
endforeach()

# 复制 HDF5 运行时 DLL（thirdparty/hdf5）
set(_aistudy_hdf5_root "${CMAKE_SOURCE_DIR}/thirdparty/hdf5")
if(EXISTS "${_aistudy_hdf5_root}/bin/Release/hdf5.dll")
    foreach(_hdf5_dll hdf5.dll hdf5_cpp.dll)
        add_custom_command(TARGET AIstudy POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_aistudy_hdf5_root}/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/${_hdf5_dll}"
                $<TARGET_FILE_DIR:AIstudy>
        )
    endforeach()
endif()