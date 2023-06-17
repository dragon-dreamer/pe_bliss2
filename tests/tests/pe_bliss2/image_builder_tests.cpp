#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "buffers/output_memory_buffer.h"

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/image_signature.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_builder.h"
#include "pe_bliss2/section/section_header.h"

#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{
class ImageBuilderTestsFixture : public ::testing::TestWithParam<bool>
{
public:
	ImageBuilderTestsFixture()
	{
		if (GetParam())
		{
			buffer_offset = 0x20;
			buf.set_wpos(buffer_offset);
		}

		auto& dos_header = instance.get_dos_header().get_descriptor();
		dos_header->e_magic = dos::dos_header::mz_magic_value;

		dos_header->e_lfanew = dos_header.packed_size + dos_stub_size;

		instance.get_dos_stub().copied_data().resize(dos_stub_size);
		instance.get_dos_stub().copied_data()[0] = dos_stub_first_byte;

		instance.get_image_signature().get_descriptor()
			= core::image_signature::pe_signature;
		instance.get_file_header().set_machine_type(
			core::file_header::machine_type::i386);
		instance.get_file_header().get_descriptor()->size_of_optional_header
			= static_cast<std::uint16_t>(instance.get_optional_header().get_size_of_structure()
				+ number_of_data_directories * core::data_directories::directory_packed_size);

		instance.get_optional_header().set_raw_number_of_rva_and_sizes(
			number_of_data_directories);

		instance.get_data_directories().set_size(number_of_data_directories);
		instance.get_data_directories().get_directories()[0]->virtual_address
			= static_cast<std::uint32_t>(first_dir_rva_byte);

		full_headers_length = dos::dos_header::descriptor_type::packed_size
			+ core::image_signature::descriptor_type::packed_size
			+ core::file_header::descriptor_type::packed_size
			+ instance.get_optional_header().get_size_of_structure()
			+ core::data_directories::directory_packed_size * number_of_data_directories
			+ dos_stub_size;
	}

	auto get_data_span() const
	{
		return std::span(data.begin() + buffer_offset, data.end());
	}

	std::size_t validate_pe_headers() const
	{
		auto data_span = get_data_span();

		EXPECT_EQ(data_span[0], std::byte{ 'M' });
		EXPECT_EQ(data_span[1], std::byte{ 'Z' });

		std::size_t offset = dos::dos_header::descriptor_type::packed_size;
		EXPECT_EQ(data_span[offset], dos_stub_first_byte);

		offset += dos_stub_size;
		//image signature
		EXPECT_EQ(data_span[offset], std::byte{ 'P' });
		EXPECT_EQ(data_span[offset + 1], std::byte{ 'E' });

		offset += core::image_signature::descriptor_type::packed_size;
		//machine
		EXPECT_EQ(data_span[offset], std::byte{ 0x4c });
		EXPECT_EQ(data_span[offset + 1], std::byte{ 0x01 });

		offset += core::file_header::descriptor_type::packed_size;
		//optional header magic (x86)
		EXPECT_EQ(data_span[offset], std::byte{ 0x0b });
		EXPECT_EQ(data_span[offset + 1], std::byte{ 0x01 });

		offset += instance.get_optional_header().get_size_of_structure();
		EXPECT_EQ(data_span[offset], first_dir_rva_byte);

		offset += number_of_data_directories
			* core::data_directories::directory_packed_size;
		return offset;
	}

public:
	static constexpr std::size_t dos_stub_size = 0x10u;
	static constexpr std::size_t number_of_data_directories = 2u;
	static constexpr std::byte dos_stub_first_byte{ 0xab };
	static constexpr std::byte first_dir_rva_byte{ 0xcd };

public:
	buffers::output_memory_buffer::buffer_type data;
	buffers::output_memory_buffer buf{ data };
	image::image instance;
	std::size_t full_headers_length{};
	std::size_t buffer_offset{};
};
} //namespace

TEST_P(ImageBuilderTestsFixture, InconsistentSectionDataError)
{
	instance.get_section_data_list().resize(1);

	expect_throw_pe_error([this] {
		image::image_builder::build(instance, buf);
	}, image::image_builder_errc::inconsistent_section_headers_and_data);
}

TEST_P(ImageBuilderTestsFixture, BuildNoSections)
{
	EXPECT_NO_THROW(image::image_builder::build(instance, buf));
	ASSERT_EQ(get_data_span().size(), full_headers_length);
	validate_pe_headers();
}

TEST_P(ImageBuilderTestsFixture, BuildOverlappingELfanew)
{
	static constexpr std::uint32_t overlapping_e_lfanew = 4u;
	instance.get_dos_header().get_descriptor()->e_lfanew = overlapping_e_lfanew;
	instance.get_dos_stub().copied_data().clear();
	EXPECT_NO_THROW(image::image_builder::build(instance, buf));

	auto data_span = get_data_span();
	ASSERT_EQ(data_span.size(), overlapping_e_lfanew
		+ core::image_signature::descriptor_type::packed_size
		+ core::file_header::descriptor_type::packed_size
		+ instance.get_optional_header().get_size_of_structure()
		+ core::data_directories::directory_packed_size * number_of_data_directories);

	EXPECT_EQ(data_span[0], std::byte{ 'M' });
	EXPECT_EQ(data_span[1], std::byte{ 'Z' });

	EXPECT_EQ(data_span[overlapping_e_lfanew], std::byte{ 'P' });
	EXPECT_EQ(data_span[overlapping_e_lfanew + 1], std::byte{ 'E' });
}

TEST_P(ImageBuilderTestsFixture, BuildNoSectionsWithFullHeadersData)
{
	static constexpr std::uint32_t extra_full_headers_length = 10u;
	static constexpr std::byte first_extra_headers_byte{ 0xef };

	instance.get_full_headers_buffer().copied_data().resize(
		full_headers_length + extra_full_headers_length);
	instance.get_full_headers_buffer().copied_data()[full_headers_length]
		= first_extra_headers_byte;

	EXPECT_NO_THROW(image::image_builder::build(instance, buf));

	auto data_span = get_data_span();
	ASSERT_EQ(data_span.size(), full_headers_length + extra_full_headers_length);

	EXPECT_EQ(data_span[full_headers_length], first_extra_headers_byte);
}

TEST_P(ImageBuilderTestsFixture, BuildWithSections)
{
	static constexpr char section1_first_name_char = 'a';
	static constexpr char section2_first_name_char = 'b';
	static constexpr std::byte section1_first_data_byte{ 0x12 };
	static constexpr std::byte section2_first_data_byte{ 0x34 };
	static constexpr std::uint32_t section1_headers_offset = 0x400;
	static constexpr std::uint32_t section1_raw_size = 0x100u;
	static constexpr std::uint32_t section2_raw_size = 0x200u;

	instance.get_section_table().get_section_headers().reserve(2);
	auto& header1 = instance.get_section_table()
		.get_section_headers().emplace_back().get_descriptor();
	auto& header2 = instance.get_section_table()
		.get_section_headers().emplace_back().get_descriptor();

	header1->name[0] = section1_first_name_char;
	header1->pointer_to_raw_data = section1_headers_offset;
	header2->name[0] = section2_first_name_char;
	header2->pointer_to_raw_data = header1->pointer_to_raw_data + section1_raw_size;

	instance.get_section_data_list().reserve(2);
	auto& data1 = instance.get_section_data_list().emplace_back().copied_data();
	auto& data2 = instance.get_section_data_list().emplace_back().copied_data();

	data1.resize(section1_raw_size);
	data1[0] = section1_first_data_byte;
	data2.resize(section2_raw_size);
	data2[0] = section2_first_data_byte;

	EXPECT_NO_THROW(image::image_builder::build(instance, buf));

	auto data_span = get_data_span();
	ASSERT_EQ(data_span.size(), header2->pointer_to_raw_data + data2.size());
	auto offset = validate_pe_headers();

	EXPECT_EQ(data_span[offset], std::byte{ section1_first_name_char });
	offset += section::section_header::descriptor_type::packed_size;
	EXPECT_EQ(data_span[offset], std::byte{ section2_first_name_char });

	EXPECT_EQ(data_span[header1->pointer_to_raw_data], section1_first_data_byte);
	EXPECT_EQ(data_span[header2->pointer_to_raw_data], section2_first_data_byte);
}

TEST_P(ImageBuilderTestsFixture, BuildWithOptionalHeaderGap)
{
	static constexpr std::uint32_t optional_header_gap = 0x20u;
	static constexpr std::byte first_extra_headers_byte{ 0xef };

	instance.get_full_headers_buffer().copied_data().resize(full_headers_length
		+ section::section_header::descriptor_type::packed_size
		+ optional_header_gap);
	instance.get_full_headers_buffer().copied_data()[full_headers_length]
		= first_extra_headers_byte;

	instance.get_section_table().get_section_headers().resize(1);
	instance.get_section_data_list().resize(1);
	instance.get_file_header().get_descriptor()->size_of_optional_header
		+= optional_header_gap;

	EXPECT_NO_THROW(image::image_builder::build(instance, buf));

	auto data_span = get_data_span();
	ASSERT_EQ(data_span.size(), full_headers_length + optional_header_gap
		+ section::section_header::descriptor_type::packed_size);
	validate_pe_headers();

	EXPECT_EQ(data_span[full_headers_length], first_extra_headers_byte);
}

namespace
{
class dummy_output_buffer final : public buffers::output_buffer_interface
{
	virtual void write(std::size_t, const std::byte*) override {}
	virtual void set_wpos(std::size_t) override {}
	virtual void advance_wpos(std::int32_t) override {}
	virtual std::size_t wpos() override
	{
		return (std::numeric_limits<std::size_t>::max)();
	}
	virtual std::size_t size() override
	{
		return 0;
	}
};
} //namespace

TEST(ImageBuilderTests, SectionOffsetOverflow)
{
	image::image instance;
	instance.get_section_data_list().resize(1);
	instance.get_section_table().get_section_headers().resize(1);

	dummy_output_buffer buf;
	expect_throw_pe_error([&instance, &buf] {
		image::image_builder::build(instance, buf);
	}, image::image_builder_errc::invalid_section_table_offset);
}

INSTANTIATE_TEST_SUITE_P(
	ImageBuilderTests,
	ImageBuilderTestsFixture,
	::testing::Values(
		false, true
	));
