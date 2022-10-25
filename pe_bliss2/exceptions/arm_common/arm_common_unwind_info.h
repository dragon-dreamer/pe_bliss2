#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "boost/endian/conversion.hpp"

#include "pe_bliss2/detail/packed_struct_base.h"
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
	invalid_function_length,
	invalid_version,
	unsupported_unwind_code
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
class [[nodiscard]] epilog_info : public detail::packed_struct_base<std::uint32_t>
{
public:
	using epilog_start_index_type = std::conditional_t<
		HasCondition, std::uint8_t, std::uint16_t>;

public:
	static constexpr std::uint32_t epilog_start_index_mask
		= HasCondition ? 0xff000000u : 0xffc00000u;
	static constexpr std::uint32_t epilog_start_index_shift
		= HasCondition ? 24u : 22u;
	static constexpr std::uint8_t unconditional_epilog = 0x0eu;

public:
	//The offset in bytes, divided by 2 (?),
	//of the epilog relative to the start of the function.
	[[nodiscard]]
	std::uint32_t get_epilog_start_offset() const noexcept;

	//The byte index of the first unwind code that describes this epilog.
	[[nodiscard]]
	epilog_start_index_type get_epilog_start_index() const noexcept;

	//The condition under which the epilogue is executed.
	//For unconditional epilogues, it should be set to 0xE, which indicates "always".
	//(An epilogue must be entirely conditional or entirely unconditional,
	//and in Thumb-2 mode, the epilogue begins with
	//the first instruction after the IT opcode.)
	[[nodiscard]]
	std::uint8_t get_epilog_condition() const noexcept
		requires (HasCondition);

public:
	void set_epilog_start_offset(std::uint32_t offset);
	void set_epilog_start_index(epilog_start_index_type index);
	void set_epilog_condition(std::uint8_t condition)
		requires (HasCondition);
};

template<std::size_t Length,
	std::uint8_t Matcher, std::uint8_t MatcherMask>
class [[nodiscard]] unwind_code_common
{
public:
	using descriptor_type = packed_byte_array<Length>;

public:
	static constexpr auto length = Length;
	static constexpr auto matcher = Matcher;
	static constexpr auto matcher_mask = MatcherMask;
	static_assert(length > 0 && length <= sizeof(std::uint32_t),
		"Invalid unwind code length");

public:
	void init() noexcept;
	[[nodiscard]]
	static constexpr bool matches(std::byte first_byte) noexcept;

public:
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept;
	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept;

public:
	template<std::size_t ByteCount>
		requires(ByteCount > 0 && ByteCount <= sizeof(std::uint32_t))
	using required_uint_type = typename std::conditional_t<
		ByteCount == sizeof(std::uint8_t),
		std::type_identity<std::uint8_t>,
		std::conditional<ByteCount == sizeof(std::uint16_t),
			std::uint16_t, std::uint32_t>>::type;

	template<std::size_t FromBit, std::size_t ToBit>
	[[nodiscard]]
	auto get_value() const noexcept;

	template<std::size_t FromBit, std::size_t ToBit, typename Value>
	[[nodiscard]]
	void set_value(Value value);

	template<std::size_t Multiple, std::size_t StartBit,
		std::size_t EndBit, auto ErrorCode, typename Value>
	void set_scaled_value(Value value);

private:
	template<std::size_t FirstBit, std::size_t ValueLength>
	[[nodiscard]]
	static consteval auto create_first_bit_mask() noexcept;

	template<std::size_t LastBit, std::size_t ValueLength>
	[[nodiscard]]
	static consteval auto create_last_bit_mask() noexcept;

	template<std::size_t FirstBit, std::size_t LastBit, std::size_t ValueLength>
	[[nodiscard]]
	static consteval auto create_bit_mask() noexcept;

private:
	descriptor_type descriptor_;
};

template<typename UnwindCodes>
struct create_unwind_code_helper {};
template<typename... UnwindCodes>
struct create_unwind_code_helper<std::variant<UnwindCodes...>>
{
	template<typename Vector>
	static void create_unwind_code(
		std::byte first_byte, Vector& code_list);
};

template<typename Vector>
void create_unwind_code(std::byte first_byte, Vector& code_list);

template<typename RuntimeFunctionEntry,
	typename PackedUnwindData, typename ExtendedUnwindRecord,
	typename... Bases>
class [[nodiscard]] runtime_function_base
	: public detail::packed_struct_base<RuntimeFunctionEntry>
	, public Bases...
{
public:
	using unwind_info_type = std::variant<
		std::monostate, PackedUnwindData, ExtendedUnwindRecord>;

public:
	[[nodiscard]]
	unwind_info_type& get_unwind_info() noexcept;
	[[nodiscard]]
	const unwind_info_type& get_unwind_info() const noexcept;

	[[nodiscard]]
	bool has_extended_unwind_record() const noexcept;

private:
	unwind_info_type unwind_info_;
};

class [[nodiscard]] extended_unwind_record_base
{
public:
	using main_header_type = packed_struct<std::uint32_t>;

	//TODO: there is some additional compiler-specific unwind data after the handler RVA.
	//dumpbin outputs it, but it is undocumented.
	using exception_handler_rva_type = packed_struct<std::uint32_t>;

public:
	[[nodiscard]]
	inline main_header_type& get_main_header() noexcept;
	[[nodiscard]]
	inline const main_header_type& get_main_header() const noexcept;

	[[nodiscard]]
	inline main_header_type& get_main_extended_header() noexcept;
	[[nodiscard]]
	inline const main_header_type& get_main_extended_header() const noexcept;

	[[nodiscard]]
	inline exception_handler_rva_type& get_exception_handler_rva() noexcept;
	[[nodiscard]]
	inline const exception_handler_rva_type& get_exception_handler_rva() const noexcept;

public:
	//The version of the remaining .xdata.
	//Currently, only version 0 is defined, so values of 1-3 aren't permitted.
	[[nodiscard]]
	std::uint8_t get_version() const noexcept;

	//The presence (1) or absence (0) of exception data.
	[[nodiscard]]
	bool has_exception_data() const noexcept;

	//Information describing a single epilog is packed into the header (1)
	//rather than requiring additional scope words later (0).
	[[nodiscard]]
	bool single_epilog_info_packed() const noexcept;

	//Extended Epilog Count and Extended Code Words are 16-bit and 8-bit fields, respectively.
	//They provide more space for encoding an unusually large number of epilogs,
	//or an unusually large number of unwind code words.
	//The extension word that contains these fields is only present
	//if both the Epilog Count and Code Words fields in the first header word are 0.
	[[nodiscard]]
	std::uint16_t get_extended_epilog_count() const noexcept;

	[[nodiscard]]
	std::uint8_t get_extended_code_words() const noexcept;

public:
	void set_version(std::uint8_t version);

	void set_has_exception_data(bool value) noexcept;

	void set_single_epilog_info_packed(bool value) noexcept;

	void set_extended_epilog_count(std::uint16_t count) noexcept;

	void set_extended_code_words(std::uint8_t count) noexcept;

protected:
	main_header_type main_header_;
	main_header_type main_extended_header_;
	exception_handler_rva_type exception_handler_rva_;
};

template<typename EpilogInfo, typename UnwindRecordOptions>
class [[nodiscard]] extended_unwind_record
	: public extended_unwind_record_base
{
public:
	//A list of information about epilog scopes, packed one to a word,
	//comes after the header and optional extended header.
	//They're stored in order of increasing starting offset.
	using epilog_info_list_type = std::vector<EpilogInfo>;

	using unwind_code_type = typename UnwindRecordOptions::unwind_code_type;
	using unwind_code_list_type = std::vector<unwind_code_type>;

public:
	static constexpr auto function_length_multiplier
		= UnwindRecordOptions::function_length_multiplier;
	static constexpr auto has_f_bit = UnwindRecordOptions::has_f_bit;

public:
	static constexpr std::uint32_t base_epilog_count_mask
		= has_f_bit ? 0xf800000u : 0x7c00000u;
	static constexpr std::uint32_t base_epilog_count_shift
		= has_f_bit ? 23u : 22u;
	static constexpr std::uint32_t base_code_words_mask
		= has_f_bit ? 0xf0000000u : 0xf8000000u;
	static constexpr std::uint32_t base_code_words_shift
		= has_f_bit ? 28u : 27u;

public:
	[[nodiscard]]
	epilog_info_list_type& get_epilog_info_list() & noexcept;
	[[nodiscard]]
	const epilog_info_list_type& get_epilog_info_list() const& noexcept;
	[[nodiscard]]
	epilog_info_list_type get_epilog_info_list() && noexcept;

	[[nodiscard]]
	unwind_code_list_type& get_unwind_code_list() & noexcept;
	[[nodiscard]]
	const unwind_code_list_type& get_unwind_code_list() const& noexcept;
	[[nodiscard]]
	unwind_code_list_type get_unwind_code_list() && noexcept;

public:
	//The total length of the function in bytes, divided by function_length_multiplier. 
	[[nodiscard]]
	std::uint32_t get_function_length() const noexcept;

	//Indicates that this record describes a function
	//fragment (1) or a full function (0).
	//A fragment implies that there is no prologue and
	//that all prologue processing should be ignored.
	[[nodiscard]]
	bool is_function_fragment() const noexcept
		requires (has_f_bit);

	//Has two meanings, depending on the state of E bit (single_epilog_info_packed):
	//If E is 0, it specifies the count of the total number of epilog scopes.
	//If E is 1, then this field specifies the index
	//of the first unwind code that describes the one and only epilog.
	[[nodiscard]]
	std::uint16_t get_epilog_count() const noexcept;

	//The number of 32-bit words needed to contain all of the unwind codes.
	[[nodiscard]]
	std::uint8_t get_code_words() const noexcept;

	[[nodiscard]]
	bool has_extended_main_header() const noexcept;
	
public:
	void set_epilog_count(std::uint16_t count) noexcept;

	void set_code_words(std::uint8_t count) noexcept;

	void set_function_length(std::uint32_t length);

	void set_is_function_fragment(bool is_fragment) noexcept
		requires (has_f_bit);

private:
	[[nodiscard]]
	std::uint8_t get_base_epilog_count() const noexcept;

	[[nodiscard]]
	std::uint8_t get_base_code_words() const noexcept;

private:
	epilog_info_list_type epilog_info_list_;
	unwind_code_list_type unwind_code_list_;
};

template<template<typename...> typename RuntimeFunctionBase, typename... Bases>
class [[nodiscard]] exception_directory_base
	: public Bases...
{
public:
	using runtime_function_list_type = std::vector<RuntimeFunctionBase<Bases...>>;

public:
	[[nodiscard]]
	runtime_function_list_type& get_runtime_function_list() & noexcept;
	[[nodiscard]]
	const runtime_function_list_type& get_runtime_function_list() const& noexcept;
	[[nodiscard]]
	runtime_function_list_type get_runtime_function_list() && noexcept;

private:
	runtime_function_list_type runtime_function_list_;
};

} //namespace namespace pe_bliss::exceptions::arm_common

#include "pe_bliss2/exceptions/arm_common/arm_common_unwind_info-inl.h"
