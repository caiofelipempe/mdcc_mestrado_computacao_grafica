add_library(geometry INTERFACE
    arithmetic.hpp
    vector.hpp
    point.hpp
    mesh.hpp
)

target_include_directories(geometry INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)