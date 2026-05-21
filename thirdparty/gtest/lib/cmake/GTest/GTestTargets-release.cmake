#----------------------------------------------------------------
# GTest import file for configuration "Release".
#----------------------------------------------------------------

set(CMAKE_IMPORT_FILE_VERSION 1)

set_property(TARGET GTest::gtest APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(GTest::gtest PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/Release/gtest.lib"
  )

list(APPEND _cmake_import_check_targets GTest::gtest)
list(APPEND _cmake_import_check_files_for_GTest::gtest "${_IMPORT_PREFIX}/lib/Release/gtest.lib")

set_property(TARGET GTest::gtest_main APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(GTest::gtest_main PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/Release/gtest_main.lib"
  )

list(APPEND _cmake_import_check_targets GTest::gtest_main)
list(APPEND _cmake_import_check_files_for_GTest::gtest_main "${_IMPORT_PREFIX}/lib/Release/gtest_main.lib")

set_property(TARGET GTest::gmock APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(GTest::gmock PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/Release/gmock.lib"
  )

list(APPEND _cmake_import_check_targets GTest::gmock)
list(APPEND _cmake_import_check_files_for_GTest::gmock "${_IMPORT_PREFIX}/lib/Release/gmock.lib")

set_property(TARGET GTest::gmock_main APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(GTest::gmock_main PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/Release/gmock_main.lib"
  )

list(APPEND _cmake_import_check_targets GTest::gmock_main)
list(APPEND _cmake_import_check_files_for_GTest::gmock_main "${_IMPORT_PREFIX}/lib/Release/gmock_main.lib")

set(CMAKE_IMPORT_FILE_VERSION)
