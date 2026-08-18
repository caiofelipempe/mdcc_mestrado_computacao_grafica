add_executable(algebra_linear
    main.cpp
    utils.hpp
)

target_link_libraries(algebra_linear
    PRIVATE
        renderer
        geometry
        result
        
        OpenGL::GL
        glfw
        imgui::imgui
)