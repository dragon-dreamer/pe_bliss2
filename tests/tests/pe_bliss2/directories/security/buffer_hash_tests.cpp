#include "pe_bliss2/security/buffer_hash.h"

#include <array>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_container_buffer.h"

#include "pe_bliss2/pe_error.h"

using namespace pe_bliss::security;

namespace
{
std::vector<std::byte> from_hex(std::string_view hex)
{
	assert((hex.size() % 2u) == 0u);
	std::vector<std::byte> result;
	result.reserve(hex.size() / 2u);
	const auto* data = hex.data();
	for (std::size_t i = 0; i < hex.size(); i += 2u, data += 2u)
	{
		std::uint8_t value{};
		std::from_chars(data, data + 2u, value, 16);
		result.emplace_back(std::byte{value});
	}
	return result;
}
} //namespace

TEST(BufferHashTests, SingleBufferHashMd5)
{
	buffers::input_container_buffer buf;
	buf.get_container() = { std::byte{1}, std::byte{0xffu}, std::byte{2} };
	EXPECT_EQ(calculate_hash(digest_algorithm::md5, buf),
		from_hex("8338abee6237257869443017bc8dabea"));
}

TEST(BufferHashTests, SingleBufferHashSha1)
{
	buffers::input_container_buffer buf;
	buf.get_container() = { std::byte{1}, std::byte{0xffu}, std::byte{2} };
	EXPECT_EQ(calculate_hash(digest_algorithm::sha1, buf),
		from_hex("6b3bf864bf741655b5d6f7cbdbf49a5f1185f5b1"));
}

TEST(BufferHashTests, SingleBufferHashSha256)
{
	buffers::input_container_buffer buf;
	buf.get_container() = { std::byte{1}, std::byte{0xffu}, std::byte{2} };
	EXPECT_EQ(calculate_hash(digest_algorithm::sha256, buf),
		from_hex("ff5362460c9af23f9c102496f1233f56aa46d47a3c86cc445f6b894dd6c98b08"));
}

TEST(BufferHashTests, SingleBufferHashSha384)
{
	buffers::input_container_buffer buf;
	buf.get_container() = { std::byte{1}, std::byte{0xffu}, std::byte{2} };
	EXPECT_EQ(calculate_hash(digest_algorithm::sha384, buf),
		from_hex("07eaa778650d3084e5059293a061ae17085932d2ff2e4a20209fb08c6"
			"369e4e6c551f59dad1f1e38f2a07ee96acb07d0"));
}

TEST(BufferHashTests, SingleBufferHashSha512)
{
	buffers::input_container_buffer buf;
	buf.get_container() = { std::byte{1}, std::byte{0xffu}, std::byte{2} };
	EXPECT_EQ(calculate_hash(digest_algorithm::sha512, buf),
		from_hex("0b11c7bbb0bcbb22dc45eaf36ad220448f302bd54befc180039b2f637"
			"2d9ef20d444b9ffdb0c0ace96d5be23bf39190508c3b724f2521ec4dd53bb1"
			"fc05af6c2"));
}

TEST(BufferHashTests, SingleBufferHashUnsupported)
{
	buffers::input_container_buffer buf;
	buf.get_container() = { std::byte{1}, std::byte{0xffu}, std::byte{2} };
	EXPECT_THROW((void)calculate_hash(digest_algorithm::unknown, buf), pe_bliss::pe_error);
}

TEST(BufferHashTests, SplitBufferHashMd5)
{
	std::vector<std::byte> buf1{ std::byte{1} };
	std::vector<std::byte> buf2;
	std::vector<std::byte> buf3{ std::byte{0xffu}, std::byte{2} };
	EXPECT_EQ(calculate_hash(digest_algorithm::md5, (std::array<std::span<const std::byte>, 3>{ buf1, buf2, buf3 })),
		from_hex("8338abee6237257869443017bc8dabea"));
}
