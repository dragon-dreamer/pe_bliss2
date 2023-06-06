#include "pe_bliss2/image/checksum.h"

#include <cstddef>

#include "gtest/gtest.h"

#include "pe_bliss2/image/image.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

TEST(ChecksumTests, Empty)
{
	image::image empty;
	expect_throw_pe_error([&empty]() {
		(void)image::calculate_checksum(empty);
	}, image::checksum_errc::invalid_checksum_offset);
}

TEST(ChecksumTests, ValidZeros)
{
	image::image instance;
	instance.get_full_headers_buffer().copied_data().resize(0x100u);
	//Add something to checksum field offset; should be ignored
	instance.get_full_headers_buffer().copied_data()[88] = std::byte{ 'a' };
	instance.get_full_headers_buffer().copied_data()[89] = std::byte{ 'b' };
	instance.get_full_headers_buffer().copied_data()[90] = std::byte{ 'c' };
	instance.get_full_headers_buffer().copied_data()[91] = std::byte{ 'd' };
	instance.get_section_data_list().resize(2u);
	instance.get_section_data_list()[0].copied_data().resize(0x200u);
	instance.get_section_data_list()[1].copied_data().resize(0x300u);
	instance.get_overlay().copied_data().resize(0x100u);
	// checksum for zero data equals data size
	EXPECT_EQ(image::calculate_checksum(instance), 0x700u);
}

TEST(ChecksumTests, ValidData)
{
	image::image instance;
	instance.get_full_headers_buffer().copied_data().resize(0x100u);
	instance.get_full_headers_buffer().copied_data()[0] = std::byte{ 1 };
	instance.get_section_data_list().resize(2u);
	instance.get_section_data_list()[0].copied_data().resize(0x200u);
	instance.get_section_data_list()[0].copied_data()[0x200 - 4u] = std::byte{ 2 };
	instance.get_section_data_list()[1].copied_data().resize(0x300u);
	instance.get_section_data_list()[1].copied_data()[4] = std::byte{ 3 };
	instance.get_overlay().copied_data().resize(0x100u);
	instance.get_overlay().copied_data()[8] = std::byte{ 5 };
	// checksum for zero data equals data size
	EXPECT_EQ(image::calculate_checksum(instance), 0x700u + 1 + 2 + 3 + 5);
}

TEST(ChecksumTests, UnalignedChecksumPos)
{
	image::image instance;
	instance.get_dos_header().get_descriptor()->e_lfanew = 1u;
	expect_throw_pe_error([&instance]() {
		(void)image::calculate_checksum(instance);
	}, image::checksum_errc::unaligned_checksum);
}

TEST(ChecksumTests, UnalignedBuffer)
{
	image::image instance;
	instance.get_full_headers_buffer().copied_data().resize(0x100u);
	instance.get_section_data_list().resize(1u);
	instance.get_section_data_list()[0].copied_data().resize(0x201u);
	instance.get_overlay().copied_data().resize(0x100u);
	expect_throw_pe_error([&instance]() {
		(void)image::calculate_checksum(instance);
	}, image::checksum_errc::unaligned_buffer);
}
