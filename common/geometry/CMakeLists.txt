add_library(geometry INTERFACE)

target_include_directories(geometry INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
    target_compile_options(geometry INTERFACE /arch:AVX2)
else()
    target_compile_options(geometry INTERFACE -march=native)
endif()