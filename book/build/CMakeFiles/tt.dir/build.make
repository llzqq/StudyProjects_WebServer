# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/lzq/webserver/book

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/lzq/webserver/book/build

# Include any dependencies generated for this target.
include CMakeFiles/tt.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/tt.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/tt.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/tt.dir/flags.make

CMakeFiles/tt.dir/src/try.cpp.o: CMakeFiles/tt.dir/flags.make
CMakeFiles/tt.dir/src/try.cpp.o: ../src/try.cpp
CMakeFiles/tt.dir/src/try.cpp.o: CMakeFiles/tt.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lzq/webserver/book/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/tt.dir/src/try.cpp.o"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/tt.dir/src/try.cpp.o -MF CMakeFiles/tt.dir/src/try.cpp.o.d -o CMakeFiles/tt.dir/src/try.cpp.o -c /home/lzq/webserver/book/src/try.cpp

CMakeFiles/tt.dir/src/try.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/tt.dir/src/try.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lzq/webserver/book/src/try.cpp > CMakeFiles/tt.dir/src/try.cpp.i

CMakeFiles/tt.dir/src/try.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/tt.dir/src/try.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lzq/webserver/book/src/try.cpp -o CMakeFiles/tt.dir/src/try.cpp.s

# Object files for target tt
tt_OBJECTS = \
"CMakeFiles/tt.dir/src/try.cpp.o"

# External object files for target tt
tt_EXTERNAL_OBJECTS =

tt: CMakeFiles/tt.dir/src/try.cpp.o
tt: CMakeFiles/tt.dir/build.make
tt: CMakeFiles/tt.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/lzq/webserver/book/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable tt"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tt.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/tt.dir/build: tt
.PHONY : CMakeFiles/tt.dir/build

CMakeFiles/tt.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/tt.dir/cmake_clean.cmake
.PHONY : CMakeFiles/tt.dir/clean

CMakeFiles/tt.dir/depend:
	cd /home/lzq/webserver/book/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/lzq/webserver/book /home/lzq/webserver/book /home/lzq/webserver/book/build /home/lzq/webserver/book/build /home/lzq/webserver/book/build/CMakeFiles/tt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/tt.dir/depend

