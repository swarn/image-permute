find_package(PNG REQUIRED)

set(SOURCES
    image.cpp
    pixels.cpp
    grid.cpp
    hilbert.cpp
    transformations.cpp
)

add_library(objects OBJECT
    grid.cpp
    hilbert.cpp
    image.cpp
    pixels.cpp
    transformations.cpp
)
target_include_directories(objects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(objects PUBLIC PNG::PNG)

add_executable(permute permute.cpp)
target_link_libraries(permute PUBLIC objects)

add_executable(abstract abstract.cpp)
target_link_libraries(abstract PUBLIC objects)
