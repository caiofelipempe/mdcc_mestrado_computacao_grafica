add_library(result INTERFACE
    result.hpp
)

target_include_directories(result INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)