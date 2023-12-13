cmake_minimum_required(VERSION 3.15)

project(tests
	VERSION 1.0.0.0
	LANGUAGES CXX)

add_executable(tests)

include(../cmake/output_options.cmake)
set_output_dirs(tests)
set_msvc_runtime_library(tests "${PE_BLISS_MSVC_RUNTIME_LIBRARY}")

target_compile_features(tests PRIVATE cxx_std_20)

target_include_directories(tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if (BUILD_SHARED_LIBS)
	target_compile_definitions(tests PRIVATE GTEST_LINKED_AS_SHARED_LIBRARY)
endif()

target_sources(tests
	PRIVATE
		main.cpp
		tests/buffers/buffer_copy_tests.cpp
		tests/buffers/buffer_helpers.h
		tests/buffers/input_buffer_helpers.h
		tests/buffers/input_buffer_section_tests.cpp
		tests/buffers/input_container_buffer_tests.cpp
		tests/buffers/input_memory_buffer_tests.cpp
		tests/buffers/input_stream_buffer_tests.cpp
		tests/buffers/input_virtual_buffer_tests.cpp
		tests/buffers/output_buffer_helpers.h
		tests/buffers/output_memory_buffer_tests.cpp
		tests/buffers/output_memory_ref_buffer_tests.cpp
		tests/buffers/output_stream_buffer_tests.cpp
		tests/buffers/ref_buffer_tests.cpp
		tests/pe_bliss2/address_converter_tests.cpp
		tests/pe_bliss2/bit_stream_tests.cpp
		tests/pe_bliss2/buffer_to_va_tests.cpp
		tests/pe_bliss2/bytes_to_va_fixture_base.h
		tests/pe_bliss2/bytes_to_va_tests.cpp
		tests/pe_bliss2/byte_array_from_va_tests.cpp
		tests/pe_bliss2/byte_container_fixture_base.h
		tests/pe_bliss2/byte_vector_from_va_tests.cpp
		tests/pe_bliss2/checksum_tests.cpp
		tests/pe_bliss2/compid_database_tests.cpp
		tests/pe_bliss2/data_directories_tests.cpp
		tests/pe_bliss2/directories
		tests/pe_bliss2/dos_header_tests.cpp
		tests/pe_bliss2/dos_stub_tests.cpp
		tests/pe_bliss2/endian_convert_tests.cpp
		tests/pe_bliss2/error_list_tests.cpp
		tests/pe_bliss2/file_header_tests.cpp
		tests/pe_bliss2/image_builder_tests.cpp
		tests/pe_bliss2/image_helper.cpp
		tests/pe_bliss2/image_helper.h
		tests/pe_bliss2/image_loader_tests.cpp
		tests/pe_bliss2/image_section_search_tests.cpp
		tests/pe_bliss2/image_shannon_entropy_tests.cpp
		tests/pe_bliss2/image_signature_tests.cpp
		tests/pe_bliss2/image_tests.cpp
		tests/pe_bliss2/input_buffer_mock.h
		tests/pe_bliss2/load_config_loader_tests.cpp
		tests/pe_bliss2/optional_header_tests.cpp
		tests/pe_bliss2/output_buffer_mock.h
		tests/pe_bliss2/overlay_tests.cpp
		tests/pe_bliss2/packed_byte_array_tests.cpp
		tests/pe_bliss2/packed_byte_vector_tests.cpp
		tests/pe_bliss2/packed_c_string_tests.cpp
		tests/pe_bliss2/packed_reflection_tests.cpp
		tests/pe_bliss2/packed_serialization_tests.cpp
		tests/pe_bliss2/packed_struct_tests.cpp
		tests/pe_bliss2/packed_utf16_string_tests.cpp
		tests/pe_bliss2/pe_error_helper.h
		tests/pe_bliss2/pe_error_tests.cpp
		tests/pe_bliss2/rich_header_tests.cpp
		tests/pe_bliss2/rva_file_offset_converter_tests.cpp
		tests/pe_bliss2/section_data_from_rva_tests.cpp
		tests/pe_bliss2/section_data_length_from_rva_tests.cpp
		tests/pe_bliss2/section_data_tests.cpp
		tests/pe_bliss2/section_header_tests.cpp
		tests/pe_bliss2/section_search_tests.cpp
		tests/pe_bliss2/section_table_tests.cpp
		tests/pe_bliss2/string_from_va_tests.cpp
		tests/pe_bliss2/string_to_va_tests.cpp
		tests/pe_bliss2/struct_from_va_tests.cpp
		tests/pe_bliss2/struct_to_va_tests.cpp
		tests/pe_bliss2/test_structs.h
		tests/pe_bliss2/directories/accelerator_table_loader_tests.cpp
		tests/pe_bliss2/directories/accelerator_table_tests.cpp
		tests/pe_bliss2/directories/arm64_exception_directory_tests.cpp
		tests/pe_bliss2/directories/arm64_exception_loader_tests.cpp
		tests/pe_bliss2/directories/arm_common_exceptions_loader_tests.cpp
		tests/pe_bliss2/directories/arm_common_exception_helpers.h
		tests/pe_bliss2/directories/arm_common_unwind_info_tests.cpp
		tests/pe_bliss2/directories/arm_exception_directory_tests.cpp
		tests/pe_bliss2/directories/arm_exception_loader_tests.cpp
		tests/pe_bliss2/directories/bitmap_reader_writer_tests.cpp
		tests/pe_bliss2/directories/bound_import_loader_tests.cpp
		tests/pe_bliss2/directories/debug_directory_tests.cpp
		tests/pe_bliss2/directories/debug_loader_tests.cpp
		tests/pe_bliss2/directories/dotnet_directory_tests.cpp
		tests/pe_bliss2/directories/dotnet_loader_tests.cpp
		tests/pe_bliss2/directories/exported_address_tests.cpp
		tests/pe_bliss2/directories/export_directory_tests.cpp
		tests/pe_bliss2/directories/export_loader_tests.cpp
		tests/pe_bliss2/directories/guid_tests.cpp
		tests/pe_bliss2/directories/icon_cursor_reader_tests.cpp
		tests/pe_bliss2/directories/icon_cursor_validation_tests.cpp
		tests/pe_bliss2/directories/icon_cursor_writer_tests.cpp
		tests/pe_bliss2/directories/imported_directory_tests.cpp
		tests/pe_bliss2/directories/import_loader_tests.cpp
		tests/pe_bliss2/directories/load_config_directory_tests.cpp
		tests/pe_bliss2/directories/manifest_tests.cpp
		tests/pe_bliss2/directories/message_table_reader_tests.cpp
		tests/pe_bliss2/directories/pugixml_manifest_accessor_tests.cpp
		tests/pe_bliss2/directories/rebase_tests.cpp
		tests/pe_bliss2/directories/relocation_entry_tests.cpp
		tests/pe_bliss2/directories/relocation_loader_tests.cpp
		tests/pe_bliss2/directories/resources_loader_tests.cpp
		tests/pe_bliss2/directories/resource_directory_tests.cpp
		tests/pe_bliss2/directories/resource_reader_tests.cpp
		tests/pe_bliss2/directories/resource_writer_tests.cpp
		tests/pe_bliss2/directories/security
		tests/pe_bliss2/directories/security_directory_loader_tests.cpp
		tests/pe_bliss2/directories/string_table_reader_writer_tests.cpp
		tests/pe_bliss2/directories/string_table_tests.cpp
		tests/pe_bliss2/directories/tls_directory_tests.cpp
		tests/pe_bliss2/directories/tls_loader_tests.cpp
		tests/pe_bliss2/directories/version_info_reader_tests.cpp
		tests/pe_bliss2/directories/version_info_tests.cpp
		tests/pe_bliss2/directories/x64_exceptions_loader_tests.cpp
		tests/pe_bliss2/directories/x64_exception_directory_tests.cpp
		tests/pe_bliss2/directories/security/attribute_map_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_certificate_store_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_format_validator_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_loader_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_page_hashes_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_pkcs7_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_program_info_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_timestamp_signature_format_validator_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_timestamp_signature_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_timestamp_signature_verifier_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_timestamp_signature_verifier_tests.cpp
		tests/pe_bliss2/directories/security/authenticode_verifier_tests.cpp
		tests/pe_bliss2/directories/security/buffer_hash_tests.cpp
		tests/pe_bliss2/directories/security/crypto_algorithms_tests.cpp
		tests/pe_bliss2/directories/security/hash_helpers_tests.cpp
		tests/pe_bliss2/directories/security/hex_string_helpers.h
		tests/pe_bliss2/directories/security/image_hash_tests.cpp
		tests/pe_bliss2/directories/security/non_contiguous_buffer.h
		tests/pe_bliss2/directories/security/pkcs7_format_validator_tests.cpp
		tests/pe_bliss2/directories/security/pkcs7_signature_tests.cpp
		tests/pe_bliss2/directories/security/pkcs7_tests.cpp
		tests/pe_bliss2/directories/security/signature_verifier_tests.cpp
		tests/pe_bliss2/directories/security/signer_info_ref_tests.cpp
		tests/pe_bliss2/directories/security/x509_certificate_store_tests.cpp
		tests/pe_bliss2/directories/security/x509_certificate_tests.cpp
		tests/utilities/math_tests.cpp
		tests/utilities/range_helpers_tests.cpp
		tests/utilities/safe_uint_tests.cpp
		tests/utilities/scoped_guard_tests.cpp
		tests/utilities/shannon_entropy_tests.cpp
		tests/utilities/static_class_tests.cpp
		tests/utilities/string_tests.cpp
)

target_link_libraries(tests PRIVATE utilities buffers pe_bliss2 gtest_main gmock_main)

if(MSVC)
	target_compile_options(tests PRIVATE "/MP")
endif()

gtest_discover_tests(tests)
