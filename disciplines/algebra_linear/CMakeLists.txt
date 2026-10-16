add_executable(algebra_linear
    main.cpp
    linear_system_solver.cpp
    linear_system_solver.hpp
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