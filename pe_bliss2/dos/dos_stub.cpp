#include "pe_bliss2/dos/dos_stub.h"

#include <exception>
#include <memory>
#include <system_error>

#include "buffers/input_buffer_section.h"
#include "pe_bliss2/pe_error.h"

namespace
{

struct dos_stub_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "dos_stub";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::dos::dos_stub_errc;
		switch (static_cast<pe_bliss::dos::dos_stub_errc>(ev))
		{
		case unable_to_read_dos_stub:
			return "Unable to read DOS stub and Rich header";
		default:
			return {};
		}
	}
};

const dos_stub_error_category dos_stub_error_category_instance;

} //namespace

namespace pe_bliss::dos
{

std::error_code make_error_code(dos_stub_errc e) noexcept
{
	return { static_cast<int>(e), dos_stub_error_category_instance };
}

void dos_stub::deserialize(const buffers::input_buffer_ptr& buffer,
	const dos_stub_load_options& options)
{
	try
	{
		auto buffer_pos = buffer->rpos();
		auto dos_stub_size = options.e_lfanew <= buffer_pos
			? 0u : options.e_lfanew - buffer_pos;
		auto buffer_section = std::make_shared<buffers::input_buffer_section>(
			buffer, buffer_pos, dos_stub_size);
		buffer_section->set_relative_offset(0);
		ref_buffer::deserialize(buffer_section, options.copy_memory);
		if (!options.copy_memory)
		{
			//Verify that enough data is available
			//and position buffer pointer to the end of the stub
			buffer->set_rpos(buffer_pos + dos_stub_size);
		}
	}
	catch (...)
	{
		std::throw_with_nested(pe_error(dos_stub_errc::unable_to_read_dos_stub));
	}
}

} //namespace pe_bliss::dos
