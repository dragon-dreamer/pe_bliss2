#include "pe_bliss2/security/security_directory_loader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <tuple>

#include "gtest/gtest.h"

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/image/image.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::security;

namespace
{
template<typename Array>
pe_bliss::image::image create_image(const Array& security_data,
	std::uint32_t size = std::tuple_size_v<Array>,
	std::uint32_t offset = 1024u)
{
	pe_bliss::image::image result;
	result.get_data_directories().set_size(16u);
	auto& dir = result.get_data_directories().get_directory(
		pe_bliss::core::data_directories::directory_type::security);
	dir->virtual_address = offset;
	dir->size = size;
	result.get_overlay().copied_data().assign(
		security_data.begin(), security_data.end());
	result.get_overlay().data()->set_absolute_offset(offset);
	return result;
}
} //namespace

TEST(SecurityDirectoryLoaderTests, Empty)
{
	pe_bliss::image::image image;
	EXPECT_FALSE(load(image));
}

namespace
{
constexpr std::array directory{
	std::byte{10}, std::byte{}, std::byte{}, std::byte{}, //length
	std::byte{}, std::byte{}, //revision
	std::byte{}, std::byte{}, //certificate_type
	std::byte{'a'}, std::byte{'b'}, //certificate data

	//alignment
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{},

	std::byte{11}, std::byte{}, std::byte{}, std::byte{}, //length
	std::byte{}, std::byte{}, //revision
	std::byte{}, std::byte{}, //certificate_type
	std::byte{1}, std::byte{2}, std::byte{3} //certificate data
};

void validate_security_directory(
	std::optional<security_directory_details>& dir, bool copy_data)
{
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir);
	auto& entries = dir->get_entries();
	ASSERT_EQ(entries.size(), 2u);

	expect_contains_errors(entries[0]);
	expect_contains_errors(entries[1]);

	EXPECT_EQ(entries[0].get_descriptor()->length, 10u);
	EXPECT_EQ(entries[1].get_descriptor()->length, 11u);

	EXPECT_EQ(entries[0].get_certificate().is_copied(), copy_data);
	EXPECT_EQ(entries[1].get_certificate().is_copied(), copy_data);

	EXPECT_EQ(entries[0].get_certificate().copied_data(), (std::vector{
		std::byte{'a'}, std::byte{'b'}
	}));
	EXPECT_EQ(entries[1].get_certificate().copied_data(), (std::vector{
		std::byte{1}, std::byte{2}, std::byte{3}
	}));
}

constexpr std::array single_entry_directory{
	std::byte{10}, std::byte{}, std::byte{}, std::byte{}, //length
	std::byte{}, std::byte{}, //revision
	std::byte{}, std::byte{}, //certificate_type
	std::byte{'a'}, std::byte{'b'}, //certificate data
};

void validate_single_entry_directory(
	std::optional<security_directory_details>& dir)
{
	ASSERT_TRUE(dir);
	auto& entries = dir->get_entries();
	ASSERT_EQ(entries.size(), 1u);
	expect_contains_errors(entries[0]);
	EXPECT_EQ(entries[0].get_certificate().copied_data(), (std::vector{
		std::byte{'a'}, std::byte{'b'}
	}));
}
} //namespace

TEST(SecurityDirectoryLoaderTests, Valid)
{
	auto dir = load(create_image(directory));
	validate_security_directory(dir, false);
}

TEST(SecurityDirectoryLoaderTests, ValidCopyData)
{
	auto dir = load(create_image(directory), { .copy_raw_data = true });
	validate_security_directory(dir, true);
}

TEST(SecurityDirectoryLoaderTests, ValidLimit)
{
	auto dir = load(create_image(directory), { .max_entries = 1u });
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		security_directory_loader_errc::too_many_entries);
	validate_single_entry_directory(dir);
}

TEST(SecurityDirectoryLoaderTests, Unaligned)
{
	auto dir = load(create_image(single_entry_directory,
		static_cast<std::uint32_t>(single_entry_directory.size()), 1025u));
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		security_directory_loader_errc::unaligned_directory);
	validate_single_entry_directory(dir);
}

TEST(SecurityDirectoryLoaderTests, ExtraData)
{
	auto dir = load(create_image(single_entry_directory,
		static_cast<std::uint32_t>(single_entry_directory.size()) + 3u));
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		security_directory_loader_errc::invalid_directory_size);
	validate_single_entry_directory(dir);
}

TEST(SecurityDirectoryLoaderTests, InvalidCertificate)
{
	constexpr std::array invalid_certificate_dir{
		std::byte{20}, std::byte{}, std::byte{}, std::byte{}, //length
		std::byte{}, std::byte{}, //revision
		std::byte{}, std::byte{}, //certificate_type
		std::byte{'a'}, std::byte{'b'}, //certificate data
	};

	auto dir = load(create_image(invalid_certificate_dir));
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir);
	auto& entries = dir->get_entries();
	ASSERT_EQ(entries.size(), 1u);
	expect_contains_errors(entries[0],
		security_directory_loader_errc::invalid_entry_size);
}

TEST(SecurityDirectoryLoaderTests, InvalidEntry)
{
	constexpr std::array invalid_entry_dir{
		std::byte{20}, std::byte{}, std::byte{}, std::byte{}, //length
	};

	auto dir = load(create_image(invalid_entry_dir));
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		security_directory_loader_errc::invalid_directory_size);
}

TEST(SecurityDirectoryLoaderTests, InvalidDirectory1)
{
	auto image = create_image(std::array<std::byte, 0>{});
	image.get_overlay().data()->set_absolute_offset(10000u);
	auto dir = load(image);
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		security_directory_loader_errc::invalid_directory);
}

TEST(SecurityDirectoryLoaderTests, InvalidDirectory)
{
	auto image = create_image(std::array<std::byte, 0>{});
	image.get_overlay().data()->set_absolute_offset(10u);
	auto dir = load(image);
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		security_directory_loader_errc::invalid_directory);
}
