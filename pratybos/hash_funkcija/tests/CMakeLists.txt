add_executable(
    hash_funkcija_test 
    hash_test.cpp
)

target_link_libraries(
    hash_funkcija_test
    hash_funkcija
    ai_hash_funkcija
    file_read
    GTest::gtest_main
)

target_compile_definitions(hash_funkcija_test PRIVATE TESTS_SOURCE_DIR="${CMAKE_BINARY_DIR}")

include(GoogleTest)
gtest_discover_tests(hash_funkcija_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

