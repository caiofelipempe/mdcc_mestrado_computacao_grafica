add_executable(modelagem_geometrica_trabalho2
    main.cpp
)

target_link_libraries(modelagem_geometrica_trabalho2
    PRIVATE
        renderer
        geometry
        result
        
        OpenGL::GL
        glfw
        imgui::imgui
)