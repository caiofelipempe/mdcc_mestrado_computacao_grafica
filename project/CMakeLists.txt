add_executable(mddc_mestrado_projeto
    main.cpp
)

target_link_libraries(mddc_mestrado_projeto
    PRIVATE
        renderer
        geometry
        result
        
        OpenGL::GL
        glfw
        imgui::imgui
        CGAL::CGAL
)