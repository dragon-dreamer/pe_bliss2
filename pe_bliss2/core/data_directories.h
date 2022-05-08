#pragma once

#include <cstdint>
#include <cstddef>
#include <system_error>
#include <type_traits>
#include <vector>

#include "pe_bliss2/detail/image_data_directory.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/packed_struct.h"

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::core
{

enum class data_directories_errc
{
	unable_to_read_data_directory = 1
};

std::error_code make_error_code(data_directories_errc) noexcept;

class [[nodiscard]] data_directories
{
public:
	static constexpr auto directory_packed_size = detail::packed_reflection
		::get_type_size<detail::image_data_directory>();

	using base_struct_type = packed_struct<detail::image_data_directory>;
	using directories_list = std::vector<base_struct_type>;

public:
	enum class directory_type : std::uint32_t
	{
		exports = 0,
		imports = 1,
		resource = 2,
		exception = 3,
		security = 4,
		basereloc = 5,
		debug = 6,
		architecture = 7,
		globalptr = 8,
		tls = 9,
		config = 10,
		bound_import = 11,
		iat = 12,
		delay_import = 13,
		com_descriptor = 14
	};

public:
	//When deserializing, buf should point to the end of optional header
	void deserialize(buffers::input_buffer_interface& buf,
		std::uint32_t number_of_rva_and_sizes, bool allow_virtual_memory = false);
	void serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part = true) const;

	[[nodiscard]]
	std::size_t size() const noexcept
	{
		return directories_.size();
	}

	void set_size(std::uint32_t size)
	{
		directories_.resize(size);
	}

	[[nodiscard]]
	bool has_directory(directory_type type) const noexcept
	{
		return directories_.size() > static_cast<std::size_t>(type);
	}

	[[nodiscard]]
	bool has_nonempty_directory(directory_type type) const noexcept
	{
		return has_directory(type) && get_directory(type)->virtual_address;
	}

	[[nodiscard]]
	base_struct_type& get_directory(directory_type type)
	{
		return directories_.at(static_cast<std::size_t>(type));
	}

	[[nodiscard]]
	const base_struct_type& get_directory(directory_type type) const
	{
		return directories_.at(static_cast<std::size_t>(type));
	}

	[[nodiscard]]
	const directories_list& get_directories() const noexcept
	{
		return directories_;
	}

	[[nodiscard]]
	directories_list& get_directories() noexcept
	{
		return directories_;
	}
	
	void remove_directory(directory_type type) noexcept
	{
		if (has_nonempty_directory(type))
			get_directory(type) = {};
	}

public:
	[[nodiscard]] bool has_imports() const noexcept
	{
		return has_nonempty_directory(directory_type::imports);
	}

	[[nodiscard]] bool has_exports() const noexcept
	{
		return has_nonempty_directory(directory_type::exports);
	}

	[[nodiscard]] bool has_resources() const noexcept
	{
		return has_nonempty_directory(directory_type::resource);
	}

	[[nodiscard]] bool has_security() const noexcept
	{
		return has_nonempty_directory(directory_type::security);
	}

	[[nodiscard]] bool has_reloc() const noexcept
	{
		return has_nonempty_directory(directory_type::basereloc);
	}

	[[nodiscard]] bool has_tls() const noexcept
	{
		return has_nonempty_directory(directory_type::tls);
	}

	[[nodiscard]] bool has_config() const noexcept
	{
		return has_nonempty_directory(directory_type::config);
	}

	[[nodiscard]] bool has_bound_import() const noexcept
	{
		return has_nonempty_directory(directory_type::bound_import);
	}
	
	[[nodiscard]] bool has_delay_import() const noexcept
	{
		return has_nonempty_directory(directory_type::delay_import);
	}
	
	[[nodiscard]] bool is_dotnet() const noexcept
	{
		return has_nonempty_directory(directory_type::com_descriptor);
	}

	[[nodiscard]] bool has_exception_directory() const noexcept
	{
		return has_nonempty_directory(directory_type::exception);
	}

	[[nodiscard]] bool has_debug() const noexcept
	{
		return has_nonempty_directory(directory_type::debug);
	}

public:
	std::uint32_t strip_data_directories(std::uint32_t min_count);

private:
	directories_list directories_;
};

} //namespace pe_bliss::core

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::core::data_directories_errc> : true_type {};
} //namespace std
