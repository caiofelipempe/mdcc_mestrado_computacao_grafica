add_library(renderer STATIC
    renderer_glfw_opengl.cpp
    camera.cpp
    drawer_opengl.cpp
)

target_include_directories(renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(renderer PUBLIC
    OpenGL::GL
    OpenGL::GLU
    GLEW::GLEW
    glfw
    imgui::imgui
    geometry
)