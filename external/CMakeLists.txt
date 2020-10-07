add_library(clipp INTERFACE)
target_include_directories(clipp SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/clipp)

add_library(xoshiro INTERFACE)
target_include_directories(xoshiro SYSTEM
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/XoshiroCpp
)

