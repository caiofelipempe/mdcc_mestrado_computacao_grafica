add_library(geometry INTERFACE
    arithmetic.hpp
    vector.hpp
    point.hpp
    mesh.hpp
    shape.hpp
)

target_compile_options(
    geometry
    INTERFACE
    -march=native
)

target_include_directories(geometry INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
    add_compile_options(/arch:AVX2)
else()
    # Ativa AVX, AVX2 e FMA de forma global e segura para todas as TUs
    add_compile_options(-mavx -mavx2 -mfma)
    # Ou para otimizar especificamente para o processador da máquina atual:
    # add_compile_options(-march=native)
endif()