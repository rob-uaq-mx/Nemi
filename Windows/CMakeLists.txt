cmake_minimum_required(VERSION 3.16)
project(nemi_cpp LANGUAGES CXX C)

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
  # /FS: force synchronous PDB writes -- needed because MSBuild compiles the
  # library's sources in parallel, and without it concurrent cl.exe
  # instances writing to the same .pdb intermittently fail with C1090.
  target_compile_options(nemi PRIVATE /W4 /utf-8 /FS)
else()
  target_compile_options(nemi PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- the CLI (nemi-cli -> `nemi`) ------------------------------------------
add_executable(nemi-cli apps/main.cpp)
set_target_properties(nemi-cli PROPERTIES OUTPUT_NAME nemi)
target_link_libraries(nemi-cli PRIVATE nemi)

# --- WinNemi: editor (Petzold's PopPad, modernized) + CmdLineCtl console ---
# Win32-only: both CmdLineCtl.c and WinNemi.cpp depend on <windows.h>.
if(WIN32)
  # CmdLineCtl: reusable C console control (see Windows/cmdln for the
  # original, untouched reference copy and its demo).
  add_library(cmdlinectl STATIC src/CmdLineCtl.c)
  target_include_directories(cmdlinectl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  if(MSVC)
    target_compile_options(cmdlinectl PRIVATE /utf-8 /FS)
  endif()

  add_executable(WinNemi WIN32
    apps/WinNemi.cpp
    apps/WinNemiFile.cpp
    apps/WinNemiFind.cpp
    apps/WinNemiFont.cpp
    apps/WinNemiPrint.cpp
    apps/WinNemi.rc
  )
  target_link_libraries(WinNemi PRIVATE nemi cmdlinectl user32 gdi32 comdlg32)
  if(MSVC)
    target_compile_options(WinNemi PRIVATE /utf-8 /FS)
  endif()
endif()

# --- tests -----------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
