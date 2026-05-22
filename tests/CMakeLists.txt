add_executable(aistudy_tests
    skill_protocol_test.cpp
    rainflow_contract_test.cpp
    skill_payload_validator_test.cpp
    skill_registry_test.cpp
    skill_host_health_test.cpp
    host_echo_contract_test.cpp
    context_handle_rules_test.cpp
    context_store_test.cpp
    dispatcher_context_error_test.cpp
    mesh_import_contract_test.cpp
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



aistudy_copy_poco_runtime_dlls(aistudy_tests)



include(GoogleTest)

gtest_discover_tests(aistudy_tests

    DISCOVERY_MODE PRE_TEST

    WORKING_DIRECTORY $<TARGET_FILE_DIR:aistudy_tests>

)


