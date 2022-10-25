#pragma once

#include <cstdint>
#include <limits>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/resources/version_info_block.h"

namespace buffers
{
class input_buffer_stateful_wrapper_ref;
} //namespace buffers

namespace pe_bliss::resources
{

enum class version_info_reader_errc
{
	unaligned_version_info_block = 1,
	invalid_value_length,
	value_read_error,
	invalid_block_length,
	key_read_error,
	excessive_data_in_buffer,
	child_read_error,
	unknown_value_type,
	invalid_string_value_length,
	block_tree_is_too_deep
};

std::error_code make_error_code(version_info_reader_errc) noexcept;

struct [[nodiscard]] version_info_read_options
{
	bool allow_virtual_data = false;
	bool copy_value_memory = false;
	std::uint32_t max_depth = 0xffu;
};

[[nodiscard]]
version_info_block_details version_info_from_resource(
	buffers::input_buffer_stateful_wrapper& buf,
	const version_info_read_options& options = {});

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::version_info_reader_errc> : true_type {};
} //namespace std
