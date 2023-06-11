cmake_minimum_required(VERSION 3.13)

project(console_dumper
	VERSION 1.0.0.0
	LANGUAGES CXX)

add_executable(console_dumper)

target_compile_features(console_dumper PRIVATE cxx_std_20)

target_sources(console_dumper
	PRIVATE
		bound_imports_dumper.h
		color_provider.h
		debug_dumper.h
		dos_header_dumper.h
		dos_stub_dumper.h
		exceptions_dumper.h
		exports_dumper.h
		file_header_dumper.h
		file_signature_dumper.h
		formatter.h
		image_factory.h
		imports_dumper.h
		load_config_dumper.h
		optional_header_dumper.h
		relocations_dumper.h
		resources_dumper.h
		rich_data_dumper.h
		section_table_dumper.h
		tls_dumper.h
		bound_imports_dumper.cpp
		debug_dumper.cpp
		dos_header_dumper.cpp
		dos_stub_dumper.cpp
		exceptions_dumper.cpp
		exports_dumper.cpp
		file_header_dumper.cpp
		file_signature_dumper.cpp
		image_factory.cpp
		imports_dumper.cpp
		load_config_dumper.cpp
		main.cpp
		optional_header_dumper.cpp
		relocations_dumper.cpp
		resources_dumper.cpp
		rich_data_dumper.cpp
		section_table_dumper.cpp
		tls_dumper.cpp
)

target_link_libraries(console_dumper PRIVATE buffers pe_bliss2)

if(MSVC)
	target_compile_options(console_dumper PRIVATE "/MP")
	target_compile_definitions(console_dumper PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
endif()
