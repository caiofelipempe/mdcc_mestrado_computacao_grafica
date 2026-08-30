cmake_minimum_required(VERSION 3.16)

project(
    geometria_computacional
    VERSION 1.0
    DESCRIPTION "Trabalhos do MDCC em C++ com OpenGL, GLFW e ImGui"
    LANGUAGES CXX
)

# -----------------------------
# Configurações globais
# -----------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Dependências via vcpkg
# -----------------------------
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(CGAL REQUIRED)

# -----------------------------
# Pastas de compilações
# -----------------------------
option(BUILD_DISCIPLINES "Compilar trabalhos das disciplinas" ON)
option(BUILD_PROJECT "Compilar projeto de conclusão" ON)

add_subdirectory(common)

if(BUILD_DISCIPLINES)
    add_subdirectory(disciplines)
endif()

if(BUILD_PROJECT)
    add_subdirectory(project)
endif()