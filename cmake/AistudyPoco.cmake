# Prebuilt Poco (Foundation + JSON + Util) under thirdparty/poco/{include,lib,bin}/<Config>
# Headers: thirdparty/poco/include/Poco/...（标准布局，#include <Poco/...>）

set(AISTUDY_POCO_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/poco")
set(AISTUDY_POCO_INCLUDE_DIR "${AISTUDY_POCO_ROOT}/include")

if(NOT EXISTS "${AISTUDY_POCO_INCLUDE_DIR}/Poco/JSON/Object.h")
    message(FATAL_ERROR
        "Poco headers not found at ${AISTUDY_POCO_INCLUDE_DIR}/Poco/ "
        "(expected include/Poco/JSON/Object.h)")
endif()

function(aistudy_import_poco_shared target_name lib_basename)
    set(_impl_release "${AISTUDY_POCO_ROOT}/lib/Release/${lib_basename}.lib")
    set(_impl_debug "${AISTUDY_POCO_ROOT}/lib/Debug/${lib_basename}d.lib")
    set(_dll_release "${AISTUDY_POCO_ROOT}/bin/Release/${lib_basename}.dll")
    set(_dll_debug "${AISTUDY_POCO_ROOT}/bin/Debug/${lib_basename}d.dll")

    if(NOT EXISTS "${_impl_release}")
        message(FATAL_ERROR "Poco prebuilt missing: ${_impl_release}")
    endif()
    if(NOT EXISTS "${_impl_debug}")
        message(FATAL_ERROR "Poco prebuilt missing: ${_impl_debug}")
    endif()

    add_library(${target_name} SHARED IMPORTED GLOBAL)
    set_target_properties(${target_name} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${AISTUDY_POCO_INCLUDE_DIR}"
        IMPORTED_IMPLIB_RELEASE "${_impl_release}"
        IMPORTED_IMPLIB_DEBUG "${_impl_debug}"
        IMPORTED_IMPLIB_RELWITHDEBINFO "${_impl_release}"
        IMPORTED_IMPLIB_MINSIZEREL "${_impl_release}"
    )
    if(EXISTS "${_dll_release}")
        set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION_RELEASE "${_dll_release}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${_dll_release}"
            IMPORTED_LOCATION_MINSIZEREL "${_dll_release}"
        )
    endif()
    if(EXISTS "${_dll_debug}")
        set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION_DEBUG "${_dll_debug}"
        )
    endif()
endfunction()

function(aistudy_copy_poco_runtime_dlls target)
    foreach(_base IN ITEMS PocoFoundation PocoJSON PocoUtil)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${AISTUDY_POCO_ROOT}/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/$<IF:$<CONFIG:Debug>,${_base}d,${_base}>.dll"
                $<TARGET_FILE_DIR:${target}>
            COMMENT "Copy ${_base} runtime for ${target}"
        )
    endforeach()
endfunction()
