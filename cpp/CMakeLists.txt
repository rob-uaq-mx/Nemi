cmake_minimum_required(VERSION 3.16)
project(nemi_cpp LANGUAGES CXX)

# --- standard & warnings ---------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# --- the library (libnemi) -------------------------------------------------
# The numeric tower (include/nemi/numeric.hpp) uses a from-scratch BigInt
# (src/bigint.cpp) — no GMP/Boost dependency, so the project builds with
# nothing but a C++17 compiler and CMake (see backlog.md Fase 4).
add_library(nemi
  src/token_kind.cpp
  src/bigint.cpp
  src/rational.cpp
  src/value.cpp
  src/lexer.cpp
  src/parser.cpp
  src/interpreter.cpp
  src/loader.cpp
  src/nemi.cpp
)
target_include_directories(nemi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
  target_compile_options(nemi PRIVATE /W4 /utf-8)
else()
  target_compile_options(nemi PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- the CLI (nemi-cli -> `nemi`) ------------------------------------------
add_executable(nemi-cli apps/main.cpp)
set_target_properties(nemi-cli PROPERTIES OUTPUT_NAME nemi)
target_link_libraries(nemi-cli PRIVATE nemi)

# --- tests -----------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
