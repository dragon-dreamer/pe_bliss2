#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <system_error>
#include <type_traits>
#include <vector>

#include "buffers/input_container_buffer.h"
#include "buffers/input_memory_buffer.h"

#include "pe_bliss2/core/image_signature.h"
#include "pe_bliss2/core/image_signature_errc.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/optional_header_errc.h"
#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/dos/dos_header_errc.h"
#include "pe_bliss2/dos/dos_stub.h"
#include "pe_bliss2/image/image_loader.h"
#include "pe_bliss2/section/section_errc.h"
#include "pe_bliss2/pe_error.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::image;

namespace
{
constexpr const char invalid_dos_header_data[] =
"MZ??????????????????????????????????????????????????????????"
"\xaa\x00\x00\xaa"; //e_lfanew

constexpr const char dos_header_data[] =
"MZ??????????????????????????????????????????????????????????"
"\x48\x00\x00\x00"; //e_lfanew

constexpr const char dos_stub_data[] = "abcdefgh";

constexpr const char invalid_pe_signature[] = "ABCD";

constexpr const char pe_signature[] = "PE\0\0";

constexpr std::array file_header_2_sections{
	std::byte{0x4c}, std::byte{0x01}, //machine (i386)
	std::byte{0x02}, std::byte{0x00}, //number_of_sections (2)
	std::byte{0x01}, std::byte{0x02}, //time_date_stamp
	std::byte{0x03}, std::byte{0x04},
	std::byte{0x05}, std::byte{0x06}, //pointer_to_symbol_table
	std::byte{0x07}, std::byte{0x08},
	std::byte{0x09}, std::byte{0x0a}, //number_of_symbols
	std::byte{0x0b}, std::byte{0x0c},
	std::byte{0x80}, std::byte{0x00}, //size_of_optional_header
	std::byte{0x00}, std::byte{0x00}  //characteristics
};

constexpr std::array file_header_invalid_opt_hdr_size{
	std::byte{0x4c}, std::byte{0x01}, //machine (i386)
	std::byte{0x02}, std::byte{0x00}, //number_of_sections (2)
	std::byte{0x01}, std::byte{0x02}, //time_date_stamp
	std::byte{0x03}, std::byte{0x04},
	std::byte{0x05}, std::byte{0x06}, //pointer_to_symbol_table
	std::byte{0x07}, std::byte{0x08},
	std::byte{0x09}, std::byte{0x0a}, //number_of_symbols
	std::byte{0x0b}, std::byte{0x0c},
	std::byte{0x01}, std::byte{0x00}, //size_of_optional_header
	std::byte{0x00}, std::byte{0x00}  //characteristics
};

constexpr std::array optional_header{
	std::byte{0x0b}, std::byte{0x02}, //magic (x64)
	std::byte{0x0e}, std::byte{0x20}, //linker version
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size of code
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size of initialize data
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size of uninitialize data
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //entry point
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //base of code
	std::byte{}, std::byte{}, std::byte{}, std::byte{0x40}, //image base
	std::byte{0x01}, std::byte{}, std::byte{}, std::byte{}, //image base
	std::byte{}, std::byte{0x10}, std::byte{}, std::byte{}, //section alignment
	std::byte{}, std::byte{0x02}, std::byte{}, std::byte{}, //file alignment
	std::byte{0x06}, std::byte{}, //major operating system version
	std::byte{}, std::byte{}, //minor operating system version
	std::byte{}, std::byte{}, //major image version
	std::byte{}, std::byte{}, //minor image system version
	std::byte{0x06}, std::byte{}, //major subsystem version
	std::byte{}, std::byte{}, //minor subsystem version
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //win32 version value
	std::byte{}, std::byte{0x80}, std::byte{}, std::byte{}, //size of image
	std::byte{}, std::byte{0x04}, std::byte{}, std::byte{}, //size of headers
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //checksum
	std::byte{0x03}, std::byte{}, //subsystem
	std::byte{0x60}, std::byte{0x01}, //DLL characteristict
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size of stack reserve
	std::byte{}, std::byte{0x10}, std::byte{}, std::byte{}, //size of stack reserve
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size of stack commit
	std::byte{}, std::byte{0x10}, std::byte{}, std::byte{}, //size of stack commit
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size of heap reserve
	std::byte{}, std::byte{0x10}, std::byte{}, std::byte{}, //size of heap reserve
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size of heap commit
	std::byte{}, std::byte{0x10}, std::byte{}, std::byte{}, //size of heap commit
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //loader flags
	std::byte{0x02}, std::byte{}, std::byte{}, std::byte{}, //number of RVA and sizes
};

constexpr std::array data_directories{
	std::byte{0x01}, std::byte{}, std::byte{}, std::byte{}, //export RVA
	std::byte{0x02}, std::byte{}, std::byte{}, std::byte{}, //export size
	std::byte{0x03}, std::byte{}, std::byte{}, std::byte{}, //import RVA
	std::byte{0x04}, std::byte{}, std::byte{}, std::byte{}, //import size
};

constexpr std::size_t second_section_raw_offset = 0x2000u;
constexpr std::size_t second_section_raw_size = 0x210u;
constexpr std::size_t first_section_raw_size = 0u;
constexpr std::size_t second_section_virtual_size = 0x2000u;
constexpr std::size_t first_section_virtual_size = 0x5000u;

constexpr std::array section_table_2_sections{
	std::byte{'t'}, std::byte{'e'}, std::byte{'s'}, std::byte{'t'}, //name
	std::byte{'s'}, std::byte{'e'}, std::byte{'c'}, std::byte{'t'}, //name
	std::byte{0x00}, std::byte{0x50}, std::byte{}, std::byte{}, //virtual size
	std::byte{0x00}, std::byte{0x10}, std::byte{}, std::byte{}, //virtual address
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //raw size
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //raw address
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //reloc address
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //line numbers
	std::byte{}, std::byte{}, //reloc number
	std::byte{}, std::byte{}, //line numbers number
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //characteristics

	std::byte{'1'}, std::byte{'2'}, std::byte{'3'}, std::byte{'4'}, //name
	std::byte{'5'}, std::byte{'6'}, std::byte{'7'}, std::byte{}, //name
	std::byte{0x00}, std::byte{0x20}, std::byte{}, std::byte{}, //virtual size
	std::byte{0x00}, std::byte{0x60}, std::byte{}, std::byte{}, //virtual address
	std::byte{0x10}, std::byte{0x02}, std::byte{}, std::byte{}, //raw size
	std::byte{0x00}, std::byte{0x20}, std::byte{}, std::byte{}, //raw address
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //reloc address
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //line numbers
	std::byte{}, std::byte{}, //reloc number
	std::byte{}, std::byte{}, //line numbers number
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //characteristics
};

template<typename Arr>
void append_array(std::vector<std::byte>& result, const Arr& arr)
{
	std::size_t size = std::size(arr);
	if (std::is_same_v<std::remove_cvref_t<
		std::remove_pointer_t<std::decay_t<Arr>>>, char>)
	{
		--size; //remove trailing nullbyte
	}
	auto bytes = reinterpret_cast<const std::byte*>(std::data(arr));
	result.insert(result.end(), bytes, bytes + size);
}

template<typename... Arr>
std::vector<std::byte> merge_arrays(const Arr&... arrs)
{
	std::vector<std::byte> result;
	(..., append_array(result, arrs));
	return result;
}

template<typename... Arr>
std::shared_ptr<buffers::input_container_buffer> buffer_for(const Arr&... arrs)
{
	auto buf = std::make_shared<buffers::input_container_buffer>();
	buf->get_container() = merge_arrays(arrs...);
	return buf;
}

void expect_fatal_error(const pe_bliss::image::image_load_result& result,
	std::error_code ec)
{
	ASSERT_FALSE(result);

	try
	{
		std::rethrow_exception(result.fatal_error);
	}
	catch (const pe_bliss::pe_error& e)
	{
		EXPECT_EQ(e.code(), ec);
	}
	catch (...)
	{
		EXPECT_TRUE(false);
	}
}

} //namespace

TEST(ImageLoaderTests, LoadInvalidDosHeader)
{
	auto buf = std::make_shared<buffers::input_memory_buffer>(
		reinterpret_cast<const std::byte*>(invalid_dos_header_data),
		sizeof(invalid_dos_header_data) - 1);
	auto result = image_loader::load(buf);
	expect_fatal_error(result, dos::dos_header_errc::unaligned_e_lfanew);
	expect_contains_errors(result.warnings);
}

TEST(ImageLoaderTests, LoadInvalidDosHeaderSkipValidation)
{
	auto buf = std::make_shared<buffers::input_memory_buffer>(
		reinterpret_cast<const std::byte*>(invalid_dos_header_data),
		sizeof(invalid_dos_header_data) - 1);
	auto result = image_loader::load(buf,
		{ .dos_header_validation = {.validate_e_lfanew = false } });
	expect_fatal_error(result, dos::dos_stub_errc::unable_to_read_dos_stub);
	expect_contains_errors(result.warnings);
}

TEST(ImageLoaderTests, LoadDosHeaderAndStub)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data));
	expect_fatal_error(result,
		core::image_signature_errc::unable_to_read_pe_signature);
	expect_contains_errors(result.warnings);
}

TEST(ImageLoaderTests, LoadInvalidPeSignature)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data, invalid_pe_signature));
	expect_fatal_error(result, core::image_signature_errc::invalid_pe_signature);
	expect_contains_errors(result.warnings);
}

TEST(ImageLoaderTests, LoadInvalidPeSignatureIgnored)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data, invalid_pe_signature),
		{ .validate_image_signature = false });
	expect_fatal_error(result, core::file_header_errc::unable_to_read_file_header);
	expect_contains_errors(result.warnings);
}

TEST(ImageLoaderTests, LoadValidPeSignature)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data, pe_signature));
	expect_fatal_error(result, core::file_header_errc::unable_to_read_file_header);
	expect_contains_errors(result.warnings);
}

TEST(ImageLoaderTests, LoadFileHeader)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_2_sections));
	expect_fatal_error(result,
		core::optional_header_errc::unable_to_read_optional_header);
	expect_contains_errors(result.warnings);
}

TEST(ImageLoaderTests, LoadOptionalHeader)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_2_sections, optional_header));
	expect_fatal_error(result,
		core::data_directories_errc::unable_to_read_data_directory);
	expect_contains_errors(result.warnings);
}

TEST(ImageLoaderTests, LoadInvalidSizeOptionalHeader)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_invalid_opt_hdr_size, optional_header));
	expect_fatal_error(result,
		core::data_directories_errc::unable_to_read_data_directory);
	expect_contains_errors(result.warnings,
		core::optional_header_errc::invalid_size_of_optional_header);
}

TEST(ImageLoaderTests, LoadInvalidSizeOptionalHeaderIgnoreError)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_invalid_opt_hdr_size, optional_header),
		{ .validate_size_of_optional_header = false });
	expect_fatal_error(result,
		core::data_directories_errc::unable_to_read_data_directory);
	expect_contains_errors(result.warnings);
}

TEST(ImageLoaderTests, LoadInvalidMagicOptionalHeader)
{
	auto invalid_optional_header = optional_header;
	invalid_optional_header[0] = {}; //corrupt optional header magic

	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_2_sections, invalid_optional_header));
	expect_fatal_error(result,
		core::optional_header_errc::invalid_pe_magic);
	expect_contains_errors(result.warnings);
}

TEST(ImageLoaderTests, LoadInvalidSectionAlignmentOptionalHeader)
{
	auto invalid_optional_header = optional_header;
	invalid_optional_header[32] = std::byte{0x1u}; //corrupt section alignment

	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_2_sections, invalid_optional_header));
	expect_fatal_error(result,
		core::data_directories_errc::unable_to_read_data_directory);
	expect_contains_errors(result.warnings,
		core::optional_header_errc::incorrect_section_alignment);
}

namespace
{
template<typename FileHeader>
void test_absent_section_table(const FileHeader& fh)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, fh,
			optional_header, data_directories));
	expect_fatal_error(result,
		section::section_errc::unable_to_read_section_table);
	expect_contains_errors(result.warnings);
}
} //namespace

TEST(ImageLoaderTests, LoadDataDirectories)
{
	test_absent_section_table(file_header_2_sections);
}

TEST(ImageLoaderTests, LoadDataDirectoriesOutOfBoundsSectionTablePointer)
{
	auto file_header_out_of_bounds_section_table = file_header_2_sections;
	file_header_out_of_bounds_section_table[17] = std::byte{ 0xffu };
	test_absent_section_table(file_header_out_of_bounds_section_table);
}

TEST(ImageLoaderTests, LoadSectionTableNoSectionData)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_2_sections, optional_header,
			data_directories, section_table_2_sections));
	EXPECT_FALSE(result.fatal_error);

	ASSERT_TRUE(result.warnings.has_errors());
	ASSERT_EQ(result.warnings.get_errors()->size(), 1u);
	EXPECT_TRUE(result.warnings.has_error(
		section::section_errc::unable_to_read_section_data, 1u));
}

TEST(ImageLoaderTests, LoadSectionTableIgnoreSectionData)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_2_sections, optional_header,
			data_directories, section_table_2_sections),
		{ .load_section_data = false });
	EXPECT_FALSE(result.fatal_error);
	expect_contains_errors(result.warnings);
}

namespace
{
struct check_options
{
	bool sections_copied = false;
	bool dos_stub_copied = false;
	bool full_headers_copied = false;
};

void test_full_image(const buffers::input_buffer_ptr& buf,
	const check_options& options)
{
	auto result = image_loader::load(buf,
		{ .eager_section_data_copy = options.sections_copied,
		.eager_dos_stub_data_copy = options.dos_stub_copied,
		.eager_full_headers_buffer_copy = options.full_headers_copied });
	EXPECT_FALSE(result.fatal_error);
	expect_contains_errors(result.warnings);

	auto& section_data_list = result.image.get_section_data_list();
	ASSERT_EQ(section_data_list.size(), 2u);
	EXPECT_EQ(section_data_list[0].is_copied(), options.sections_copied);
	ASSERT_EQ(section_data_list[0].data()->physical_size(), first_section_raw_size);
	ASSERT_EQ(section_data_list[0].data()->virtual_size(),
		first_section_virtual_size - first_section_raw_size);
	EXPECT_TRUE(section_data_list[0].data()->is_stateless());

	EXPECT_EQ(section_data_list[1].is_copied(), options.sections_copied);
	ASSERT_EQ(section_data_list[1].data()->physical_size(), second_section_raw_size);
	ASSERT_EQ(section_data_list[1].data()->virtual_size(),
		second_section_virtual_size - second_section_raw_size);
	EXPECT_TRUE(section_data_list[1].data()->is_stateless());
	EXPECT_EQ(section_data_list[1].copied_data()[0], std::byte{ 0xab });

	auto& section_headers = result.image.get_section_table().get_section_headers();
	EXPECT_EQ(section_headers[0].get_name(), "testsect");
	EXPECT_EQ(section_headers[1].get_rva(), 0x6000u);

	EXPECT_EQ(result.image.get_dos_header().get_descriptor()->e_lfanew, 0x48u);

	EXPECT_EQ(result.image.get_dos_stub().is_copied(), options.dos_stub_copied);
	EXPECT_EQ(result.image.get_dos_stub().copied_data()[0], std::byte{ 'a' });

	EXPECT_EQ(result.image.get_image_signature().get_signature(),
		core::image_signature::pe_signature);

	EXPECT_EQ(result.image.get_file_header().get_machine_type(),
		core::file_header::machine_type::i386);

	EXPECT_EQ(result.image.get_optional_header().get_number_of_rva_and_sizes(), 2u);

	ASSERT_EQ(result.image.get_data_directories().get_directories().size(), 2u);
	EXPECT_EQ(result.image.get_data_directories().get_directory(
		core::data_directories::directory_type::imports)->virtual_address, 0x3u);

	EXPECT_EQ(result.image.get_overlay().size(), 0u);

	EXPECT_EQ(result.image.get_full_headers_buffer().is_copied(),
		options.full_headers_copied);
	// size of headers = 0x400 = 1024
	EXPECT_EQ(result.image.get_full_headers_buffer().size(), 1024u);
}
} //namespace

TEST(ImageLoaderTests, LoadFullImage)
{
	auto buf = buffer_for(dos_header_data, dos_stub_data,
		pe_signature, file_header_2_sections, optional_header,
		data_directories, section_table_2_sections);

	auto& container = buf->get_container();
	container.resize(second_section_raw_offset
		+ second_section_raw_size);
	container[second_section_raw_offset] = std::byte{ 0xab };

	test_full_image(buf, {});
	test_full_image(buf, { .sections_copied = true });
	test_full_image(buf, { .dos_stub_copied = true });
	test_full_image(buf, { .full_headers_copied = true });
}

namespace
{
auto get_invalid_section_table()
{
	auto invalid_section_table = section_table_2_sections;
	//lower byte of second section virtual address
	invalid_section_table[52] = std::byte{ 1 };
	return invalid_section_table;
}
} //namespace

TEST(ImageLoaderTests, LoadInvalidSectionTable)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_2_sections, optional_header,
			data_directories, get_invalid_section_table()));
	EXPECT_FALSE(result.fatal_error);

	expect_contains_errors(result.warnings,
		section::section_errc::invalid_section_virtual_address_alignment,
		section::section_errc::virtual_gap_between_sections,
		core::optional_header_errc::invalid_size_of_image,
		section::section_errc::unable_to_read_section_data);

	EXPECT_TRUE(result.warnings.has_error(
		section::section_errc::invalid_section_virtual_address_alignment, 1u));
	EXPECT_TRUE(result.warnings.has_error(
		section::section_errc::virtual_gap_between_sections, 1u));
}

TEST(ImageLoaderTests, LoadInvalidSectionTableIgnoreSectionErrors)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_2_sections, optional_header,
			data_directories, get_invalid_section_table()),
		{ .validate_sections = false });
	EXPECT_FALSE(result.fatal_error);

	expect_contains_errors(result.warnings,
		core::optional_header_errc::invalid_size_of_image,
		section::section_errc::unable_to_read_section_data);
}

TEST(ImageLoaderTests, LoadInvalidSectionTableIgnoreSectionAndImageSizeErrors)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_2_sections, optional_header,
			data_directories, get_invalid_section_table()),
		{ .validate_sections = false, .validate_size_of_image = false });
	EXPECT_FALSE(result.fatal_error);

	expect_contains_errors(result.warnings,
		section::section_errc::unable_to_read_section_data);
}

TEST(ImageLoaderTests, LoadSectionTableNoFullHeadersBuffer)
{
	auto result = image_loader::load(
		buffer_for(dos_header_data, dos_stub_data,
			pe_signature, file_header_2_sections, optional_header,
			data_directories, section_table_2_sections),
		{ .load_section_data = false, .load_full_headers_buffer = false });
	EXPECT_EQ(result.image.get_full_headers_buffer().size(), 0u);
}

TEST(ImageLoaderTests, LoadOverlay)
{
	auto buf = buffer_for(dos_header_data, dos_stub_data,
		pe_signature, file_header_2_sections, optional_header,
		data_directories, section_table_2_sections);

	auto& container = buf->get_container();
	static constexpr std::array overlay{
		std::byte{1}, std::byte{2}, std::byte{3} };
	container.resize(second_section_raw_offset
		+ second_section_raw_size + overlay.size());
	std::copy(overlay.begin(), overlay.end(),
		container.end() - overlay.size());

	for (bool copy_overlay : { false, true })
	{
		auto result = image_loader::load(buf,
			{ .eager_overlay_data_copy = copy_overlay });
		EXPECT_FALSE(result.fatal_error);
		expect_contains_errors(result.warnings);

		EXPECT_NE(result.image.get_overlay().physical_size(), 0u);
		EXPECT_EQ(result.image.get_overlay().is_copied(), copy_overlay);

		ASSERT_EQ(result.image.get_overlay().copied_data(),
			(std::vector{ std::byte{1}, std::byte{2}, std::byte{3} }));
	}
}

TEST(ImageLoaderTests, IgnoreOverlay)
{
	auto buf = buffer_for(dos_header_data, dos_stub_data,
		pe_signature, file_header_2_sections, optional_header,
		data_directories, section_table_2_sections);

	auto& container = buf->get_container();
	container.resize(second_section_raw_offset
		+ second_section_raw_size + 10u);

	auto result = image_loader::load(buf,
		{ .load_overlay = false });
	EXPECT_FALSE(result.fatal_error);
	expect_contains_errors(result.warnings);

	EXPECT_EQ(result.image.get_overlay().size(), 0u);
}
