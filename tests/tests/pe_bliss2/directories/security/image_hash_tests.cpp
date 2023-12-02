#include "pe_bliss2/security/image_hash.h"

#include <cstddef>

#include "gtest/gtest.h"

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/pe_error.h"

#include "tests/pe_bliss2/directories/security/hex_string_helpers.h"

using namespace pe_bliss::security;

TEST(ImageHashTest, UnknownAlgorithm)
{
	pe_bliss::image::image image;
	ASSERT_THROW((void)calculate_hash(digest_algorithm::unknown, image, nullptr),
		pe_bliss::pe_error);
}

TEST(ImageHashTest, EmptyImage)
{
	pe_bliss::image::image image;
	ASSERT_THROW((void)calculate_hash(digest_algorithm::md5, image, nullptr),
		pe_bliss::pe_error);
}

TEST(ImageHashTest, NoPageHashesNoSecurityDir)
{
	pe_bliss::image::image image;
	auto& headers = image.get_full_headers_buffer().copied_data();
	headers.resize(512u);
	ASSERT_THROW((void)calculate_hash(digest_algorithm::md5, image, nullptr),
		pe_bliss::pe_error);
}

TEST(ImageHashTest, NoPageHashesWithOutOfBoundsSecurityDir)
{
	pe_bliss::image::image image;
	auto& headers = image.get_full_headers_buffer().copied_data();
	headers.resize(512u);
	image.get_data_directories().set_size(16u);
	image.get_data_directories().get_directory(
		pe_bliss::core::data_directories::directory_type::security)->virtual_address = 1u;
	image.get_data_directories().get_directory(
		pe_bliss::core::data_directories::directory_type::security)->size = 1u;
	ASSERT_THROW((void)calculate_hash(digest_algorithm::md5, image, nullptr),
		pe_bliss::pe_error);
}

namespace
{
void init_headers(pe_bliss::image::image& image)
{
	image.get_dos_header().get_descriptor()->e_lfanew = 0x10u;
	image.get_data_directories().set_size(16u);
	image.get_data_directories().get_directory(
		pe_bliss::core::data_directories::directory_type::security)->virtual_address = 1u;

	static constexpr std::size_t checksum_offset = 104u;
	static constexpr std::size_t cert_table_entry_offset = 168u;

	auto& headers = image.get_full_headers_buffer().copied_data();
	headers.resize(512u);
	headers[1u] = std::byte{ 0x10u };
	headers[checksum_offset + 1] = std::byte{ 0x10u };
	headers[cert_table_entry_offset + 2] = std::byte{ 0x10u };
	headers[200u] = std::byte{ 0x10u };
}

void init_full_section_data(pe_bliss::image::image& image)
{
	auto& sections = image.get_full_sections_buffer().copied_data();
	sections.resize(1024u);
	sections[1u] = std::byte{ 0x10u };
	sections[1000u] = std::byte{ 0x10u };
}

void init_section_data(pe_bliss::image::image& image)
{
	//Sections are added in reverse order of "pointer_to_raw_data" values,
	//should be sorder by the hashing code in the right order.

	{
		auto& data = image.get_section_data_list().emplace_back().copied_data();
		data.resize(324u);
		data[300u] = std::byte{ 0x10u };

		auto& header = image.get_section_table().get_section_headers().emplace_back();
		header.get_descriptor()->pointer_to_raw_data = 2000u;
		header.get_descriptor()->size_of_raw_data = static_cast<std::uint32_t>(data.size());
		image.get_section_data_list().back().data()->set_absolute_offset(2000u);
	}

	{
		auto& data = image.get_section_data_list().emplace_back().copied_data();
		data.resize(700u);
		data[1u] = std::byte{ 0x10u };

		auto& header = image.get_section_table().get_section_headers().emplace_back();
		header.get_descriptor()->pointer_to_raw_data = 1000u;
		header.get_descriptor()->size_of_raw_data = static_cast<std::uint32_t>(data.size());
		image.get_section_data_list().back().data()->set_absolute_offset(1000u);
	}

	//Empty should be ignored
	image.get_section_data_list().emplace_back();
	image.get_section_table().get_section_headers().emplace_back();
}

void init_overlay(pe_bliss::image::image& image, std::size_t extra_size)
{
	auto& overlay = image.get_overlay().copied_data();
	image.get_data_directories().get_directory(
		pe_bliss::core::data_directories::directory_type::security)->size = 100u;
	overlay.resize(100u + extra_size);
	overlay[0u] = std::byte{ 0x10u };
}
} //namespace

TEST(ImageHashTest, ValidImageNoPageHashesNoSectionsNoOverlay)
{
	pe_bliss::image::image image;
	init_headers(image);

	auto hash = calculate_hash(digest_algorithm::md5, image, nullptr);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes("79b59180d83f3ad160b2eca0fb95c3f7"));
}

TEST(ImageHashTest, ValidImageNoPageHashesFullSectionsNoOverlay)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_full_section_data(image);

	auto hash = calculate_hash(digest_algorithm::md5, image, nullptr);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes("d288d4dd593b76b690a6e747612c868b"));
}

TEST(ImageHashTest, ValidImageNoPageHashesFullSectionsNoExtraOverlay)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_full_section_data(image);
	init_overlay(image, 0u);

	auto hash = calculate_hash(digest_algorithm::sha1, image, nullptr);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes(
		"4a29d778ac3e9f646f3254b293965ce3ad4c0e83"));
}

TEST(ImageHashTest, ValidImageNoPageHashesFullSectionsWithExtraOverlay)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_full_section_data(image);
	init_overlay(image, 1u);

	auto hash = calculate_hash(digest_algorithm::sha384, image, nullptr);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes(
		"aed257a4db567ae6bb4110677b49e57e794797ac6e958618c3fd931cd2e7e8caff6ead3e09a"
		"29708c2823a15daf2c073"));
}

TEST(ImageHashTest, ValidImageNoPageHashesNoFullSectionsWithExtraOverlay)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_section_data(image);
	init_overlay(image, 1u);

	auto hash = calculate_hash(digest_algorithm::sha384, image, nullptr);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes(
		"aed257a4db567ae6bb4110677b49e57e794797ac6e958618c3fd931cd2e7e8caff6ead3e09a"
		"29708c2823a15daf2c073"));
}

TEST(ImageHashTest, InvalidSectionHeaderDataMismatch)
{
	pe_bliss::image::image image;
	init_headers(image);
	// Add two section data elements but only one header
	image.get_section_data_list().emplace_back();
	image.get_section_data_list().emplace_back();
	image.get_section_table().get_section_headers().emplace_back();
	init_overlay(image, 1u);

	ASSERT_THROW((void)calculate_hash(digest_algorithm::sha384, image, nullptr),
		pe_bliss::pe_error);
}

TEST(ImageHashTest, ValidImageWithPageHashesNoSectionsNoOverlay)
{
	pe_bliss::image::image image;
	init_headers(image);

	const page_hash_options opts{ .algorithm = digest_algorithm::sha1 };
	auto hash = calculate_hash(digest_algorithm::md5, image, &opts);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes("79b59180d83f3ad160b2eca0fb95c3f7"));

	ASSERT_EQ(hash.page_hashes,
		hex_string_to_bytes(
			"000000007da29727d2fe54cd589bf1bd6f8f4f0c6283774c"
			"000200000000000000000000000000000000000000000000"));
}

TEST(ImageHashTest, ValidImageWithPageHashesFullSectionsNoOverlay)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_full_section_data(image);

	const page_hash_options opts{ .algorithm = digest_algorithm::sha1 };
	auto hash = calculate_hash(digest_algorithm::md5, image, &opts);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes("d288d4dd593b76b690a6e747612c868b"));

	//Page hashes are the same as in the "ValidImageWithPageHashesNoSectionsNoOverlay"
	//test, because full section data is not enough to compute per-section page hashes.
	ASSERT_EQ(hash.page_hashes,
		hex_string_to_bytes(
			"000000007da29727d2fe54cd589bf1bd6f8f4f0c6283774c"
			"000200000000000000000000000000000000000000000000"));
}

TEST(ImageHashTest, ValidImageWithPageHashesFullSectionsNoExtraOverlay)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_full_section_data(image);
	init_overlay(image, 0u);

	const page_hash_options opts{ .algorithm = digest_algorithm::md5 };
	auto hash = calculate_hash(digest_algorithm::sha1, image, &opts);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes(
		"4a29d778ac3e9f646f3254b293965ce3ad4c0e83"));

	ASSERT_EQ(hash.page_hashes,
		hex_string_to_bytes(
			"000000009cda80b77c3b6e70e27439645064d448"
			"0002000000000000000000000000000000000000"));
}

TEST(ImageHashTest, ValidImageWithPageHashesFullSectionsWithExtraOverlay)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_full_section_data(image);
	init_overlay(image, 1u);

	const page_hash_options opts{ .algorithm = digest_algorithm::md5 };
	auto hash = calculate_hash(digest_algorithm::sha384, image, &opts);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes(
		"aed257a4db567ae6bb4110677b49e57e794797ac6e958618c3fd931cd2e7e8caff6ead3e09a"
		"29708c2823a15daf2c073"));

	//Page hashes are the same as in the "ValidImageWithPageHashesFullSectionsNoExtraOverlay"
	//test, because the overlay is not included in the page hashes.
	ASSERT_EQ(hash.page_hashes,
		hex_string_to_bytes(
			"000000009cda80b77c3b6e70e27439645064d448"
			"0002000000000000000000000000000000000000"));
}

TEST(ImageHashTest, ValidImageWithPageHashesNoFullSectionsWithExtraOverlay)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_section_data(image);
	init_overlay(image, 1u);

	const page_hash_options opts{ .algorithm = digest_algorithm::md5 };
	auto hash = calculate_hash(digest_algorithm::sha384, image, &opts);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes(
		"aed257a4db567ae6bb4110677b49e57e794797ac6e958618c3fd931cd2e7e8caff6ead3e09a"
		"29708c2823a15daf2c073"));

	ASSERT_EQ(hash.page_hashes,
		hex_string_to_bytes(
			"000000009cda80b77c3b6e70e27439645064d448"
			"e8030000754ff93a26f7795a509aa45ccadfbf71"
			"d007000057e886e90fe59442a5fd45b4e97c9803"
			"1409000000000000000000000000000000000000"));
}

TEST(ImageHashTest, ValidImageWithPageHashesNoFullSectionsWithExtraOverlayMaxPageSize)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_section_data(image);
	init_overlay(image, 1u);

	const page_hash_options opts{ .algorithm = digest_algorithm::md5,
		.max_page_hashes_size = 80u };
	auto hash = calculate_hash(digest_algorithm::sha384, image, &opts);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes(
		"aed257a4db567ae6bb4110677b49e57e794797ac6e958618c3fd931cd2e7e8caff6ead3e09a"
		"29708c2823a15daf2c073"));

	ASSERT_EQ(hash.page_hashes,
		hex_string_to_bytes(
			"000000009cda80b77c3b6e70e27439645064d448"
			"e8030000754ff93a26f7795a509aa45ccadfbf71"
			"d007000057e886e90fe59442a5fd45b4e97c9803"
			"1409000000000000000000000000000000000000"));
}

TEST(ImageHashTest, ValidImageWithPageHashesNoFullSectionsWithExtraOverlayMaxPageSizeTooSmall)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_section_data(image);
	init_overlay(image, 1u);

	const page_hash_options opts{ .algorithm = digest_algorithm::md5,
		.max_page_hashes_size = 79u };
	auto hash = calculate_hash(digest_algorithm::sha384, image, &opts);
	ASSERT_EQ(hash.page_hash_errc, hash_calculator_errc::page_hashes_data_too_big);
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes(
		"aed257a4db567ae6bb4110677b49e57e794797ac6e958618c3fd931cd2e7e8caff6ead3e09a"
		"29708c2823a15daf2c073"));

	ASSERT_TRUE(hash.page_hashes.empty());
}

TEST(ImageHashTest, ValidImageWithPageHashesWithFullSectionsWithExtraOverlayMaxPageSize)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_section_data(image);
	init_full_section_data(image);
	init_overlay(image, 1u);

	const page_hash_options opts{ .algorithm = digest_algorithm::md5,
		.max_page_hashes_size = 80u };
	auto hash = calculate_hash(digest_algorithm::sha384, image, &opts);
	ASSERT_EQ(hash.page_hash_errc, (std::errc{}));
	ASSERT_EQ(hash.image_hash, hex_string_to_bytes(
		"aed257a4db567ae6bb4110677b49e57e794797ac6e958618c3fd931cd2e7e8caff6ead3e09a"
		"29708c2823a15daf2c073"));

	ASSERT_EQ(hash.page_hashes,
		hex_string_to_bytes(
			"000000009cda80b77c3b6e70e27439645064d448"
			"e8030000754ff93a26f7795a509aa45ccadfbf71"
			"d007000057e886e90fe59442a5fd45b4e97c9803"
			"1409000000000000000000000000000000000000"));
}

TEST(ImageHashTest, VerifyImageHashInvalid)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_section_data(image);
	init_full_section_data(image);
	init_overlay(image, 1u);

	const auto result = verify_image_hash(hex_string_to_bytes("1234"),
		digest_algorithm::sha384, image, {}, {});
	ASSERT_FALSE(result);
	ASSERT_FALSE(result.image_hash_valid);
	ASSERT_FALSE(result.page_hashes_valid);
	ASSERT_FALSE(result.page_hashes_check_errc);
}

TEST(ImageHashTest, VerifyImageHashValid)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_section_data(image);
	init_full_section_data(image);
	init_overlay(image, 1u);

	const auto result = verify_image_hash(hex_string_to_bytes(
		"aed257a4db567ae6bb4110677b49e57e794797ac6e958618c3fd931cd2e7e8caff6ead3e09a"
		"29708c2823a15daf2c073"),
		digest_algorithm::sha384, image, {}, {});
	ASSERT_TRUE(result);
	ASSERT_TRUE(result.image_hash_valid);
	ASSERT_FALSE(result.page_hashes_valid);
	ASSERT_FALSE(result.page_hashes_check_errc);
}

TEST(ImageHashTest, VerifyImageHashValidWithPageHashes)
{
	pe_bliss::image::image image;
	init_headers(image);
	init_section_data(image);
	init_full_section_data(image);
	init_overlay(image, 1u);

	const auto result = verify_image_hash(hex_string_to_bytes(
		"aed257a4db567ae6bb4110677b49e57e794797ac6e958618c3fd931cd2e7e8caff6ead3e09a"
		"29708c2823a15daf2c073"),
		digest_algorithm::sha384, image, hex_string_to_bytes(
			"000000009cda80b77c3b6e70e27439645064d448"
			"e8030000754ff93a26f7795a509aa45ccadfbf71"
			"d007000057e886e90fe59442a5fd45b4e97c9803"
			"1409000000000000000000000000000000000000"),
		page_hash_options{
			.algorithm = digest_algorithm::md5,
			.max_page_hashes_size = 80u });
	ASSERT_TRUE(result);
	ASSERT_TRUE(result.image_hash_valid);
	ASSERT_TRUE(result.page_hashes_valid);
	ASSERT_FALSE(result.page_hashes_check_errc);
}
