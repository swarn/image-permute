find_package(PNG REQUIRED)

add_library(common OBJECT
    colors.cpp
    hilbert.cpp
    image.cpp
)
target_link_libraries(common
    PRIVATE
        project_warnings
        PNG::PNG
    PUBLIC
        xoshiro
)

add_library(grid OBJECT grid.cpp)
target_link_libraries(grid PRIVATE project_warnings PUBLIC xoshiro)

add_library(permutations OBJECT permutations.cpp)
target_link_libraries(permutations PRIVATE project_warnings PUBLIC xoshiro)

add_executable(permute main_permute.cpp)
target_link_libraries(permute
    PRIVATE
        project_warnings
        clipp
        common
        permutations
)

add_executable(abstract main_abstract.cpp)
target_link_libraries(abstract
    PRIVATE
        project_warnings
        clipp
        common
        grid
)
