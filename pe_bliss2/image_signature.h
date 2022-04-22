#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/detail/packed_struct_base.h"

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss
{

enum class image_signature_errc
{
	invalid_pe_signature = 1,
	unable_to_read_pe_signature
};

std::error_code make_error_code(image_signature_errc) noexcept;

class [[nodiscard]] image_signature :
	public detail::packed_struct_base<std::uint32_t>
{
public:
	using signature_type = std::uint32_t;

public:
	//PE\0\0, little endian
	static constexpr signature_type pe_signature = 0x4550;

public:
	//When deserializing, buffer should point to the image signature DWORD
	void deserialize(buffers::input_buffer_interface& buf,
		bool allow_virtual_memory = false);
	void serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part = true) const;

public:
	[[nodiscard]]
	signature_type get_signature() const noexcept
	{
		return base_struct().get();
	}

	void set_signature(signature_type signature) noexcept
	{
		base_struct() = signature;
	}

	[[nodiscard]]
	pe_error_wrapper validate() const noexcept;
};

} //namespace pe_bliss

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::image_signature_errc> : true_type {};
} //namespace std
