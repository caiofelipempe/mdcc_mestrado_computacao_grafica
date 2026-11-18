enable_language(C)

add_executable(algebra_linear
    main.c
    linear_algebra.c
    linear_algebra.h
)

set_target_properties(algebra_linear PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(algebra_linear PRIVATE ${MATH_LIBRARY})
endif()
