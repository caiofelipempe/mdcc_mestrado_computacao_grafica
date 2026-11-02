add_executable(modelagem_geometrica_trabalho3
    main.cpp
)

target_link_libraries(modelagem_geometrica_trabalho3
    PRIVATE
        renderer
        geometry
        result
        modelagem_geometrica_utils
        
        OpenGL::GL
        glfw
        imgui::imgui
)