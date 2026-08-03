add_executable(corpus_test corpus_test.cpp)
target_link_libraries(corpus_test PRIVATE nemi)

# Point the test at the shared corpus at the repo root (cpp/../examples) and
# at the shared common library (cpp/../bibcom, spec §19.3 "biblioteca común").
target_compile_definitions(corpus_test PRIVATE
  NEMI_EXAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../examples"
  NEMI_BIBCOM_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../bibcom")

if(MSVC)
  target_compile_options(corpus_test PRIVATE /utf-8)
endif()

add_test(NAME corpus COMMAND corpus_test)
