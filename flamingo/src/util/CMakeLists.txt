cmake_minimum_required(VERSION 2.6)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") 
  set(OS_DEPENDENT_LIBS)
else()
  set(OS_DEPENDENT_LIBS rt)	
endif()

set(UTIL_LIB_SRC 
  src/appsearch.cc 
  src/array.cc
  src/gram.cc
  src/input.cc
  src/io.cc
  src/looptimer.cc
  src/misc.cc
  src/output.cc
  src/querygrams.cc
  src/querygroup.cc
  src/range.cc
  src/scan.cc
  src/simfuncs.cc
  src/stringshuffler.cc
  src/time.cc 
)

set(UTIL_UNITTEST_EXEC_SRC
  src/unittest.cc
)

include_directories (
  include
  lib
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  .
)

link_directories (${CMAKE_BINARY_DIR}/) 

add_definitions(-Wall -O3 -Wno-unused)
add_library(util-lib SHARED ${UTIL_LIB_SRC})
target_link_libraries(util-lib ${OS_DEPENDENT_LIBS} ${CMAKE_REQUIRED_LIBRARIES})

add_executable(unittest ${UTIL_UNITTEST_EXEC_SRC})
add_dependencies(unittest util-lib)
target_link_libraries(unittest util-lib) 

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(CMAKE_REQUIRED_LIBRARIES "util-lib;${CMAKE_REQUIRED_LIBRARIES}")
