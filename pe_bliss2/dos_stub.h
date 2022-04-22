#pragma once

#include <cstdint>
#include <vector>
#include <system_error>
#include <type_traits>

namespace buffers
{
class input_buffer_interface;
} //namespace buffers

namespace pe_bliss
{

enum class dos_stub_errc
{
	unable_to_read_dos_stub = 1
};

std::error_code make_error_code(dos_stub_errc) noexcept;

//When deserializing, buf should point to the beginning of the DOS stub
class [[nodiscard]] dos_stub
{
public:
	using dos_stub_data_type = std::vector<std::byte>;

public:
	//When deserializing, buf should point to DOS stub start (right after DOS header)
	void deserialize(buffers::input_buffer_interface& buf,
		std::size_t e_lfanew);

	[[nodiscard]] dos_stub_data_type& data() noexcept
	{
		return data_;
	}

	[[nodiscard]] const dos_stub_data_type& data() const noexcept
	{
		return data_;
	}

	[[nodiscard]] std::size_t buffer_pos() const noexcept
	{
		return buffer_pos_;
	}

private:
	std::size_t buffer_pos_ = 0;
	dos_stub_data_type data_;
};

} //namespace pe_bliss

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::dos_stub_errc> : true_type {};
} //namespace std
