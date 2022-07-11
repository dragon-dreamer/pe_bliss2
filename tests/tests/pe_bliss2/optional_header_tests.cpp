#include "gtest/gtest.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

#include "buffers/input_memory_buffer.h"
#include "buffers/output_memory_buffer.h"

#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/core/optional_header_errc.h"
#include "pe_bliss2/core/optional_header_validator.h"
#include "pe_bliss2/detail/image_data_directory.h"
#include "pe_bliss2/detail/image_optional_header.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/section/section_header.h"
#include "pe_bliss2/pe_error.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::core;

namespace
{
template<typename T>
class OptionalHeaderTests : public testing::Test
{
public:
	using type = T;
	static constexpr auto is_32bit = std::is_same_v<type,
		optional_header::optional_header_32_type>;
};

using tested_types = ::testing::Types<optional_header::optional_header_32_type,
	optional_header::optional_header_64_type>;
} //namespace

TYPED_TEST_SUITE(OptionalHeaderTests, tested_types);

TYPED_TEST(OptionalHeaderTests, EmptyTest)
{
	using type = typename TestFixture::type;

	optional_header header;
	header.initialize_with<type>();

	EXPECT_EQ(header.get_number_of_rva_and_sizes(), 0u);
	EXPECT_EQ(header.get_size_of_structure(),
		type::packed_size + sizeof(optional_header::magic_type));
	EXPECT_EQ(header.get_magic(), (std::is_same_v<type, optional_header::optional_header_32_type>
		? optional_header::magic::pe32 : optional_header::magic::pe64));
	EXPECT_EQ(header.get_subsystem(), optional_header::subsystem::unknown);
	EXPECT_FALSE(header.is_windows_console());
	EXPECT_FALSE(header.is_windows_gui());
	EXPECT_EQ(header.get_dll_characteristics(), 0u);

	error_list errors;
	EXPECT_FALSE(validate(header, {}, false, errors));
	errors.clear_errors();
	EXPECT_FALSE(validate(header, {}, true, errors));

	EXPECT_FALSE(header.is_low_alignment());
}

TYPED_TEST(OptionalHeaderTests, NumberOfRvaAndSizesTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();
	header.set_raw_number_of_rva_and_sizes(3u);
	EXPECT_EQ(header.get_raw_number_of_rva_and_sizes(), 3u);
	EXPECT_EQ(header.get_number_of_rva_and_sizes(), 3u);

	header.set_raw_number_of_rva_and_sizes(
		optional_header::max_number_of_rva_and_sizes + 5u);
	EXPECT_EQ(header.get_raw_number_of_rva_and_sizes(),
		optional_header::max_number_of_rva_and_sizes + 5u);
	EXPECT_EQ(header.get_number_of_rva_and_sizes(),
		optional_header::max_number_of_rva_and_sizes);
}

TYPED_TEST(OptionalHeaderTests, SubsystemTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	header.set_raw_subsystem(
		static_cast<std::uint16_t>(optional_header::subsystem::windows_cui));
	EXPECT_EQ(header.get_raw_subsystem(),
		static_cast<std::uint16_t>(optional_header::subsystem::windows_cui));
	EXPECT_EQ(header.get_subsystem(),
		optional_header::subsystem::windows_cui);
	EXPECT_TRUE(header.is_windows_console());
	EXPECT_FALSE(header.is_windows_gui());

	header.set_raw_subsystem(
		static_cast<std::uint16_t>(optional_header::subsystem::windows_gui));
	EXPECT_TRUE(header.is_windows_gui());
	EXPECT_FALSE(header.is_windows_console());
}

TYPED_TEST(OptionalHeaderTests, DllCharacteristicsTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	header.set_raw_dll_characteristics(
		optional_header::dll_characteristics::appcontainer
		| optional_header::dll_characteristics::dynamic_base);
	EXPECT_EQ(header.get_raw_dll_characteristics(),
		optional_header::dll_characteristics::appcontainer
		| optional_header::dll_characteristics::dynamic_base);
	EXPECT_EQ(header.get_dll_characteristics(),
		optional_header::dll_characteristics::appcontainer
		| optional_header::dll_characteristics::dynamic_base);
}

TYPED_TEST(OptionalHeaderTests, AccessTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	header.set_raw_subsystem(
		static_cast<std::uint16_t>(optional_header::subsystem::windows_cui));

	EXPECT_EQ((header.access([](const auto& obj) {
		return obj.subsystem;
	})), static_cast<std::uint16_t>(optional_header::subsystem::windows_cui));
}

TYPED_TEST(OptionalHeaderTests, LowAlignmentTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();
	header.set_raw_file_alignment(1);
	EXPECT_FALSE(header.is_low_alignment());
	header.set_raw_section_alignment(1);
	EXPECT_TRUE(header.is_low_alignment());
}

namespace
{

template<auto Getter, auto Setter, auto Lambda>
struct getter_setter
{
	static void test(optional_header& header)
	{
		static std::uint32_t value = 1u;
		EXPECT_EQ((header.*Getter)(), 0u);
		EXPECT_EQ(header.access(Lambda), 0u);
		(header.*Setter)(value);
		EXPECT_EQ((header.*Getter)(), value);
		EXPECT_EQ(header.access(Lambda), value);
		++value;
	}
};

} //namespace

TYPED_TEST(OptionalHeaderTests, SettersGettersTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	getter_setter<&optional_header::get_raw_address_of_entry_point,
		&optional_header::set_raw_address_of_entry_point,
		[](auto& obj) { return obj.address_of_entry_point; }>::test(header);
	getter_setter<&optional_header::get_raw_major_linker_version,
		&optional_header::set_raw_major_linker_version,
		[](auto& obj) { return obj.major_linker_version; }>::test(header);
	getter_setter<&optional_header::get_raw_minor_linker_version,
		&optional_header::set_raw_minor_linker_version,
		[](auto& obj) { return obj.minor_linker_version; }>::test(header);
	getter_setter<&optional_header::get_raw_size_of_code,
		&optional_header::set_raw_size_of_code,
		[](auto& obj) { return obj.size_of_code; }>::test(header);
	getter_setter<&optional_header::get_raw_size_of_initialized_data,
		&optional_header::set_raw_size_of_initialized_data,
		[](auto& obj) { return obj.size_of_initialized_data; }>::test(header);
	getter_setter<&optional_header::get_raw_size_of_uninitialized_data,
		&optional_header::set_raw_size_of_uninitialized_data,
		[](auto& obj) { return obj.size_of_uninitialized_data; }>::test(header);
	getter_setter<&optional_header::get_raw_base_of_code,
		&optional_header::set_raw_base_of_code,
		[](auto& obj) { return obj.base_of_code; }>::test(header);
	
	if constexpr (TestFixture::is_32bit)
	{
		getter_setter<&optional_header::get_raw_base_of_data,
			&optional_header::set_raw_base_of_data,
			[](auto& obj) {
				if constexpr (std::is_same_v<std::remove_cvref_t<decltype(obj)>,
					detail::image_optional_header_32>)
				{
					return obj.base_of_data;
				}
				else
				{
					return 0u;
				}
		}>::test(header);
	}
	else
	{
		expect_throw_pe_error([&header] {
			(void)header.get_raw_base_of_data();
		}, optional_header_errc::no_base_of_data_field);
		expect_throw_pe_error([&header] {
			header.set_raw_base_of_data(123u);
		}, optional_header_errc::no_base_of_data_field);
	}
	
	getter_setter<&optional_header::get_raw_image_base,
		&optional_header::set_raw_image_base,
		[](auto& obj) { return static_cast<std::uint64_t>(obj.image_base); }>::test(header);
	getter_setter<&optional_header::get_raw_section_alignment,
		&optional_header::set_raw_section_alignment,
		[](auto& obj) { return obj.section_alignment; }>::test(header);
	getter_setter<&optional_header::get_raw_file_alignment,
		&optional_header::set_raw_file_alignment,
		[](auto& obj) { return obj.file_alignment; }>::test(header);
	getter_setter<&optional_header::get_raw_major_operating_system_version,
		&optional_header::set_raw_major_operating_system_version,
		[](auto& obj) { return obj.major_operating_system_version; }>::test(header);
	getter_setter<&optional_header::get_raw_minor_operating_system_version,
		&optional_header::set_raw_minor_operating_system_version,
		[](auto& obj) { return obj.minor_operating_system_version; }>::test(header);
	getter_setter<&optional_header::get_raw_major_image_version,
		&optional_header::set_raw_major_image_version,
		[](auto& obj) { return obj.major_image_version; }>::test(header);
	getter_setter<&optional_header::get_raw_minor_image_version,
		&optional_header::set_raw_minor_image_version,
		[](auto& obj) { return obj.minor_image_version; }>::test(header);
	getter_setter<&optional_header::get_raw_major_subsystem_version,
		&optional_header::set_raw_major_subsystem_version,
		[](auto& obj) { return obj.major_subsystem_version; }>::test(header);
	getter_setter<&optional_header::get_raw_minor_subsystem_version,
		&optional_header::set_raw_minor_subsystem_version,
		[](auto& obj) { return obj.minor_subsystem_version; }>::test(header);
	getter_setter<&optional_header::get_raw_win32_version_value,
		&optional_header::set_raw_win32_version_value,
		[](auto& obj) { return obj.win32_version_value; }>::test(header);
	getter_setter<&optional_header::get_raw_size_of_image,
		&optional_header::set_raw_size_of_image,
		[](auto& obj) { return obj.size_of_image; }>::test(header);
	getter_setter<&optional_header::get_raw_size_of_headers,
		&optional_header::set_raw_size_of_headers,
		[](auto& obj) { return obj.size_of_headers; }>::test(header);
	getter_setter<&optional_header::get_raw_checksum,
		&optional_header::set_raw_checksum,
		[](auto& obj) { return obj.checksum; }>::test(header);
	getter_setter<&optional_header::get_raw_subsystem,
		&optional_header::set_raw_subsystem,
		[](auto& obj) { return obj.subsystem; }>::test(header);
	getter_setter<&optional_header::get_raw_dll_characteristics,
		&optional_header::set_raw_dll_characteristics,
		[](auto& obj) { return obj.dll_characteristics; }>::test(header);
	getter_setter<&optional_header::get_raw_size_of_stack_reserve,
		&optional_header::set_raw_size_of_stack_reserve,
		[](auto& obj) { return static_cast<std::uint64_t>(
			obj.size_of_stack_reserve); }>::test(header);
	getter_setter<&optional_header::get_raw_size_of_stack_commit,
		&optional_header::set_raw_size_of_stack_commit,
		[](auto& obj) { return static_cast<std::uint64_t>(
			obj.size_of_stack_commit); }>::test(header);
	getter_setter<&optional_header::get_raw_size_of_heap_reserve,
		&optional_header::set_raw_size_of_heap_reserve,
		[](auto& obj) { return static_cast<std::uint64_t>(
			obj.size_of_heap_reserve); }>::test(header);
	getter_setter<&optional_header::get_raw_size_of_heap_commit,
		&optional_header::set_raw_size_of_heap_commit,
		[](auto& obj) { return static_cast<std::uint64_t>(
			obj.size_of_heap_commit); }>::test(header);
	getter_setter<&optional_header::get_raw_loader_flags,
		&optional_header::set_raw_loader_flags,
		[](auto& obj) { return obj.loader_flags; }>::test(header);
	getter_setter<&optional_header::get_raw_number_of_rva_and_sizes,
		&optional_header::set_raw_number_of_rva_and_sizes,
		[](auto& obj) { return obj.number_of_rva_and_sizes; }>::test(header);
}

TYPED_TEST(OptionalHeaderTests, SerializeDeserializeTest)
{
	char header_data[] = "\x0b\x02\x12\x34\xaa\xbb\xcc\xdd";
	if constexpr (TestFixture::is_32bit)
		header_data[1] = '\x01';

	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(header_data),
		std::size(header_data) - 1);

	{
		optional_header header;
		expect_throw_pe_error([&header, &buf] { header.deserialize(buf, false); },
			optional_header_errc::unable_to_read_optional_header);
	}
	{
		buf.set_rpos(1);
		optional_header header;
		expect_throw_pe_error([&header, &buf] { header.deserialize(buf, false); },
			optional_header_errc::invalid_pe_magic);
	}
	{
		buf.set_rpos(0);
		optional_header header;
		ASSERT_NO_THROW(header.deserialize(buf, true));
		EXPECT_EQ(header.get_magic(), TestFixture::is_32bit
			? optional_header::magic::pe32 : optional_header::magic::pe64);
		EXPECT_EQ(header.get_raw_major_linker_version(), 0x12u);
		EXPECT_EQ(header.get_raw_minor_linker_version(), 0x34u);

		std::vector<std::byte> outdata;
		buffers::output_memory_buffer outbuf(outdata);
		ASSERT_NO_THROW(header.serialize(outbuf, false));
		ASSERT_EQ(outdata.size(), buf.size());
		EXPECT_TRUE(std::equal(outdata.cbegin(), outdata.cend(),
			reinterpret_cast<const std::byte*>(header_data)));
	}
}

TYPED_TEST(OptionalHeaderTests, ValidateAddressOfEntryPointTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	EXPECT_EQ(validate_address_of_entry_point(header, false),
		optional_header_errc::invalid_address_of_entry_point);
	EXPECT_NO_THROW(validate_address_of_entry_point(header, true)
		.throw_on_error());

	header.set_raw_address_of_entry_point(1u);
	EXPECT_NO_THROW(validate_address_of_entry_point(header, false)
		.throw_on_error());
	EXPECT_NO_THROW(validate_address_of_entry_point(header, true)
		.throw_on_error());

	header.set_raw_size_of_headers(100u);
	EXPECT_EQ(validate_address_of_entry_point(header, false),
		optional_header_errc::invalid_address_of_entry_point);
	EXPECT_EQ(validate_address_of_entry_point(header, true),
		optional_header_errc::invalid_address_of_entry_point);
}

TYPED_TEST(OptionalHeaderTests, ValidateImageBaseTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	EXPECT_NO_THROW(validate_image_base(header, true).throw_on_error());
	EXPECT_NO_THROW(validate_image_base(header, false).throw_on_error());

	header.set_raw_image_base(0x123u);
	EXPECT_EQ(validate_image_base(header, true),
		optional_header_errc::unaligned_image_base);
	EXPECT_EQ(validate_image_base(header, false),
		optional_header_errc::unaligned_image_base);

	header.set_raw_image_base(0x40000u);
	EXPECT_NO_THROW(validate_image_base(header, true).throw_on_error());
	EXPECT_NO_THROW(validate_image_base(header, false).throw_on_error());

	if (header.get_magic() == optional_header::magic::pe32)
		header.set_raw_image_base(0xf0000000u);
	else
		header.set_raw_image_base(0xf000'00000000ull);

	EXPECT_NO_THROW(validate_image_base(header, true).throw_on_error());
	EXPECT_EQ(validate_image_base(header, false),
		optional_header_errc::too_large_image_base);

	header.set_raw_size_of_image(0x10000u);
	if (header.get_magic() == optional_header::magic::pe32)
		header.set_raw_image_base(0xfffff0000u);
	else
		header.set_raw_image_base(0xffffffff'ffff0000ull);
	EXPECT_EQ(validate_image_base(header, true),
		optional_header_errc::too_large_image_base);
}

TYPED_TEST(OptionalHeaderTests, ValidateFileAlignmentTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	EXPECT_EQ(validate_file_alignment(header),
		optional_header_errc::incorrect_file_alignment);

	header.set_raw_file_alignment(0x8000u);
	header.set_raw_section_alignment(0x10000u);
	EXPECT_NO_THROW(validate_file_alignment(header).throw_on_error());

	header.set_raw_section_alignment(0x4000u);
	EXPECT_EQ(validate_file_alignment(header),
		optional_header_errc::file_alignment_out_of_range);

	header.set_raw_file_alignment(256u);
	EXPECT_EQ(validate_file_alignment(header),
		optional_header_errc::file_alignment_out_of_range);

	header.set_raw_file_alignment(1u);
	header.set_raw_section_alignment(1u);
	EXPECT_NO_THROW(validate_file_alignment(header).throw_on_error());

	header.set_raw_file_alignment(5u);
	EXPECT_EQ(validate_file_alignment(header),
		optional_header_errc::incorrect_file_alignment);
}

TYPED_TEST(OptionalHeaderTests, ValidateSectionAlignmentTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	EXPECT_EQ(validate_section_alignment(header),
		optional_header_errc::incorrect_section_alignment);

	header.set_raw_file_alignment(0x8000u);
	header.set_raw_section_alignment(0x10000u);
	EXPECT_NO_THROW(validate_section_alignment(header).throw_on_error());

	header.set_raw_section_alignment(0x4000u);
	EXPECT_EQ(validate_section_alignment(header),
		optional_header_errc::section_alignment_out_of_range);

	header.set_raw_section_alignment(256u);
	EXPECT_EQ(validate_section_alignment(header),
		optional_header_errc::section_alignment_out_of_range);

	header.set_raw_file_alignment(1u);
	header.set_raw_section_alignment(1u);
	EXPECT_NO_THROW(validate_section_alignment(header).throw_on_error());

	header.set_raw_section_alignment(5u);
	EXPECT_EQ(validate_section_alignment(header),
		optional_header_errc::incorrect_section_alignment);
}

TYPED_TEST(OptionalHeaderTests, ValidateSubsystemVersionTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	EXPECT_EQ(validate_subsystem_version(header),
		optional_header_errc::too_low_subsystem_version);

	header.set_raw_minor_subsystem_version(
		optional_header::min_minor_subsystem_version);
	header.set_raw_major_subsystem_version(
		optional_header::min_major_subsystem_version);
	EXPECT_NO_THROW(validate_subsystem_version(header).throw_on_error());

	header.set_raw_minor_subsystem_version(
		optional_header::min_minor_subsystem_version - 1u);
	EXPECT_EQ(validate_subsystem_version(header),
		optional_header_errc::too_low_subsystem_version);
}

TYPED_TEST(OptionalHeaderTests, ValidateSizeOfHeapTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	EXPECT_NO_THROW(validate_size_of_heap(header).throw_on_error());

	header.set_raw_size_of_heap_commit(10u);
	EXPECT_EQ(validate_size_of_heap(header),
		optional_header_errc::invalid_size_of_heap);

	header.set_raw_size_of_heap_reserve(10u);
	EXPECT_NO_THROW(validate_size_of_heap(header).throw_on_error());
}

TYPED_TEST(OptionalHeaderTests, ValidateSizeOfStackTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	EXPECT_NO_THROW(validate_size_of_stack(header).throw_on_error());

	header.set_raw_size_of_stack_commit(10u);
	EXPECT_EQ(validate_size_of_stack(header),
		optional_header_errc::invalid_size_of_stack);

	header.set_raw_size_of_stack_reserve(10u);
	EXPECT_NO_THROW(validate_size_of_stack(header).throw_on_error());
}

TYPED_TEST(OptionalHeaderTests, ValidateSizeOfHeadersTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	EXPECT_NO_THROW(validate_size_of_headers(header).throw_on_error());

	header.set_raw_size_of_headers(0x10000u);
	EXPECT_EQ(validate_size_of_headers(header),
		optional_header_errc::invalid_size_of_headers);

	header.set_raw_section_alignment(0x20000u);
	EXPECT_EQ(validate_size_of_headers(header),
		optional_header_errc::invalid_size_of_headers);

	header.set_raw_size_of_image(0x10000u);
	EXPECT_NO_THROW(validate_size_of_headers(header).throw_on_error());
}

TYPED_TEST(OptionalHeaderTests, ValidateTest)
{
	optional_header header;
	header.initialize_with<typename TestFixture::type>();

	header.set_raw_address_of_entry_point(1u);
	header.set_raw_file_alignment(0x8000u);
	header.set_raw_section_alignment(0x10000u);
	header.set_raw_minor_subsystem_version(
		optional_header::min_minor_subsystem_version);
	header.set_raw_major_subsystem_version(
		optional_header::min_major_subsystem_version);
	header.set_raw_image_base(0x123u);

	header.set_raw_address_of_entry_point(0x1000u);
	header.set_raw_size_of_headers(0x1000u);
	header.set_raw_size_of_image(0x1000u);

	error_list errors;
	EXPECT_TRUE(validate(header, {}, false, errors));
	EXPECT_FALSE(errors.has_errors());
	EXPECT_TRUE(validate(header, {}, true, errors));
	EXPECT_FALSE(errors.has_errors());

	header.set_raw_address_of_entry_point(1u);
	EXPECT_FALSE(validate(header, {}, false, errors));
	expect_contains_errors(errors,
		optional_header_errc::invalid_address_of_entry_point);
	errors.clear_errors();
	EXPECT_FALSE(validate(header, {}, true, errors));
	expect_contains_errors(errors,
		optional_header_errc::invalid_address_of_entry_point);
	errors.clear_errors();
	EXPECT_TRUE(validate(header, { .validate_address_of_entry_point = false },
		false, errors));
	EXPECT_FALSE(errors.has_errors());
	EXPECT_TRUE(validate(header, { .validate_address_of_entry_point = false },
		true, errors));
	EXPECT_FALSE(errors.has_errors());
	header.set_raw_address_of_entry_point(0x1000u);

	header.set_raw_file_alignment(0x8001u);
	EXPECT_FALSE(validate(header, {}, false, errors));
	expect_contains_errors(errors,
		optional_header_errc::incorrect_file_alignment);
	errors.clear_errors();
	EXPECT_FALSE(validate(header, {}, true, errors));
	expect_contains_errors(errors,
		optional_header_errc::incorrect_file_alignment);
	errors.clear_errors();
	EXPECT_TRUE(validate(header, { .validate_alignments = false },
		false, errors));
	EXPECT_TRUE(validate(header, { .validate_alignments = false },
		true, errors));
	header.set_raw_file_alignment(0x8000u);

	header.set_raw_section_alignment(0x10001u);
	EXPECT_FALSE(validate(header, {}, false, errors));
	expect_contains_errors(errors,
		optional_header_errc::incorrect_section_alignment);
	errors.clear_errors();
	EXPECT_FALSE(validate(header, {}, true, errors));
	expect_contains_errors(errors,
		optional_header_errc::incorrect_section_alignment);
	errors.clear_errors();
	EXPECT_TRUE(validate(header, { .validate_alignments = false },
		false, errors));
	EXPECT_TRUE(validate(header, { .validate_alignments = false },
		true, errors));
	header.set_raw_section_alignment(0x10000u);

	header.set_raw_minor_subsystem_version(
		optional_header::min_minor_subsystem_version - 1u);
	EXPECT_FALSE(validate(header, {}, false, errors));
	expect_contains_errors(errors,
		optional_header_errc::too_low_subsystem_version);
	errors.clear_errors();
	EXPECT_FALSE(validate(header, {}, true, errors));
	expect_contains_errors(errors,
		optional_header_errc::too_low_subsystem_version);
	errors.clear_errors();
	EXPECT_TRUE(validate(header, { .validate_subsystem_version = false },
		false, errors));
	EXPECT_TRUE(validate(header, { .validate_subsystem_version = false },
		true, errors));
	header.set_raw_minor_subsystem_version(
		optional_header::min_minor_subsystem_version);

	header.set_raw_size_of_heap_commit(1u);
	EXPECT_FALSE(validate(header, {}, false, errors));
	expect_contains_errors(errors,
		optional_header_errc::invalid_size_of_heap);
	errors.clear_errors();
	EXPECT_FALSE(validate(header, {}, true, errors));
	expect_contains_errors(errors,
		optional_header_errc::invalid_size_of_heap);
	errors.clear_errors();
	EXPECT_TRUE(validate(header, { .validate_size_of_heap = false },
		false, errors));
	EXPECT_TRUE(validate(header, { .validate_size_of_heap = false },
		true, errors));
	header.set_raw_size_of_heap_commit(0u);

	header.set_raw_size_of_stack_commit(1u);
	EXPECT_FALSE(validate(header, {}, false, errors));
	expect_contains_errors(errors,
		optional_header_errc::invalid_size_of_stack);
	errors.clear_errors();
	EXPECT_FALSE(validate(header, {}, true, errors));
	expect_contains_errors(errors,
		optional_header_errc::invalid_size_of_stack);
	errors.clear_errors();
	EXPECT_TRUE(validate(header, { .validate_size_of_stack = false },
		false, errors));
	EXPECT_TRUE(validate(header, { .validate_size_of_stack = false },
		true, errors));
	header.set_raw_size_of_stack_commit(0u);

	header.set_raw_size_of_image(0u);
	EXPECT_FALSE(validate(header, {}, false, errors));
	expect_contains_errors(errors,
		optional_header_errc::invalid_size_of_headers);
	errors.clear_errors();
	EXPECT_FALSE(validate(header, {}, true, errors));
	expect_contains_errors(errors,
		optional_header_errc::invalid_size_of_headers);
	errors.clear_errors();
	EXPECT_TRUE(validate(header, { .validate_size_of_headers = false },
		false, errors));
	EXPECT_TRUE(validate(header, { .validate_size_of_headers = false },
		true, errors));
}

TEST(OptionalHeaderTests, ValidateSizeOfOptionalHeaderTest)
{
	optional_header header;

	EXPECT_EQ(validate_size_of_optional_header(0u, header),
		optional_header_errc::invalid_size_of_optional_header);

	EXPECT_EQ(validate_size_of_optional_header(header.get_size_of_structure() - 1,
		header), optional_header_errc::invalid_size_of_optional_header);

	EXPECT_NO_THROW(validate_size_of_optional_header(
		header.get_size_of_structure(), header).throw_on_error());

	header.set_raw_number_of_rva_and_sizes(3u);
	EXPECT_EQ(validate_size_of_optional_header(header.get_size_of_structure(), header),
		optional_header_errc::invalid_size_of_optional_header);

	static constexpr auto data_dir_size = detail::packed_reflection
		::get_type_size<detail::image_data_directory>();
	EXPECT_NO_THROW(validate_size_of_optional_header(
		static_cast<std::uint16_t>(header.get_size_of_structure()
			+ data_dir_size * 3u), header)
		.throw_on_error());
}

TEST(OptionalHeaderTests, ValidateSizeOfImageTest1)
{
	optional_header header;
	EXPECT_NO_THROW(validate_size_of_image(nullptr, header).throw_on_error());
}

TEST(OptionalHeaderTests, ValidateSizeOfImageTest2)
{
	optional_header header;
	header.set_raw_size_of_image(0x2000u);
	header.set_raw_section_alignment(0x1000u);

	section::section_header section_header;
	section_header.set_virtual_size(0xa00u);
	section_header.base_struct()->virtual_address = 0x1000u;
	EXPECT_NO_THROW(validate_size_of_image(&section_header, header).throw_on_error());

	header.set_raw_size_of_image(0x1a00u);
	EXPECT_EQ(validate_size_of_image(&section_header, header),
		optional_header_errc::invalid_size_of_image);
}
