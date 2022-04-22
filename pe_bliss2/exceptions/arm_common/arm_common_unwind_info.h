#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <list>
#include <system_error>
#include <type_traits>
#include <variant>

#include "boost/endian/conversion.hpp"

#include "pe_bliss2/packed_byte_array.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss::exceptions::arm_common
{

enum class exception_directory_errc
{
	invalid_epilog_start_offset = 1,
	invalid_epilog_start_index,
	invalid_epilog_condition,
	invalid_function_length
};

} //namespace pe_bliss::exceptions::arm_common

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::exceptions::arm_common::exception_directory_errc> : true_type {};
} //namespace std

namespace pe_bliss::exceptions::arm_common
{

std::error_code make_error_code(exception_directory_errc) noexcept;

template<bool HasCondition>
class epilog_info
{
public:
	using descriptor_type = packed_struct<std::uint32_t>;
	using epilog_start_index_type = std::conditional_t<HasCondition, std::uint8_t, std::uint16_t>;

public:
	static constexpr std::uint32_t epilog_start_index_mask = HasCondition ? 0xff000000u : 0xffc00000u;
	static constexpr std::uint32_t epilog_start_index_shift = HasCondition ? 24u : 22u;
	static constexpr std::uint8_t unconditional_epilog = 0x0eu;

public:
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

public:
	//The offset in bytes, divided by 2 (?), of the epilog relative to the start of the function.
	[[nodiscard]]
	std::uint32_t get_epilog_start_offset() const noexcept
	{
		return (descriptor_.get() & 0x3ffffu) * 2u;
	}

	//The byte index of the first unwind code that describes this epilog.
	[[nodiscard]]
	epilog_start_index_type get_epilog_start_index() const noexcept
	{
		return static_cast<epilog_start_index_type>((descriptor_.get() & epilog_start_index_mask)
			>> epilog_start_index_shift);
	}

	//The condition under which the epilogue is executed.
	//For unconditional epilogues, it should be set to 0xE, which indicates "always".
	//(An epilogue must be entirely conditional or entirely unconditional,
	//and in Thumb-2 mode, the epilogue begins with the first instruction after the IT opcode.)
	[[nodiscard]]
	std::uint8_t get_epilog_condition() const noexcept
		requires (HasCondition)
	{
		return static_cast<std::uint8_t>((descriptor_.get() & 0xf00000u) >> 20u);
	}

public:
	void set_epilog_start_offset(std::uint32_t offset);
	void set_epilog_start_index(epilog_start_index_type index);
	void set_epilog_condition(std::uint8_t condition)
		requires (HasCondition);

private:
	descriptor_type descriptor_;
};

template<std::size_t Length>
class unwind_code_common
{
public:
	using descriptor_type = packed_byte_array<Length>;

public:
	static constexpr auto length = Length;

public:
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

protected:
	template<std::size_t ByteCount>
	using required_uint_type = typename std::conditional_t<ByteCount == sizeof(std::uint8_t),
		std::type_identity<std::uint8_t>,
		std::conditional<ByteCount == sizeof(std::uint16_t), std::uint16_t, std::uint32_t>>::type;

	template<std::size_t FromBit, std::size_t ToBit>
	[[nodiscard]]
	auto get_value() const noexcept
	{
		static_assert(FromBit <= ToBit);
		constexpr std::size_t byte_count = (ToBit - FromBit + CHAR_BIT) / CHAR_BIT;
		static_assert(byte_count && byte_count <= sizeof(std::uint32_t));
		using result_type = required_uint_type<byte_count>;
		using source_type = required_uint_type<Length>;

		source_type src{};
		std::memcpy(&src, descriptor_.value().data(), Length);
		boost::endian::big_to_native_inplace(src);

		src &= create_first_bit_mask<FromBit, sizeof(source_type)>();
		src >>= (sizeof(source_type) * CHAR_BIT - 1u - ToBit);
		return static_cast<result_type>(src);
	}

	template<std::size_t FromBit, std::size_t ToBit, typename Value>
	[[nodiscard]]
	void set_value(Value value)
	{
		static_assert(FromBit <= ToBit);
		constexpr std::size_t byte_count = (ToBit - FromBit + CHAR_BIT) / CHAR_BIT;
		static_assert(byte_count && byte_count <= sizeof(std::uint32_t));
		using result_type = required_uint_type<byte_count>;
		using source_type = required_uint_type<Length>;

		constexpr auto max_value = create_bit_mask<FromBit, ToBit, sizeof(source_type)>()
			>> (sizeof(source_type) * CHAR_BIT - 1u - ToBit);
		if (value > max_value)
			throw pe_error(utilities::generic_errc::integer_overflow);

		source_type src{};
		std::memcpy(&src, descriptor_.value().data(), Length);
		boost::endian::big_to_native_inplace(src);
		src &= ~create_bit_mask<FromBit, ToBit, sizeof(source_type)>();
		src |= value << (sizeof(source_type) * CHAR_BIT - 1u - ToBit);
		boost::endian::native_to_big_inplace(src);
		std::memcpy(descriptor_.value().data(), &src, Length);
	}

	template<std::size_t Multiple, std::size_t StartBit,
		std::size_t EndBit, auto ErrorCode, typename Value>
		void set_scaled_value(Value value)
	{
		if constexpr (Multiple > 1u)
		{
			if (value % Multiple)
				throw pe_error(ErrorCode);

			value /= Multiple;
		}

		try
		{
			set_value<StartBit, EndBit>(value);
		}
		catch (const pe_error&)
		{
			throw pe_error(ErrorCode);
		}
	}

private:
	template<std::size_t FirstBit, std::size_t ValueLength>
	[[nodiscard]]
	static consteval auto create_first_bit_mask() noexcept
	{
		return (1u << (ValueLength * CHAR_BIT - FirstBit)) - 1u;
	}

	template<std::size_t LastBit, std::size_t ValueLength>
	[[nodiscard]]
	static consteval auto create_last_bit_mask() noexcept
	{
		return ~create_first_bit_mask<LastBit + 1u, ValueLength>();
	}

	template<std::size_t FirstBit, std::size_t LastBit, std::size_t ValueLength>
	[[nodiscard]]
	static consteval auto create_bit_mask() noexcept
	{
		return create_first_bit_mask<FirstBit, ValueLength>()
			& create_last_bit_mask<LastBit, ValueLength>();
	}

private:
	descriptor_type descriptor_;
};

template<typename RuntimeFunctionEntry, typename PackedUnwindData, typename ExtendedUnwindRecord,
	typename... Bases>
class runtime_function_base
	: public Bases...
{
public:
	using descriptor_type = packed_struct<RuntimeFunctionEntry>;
	using unwind_info_type = std::variant<std::monostate, PackedUnwindData, ExtendedUnwindRecord>;

public:
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	unwind_info_type& get_unwind_info() noexcept
	{
		return unwind_info_;
	}

	[[nodiscard]]
	const unwind_info_type& get_unwind_info() const noexcept
	{
		return unwind_info_;
	}

	[[nodiscard]]
	bool has_extended_unwind_record() const noexcept
	{
		return !(descriptor_->unwind_data & 0b11u);
	}

private:
	descriptor_type descriptor_;
	unwind_info_type unwind_info_;
};

template<typename EpilogInfo, typename UnwindRecordOptions>
class extended_unwind_record
{
public:
	using main_header_type = packed_struct<std::uint32_t>;
	//A list of information about epilog scopes, packed one to a word,
	//comes after the header and optional extended header.
	//They're stored in order of increasing starting offset.
	using epilog_info_list_type = std::list<EpilogInfo>;

	using unwind_code_type = typename UnwindRecordOptions::unwind_code_type;
	using unwind_code_list_type = std::list<unwind_code_type>;

	//XXX: there is some additional compiler-specific unwind data after the handler RVA.
	//dumpbin outputs it, but it is undocumented.
	using exception_handler_rva_type = packed_struct<std::uint32_t>;

public:
	static constexpr auto function_length_multiplier = UnwindRecordOptions::function_length_multiplier;
	static constexpr auto has_f_bit = UnwindRecordOptions::has_f_bit;

private:
	static constexpr std::uint32_t base_epilog_count_mask = has_f_bit ? 0xf800000u : 0x7c00000u;
	static constexpr std::uint32_t base_epilog_count_shift = has_f_bit ? 23u : 22u;
	static constexpr std::uint32_t base_code_words_mask = has_f_bit ? 0xf0000000u : 0xf8000000u;
	static constexpr std::uint32_t base_code_words_shift = has_f_bit ? 28u : 27u;

public:
	[[nodiscard]]
	main_header_type& get_main_header() noexcept
	{
		return main_header_;
	}

	[[nodiscard]]
	const main_header_type& get_main_header() const noexcept
	{
		return main_header_;
	}

	[[nodiscard]]
	main_header_type& get_main_extended_header() noexcept
	{
		return main_extended_header_;
	}

	[[nodiscard]]
	const main_header_type& get_main_extended_header() const noexcept
	{
		return main_extended_header_;
	}

	[[nodiscard]]
	epilog_info_list_type& get_epilog_info_list() noexcept
	{
		return epilog_info_list_;
	}

	[[nodiscard]]
	const epilog_info_list_type& get_epilog_info_list() const noexcept
	{
		return epilog_info_list_;
	}

	[[nodiscard]]
	unwind_code_list_type& get_unwind_code_list() noexcept
	{
		return unwind_code_list_;
	}

	[[nodiscard]]
	const unwind_code_list_type& get_unwind_code_list() const noexcept
	{
		return unwind_code_list_;
	}

	[[nodiscard]]
	exception_handler_rva_type& get_exception_handler_rva() noexcept
	{
		return exception_handler_rva_;
	}

	[[nodiscard]]
	const exception_handler_rva_type& get_exception_handler_rva() const noexcept
	{
		return exception_handler_rva_;
	}

public:
	//The total length of the function in bytes, divided by function_length_multiplier. 
	[[nodiscard]]
	std::uint32_t get_function_length() const noexcept
	{
		return (main_header_.get() & 0x3ffffu) * function_length_multiplier;
	}

	//The version of the remaining .xdata.
	//Currently, only version 0 is defined, so values of 1-3 aren't permitted.
	[[nodiscard]]
	std::uint8_t get_version() const noexcept
	{
		return static_cast<std::uint8_t>((main_header_.get() & 0xc0000u) >> 18u);
	}

	//The presence (1) or absence (0) of exception data.
	[[nodiscard]]
	bool has_exception_data() const noexcept
	{
		return (main_header_.get() & 0x100000u) != 0u;
	}

	//Information describing a single epilog is packed into the header (1)
	//rather than requiring additional scope words later (0).
	[[nodiscard]]
	bool single_epilog_info_packed() const noexcept
	{
		return (main_header_.get() & 0x200000u) != 0u;
	}

	//Indicates that this record describes a function fragment (1) or a full function (0).
	//A fragment implies that there is no prologue and that all prologue processing should be ignored.
	[[nodiscard]]
	bool is_function_fragment() const noexcept
		requires (has_f_bit)
	{
		return (main_header_.get() & 0x400000u) != 0u;
	}

	//Has two meanings, depending on the state of E bit (single_epilog_info_packed):
	//If E is 0, it specifies the count of the total number of epilog scopes.
	//If E is 1, then this field specifies the index of the first unwind code that describes the one and only epilog.
	[[nodiscard]]
	std::uint16_t get_epilog_count() const noexcept
	{
		return has_extended_main_header() ? get_extended_epilog_count() : get_base_epilog_count();
	}

	//The number of 32-bit words needed to contain all of the unwind codes.
	[[nodiscard]]
	std::uint8_t get_code_words() const noexcept
	{
		return has_extended_main_header() ? get_extended_code_words() : get_base_code_words();
	}

	[[nodiscard]]
	bool has_extended_main_header() const noexcept
	{
		return !get_base_epilog_count() && !get_base_code_words();
	}
	
public:
	void set_function_length(std::uint32_t length)
	{
		if (length % 4u)
			throw pe_error(exception_directory_errc::invalid_function_length);

		length /= 4u;
		if (length > 0x3ffffu)
			throw pe_error(exception_directory_errc::invalid_function_length);

		main_header_.get() &= ~0x3ffffu;
		main_header_.get() |= length;
	}

	void set_version(std::uint8_t version)
	{
		if (version != 0u)
			throw pe_error(exception_directory_errc::invalid_version);

		main_header_.get() &= ~0xc0000u;
	}

	void set_has_exception_data(bool value) noexcept
	{
		if (value)
			main_header_.get() |= 0x100000u;
		else
			main_header_.get() &= ~0x100000u;
	}

	void set_single_epilog_info_packed(bool value) noexcept
	{
		if (value)
			main_header_.get() |= 0x200000u;
		else
			main_header_.get() &= ~0x200000u;
	}

	void set_epilog_count(std::uint16_t count) noexcept
	{
		main_extended_header_.get() &= ~0xffffu;
		main_header_.get() &= ~base_epilog_count_mask;

		if (count > 0x1fu)
		{
			auto code_words = get_code_words();

			//Put epilog count to extended header
			main_extended_header_.get() |= count;

			//And move code words count to extended header
			main_header_.get() &= ~base_code_words_mask;
			main_extended_header_.get() &= ~0xff0000u;
			main_extended_header_.get() |= code_words << 16u;
		}
		else
		{
			if (get_base_code_words())
			{
				//Both epilog count and code words can be put to base header
				main_header_.get() |= count << base_epilog_count_shift;
			}
			else
			{
				main_extended_header_.get() |= count;
			}
		}
	}

	void set_code_words(std::uint8_t count) noexcept
	{
		main_extended_header_.get() &= ~0xff0000u;
		main_header_.get() &= ~base_code_words_mask;

		if (count > 0x1fu)
		{
			auto epilog_count = get_epilog_count();

			//Put code words to extended header
			main_extended_header_.get() |= static_cast<std::uint32_t>(count) << 16u;

			//And move epilog count to extended header
			main_header_.get() &= ~base_epilog_count_mask;
			main_extended_header_.get() &= ~0xffffu;
			main_extended_header_.get() |= epilog_count;
		}
		else
		{
			if (get_base_epilog_count())
			{
				//Both epilog count and code words can be put to base header
				main_header_.get() |= static_cast<std::uint32_t>(count)
					<< base_code_words_shift;
			}
			else
			{
				main_extended_header_.get() |= static_cast<std::uint32_t>(count) << 16u;
			}
		}
	}

private:
	[[nodiscard]]
	std::uint8_t get_base_epilog_count() const noexcept
	{
		return static_cast<std::uint8_t>((main_header_.get() & base_epilog_count_mask)
			>> base_epilog_count_shift);
	}

	[[nodiscard]]
	std::uint8_t get_base_code_words() const noexcept
	{
		return static_cast<std::uint8_t>((main_header_.get() & base_code_words_mask)
			>> base_code_words_shift);
	}

	[[nodiscard]]
	std::uint16_t get_extended_epilog_count() const noexcept
	{
		return static_cast<std::uint16_t>(main_extended_header_.get() & 0xffffu);
	}

	[[nodiscard]]
	std::uint8_t get_extended_code_words() const noexcept
	{
		return static_cast<std::uint8_t>((main_extended_header_.get() & 0xff0000u) >> 16u);
	}

private:
	main_header_type main_header_;
	main_header_type main_extended_header_;
	epilog_info_list_type epilog_info_list_;
	unwind_code_list_type unwind_code_list_;
	exception_handler_rva_type exception_handler_rva_;
};

template<template<typename...> typename RuntimeFunctionBase, typename... Bases>
class exception_directory_base
	: public Bases...
{
public:
	using runtime_function_list_type = std::list<RuntimeFunctionBase<Bases...>>;

public:
	[[nodiscard]]
	runtime_function_list_type& get_runtime_function_list() noexcept
	{
		return runtime_function_list_;
	}

	[[nodiscard]]
	const runtime_function_list_type& get_runtime_function_list() const noexcept
	{
		return runtime_function_list_;
	}

private:
	runtime_function_list_type runtime_function_list_;
};

} //namespace namespace pe_bliss::exceptions::arm_common
