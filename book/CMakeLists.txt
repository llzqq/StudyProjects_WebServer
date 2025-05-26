cmake_minimum_required(VERSION 3.1)
project(book)

set(CMAKE_C_COMPILER /usr/bin/clang) 
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -ggdb -Wall ")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${PROJECT_SOURCE_DIR}/build)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

add_executable(curr_try src/try.cpp)
target_link_libraries(curr_try )

add_executable(client src/client.cpp)
target_link_libraries(client)

add_executable(http src/11http_answer.cpp)
target_link_libraries(http)

# add_executable(send src/4tcp_data_s.cpp)
# target_link_libraries(send)

# add_executable(recv src/4tcp_data_r.cpp)
# target_link_libraries(recv)
