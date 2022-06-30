cmake_minimum_required(VERSION 2.6)

set(TOPK_LIB_SRC 
  src/display.cc
  src/misc.cc
  src/topk.cc
  src/topkheap.cc
  src/topkheap.v1.cc
  src/topkindex.cc
  src/topkindex.v1.cc
  src/topkiterative.cc
  src/topkroundrobin.cc
  src/topkscan.cc
  src/topksearch.cc
  src/topktwophase.cc
  src/perftest.cc
  src/perftest_exps.cc
  src/perftest_util.cc
  src/oldlistmerger/heap.cc
  src/oldlistmerger/utilities.cc
)

set(TOPK_UNITTEST_EXEC_SRC
  src/unittest.cc
)

set(TOPK_PERFTEST_EXEC_SRC
  src/perftest.cc
)

set(TOPK_PERFTEST_UTIL_EXEC_SRC
  src/perftest_util.cc
)

set(TOPK_PERFTEST_EXPS_EXEC_SRC
  src/perftest_exps.cc
)

set(TOPK_EXAMPLE_EXEC_SRC
  src/example.cc
)

include_directories (
  .
  ./src/
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  include
  lib  
)

link_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../util/build/
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/build/
) 

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../util/ ${CMAKE_CURRENT_SOURCE_DIR}/../util/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common/ ${CMAKE_CURRENT_SOURCE_DIR}/../common/)

add_definitions(-Wall -O3 -Wno-unused)
add_library(topk-lib SHARED ${TOPK_LIB_SRC})
target_link_libraries(topk-lib common-lib util-lib ${CMAKE_REQUIRED_LIBRARIES})

add_executable(perftest_topk ${TOPK_PERFTEST_EXEC_SRC})
add_dependencies(perftest_topk topk-lib common-lib util-lib)
IF(NOT APPLE) 
  target_link_libraries(perftest_topk topk-lib common-lib util-lib rt) 
ELSE() 
  target_link_libraries(perftest_topk topk-lib common-lib util-lib) 
ENDIF()

add_executable(perftest_util_topk ${TOPK_PERFTEST_UTIL_EXEC_SRC})
add_dependencies(perftest_util_topk topk-lib common-lib util-lib)
IF(NOT APPLE) 
  target_link_libraries(perftest_util_topk topk-lib common-lib util-lib rt) 
ELSE() 
  target_link_libraries(perftest_util_topk topk-lib common-lib util-lib) 
ENDIF() 

add_executable(perftest_exps_topk ${TOPK_PERFTEST_EXPS_EXEC_SRC})
add_dependencies(perftest_exps_topk topk-lib common-lib util-lib)
IF(NOT APPLE) 
  target_link_libraries(perftest_exps_topk topk-lib common-lib util-lib rt) 
ELSE() 
  target_link_libraries(perftest_exps_topk topk-lib common-lib util-lib) 
ENDIF() 

add_executable(example_topk ${TOPK_EXAMPLE_EXEC_SRC})
add_dependencies(example_topk topk-lib common-lib util-lib)
IF(NOT APPLE) 
  target_link_libraries(example_topk topk-lib common-lib util-lib rt) 
ELSE() 
  target_link_libraries(example_topk topk-lib common-lib util-lib) 
ENDIF() 

add_executable(unittest_topk ${TOPK_UNITTEST_EXEC_SRC})
add_dependencies(unittest_topk topk-lib common-lib util-lib)
IF(NOT APPLE) 
  target_link_libraries(unittest_topk topk-lib common-lib util-lib rt) 
ELSE() 
  target_link_libraries(unittest_topk topk-lib common-lib util-lib) 
ENDIF() 

set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(CMAKE_REQUIRED_LIBRARIES "topk-lib;${CMAKE_REQUIRED_LIBRARIES}")
