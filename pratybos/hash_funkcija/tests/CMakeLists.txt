add_executable(
    hash_funkcija_test 
    hash_test.cpp
    hasher_test.cpp
)

target_link_libraries(
    hash_funkcija_test
    hash_funkcija
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(hash_funkcija_test)

