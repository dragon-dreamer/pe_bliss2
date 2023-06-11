cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0091 NEW)

project(pe_bliss2_all
	VERSION 1.0.0.0
	LANGUAGES CXX)

option(PE_BLISS_ENABLE_TESTING "Enable pe_bliss2 and supporting libraries tests" ON)

if (MSVC)
	option(PE_BLISS_STATIC_MSVC_RUNTIME "Link all binaries with MSVC runtime statically" OFF)
	if (PE_BLISS_STATIC_MSVC_RUNTIME)
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif()
endif()

add_subdirectory(external/pugixml)
add_subdirectory(buffers)
add_subdirectory(utilities)
add_subdirectory(pe_bliss2)
add_subdirectory(console_dumper)

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
