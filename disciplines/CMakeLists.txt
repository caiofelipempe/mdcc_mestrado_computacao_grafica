option(BUILD_ALGEBRA_LINEAR "Compilar trabalhos da disciplina de Álgebra Linear" ON)
option(BUILD_MODELAGEM_GEOMETRICA "Compilar trabalhos da disciplina de Modelagem Geométrica" ON)

if(BUILD_ALGEBRA_LINEAR)
    add_subdirectory(algebra_linear)
endif()

if(BUILD_MODELAGEM_GEOMETRICA)
    add_subdirectory(modelagem_geometrica)
endif()