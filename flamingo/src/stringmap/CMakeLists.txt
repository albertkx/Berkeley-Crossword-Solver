cmake_minimum_required(VERSION 2.6)

set(STRINGMAP_LIB_SRC 
  src/editdistance.cpp
  src/element.cpp
  src/index.cpp
  src/mapper.cpp
  src/node.cpp
  src/rect.cpp
  src/split_l.cpp
  #src/split_q.cpp # change this line to switch between split types
  src/stringmap.cpp
)

set(STRINGMAP_UNITTEST_EXEC_SRC
  src/stringmap_unittest.cpp
)

include_directories (
  .
  ./src/
  ${CMAKE_SOURCE_DIR}/../
  include
  lib  
)

add_definitions(-Wall -O3)
add_library(stringmap-lib SHARED ${STRINGMAP_LIB_SRC})
target_link_libraries(stringmap-lib ${CMAKE_REQUIRED_LIBRARIES})

add_executable(unittest_stringmap ${STRINGMAP_UNITTEST_EXEC_SRC})
add_dependencies(unittest_stringmap stringmap-lib)
target_link_libraries(unittest_stringmap stringmap-lib)


set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(CMAKE_REQUIRED_LIBRARIES "stringmap-lib;${CMAKE_REQUIRED_LIBRARIES}")
