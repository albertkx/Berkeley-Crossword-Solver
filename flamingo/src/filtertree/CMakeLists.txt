cmake_minimum_required(VERSION 2.6)

set(FILTERTREE_LIB_SRC 
  src/filtertreenode.cc
  src/ftindexerabs.cc
  src/ftindexercombinelists.cc
  src/ftindexerdiscardlists.cc
  src/ftindexermemabs.cc
  src/ftindexermem.cc
  src/ftindexerondisk.cc
  src/ftsearcherabs.cc
  src/ftsearchermem.cc
  src/ftsearcherondisk.cc
  src/gramlistondisk.cc
  src/gramlistsimple.cc
  src/gramlistunion.cc
  src/statsgen.cc
  src/statsutil.cc
  src/stringcontainer.cc
  src/stringrm.cc
)

set(WRAPPERS_LIB_SRC 
  src/wrappers/wrappers.cc
)

set(WRAPPERS_EXAMPLE_EXEC_SRC 
  src/wrappers/example.cc
)

set(FILTERTREE_UNITTEST_EXEC_SRC
  src/unittest.cc
)

set(FILTERTREE_PERFTEST_EXEC_SRC
  src/perftest.cc
)

set(FILTERTREE_EXAMPLE_EXEC_SRC
  src/example.cc
)

include_directories (
  .
  ./wrappers/
  ${CMAKE_SOURCE_DIR}/../
  include
  lib  
)

link_directories(
  ${CMAKE_SOURCE_DIR}/../common/build/
  ${CMAKE_SOURCE_DIR}/../util/build/
  ${CMAKE_SOURCE_DIR}/../listmerger/build/
) 

add_subdirectory(../common/ ../common/)
add_subdirectory(../listmerger/ ../listmerger/)

add_definitions(-Wall -O3 -DDEBUG_TIMER_FANCY -DDEBUG_STAT -DED_MATRIX_DIM=2000)
add_library(filtertree-lib SHARED ${FILTERTREE_LIB_SRC})
target_link_libraries(filtertree-lib common-lib util-lib listmerger-lib ${CMAKE_REQUIRED_LIBRARIES})

add_executable(perftest_ft ${FILTERTREE_PERFTEST_EXEC_SRC})
add_dependencies(perftest_ft filtertree-lib common-lib util-lib listmerger-lib)
target_link_libraries(perftest_ft filtertree-lib common-lib util-lib listmerger-lib)

add_executable(example_ft ${FILTERTREE_EXAMPLE_EXEC_SRC})
add_dependencies(example_ft filtertree-lib common-lib util-lib listmerger-lib)
target_link_libraries(example_ft filtertree-lib common-lib util-lib listmerger-lib)

add_executable(unittest_ft ${FILTERTREE_UNITTEST_EXEC_SRC})
add_dependencies(unittest_ft filtertree-lib common-lib util-lib listmerger-lib)
target_link_libraries(unittest_ft filtertree-lib common-lib util-lib listmerger-lib)


add_definitions(-Wall -O3 -DDEBUG_TIMER_FANCY -DDEBUG_STAT -DED_MATRIX_DIM=2000)
add_library(wrappers-lib SHARED ${WRAPPERS_LIB_SRC})
target_link_libraries(wrappers-lib ${CMAKE_REQUIRED_LIBRARIES})

add_executable(example_wrappers ${WRAPPERS_EXAMPLE_EXEC_SRC})
add_dependencies(example_wrappers filtertree-lib wrappers-lib common-lib util-lib listmerger-lib)
target_link_libraries(example_wrappers filtertree-lib wrappers-lib common-lib util-lib listmerger-lib)


set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/")

set(CMAKE_REQUIRED_LIBRARIES "filtertree-lib;${CMAKE_REQUIRED_LIBRARIES}")
