# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ginsberg/xw/drfill/flamingo/src/filtertree

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ginsberg/xw/drfill/flamingo/src/filtertree

# Include any dependencies generated for this target.
include CMakeFiles/perftest_ft.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/perftest_ft.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/perftest_ft.dir/flags.make

CMakeFiles/perftest_ft.dir/src/perftest.cc.o: CMakeFiles/perftest_ft.dir/flags.make
CMakeFiles/perftest_ft.dir/src/perftest.cc.o: src/perftest.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ginsberg/xw/drfill/flamingo/src/filtertree/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/perftest_ft.dir/src/perftest.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/perftest_ft.dir/src/perftest.cc.o -c /home/ginsberg/xw/drfill/flamingo/src/filtertree/src/perftest.cc

CMakeFiles/perftest_ft.dir/src/perftest.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/perftest_ft.dir/src/perftest.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ginsberg/xw/drfill/flamingo/src/filtertree/src/perftest.cc > CMakeFiles/perftest_ft.dir/src/perftest.cc.i

CMakeFiles/perftest_ft.dir/src/perftest.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/perftest_ft.dir/src/perftest.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ginsberg/xw/drfill/flamingo/src/filtertree/src/perftest.cc -o CMakeFiles/perftest_ft.dir/src/perftest.cc.s

CMakeFiles/perftest_ft.dir/src/perftest.cc.o.requires:

.PHONY : CMakeFiles/perftest_ft.dir/src/perftest.cc.o.requires

CMakeFiles/perftest_ft.dir/src/perftest.cc.o.provides: CMakeFiles/perftest_ft.dir/src/perftest.cc.o.requires
	$(MAKE) -f CMakeFiles/perftest_ft.dir/build.make CMakeFiles/perftest_ft.dir/src/perftest.cc.o.provides.build
.PHONY : CMakeFiles/perftest_ft.dir/src/perftest.cc.o.provides

CMakeFiles/perftest_ft.dir/src/perftest.cc.o.provides.build: CMakeFiles/perftest_ft.dir/src/perftest.cc.o


# Object files for target perftest_ft
perftest_ft_OBJECTS = \
"CMakeFiles/perftest_ft.dir/src/perftest.cc.o"

# External object files for target perftest_ft
perftest_ft_EXTERNAL_OBJECTS =

build/perftest_ft: CMakeFiles/perftest_ft.dir/src/perftest.cc.o
build/perftest_ft: CMakeFiles/perftest_ft.dir/build.make
build/perftest_ft: build/libfiltertree-lib.so
build/perftest_ft: /home/ginsberg/xw/drfill/flamingo/src/common/build/libcommon-lib.so
build/perftest_ft: /home/ginsberg/xw/drfill/flamingo/src/util/build/libutil-lib.so
build/perftest_ft: /home/ginsberg/xw/drfill/flamingo/src/listmerger/build/liblistmerger-lib.so
build/perftest_ft: CMakeFiles/perftest_ft.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ginsberg/xw/drfill/flamingo/src/filtertree/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable build/perftest_ft"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/perftest_ft.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/perftest_ft.dir/build: build/perftest_ft

.PHONY : CMakeFiles/perftest_ft.dir/build

CMakeFiles/perftest_ft.dir/requires: CMakeFiles/perftest_ft.dir/src/perftest.cc.o.requires

.PHONY : CMakeFiles/perftest_ft.dir/requires

CMakeFiles/perftest_ft.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/perftest_ft.dir/cmake_clean.cmake
.PHONY : CMakeFiles/perftest_ft.dir/clean

CMakeFiles/perftest_ft.dir/depend:
	cd /home/ginsberg/xw/drfill/flamingo/src/filtertree && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ginsberg/xw/drfill/flamingo/src/filtertree /home/ginsberg/xw/drfill/flamingo/src/filtertree /home/ginsberg/xw/drfill/flamingo/src/filtertree /home/ginsberg/xw/drfill/flamingo/src/filtertree /home/ginsberg/xw/drfill/flamingo/src/filtertree/CMakeFiles/perftest_ft.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/perftest_ft.dir/depend
