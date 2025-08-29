cmake_minimum_required(VERSION 3.10)
project(iPerfer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_executable(iPerfer
    iperfer.cpp
    client.cpp
    server.cpp
)

# Add your setup code above this line, then uncomment the line below

target_link_libraries(iPerfer spdlog::spdlog cxxopts::cxxopts)