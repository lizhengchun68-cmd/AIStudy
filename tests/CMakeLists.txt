add_executable(aistudy_tests
    skill_protocol_test.cpp
)

target_compile_definitions(aistudy_tests
    PRIVATE
        AISTUDY_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(aistudy_tests
    PRIVATE
        scheduler
        GTest::gtest_main
)

# 主工程 Debug 使用 /MD；链接 lib/Release 中的 gtest（与 Poco 策略一致）
if(MSVC)
    set(_gtest_lib_release "${CMAKE_SOURCE_DIR}/thirdparty/gtest/lib/Release")
    foreach(_gtest_lib IN ITEMS gtest gtest_main gmock gmock_main)
        if(TARGET GTest::${_gtest_lib})
            set_target_properties(GTest::${_gtest_lib} PROPERTIES
                IMPORTED_LOCATION_DEBUG "${_gtest_lib_release}/${_gtest_lib}.lib"
            )
        endif()
    endforeach()
endif()

# 与 AIstudy 相同：复制 Poco 运行时 DLL 到测试可执行文件目录
set(_poco_bin_dir "${CMAKE_SOURCE_DIR}/thirdparty/poco/bin")
foreach(_poco_dll PocoFoundation.dll PocoJSON.dll)
    if(EXISTS "${_poco_bin_dir}/${_poco_dll}")
        add_custom_command(TARGET aistudy_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_poco_bin_dir}/${_poco_dll}"
                $<TARGET_FILE_DIR:aistudy_tests>
        )
    endif()
endforeach()

include(GoogleTest)
gtest_discover_tests(aistudy_tests
    DISCOVERY_MODE PRE_TEST
    WORKING_DIRECTORY $<TARGET_FILE_DIR:aistudy_tests>
)
