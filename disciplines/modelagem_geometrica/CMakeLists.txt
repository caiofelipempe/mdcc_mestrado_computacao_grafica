option(BUILD_TRABALHO_1 "Compilar trabalho 1" ON)
option(BUILD_TRABALHO_2 "Compilar trabalho 2" ON)
option(BUILD_TRABALHO_3 "Compilar trabalho 3" ON)

add_subdirectory(utils)

if(BUILD_TRABALHO_1)
    add_subdirectory(trabalho_1)
endif()

if(BUILD_TRABALHO_2)
    add_subdirectory(trabalho_2)
endif()

if(BUILD_TRABALHO_3)
    add_subdirectory(trabalho_3)
endif()
