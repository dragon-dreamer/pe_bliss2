#pragma once

namespace pe_bliss::exceptions::arm_common
{
template<std::size_t Length, std::uint8_t Matcher, std::uint8_t MatcherMask>
void unwind_code_common<Length, Matcher, MatcherMask>::init() noexcept
{
	if (!descriptor_.physical_size())
		descriptor_.set_physical_size(length);

	auto value = static_cast<std::uint8_t>(descriptor_[0]);
	value &= ~matcher_mask;
	value |= matcher;
	descriptor_[0] = static_cast<std::byte>(value);
}

template<std::size_t Length, std::uint8_t Matcher, std::uint8_t MatcherMask>
constexpr bool unwind_code_common<Length, Matcher, MatcherMask>
	::matches(std::byte first_byte) noexcept
{
	auto value = static_cast<std::uint8_t>(first_byte);
	value &= matcher_mask;
	return value == matcher;
}

template<std::size_t Length, std::uint8_t Matcher, std::uint8_t MatcherMask>
typename unwind_code_common<Length, Matcher, MatcherMask>::descriptor_type&
	unwind_code_common<Length, Matcher, MatcherMask>::get_descriptor() noexcept
{
	return descriptor_;
}

template<std::size_t Length, std::uint8_t Matcher, std::uint8_t MatcherMask>
const typename unwind_code_common<Length, Matcher, MatcherMask>::descriptor_type&
	unwind_code_common<Length, Matcher, MatcherMask>::get_descriptor() const noexcept
{
	return descriptor_;
}

template<std::size_t Length, std::uint8_t Matcher, std::uint8_t MatcherMask>
template<std::size_t FromBit, std::size_t ToBit>
auto unwind_code_common<Length, Matcher, MatcherMask>::get_value() const noexcept
{
	static_assert(FromBit <= ToBit);
	constexpr std::size_t byte_count
		= (ToBit - FromBit + CHAR_BIT) / CHAR_BIT;
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

template<std::size_t Length, std::uint8_t Matcher, std::uint8_t MatcherMask>
template<std::size_t FromBit, std::size_t ToBit, typename Value>
void unwind_code_common<Length, Matcher, MatcherMask>::set_value(Value value)
{
	static_assert(FromBit <= ToBit);
	constexpr std::size_t byte_count
		= (ToBit - FromBit + CHAR_BIT) / CHAR_BIT;
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

template<std::size_t Length, std::uint8_t Matcher, std::uint8_t MatcherMask>
template<std::size_t Multiple, std::size_t StartBit,
	std::size_t EndBit, auto ErrorCode, typename Value>
void unwind_code_common<Length, Matcher, MatcherMask>::set_scaled_value(Value value)
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
		std::throw_with_nested(pe_error(ErrorCode));
	}
}

template<std::size_t Length, std::uint8_t Matcher, std::uint8_t MatcherMask>
template<std::size_t FirstBit, std::size_t ValueLength>
consteval auto unwind_code_common<Length, Matcher, MatcherMask>
	::create_first_bit_mask() noexcept
{
	return (1ull << (ValueLength * CHAR_BIT - FirstBit)) - 1ull;
}

template<std::size_t Length, std::uint8_t Matcher, std::uint8_t MatcherMask>
template<std::size_t LastBit, std::size_t ValueLength>
consteval auto unwind_code_common<Length,
	Matcher, MatcherMask>::create_last_bit_mask() noexcept
{
	return ~create_first_bit_mask<LastBit + 1u, ValueLength>();
}

template<std::size_t Length, std::uint8_t Matcher, std::uint8_t MatcherMask>
template<std::size_t FirstBit, std::size_t LastBit, std::size_t ValueLength>
consteval auto unwind_code_common<Length,
	Matcher, MatcherMask>::create_bit_mask() noexcept
{
	return create_first_bit_mask<FirstBit, ValueLength>()
		& create_last_bit_mask<LastBit, ValueLength>();
}

template<typename RuntimeFunctionEntry,
	typename PackedUnwindData, typename ExtendedUnwindRecord,
	typename... Bases>
typename runtime_function_base<RuntimeFunctionEntry, PackedUnwindData,
	ExtendedUnwindRecord, Bases...>::unwind_info_type& runtime_function_base<RuntimeFunctionEntry,
	PackedUnwindData, ExtendedUnwindRecord, Bases...>::get_unwind_info() noexcept
{
	return unwind_info_;
}

template<typename RuntimeFunctionEntry,
	typename PackedUnwindData, typename ExtendedUnwindRecord,
	typename... Bases>
const typename runtime_function_base<RuntimeFunctionEntry, PackedUnwindData,
	ExtendedUnwindRecord, Bases...>::unwind_info_type& runtime_function_base<RuntimeFunctionEntry,
	PackedUnwindData, ExtendedUnwindRecord, Bases...>::get_unwind_info() const noexcept
{
	return unwind_info_;
}

template<typename RuntimeFunctionEntry,
	typename PackedUnwindData, typename ExtendedUnwindRecord,
	typename... Bases>
bool runtime_function_base<RuntimeFunctionEntry,
	PackedUnwindData, ExtendedUnwindRecord, Bases...>
	::has_extended_unwind_record() const noexcept
{
	//Flag is a 2-bit field that indicates how to interpret the remaining
	//30 bits of the second .pdata word. If Flag is 0, then the remaining bits
	//form an Exception Information RVA (with the two lowest bits implicitly 0).
	//If Flag is non-zero, then the remaining bits form a Packed Unwind Data structure.
	return !(this->descriptor_->unwind_data & 0b11u);
}

inline extended_unwind_record_base::main_header_type&
	extended_unwind_record_base::get_main_header() noexcept
{
	return main_header_;
}

inline const extended_unwind_record_base::main_header_type&
	extended_unwind_record_base::get_main_header() const noexcept
{
	return main_header_;
}

inline extended_unwind_record_base::main_header_type&
	extended_unwind_record_base::get_main_extended_header() noexcept
{
	return main_extended_header_;
}

inline const extended_unwind_record_base::main_header_type&
	extended_unwind_record_base::get_main_extended_header() const noexcept
{
	return main_extended_header_;
}

inline extended_unwind_record_base::exception_handler_rva_type&
	extended_unwind_record_base::get_exception_handler_rva() noexcept
{
	return exception_handler_rva_;
}

inline const extended_unwind_record_base::exception_handler_rva_type&
	extended_unwind_record_base::get_exception_handler_rva() const noexcept
{
	return exception_handler_rva_;
}

template<typename EpilogInfo, typename UnwindRecordOptions>
typename extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::epilog_info_list_type& extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::get_epilog_info_list() & noexcept
{
	return epilog_info_list_;
}

template<typename EpilogInfo, typename UnwindRecordOptions>
const typename extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::epilog_info_list_type& extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::get_epilog_info_list() const& noexcept
{
	return epilog_info_list_;
}

template<typename EpilogInfo, typename UnwindRecordOptions>
typename extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::epilog_info_list_type extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::get_epilog_info_list() && noexcept
{
	return std::move(epilog_info_list_);
}

template<typename EpilogInfo, typename UnwindRecordOptions>
typename extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::unwind_code_list_type& extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::get_unwind_code_list() & noexcept
{
	return unwind_code_list_;
}

template<typename EpilogInfo, typename UnwindRecordOptions>
const typename extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::unwind_code_list_type& extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::get_unwind_code_list() const& noexcept
{
	return unwind_code_list_;
}

template<typename EpilogInfo, typename UnwindRecordOptions>
typename extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::unwind_code_list_type extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::get_unwind_code_list() && noexcept
{
	return std::move(unwind_code_list_);
}

template<typename EpilogInfo, typename UnwindRecordOptions>
std::uint32_t extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::get_function_length() const noexcept
{
	return (main_header_.get() & 0x3ffffu) * function_length_multiplier;
}

template<typename EpilogInfo, typename UnwindRecordOptions>
void extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::set_function_length(std::uint32_t length)
{
	if (length % function_length_multiplier)
		throw pe_error(exception_directory_errc::invalid_function_length);

	length /= function_length_multiplier;
	if (length > 0x3ffffu)
		throw pe_error(exception_directory_errc::invalid_function_length);

	main_header_.get() &= ~0x3ffffu;
	main_header_.get() |= length;
}

template<typename EpilogInfo, typename UnwindRecordOptions>
std::uint16_t extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::get_epilog_count() const noexcept
{
	return has_extended_main_header()
		? get_extended_epilog_count() : get_base_epilog_count();
}

template<typename EpilogInfo, typename UnwindRecordOptions>
std::uint8_t extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::get_code_words() const noexcept
{
	return has_extended_main_header()
		? get_extended_code_words() : get_base_code_words();
}

template<typename EpilogInfo, typename UnwindRecordOptions>
bool extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::has_extended_main_header() const noexcept
{
	return !get_base_epilog_count() && !get_base_code_words();
}

template<typename EpilogInfo, typename UnwindRecordOptions>
void extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::set_epilog_count(std::uint16_t count) noexcept
{
	auto code_words = get_code_words();

	main_extended_header_.get() &= ~0xffffu;
	main_header_.get() &= ~base_epilog_count_mask;

	if (count > (base_epilog_count_mask >> base_epilog_count_shift))
	{
		//Put epilog count to extended header
		main_extended_header_.get() |= count;

		//And move code words count to extended header
		main_header_.get() &= ~base_code_words_mask;
		main_extended_header_.get() &= ~0xff0000u;
		main_extended_header_.get() |= code_words << 16u;
	}
	else
	{
		if (code_words <= (base_code_words_mask >> base_code_words_shift))
		{
			//Both epilog count and code words can be put to base header
			main_header_.get() &= ~base_code_words_mask;
			main_header_.get() |= static_cast<std::uint32_t>(code_words)
				<< base_code_words_shift;
			main_header_.get() |= static_cast<std::uint32_t>(count)
				<< base_epilog_count_shift;
		}
		else
		{
			main_extended_header_.get() |= count;
		}
	}
}

template<typename EpilogInfo, typename UnwindRecordOptions>
void extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::set_code_words(std::uint8_t count) noexcept
{
	auto epilog_count = get_epilog_count();

	main_extended_header_.get() &= ~0xff0000u;
	main_header_.get() &= ~base_code_words_mask;

	if (count > (base_code_words_mask >> base_code_words_shift))
	{
		//Put code words to extended header
		main_extended_header_.get()
			|= static_cast<std::uint32_t>(count) << 16u;

		//And move epilog count to extended header
		main_header_.get() &= ~base_epilog_count_mask;
		main_extended_header_.get() &= ~0xffffu;
		main_extended_header_.get() |= epilog_count;
	}
	else
	{
		if (epilog_count <= (base_epilog_count_mask >> base_epilog_count_shift))
		{
			//Both epilog count and code words can be put to base header
			main_header_.get() &= ~base_epilog_count_mask;
			main_header_.get() |= static_cast<std::uint32_t>(count)
				<< base_code_words_shift;
			main_header_.get() |= static_cast<std::uint32_t>(epilog_count)
				<< base_epilog_count_shift;
		}
		else
		{
			main_extended_header_.get()
				|= static_cast<std::uint32_t>(count) << 16u;
		}
	}
}

template<typename EpilogInfo, typename UnwindRecordOptions>
	std::uint8_t extended_unwind_record<EpilogInfo,
		UnwindRecordOptions>::get_base_epilog_count() const noexcept
{
	return static_cast<std::uint8_t>(
		(main_header_.get() & base_epilog_count_mask)
		>> base_epilog_count_shift);
}

template<typename EpilogInfo, typename UnwindRecordOptions>
std::uint8_t extended_unwind_record<EpilogInfo,
	UnwindRecordOptions>::get_base_code_words() const noexcept
{
	return static_cast<std::uint8_t>(
		(main_header_.get() & base_code_words_mask)
		>> base_code_words_shift);
}

template<template<typename...> typename RuntimeFunctionBase, typename... Bases>
typename exception_directory_base<RuntimeFunctionBase,
	Bases...>::runtime_function_list_type& exception_directory_base<RuntimeFunctionBase,
	Bases...>::get_runtime_function_list() & noexcept
{
	return runtime_function_list_;
}

template<template<typename...> typename RuntimeFunctionBase, typename... Bases>
const typename exception_directory_base<RuntimeFunctionBase,
	Bases...>::runtime_function_list_type& exception_directory_base<RuntimeFunctionBase,
	Bases...>::get_runtime_function_list() const& noexcept
{
	return runtime_function_list_;
}

template<template<typename...> typename RuntimeFunctionBase, typename... Bases>
typename exception_directory_base<RuntimeFunctionBase,
	Bases...>::runtime_function_list_type exception_directory_base<RuntimeFunctionBase,
	Bases...>::get_runtime_function_list() && noexcept
{
	return std::move(runtime_function_list_);
}

namespace
{
template<typename UnwindCode, typename Vector>
bool try_create_unwind_code(
	std::byte first_byte, Vector& code_list)
{
	if (UnwindCode::matches(first_byte))
	{
		code_list.emplace_back(std::in_place_type<UnwindCode>);
		return true;
	}
	return false;
}
} //namespace

template<typename... UnwindCodes>
template<typename Vector>
void create_unwind_code_helper<std::variant<UnwindCodes...>>::create_unwind_code(
	std::byte first_byte, Vector& code_list)
{
	bool found = (... || try_create_unwind_code<UnwindCodes>(first_byte, code_list));
	if (!found)
		throw pe_error(exception_directory_errc::unsupported_unwind_code);
}

template<typename Vector>
void create_unwind_code(
	std::byte first_byte, Vector& code_list)
{
	create_unwind_code_helper<typename Vector::value_type>
		::create_unwind_code(first_byte, code_list);
}

} //namespace namespace pe_bliss::exceptions::arm_common
