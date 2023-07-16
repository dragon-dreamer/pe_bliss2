cmake_minimum_required(VERSION 3.15)

project(utilities
	VERSION 1.0.0.0
	LANGUAGES CXX)

include(../cmake/library_options.cmake)

add_library(utilities ${UTILITIES_BUILD_TYPE})

include(../cmake/output_options.cmake)
set_output_dirs(utilities)
set_msvc_runtime_library(utilities "${PE_BLISS_MSVC_RUNTIME_LIBRARY}")

target_include_directories(utilities PUBLIC include)

target_compile_features(utilities PRIVATE cxx_std_20)

target_sources(utilities
	PUBLIC
		include/utilities/generic_error.h
		include/utilities/hash.h
		include/utilities/math.h
		include/utilities/safe_uint.h
		include/utilities/range_helpers.h
		include/utilities/scoped_guard.h
		include/utilities/shannon_entropy.h
		include/utilities/static_class.h
		include/utilities/string.h
		include/utilities/variant_helpers.h
	PRIVATE
		src/generic_error.cpp
		src/shannon_entropy.cpp
)

if(MSVC)
	target_compile_options(utilities PRIVATE "/MP")
endif()
