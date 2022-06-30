cmake_minimum_required(VERSION 2.6)

set(LISTMERGER_LIB_SRC 
  src/divideskipmerger.cc
  src/example.cc
  src/heapmerger.cc
  src/mergeoptmerger.cc
  src/mergeskipmerger.cc
  src/ondiskmergeradapt.cc
  src/ondiskmergersimple.cc
  src/scancountmerger.cc
)

set(LISTMERGER_UNITTEST_EXEC_SRC
  src/unittest.cc
)

set(LISTMERGER_EXAMPLE_EXEC_SRC
  src/example.cc
)

add_subdirectory(../util/ ../util/)

link_directories(
  ${CMAKE_SOURCE_DIR}/../util/build/
) 

include_directories (
  include
  lib  
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../util/
  .
)

add_definitions(-Wall -O3 -DDEBUG_TIMER_FANCY)
add_library(listmerger-lib SHARED ${LISTMERGER_LIB_SRC})
target_link_libraries(listmerger-lib ${CMAKE_REQUIRED_LIBRARIES})

add_executable(unittest_lm ${LISTMERGER_UNITTEST_EXEC_SRC})
add_dependencies(unittest_lm listmerger-lib util-lib)
target_link_libraries(unittest_lm listmerger-lib util-lib)

add_executable(example_lm ${LISTMERGER_EXAMPLE_EXEC_SRC})
add_dependencies(example_lm listmerger-lib)
target_link_libraries(example_lm listmerger-lib)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(CMAKE_REQUIRED_LIBRARIES "listmerger-lib;${CMAKE_REQUIRED_LIBRARIES}")
