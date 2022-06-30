cmake_minimum_required(VERSION 2.6)

set(LBAKTREE_LIB_SRC 
  src/kwdsstorage.cc
  src/lbaktree.cc
  src/rstartree/rstartree.cc
  src/rstartree/storage.cc
  src/rstartree/util.cc
)

set(LBAKTREE_EXAMPLE_EXEC_SRC
  src/example.cc
)

include_directories (
  ${CMAKE_SOURCE_DIR}/../
  ${CMAKE_SOURCE_DIR}/../filtertree
  rtartree
  .
  include
  lib  
)

link_directories(
  ${CMAKE_SOURCE_DIR}/../common/build/
  ${CMAKE_SOURCE_DIR}/../util/build/
  ${CMAKE_SOURCE_DIR}/../listmerger/build
  ${CMAKE_SOURCE_DIR}/../filtertree/build/
) 

add_subdirectory(${CMAKE_SOURCE_DIR}/../filtertree/ ${CMAKE_SOURCE_DIR}/../filtertree/)

add_definitions(-Wall -O3 -DDEBUG_TIMER_FANCY -DDEBUG_STAT -DED_MATRIX_DIM=2000)
add_library(lbaktree-lib SHARED ${LBAKTREE_LIB_SRC})
target_link_libraries(lbaktree-lib ${CMAKE_REQUIRED_LIBRARIES})

add_executable(example_lbaktree ${LBAKTREE_EXAMPLE_EXEC_SRC})
add_dependencies(example_lbaktree lbaktree-lib filtertree-lib common-lib util-lib listmerger-lib)
target_link_libraries(example_lbaktree lbaktree-lib filtertree-lib common-lib util-lib listmerger-lib rt)

set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(CMAKE_REQUIRED_LIBRARIES "lbaktree-lib;${CMAKE_REQUIRED_LIBRARIES}")
