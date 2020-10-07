find_package(PNG REQUIRED)

add_library(common OBJECT
    hilbert.cpp
    image.cpp
    pixels.cpp
)
target_link_libraries(common
    PRIVATE
        project_warnings
        PNG::PNG
)

add_library(grid OBJECT grid.cpp)
target_link_libraries(grid PRIVATE project_warnings)

add_library(transformations OBJECT transformations.cpp)
target_link_libraries(transformations PRIVATE project_warnings)

add_executable(permute permute.cpp)
target_link_libraries(permute
    PRIVATE
        project_warnings
        clipp
        common
        transformations
)

add_executable(abstract abstract.cpp)
target_link_libraries(abstract
    PRIVATE
        project_warnings
        clipp
        common
        grid
)
