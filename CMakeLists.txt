cmake_minimum_required(VERSION 3.10)
project(LocalSearchEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES 
	main.cpp
	indexer.cpp)

find_package(pugixml CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

add_executable(LocalSearchEngine main.cpp "indexer.cpp")

target_link_libraries(LocalSearchEngine PRIVATE 
    pugixml::pugixml
    nlohmann_json::nlohmann_json
    CLI11::CLI11
)
target_include_directories(LocalSearchEngine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})