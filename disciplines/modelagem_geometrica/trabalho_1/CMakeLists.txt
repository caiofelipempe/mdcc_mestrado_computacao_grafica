add_executable(modelagem_geometrica
    main.cpp
)

target_link_libraries(modelagem_geometrica
    PRIVATE
        renderer
        geometry
        result
        
        OpenGL::GL
        glfw
        imgui::imgui
)