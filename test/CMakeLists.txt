find_package(Catch2 REQUIRED)

add_library(catch_main STATIC catch_main.cpp)
target_link_libraries(catch_main
    PRIVATE
        project_warnings
    PUBLIC
        Catch2::Catch2
)

add_executable(tests tests.cpp)
target_link_libraries(tests
    PRIVATE
        project_warnings
        catch_main
        common
        grid
        permutations
)
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

include(Catch)
catch_discover_tests(tests)

