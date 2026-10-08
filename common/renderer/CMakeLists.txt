file(GLOB RENDERER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(renderer STATIC ${RENDERER_SOURCES})

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