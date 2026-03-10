cmake_minimum_required(VERSION 3.0)
project(benchmark)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)

include_directories(${PROJECT_SOURCE_DIR}/../src/include)
include_directories(${PROJECT_SOURCE_DIR}/../example)

link_directories(${PROJECT_SOURCE_DIR}/../lib)
link_directories(/usr/lib/x86_64-linux-gnu)

add_executable(benchmark benchmark.cc ../example/user.pb.cc)

target_link_libraries(benchmark rpc protobuf pthread zookeeper_mt)
