#pragma once

#include <cstdint>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/detail/packed_struct_base.h"

namespace buffers
{
class input_buffer_stateful_wrapper_ref;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::core
{

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
	void deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
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
};

} //namespace pe_bliss::core
