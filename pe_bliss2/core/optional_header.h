#pragma once

#include <array>
#include <cstdint>
#include <variant>

#include "pe_bliss2/detail/image_optional_header.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"

#include "utilities/static_class.h"

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::core
{

// validate() method does not validate image base, there is
// a separate validate_image_base() method which should be used.
struct [[nodiscard]] optional_header_validation_options
{
	//TODO: sometimes check_***, sometimes validate_***, make names similar
	bool check_address_of_entry_point = true;
	bool check_alignments = true;
	bool check_subsystem_version = true;
	bool check_size_of_heap = true;
	bool check_size_of_stack = true;
	bool check_size_of_headers = true;
};

class [[nodiscard]] optional_header
{
public:
	using optional_header_32_type = packed_struct<
		detail::image_optional_header_32>;
	using optional_header_64_type = packed_struct<
		detail::image_optional_header_64>;
	using base_struct_type = std::variant<
		optional_header_32_type, optional_header_64_type>;

public:
	using magic_type = std::uint16_t;

	enum class magic : magic_type
	{
		pe32 = 0x10b,
		pe64 = 0x20b
	};

	enum class subsystem : std::uint16_t
	{
		unknown = 0,
		native = 1,
		windows_gui = 2,
		windows_cui = 3,
		os2_cui = 5,
		posix_cui = 7,
		windows_ce_gui = 9,
		efi_application = 10,
		efi_boot_service_driver = 11,
		efi_runtime_driver = 12,
		efi_rom = 13,
		xbox = 14,
		windows_boot_application = 16,
		xbox_code_catalog = 17
	};

	struct dll_characteristics final : utilities::static_class
	{
		enum value : std::uint16_t
		{
			high_entropy_va = 0x0020, //ASLR with 64 bit address space.
			dynamic_base = 0x0040, //The DLL can be relocated at load time.
			//Code integrity checks are forced.If you set this flag and a section contains only uninitialized data,
			//set the PointerToRawData member of IMAGE_SECTION_HEADER for that section to zero;
			//otherwise, the image will fail to load because the digital signature cannot be verified.
			force_integrity = 0x0080,
			nx_compat = 0x0100, //The image is compatible with data execution prevention(DEP).
			no_isolation = 0x0200, //The image is isolation aware, but should not be isolated.
			//The image does not use structured exception handling(SEH). No handlers can be called in this image.
			no_seh = 0x0400,
			no_bind = 0x0800, //Do not bind the image.
			appcontainer = 0x1000, //Image should execute in an AppContainer.
			wdm_driver = 0x2000, //A WDM driver.
			guard_cf = 0x4000, //Image supports Control Flow Guard.
			terminal_server_aware = 0x8000 //The image is terminal server aware. 
		};
	};

	static constexpr std::uint32_t max_number_of_rva_and_sizes = 16;
	static constexpr std::uint32_t minimum_file_alignment = 0x200;
	static constexpr std::uint32_t minimum_section_alignment = 0x1000;
	static constexpr std::uint16_t min_major_subsystem_version = 3u;
	static constexpr std::uint16_t min_minor_subsystem_version = 10u;

public:
	//When deserializing, buf should point to optional header start (magic field)
	void deserialize(buffers::input_buffer_interface& buf,
		bool allow_virtual_memory = false);
	void serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part = true) const;

	[[nodiscard]]
	std::uint32_t get_number_of_rva_and_sizes() const noexcept;

	[[nodiscard]]
	std::uint32_t get_size_of_structure() const noexcept;

	[[nodiscard]]
	magic get_magic() const noexcept
	{
		return header_.index() == 0 ? magic::pe32 : magic::pe64;
	}

	[[nodiscard]]
	subsystem get_subsystem() const noexcept
	{
		return static_cast<subsystem>(get_raw_subsystem());
	}

	[[nodiscard]] bool is_windows_console() const noexcept
	{
		return get_subsystem() == subsystem::windows_cui;
	}

	[[nodiscard]] bool is_windows_gui() const noexcept
	{
		return get_subsystem() == subsystem::windows_gui;
	}

	[[nodiscard]]
	dll_characteristics::value get_dll_characteristics() const noexcept
	{
		return static_cast<dll_characteristics::value>(
			get_raw_dll_characteristics());
	}

	[[nodiscard]]
	base_struct_type& base_struct() noexcept
	{
		return header_;
	}

	[[nodiscard]]
	const base_struct_type& base_struct() const noexcept
	{
		return header_;
	}

	template<typename T>
	void initialize_with() noexcept
	{
		header_.emplace<T>();
	}

	template<typename Func>
	decltype(auto) access(Func&& func) const
	{
		return std::visit([func = std::forward<Func>(func)](const auto& obj) mutable {
			return std::forward<Func>(func)(obj.get()); }, header_);
	}

	template<typename Func>
	decltype(auto) access(Func&& func)
	{
		return std::visit([func = std::forward<Func>(func)](auto& obj) mutable {
			return std::forward<Func>(func)(obj.get()); }, header_);
	}

	[[nodiscard]]
	pe_error_wrapper validate(const optional_header_validation_options& options,
		bool is_dll) const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_address_of_entry_point(bool is_dll) const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_image_base(bool has_relocations) const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_file_alignment() const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_section_alignment() const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_subsystem_version() const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_size_of_heap() const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_size_of_stack() const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_size_of_headers() const noexcept;

	[[nodiscard]]
	bool is_low_alignment() const noexcept;

public:
	[[nodiscard]]
	std::uint8_t get_raw_major_linker_version() const noexcept;
	[[nodiscard]]
	std::uint8_t get_raw_minor_linker_version() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_size_of_code() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_size_of_initialized_data() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_size_of_uninitialized_data() const noexcept;
	[[nodiscard]]
	rva_type get_raw_address_of_entry_point() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_base_of_code() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_base_of_data() const;
	[[nodiscard]]
	std::uint64_t get_raw_image_base() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_section_alignment() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_file_alignment() const noexcept;
	[[nodiscard]]
	std::uint16_t get_raw_major_operating_system_version() const noexcept;
	[[nodiscard]]
	std::uint16_t get_raw_minor_operating_system_version() const noexcept;
	[[nodiscard]]
	std::uint16_t get_raw_major_image_version() const noexcept;
	[[nodiscard]]
	std::uint16_t get_raw_minor_image_version() const noexcept;
	[[nodiscard]]
	std::uint16_t get_raw_major_subsystem_version() const noexcept;
	[[nodiscard]]
	std::uint16_t get_raw_minor_subsystem_version() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_win32_version_value() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_size_of_image() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_size_of_headers() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_checksum() const noexcept;
	[[nodiscard]]
	std::uint16_t get_raw_subsystem() const noexcept;
	[[nodiscard]]
	std::uint16_t get_raw_dll_characteristics() const noexcept;
	[[nodiscard]]
	std::uint64_t get_raw_size_of_stack_reserve() const noexcept;
	[[nodiscard]]
	std::uint64_t get_raw_size_of_stack_commit() const noexcept;
	[[nodiscard]]
	std::uint64_t get_raw_size_of_heap_reserve() const noexcept;
	[[nodiscard]]
	std::uint64_t get_raw_size_of_heap_commit() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_loader_flags() const noexcept;
	[[nodiscard]]
	std::uint32_t get_raw_number_of_rva_and_sizes() const noexcept;

public:
	void set_raw_major_linker_version(std::uint8_t version) noexcept;
	void set_raw_minor_linker_version(std::uint8_t version) noexcept;
	void set_raw_size_of_code(std::uint32_t size) noexcept;
	void set_raw_size_of_initialized_data(std::uint32_t size) noexcept;
	void set_raw_size_of_uninitialized_data(std::uint32_t size) noexcept;
	void set_raw_address_of_entry_point(rva_type address) noexcept;
	void set_raw_base_of_code(std::uint32_t base) noexcept;
	void set_raw_base_of_data(std::uint32_t base);
	void set_raw_image_base(std::uint64_t base) noexcept;
	void set_raw_section_alignment(std::uint32_t alignment) noexcept;
	void set_raw_file_alignment(std::uint32_t alignment) noexcept;
	void set_raw_major_operating_system_version(std::uint16_t version) noexcept;
	void set_raw_minor_operating_system_version(std::uint16_t version) noexcept;
	void set_raw_major_image_version(std::uint16_t version) noexcept;
	void set_raw_minor_image_version(std::uint16_t version) noexcept;
	void set_raw_major_subsystem_version(std::uint16_t version) noexcept;
	void set_raw_minor_subsystem_version(std::uint16_t version) noexcept;
	void set_raw_win32_version_value(std::uint32_t version) noexcept;
	void set_raw_size_of_image(std::uint32_t size) noexcept;
	void set_raw_size_of_headers(std::uint32_t size) noexcept;
	void set_raw_checksum(std::uint32_t checksum) noexcept;
	void set_raw_subsystem(std::uint16_t subsystem) noexcept;
	void set_raw_dll_characteristics(std::uint16_t characteristics) noexcept;
	void set_raw_size_of_stack_reserve(std::uint64_t size) noexcept;
	void set_raw_size_of_stack_commit(std::uint64_t size) noexcept;
	void set_raw_size_of_heap_reserve(std::uint64_t size) noexcept;
	void set_raw_size_of_heap_commit(std::uint64_t size) noexcept;
	void set_raw_loader_flags(std::uint32_t flags) noexcept;
	void set_raw_number_of_rva_and_sizes(std::uint32_t number) noexcept;

private:
	base_struct_type header_;
};

} //namespace pe_bliss::detail::core
