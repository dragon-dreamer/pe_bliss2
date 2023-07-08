cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0091 NEW)

if (NOT DEFINED PE_BLISS_ROOT_PROJECT)
	set(PE_BLISS_ROOT_PROJECT OFF)
	if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
		set(PE_BLISS_ROOT_PROJECT ON)
	endif()
endif()

project(pe_bliss2_all
	VERSION 1.0.0.0
	LANGUAGES CXX)

option(PE_BLISS_ENABLE_TESTING
	"Enable pe_bliss2 and supporting libraries tests"
	${PE_BLISS_ROOT_PROJECT})

option(PE_BLISS_BUILD_CONSOLE_DUMPER
	"Build portable executable console dumper"
	${PE_BLISS_ROOT_PROJECT})

if (MSVC)
	option(PE_BLISS_STATIC_MSVC_RUNTIME "Link all binaries with MSVC runtime statically" OFF)
	if (PE_BLISS_STATIC_MSVC_RUNTIME)
		set(PE_BLISS_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	else()
		set(PE_BLISS_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
	endif()
	
	set(CMAKE_MSVC_RUNTIME_LIBRARY "${PE_BLISS_MSVC_RUNTIME_LIBRARY}")
endif()

set(CRYPTOPP_INSTALL OFF CACHE INTERNAL "" FORCE)
set(CRYPTOPP_BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
set(CRYPTOPP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/external/cryptopp-8_8_0/cryptopp")
add_subdirectory(external/cryptopp-8_8_0-cmake)

add_subdirectory(external/pugixml)
add_subdirectory(external/simple_asn1)
add_subdirectory(buffers)
add_subdirectory(utilities)
add_subdirectory(pe_bliss2)

if (PE_BLISS_BUILD_CONSOLE_DUMPER)
	add_subdirectory(console_dumper)
endif()

if (PE_BLISS_ENABLE_TESTING)
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
