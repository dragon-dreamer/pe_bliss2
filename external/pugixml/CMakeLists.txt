cmake_minimum_required(VERSION 3.13)

project(pugixml
	VERSION 1.0.0.0
	LANGUAGES CXX)

add_library(pugixml INTERFACE)

target_include_directories(pugixml INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
