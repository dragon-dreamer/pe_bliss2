cmake_minimum_required(VERSION 3.15)

project(buffers
	VERSION 1.0.0.0
	LANGUAGES CXX)

include(../cmake/library_options.cmake)

add_library(buffers ${BUFFERS_BUILD_TYPE})

include(../cmake/output_options.cmake)
set_output_dirs(buffers)
set_msvc_runtime_library(buffers "${PE_BLISS_MSVC_RUNTIME_LIBRARY}")

target_include_directories(buffers PUBLIC include)

target_compile_features(buffers PRIVATE cxx_std_20)

target_sources(buffers
	PUBLIC
		include/buffers/buffer_copy.h
		include/buffers/buffer_interface.h
		include/buffers/input_buffer_interface.h
		include/buffers/input_buffer_section.h
		include/buffers/input_buffer_state.h
		include/buffers/input_buffer_stateful_wrapper.h
		include/buffers/input_container_buffer.h
		include/buffers/input_memory_buffer.h
		include/buffers/input_stream_buffer.h
		include/buffers/input_virtual_buffer.h
		include/buffers/output_buffer_interface.h
		include/buffers/output_memory_buffer.h
		include/buffers/output_memory_ref_buffer.h
		include/buffers/output_stream_buffer.h
		include/buffers/ref_buffer.h
	PRIVATE
		src/buffer_copy.cpp
		src/input_buffer_section.cpp
		src/input_buffer_state.cpp
		src/input_buffer_stateful_wrapper.cpp
		src/input_container_buffer.cpp
		src/input_memory_buffer.cpp
		src/input_stream_buffer.cpp
		src/input_virtual_buffer.cpp
		src/output_memory_buffer.cpp
		src/output_memory_ref_buffer.cpp
		src/output_stream_buffer.cpp
		src/ref_buffer.cpp
)

target_link_libraries(buffers utilities)

if(MSVC)
	target_compile_options(buffers PRIVATE "/MP")
endif()
