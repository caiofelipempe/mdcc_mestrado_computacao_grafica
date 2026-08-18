option(BUILD_ALGEBRA_LINEAR "Compilar trabalhos da disciplina de Álgebra Linear" ON)

if(BUILD_ALGEBRA_LINEAR)
    add_subdirectory(algebra_linear)
endif()