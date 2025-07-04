cmake_minimum_required(VERSION "3.19.2")

add_subdirectory("cpr")
add_subdirectory("json")


project(main1)
set(CMAKE_CXX_STANDARD 17)
add_executable(main1 "bin/main.cpp")

target_include_directories(main1 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_link_libraries(main1 PRIVATE cpr)
target_link_libraries(main1 PRIVATE nlohmann_json)
add_definitions(-DPATH_TO_CONFIG="${CMAKE_SOURCE_DIR}/bin/example_config.json")
add_definitions(-DPATH_TO_SAVE="${CMAKE_SOURCE_DIR}/bin/example_save.json")




