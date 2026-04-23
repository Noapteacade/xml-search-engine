cmake_minimum_required(VERSION 3.10)
project(LocalSearchEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(pugixml CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(LocalSearchEngine main.cpp)

target_link_libraries(LocalSearchEngine PRIVATE pugixml::pugixml)
target_link_libraries(LocalSearchEngine PRIVATE nlohmann_json::nlohmann_json)