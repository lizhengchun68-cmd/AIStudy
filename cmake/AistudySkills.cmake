# Verify skills/*/manifest.json matches src/scheduler/skill_registry_bindings.txt

function(aistudy_verify_skill_registry source_dir)
    set(bindings_file "${source_dir}/src/scheduler/skill_registry_bindings.txt")
    if(NOT EXISTS "${bindings_file}")
        message(FATAL_ERROR "Missing skill registry bindings file: ${bindings_file}")
    endif()

    file(READ "${bindings_file}" bindings_raw)
    string(REPLACE "\r\n" "\n" bindings_raw "${bindings_raw}")
    string(STRIP "${bindings_raw}" bindings_raw)
    string(REPLACE "\n" ";" bindings_list "${bindings_raw}")

    set(binding_ids "")
    foreach(line ${bindings_list})
        string(STRIP "${line}" line)
        if(line STREQUAL "")
            continue()
        endif()
        string(SUBSTRING "${line}" 0 1 first_char)
        if(first_char STREQUAL "#")
            continue()
        endif()
        list(APPEND binding_ids "${line}")
    endforeach()

    if("${binding_ids}" STREQUAL "")
        message(FATAL_ERROR "skill_registry_bindings.txt has no skill ids")
    endif()

    set(manifest_ids "")
    file(GLOB skill_manifests RELATIVE "${source_dir}/skills" "${source_dir}/skills/*/manifest.json")
    foreach(rel_path ${skill_manifests})
        get_filename_component(skill_dir "${rel_path}" DIRECTORY)
        list(APPEND manifest_ids "${skill_dir}")
    endforeach()
    list(REMOVE_DUPLICATES manifest_ids)
    list(SORT manifest_ids)
    list(SORT binding_ids)

    foreach(id ${binding_ids})
        set(manifest_path "${source_dir}/skills/${id}/manifest.json")
        if(NOT EXISTS "${manifest_path}")
            message(FATAL_ERROR
                "Skill binding '${id}' has no manifest at skills/${id}/manifest.json")
        endif()
    endforeach()

    foreach(id ${manifest_ids})
        if(NOT id IN_LIST binding_ids)
            message(FATAL_ERROR
                "skills/${id}/manifest.json exists but '${id}' is missing from "
                "src/scheduler/skill_registry_bindings.txt (and kBindings)")
        endif()
    endforeach()

    message(STATUS "Skill registry verify OK: ${binding_ids}")
endfunction()
