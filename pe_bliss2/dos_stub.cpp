#include "pe_bliss2/dos_stub.h"

#include <system_error>

#include "buffers/input_buffer_interface.h"
#include "pe_bliss2/pe_error.h"

namespace
{

struct dos_stob_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "dos_stub";
	}

	std::string message(int ev) const override
	{
		switch (static_cast<pe_bliss::dos_stub_errc>(ev))
		{
		case pe_bliss::dos_stub_errc::unable_to_read_dos_stub:
			return "Unable to read DOS stub and Rich header";
		default:
			return {};
		}
	}
};

const dos_stob_error_category dos_stob_error_category_instance;

} //namespace

namespace pe_bliss
{

std::error_code make_error_code(dos_stub_errc e) noexcept
{
	return { static_cast<int>(e), dos_stob_error_category_instance };
}

void dos_stub::deserialize(buffers::input_buffer_interface& buf, std::size_t e_lfanew)
{
	buffer_pos_ = buf.rpos();

	if (e_lfanew <= buffer_pos_)
	{
		data_.clear();
		return;
	}

	data_.resize(e_lfanew - buffer_pos_);
	if (buf.read(data_.size(), data_.data()) != data_.size())
		throw pe_error(dos_stub_errc::unable_to_read_dos_stub);
}

} //namespace pe_bliss
