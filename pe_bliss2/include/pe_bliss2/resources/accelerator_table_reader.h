#pragma once

#include <cstddef>
#include <system_error>
#include <type_traits>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/resources/accelerator_table.h"

namespace pe_bliss::resources
{

enum class accelerator_table_reader_errc
{
	too_many_accelerators = 1,
};

struct [[nodiscard]] accelerator_table_read_options
{
	bool allow_virtual_data = false;
	std::uint16_t max_accelerator_count = 0xfff;
};

std::error_code make_error_code(accelerator_table_reader_errc) noexcept;

[[nodiscard]]
accelerator_table_details accelerator_table_from_resource(
	buffers::input_buffer_stateful_wrapper_ref& buf,
	const accelerator_table_read_options& options = {});

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::accelerator_table_reader_errc> : true_type {};
} //namespace std
