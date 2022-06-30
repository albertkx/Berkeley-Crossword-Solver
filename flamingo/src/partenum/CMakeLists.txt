cmake_minimum_required(VERSION 2.6)

set(PARTENUM_LIB_SRC 
  src/partenum.cc
)

set(PARTENUM_UNITTEST_EXEC_SRC
  src/unittest.cc
)

set(PARTENUM_EXAMPLE_EXEC_SRC
  src/example.cc
)

include_directories (
  .
  ./src/
  ${CMAKE_SOURCE_DIR}/../
  include
  lib  
)

link_directories(
  ${CMAKE_SOURCE_DIR}/../util/build/
) 

add_subdirectory(../util/ ../util/)

add_definitions(-Wall -O3)
add_library(partenum-lib SHARED ${PARTENUM_LIB_SRC})
target_link_libraries(partenum-lib ${CMAKE_REQUIRED_LIBRARIES})

add_executable(example_partenum ${PARTENUM_EXAMPLE_EXEC_SRC})
add_dependencies(example_partenum partenum-lib util-lib)
target_link_libraries(example_partenum partenum-lib util-lib rt)

add_executable(unittest_partenum ${PARTENUM_UNITTEST_EXEC_SRC})
add_dependencies(unittest_partenum partenum-lib util-lib)
target_link_libraries(unittest_partenum partenum-lib util-lib rt)


set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(CMAKE_REQUIRED_LIBRARIES "partenum-lib;${CMAKE_REQUIRED_LIBRARIES}")
