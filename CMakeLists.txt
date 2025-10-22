cmake_minimum_required(VERSION 3.31)
project(hash_funkcija)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type specified, defaulting to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

if(MSVC)
  # Visual Studio / MSVC
  set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} /O2 /DNDEBUG /GL")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG /GL")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
else()
  # GCC / Clang
  set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG -march=native -flto")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -march=native -flto")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT _ipo_supported)
if(_ipo_supported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

enable_testing()

include(CheckCXXSourceCompiles)
set(_parallel_test_source "#include <execution>\n#include <numeric>\nint main() { int data[2] = {1, 2}; auto result = std::transform_reduce(std::execution::par, data, data + 2, 0, std::plus<int>(), [](int v){ return v; }); return result; }")
check_cxx_source_compiles("${_parallel_test_source}" HASHF_HAVE_STD_PARALLEL)
unset(_parallel_test_source)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(googletest)



configure_file(
        ${CMAKE_SOURCE_DIR}/konstitucija.txt
        ${CMAKE_BINARY_DIR}/konstitucija.txt
        COPYONLY
)
configure_file(
  ${CMAKE_SOURCE_DIR}/draw_konstitucija_chart.py
  ${CMAKE_BINARY_DIR}/draw_konstitucija_chart.py
  COPYONLY
)
add_library(hash_funkcija
src/crypto/Hasher.cpp
)
add_library(ai_hash_funkcija
src/crypto/AIHasher.cpp)
add_library(sha256_hash_funkcija
src/crypto/sha256_hasher.cpp)
add_library(file_read
src/io/FileRead.cpp
)
add_library(parser_helper
src/cli/parsing_helper_funcs.cpp
)
add_library(test_file_gen
src/file_gen/test_file_generator.cpp)
add_library(utils
src/utils.cpp)
add_executable(main
main.cpp)
add_executable(benchmark
tests/benchmark.cpp)
add_executable(draw_konstitucija
src/cli/draw_chart.cpp)
add_executable(task 
tests/task.cpp)
target_sources(draw_konstitucija PRIVATE src/cli/draw_konstitucija_chart.cpp)
add_library(project_includes INTERFACE)
target_include_directories(project_includes INTERFACE ${CMAKE_SOURCE_DIR}/include)

if(NOT HASHF_HAVE_STD_PARALLEL)
  find_package(TBB REQUIRED)
  target_link_libraries(hash_funkcija PUBLIC TBB::tbb)
endif()

target_link_libraries(hash_funkcija PUBLIC project_includes)
if(HASHF_HAVE_STD_PARALLEL)
  target_compile_definitions(hash_funkcija PUBLIC HASHF_HAS_STD_PARALLEL=1 HASHF_HAS_TBB=0)
else()
  target_compile_definitions(hash_funkcija PUBLIC HASHF_HAS_STD_PARALLEL=0 HASHF_HAS_TBB=1)
  target_link_libraries(hash_funkcija PUBLIC TBB::tbb)
endif()
target_link_libraries(sha256_hash_funkcija PUBLIC project_includes)
target_link_libraries(ai_hash_funkcija PUBLIC project_includes)
# Find OpenSSL for SHA256 support
find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
  message(STATUS "Found OpenSSL ${OPENSSL_VERSION}")
  target_link_libraries(sha256_hash_funkcija PUBLIC OpenSSL::Crypto)
else()
  message(FATAL_ERROR "OpenSSL not found - required for SHA256 hashing")
endif()
target_link_libraries(file_read PUBLIC project_includes)
target_link_libraries(test_file_gen PUBLIC project_includes)
target_link_libraries(parser_helper PUBLIC project_includes)
target_link_libraries(draw_konstitucija PUBLIC project_includes)
target_link_libraries(task PUBLIC project_includes)
target_link_libraries(main PRIVATE hash_funkcija file_read parser_helper test_file_gen sha256_hash_funkcija ai_hash_funkcija)
target_link_libraries(benchmark PRIVATE hash_funkcija sha256_hash_funkcija ai_hash_funkcija)
target_link_libraries(task PRIVATE sha256_hash_funkcija hash_funkcija ai_hash_funkcija)
add_subdirectory(tests)
