cmake_minimum_required(VERSION 3.12)

project(pe_bliss2_all
	VERSION 1.0.0.0
	LANGUAGES CXX)

add_subdirectory(external/pugixml)
add_subdirectory(buffers)
add_subdirectory(utilities)
add_subdirectory(pe_bliss2)
add_subdirectory(console_dumper)

option(PE_BLISS_ENABLE_TESTING "Enable pe_bliss2 and supporting libraries tests" ON)

if(PE_BLISS_ENABLE_TESTING)
	enable_testing()
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	set(BUILD_GMOCK ON CACHE INTERNAL "" FORCE)
	set(INSTALL_GTEST OFF CACHE INTERNAL "" FORCE)
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	add_subdirectory(external/googletest)
	include(GoogleTest)
	include(CTest)
	add_subdirectory(tests)
endif()
