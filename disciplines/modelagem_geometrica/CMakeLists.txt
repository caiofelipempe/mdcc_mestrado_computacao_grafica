option(BUILD_TRABALHO_1 "Compilar trabalho 1" ON)
option(BUILD_TRABALHO_2 "Compilar trabalho 2" ON)

if(BUILD_TRABALHO_1)
    add_subdirectory(trabalho_1)
endif()

if(BUILD_TRABALHO_2)
    add_subdirectory(trabalho_2)
endif()

add_subdirectory(utils)