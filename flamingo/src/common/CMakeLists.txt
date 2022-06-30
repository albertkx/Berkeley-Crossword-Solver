cmake_minimum_required(VERSION 2.6)

set(COMMON_LIB_SRC 
  src/compressionargs.cc
  src/filtertypes.cc
  src/gramgen.cc
  src/query.cc
  src/simmetric.cc
  src/typedef.cc
)

set(COMMON_UNITTEST_EXEC_SRC
  src/unittest.cc
)

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)

link_directories (${CMAKE_BINARY_DIR}/) 

add_definitions(-Wall -O3 -Wno-unused)
add_library(common-lib SHARED ${COMMON_LIB_SRC})
target_link_libraries(common-lib ${CMAKE_REQUIRED_LIBRARIES})

# TODO there is some issue with unittest, fix it
#add_executable(unittest ${COMMON_UNITTEST_EXEC_SRC})
#add_dependencies(unittest common-lib)
#target_link_libraries(unittest common-lib) 

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(CMAKE_REQUIRED_LIBRARIES "common-lib;${CMAKE_REQUIRED_LIBRARIES}")
