cmake_minimum_required(VERSION 3.31)
project(hash_funkcija)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(googletest)
    
add_library(hash_funkcija
    Hasher.cpp
    )
add_library(file_read
    io/FileRead.cpp
)
add_library(parser_helper
    parsing_helper_funcs.cpp
)
add_library(test_file_gen
    generate_test_files.cpp)
add_executable(main
    main.cpp)
target_include_directories(hash_funkcija PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(main PRIVATE hash_funkcija file_read parser_helper test_file_gen)
add_subdirectory(tests)
