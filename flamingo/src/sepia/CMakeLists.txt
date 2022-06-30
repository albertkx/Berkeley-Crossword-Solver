cmake_minimum_required(VERSION 2.6)

set(SEPIA_LIB_SRC 
  src/duplicate.cc
  src/sample.cc
  src/sepia.cc
  src/cluster/cluster.cc
  src/cluster/clusters.cc
  src/cluster/ftable.cc
  src/cluster/lexic.cc
  src/cluster/medoids.cc
  src/freqest/errorcorr.cc
  src/freqest/freqest.cc
  src/freqest/predicate.cc
  src/freqest/record.cc
  src/freqest/test.cc
  src/freqest/update.cc
  src/ppdtable/allrand.cc
  src/ppdtable/closelex.cc
  src/ppdtable/closerand.cc
  src/ppdtable/closeunique.cc
  src/ppdtable/indexentry.cc
  src/ppdtable/ppdentry.cc
  src/ppdtable/ppdpair.cc
  src/ppdtable/ppdsample.cc
  src/ppdtable/ppdtable.cc
  src/ppdtable/ppdtriple.cc
  src/simfunc/editvect.cc
  src/simfunc/jaccvect.cc
  src/simfunc/simdist.cc
  src/simfunc/simvect.cc
)

set(SEPIA_UNITTEST_EXEC_SRC
  src/unittest.cc
)

set(SEPIA_PERFTEST_EXEC_SRC
  src/perftest.cc
)

set(SEPIA_EXAMPLE_EXEC_SRC
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

add_definitions(-Wall -O3 -DSIM_DIST=1 -DSIM_VECT=1 -DDATASET_LEN=20 -DQ_GRAM=2)
add_library(sepia-lib SHARED ${SEPIA_LIB_SRC})
target_link_libraries(sepia-lib ${CMAKE_REQUIRED_LIBRARIES})

add_executable(perftest_sepia ${SEPIA_PERFTEST_EXEC_SRC})
add_dependencies(perftest_sepia sepia-lib util-lib)
target_link_libraries(perftest_sepia sepia-lib util-lib rt)

add_executable(example_sepia ${SEPIA_EXAMPLE_EXEC_SRC})
add_dependencies(example_sepia sepia-lib util-lib)
target_link_libraries(example_sepia sepia-lib util-lib rt)

add_executable(unittest_sepia ${SEPIA_UNITTEST_EXEC_SRC})
add_dependencies(unittest_sepia sepia-lib util-lib)
target_link_libraries(unittest_sepia sepia-lib util-lib rt)


set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(CMAKE_REQUIRED_LIBRARIES "sepia-lib;${CMAKE_REQUIRED_LIBRARIES}")
