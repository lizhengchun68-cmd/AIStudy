add_library(kernel STATIC
    rainflow/rainflow.cpp
    rainflow/rainflow.h
    rainflow/internal/rainflow_algorithm.h
    rainflow/internal/rainflow_three_point.h
    rainflow/internal/rainflow_three_point.cpp
    rainflow/internal/rainflow_four_point.h
    rainflow/internal/rainflow_four_point.cpp   
    rainflow/internal/rainflow_modified_four_point.h
    rainflow/internal/rainflow_modified_four_point.cpp
)

target_include_directories(kernel
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kernel
    PRIVATE
        common
)