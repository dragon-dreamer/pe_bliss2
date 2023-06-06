#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <memory>
#include <vector>

#include "buffers/input_buffer_section.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_container_buffer.h"
#include "buffers/input_memory_buffer.h"
#include "buffers/output_memory_buffer.h"

#include "pe_bliss2/detail/rich/rich_header_utils.h"
#include "pe_bliss2/rich/rich_compid.h"
#include "pe_bliss2/rich/rich_header.h"
#include "pe_bliss2/rich/rich_header_builder.h"
#include "pe_bliss2/rich/rich_header_loader.h"

#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::rich;
using namespace pe_bliss::detail::rich;

TEST(RichHeaderTests, EmptyTest)
{
	rich_header header;
	EXPECT_EQ(header.get_absolute_offset(), 0u);
	EXPECT_EQ(header.get_dos_stub_offset(), 0u);
	EXPECT_EQ(header.get_checksum(), 0u);
	EXPECT_TRUE(header.get_compids().empty());
}

TEST(RichHeaderTests, GetSetTest)
{
	rich_header header;
	header.set_checksum(123u);
	EXPECT_EQ(header.get_checksum(), 123u);

	header.set_dos_stub_offset(456u, 789u);
	EXPECT_EQ(header.get_absolute_offset(), 789u);
	EXPECT_EQ(header.get_dos_stub_offset(), 456u);
}

TEST(RichHeaderTests, FindChecksumTests)
{
	{
		buffers::input_container_buffer buffer;
		buffers::input_buffer_stateful_wrapper_ref ref(buffer);
		EXPECT_EQ(rich_header_utils::find_checksum(ref), 0u);
	}

	{
		static constexpr const std::array data{
			std::byte{'R'}, std::byte{'i'}, std::byte{'c'}, std::byte{'h'},
			std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}
		};

		{
			buffers::input_memory_buffer buffer(data.data(), data.size() - 1);
			buffers::input_buffer_stateful_wrapper_ref ref(buffer);
			EXPECT_EQ(rich_header_utils::find_checksum(ref), 0u);
		}

		{
			buffers::input_memory_buffer buffer(data.data(), data.size());
			buffers::input_buffer_stateful_wrapper_ref ref(buffer);
			EXPECT_EQ(rich_header_utils::find_checksum(ref), 4u);
		}

		{
			buffers::input_memory_buffer buffer(data.data(), data.size());
			buffers::input_buffer_stateful_wrapper_ref ref(buffer);
			ref.set_rpos(1);
			EXPECT_EQ(rich_header_utils::find_checksum(ref), 0u);
		}
	}

	{
		static constexpr const std::array data{
			std::byte{'R'}, std::byte{'i'}, std::byte{'c'}, std::byte{'h'},
			std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
			std::byte{'R'}, std::byte{'i'}, std::byte{'c'}, std::byte{'h'},
			std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}
		};
		buffers::input_memory_buffer buffer(data.data(), data.size());
		buffers::input_buffer_stateful_wrapper_ref ref(buffer);
		EXPECT_EQ(rich_header_utils::find_checksum(ref), 12u);
	}
	
	{
		static constexpr const std::array data{
			std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
			std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}
		};
		buffers::input_memory_buffer buffer(data.data(), data.size());
		buffers::input_buffer_stateful_wrapper_ref ref(buffer);
		EXPECT_EQ(rich_header_utils::find_checksum(ref), 0u);
	}
}

TEST(RichHeaderTests, DecodeChecksumTest)
{
	const std::vector<std::byte> data{
		std::byte{},
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}
	};
	buffers::input_memory_buffer buffer(data.data(), data.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buffer);
	ref.set_rpos(1u);
	EXPECT_EQ(rich_header_utils::decode_checksum(ref), 0x04030201u);
}

namespace
{
constexpr std::uint16_t build_number1 = 0x11u;
constexpr std::uint16_t build_number2[]{ 0x6au, 0xdeu };
constexpr std::uint16_t prod_id1 = 0xefu;
constexpr std::uint16_t prod_id2[]{ 0x3u, 0xabu };
constexpr std::uint32_t use_count1 = 0x15u;
constexpr std::uint32_t use_count2[]{ 0xffu, 0x57u };

constexpr rich_compid compid1{
	.build_number = build_number1,
	.prod_id = prod_id1,
	.use_count = use_count1
};
constexpr rich_compid compid2{
	.build_number = (build_number2[1] << CHAR_BIT) | build_number2[0],
	.prod_id = (prod_id2[1] << CHAR_BIT) | prod_id2[0],
	.use_count = (use_count2[1] << CHAR_BIT) | use_count2[0]
};

constexpr std::array dos_stub{
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u },
	std::byte{ 0x78u ^ static_cast<std::uint8_t>('D') },
	std::byte{ 0x56u ^ static_cast<std::uint8_t>('a') },
	std::byte{ 0x34u ^ static_cast<std::uint8_t>('n') },
	std::byte{ 0x12u ^ static_cast<std::uint8_t>('S') },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u },

	std::byte{ build_number1 ^ 0x78u }, std::byte{ 0x56u },
	std::byte{ prod_id1 ^ 0x34u }, std::byte{ 0x12u },
	std::byte{ use_count1 ^ 0x78u }, std::byte{ 0x56u },
		std::byte{ 0x34u }, std::byte{ 0x12u },

	std::byte{ build_number2[0] ^ 0x78u }, std::byte{ build_number2[1] ^ 0x56u },
	std::byte{ prod_id2[0] ^ 0x34u }, std::byte{ prod_id2[1] ^ 0x12u },
	std::byte{ use_count2[0] ^ 0x78u }, std::byte{ use_count2[1] ^ 0x56u },
		std::byte{ 0x34u }, std::byte{ 0x12u },

	std::byte{ 'R' }, std::byte{ 'i' }, std::byte{ 'c' }, std::byte{ 'h' },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u }
};

constexpr rich_header_utils::checksum_type dos_stub_checksum = 0x12345678u;
} //namespace

TEST(RichHeaderTests, DecodeDansSignatureTest)
{
	std::vector<std::byte> data(dos_stub.cbegin(), dos_stub.cend());
	buffers::input_memory_buffer buffer(data.data(), data.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buffer);
	ref.set_rpos(buffer.size());
	EXPECT_EQ(rich_header_utils::find_dans_signature(
		ref, dos_stub_checksum), 16u);

	data[16u] = std::byte{};
	ref.set_rpos(ref.size());
	expect_throw_pe_error([&] {
		(void)rich_header_utils::find_dans_signature(
			ref, dos_stub_checksum);
	}, rich_header_loader_errc::no_dans_signature);

	ref.set_rpos(0u);
	expect_throw_pe_error([&] {
		(void)rich_header_utils::find_dans_signature(
			ref, dos_stub_checksum);
	}, rich_header_loader_errc::no_dans_signature);
}

TEST(RichHeaderTests, DecodeCompidsTest)
{
	std::vector<rich_compid> compids;
	buffers::input_memory_buffer buffer(dos_stub.data(), dos_stub.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buffer);
	ref.set_rpos(16u);
	expect_throw_pe_error([&] {
		rich_header_utils::decode_compids(ref,
			ref.size() - 5u, dos_stub_checksum, compids);
	}, rich_header_loader_errc::unable_to_decode_compids);
	EXPECT_TRUE(compids.empty());

	ref.set_rpos(15u); //Check alignment
	ASSERT_NO_THROW(rich_header_utils::decode_compids(ref,
		ref.size() - 4u, dos_stub_checksum, compids));
	ASSERT_EQ(compids.size(), 2u);
	EXPECT_EQ(compids.at(0), compid1);
	EXPECT_EQ(compids.at(1), compid2);
}

TEST(RichHeaderTests, DeserializeTest1)
{
	buffers::input_memory_buffer buffer(dos_stub.data(), dos_stub.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buffer);
	buffer.set_absolute_offset(100u);
	auto header = load(ref);
	ASSERT_TRUE(header.has_value());
	EXPECT_EQ(header->get_dos_stub_offset(), 16u);
	EXPECT_EQ(header->get_absolute_offset(), buffer.absolute_offset() + 16u);
	EXPECT_EQ(header->get_checksum(), dos_stub_checksum);
	EXPECT_EQ(header->get_compids().size(), 2u);
	EXPECT_EQ(header->get_compids().at(0), compid1);
	EXPECT_EQ(header->get_compids().at(1), compid2);
}

TEST(RichHeaderTests, DeserializeTest2)
{
	buffers::input_memory_buffer buffer(dos_stub.data(), dos_stub.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buffer);
	ref.set_rpos(ref.size() - 6u);
	EXPECT_FALSE(load(ref).has_value());
}

TEST(RichHeaderTests, DeserializeTest3)
{
	std::vector<std::byte> data(dos_stub.cbegin(), dos_stub.cend());
	buffers::input_memory_buffer buffer(data.data(), data.size());
	data[16u] = std::byte{};

	buffers::input_buffer_stateful_wrapper_ref ref(buffer);
	expect_throw_pe_error([&] {
		(void)load(ref);
	}, rich_header_loader_errc::no_dans_signature);
}

TEST(RichHeaderTests, CalculateSize1)
{
	buffers::input_memory_buffer buffer(dos_stub.data(), dos_stub.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buffer);
	auto header = load(ref);
	ASSERT_TRUE(header.has_value());
	EXPECT_EQ(get_built_size(*header), 40u);
}

TEST(RichHeaderTests, CalculateSize2)
{
	rich_header header;
	EXPECT_EQ(get_built_size(header), 24u);
}

TEST(RichHeaderTests, CalculateSize3)
{
	rich_header header;
	header.set_dos_stub_offset(1u, 1u);
	EXPECT_EQ(get_built_size(header), 24u + 16u - 1u);
}

TEST(RichHeaderTests, CalculateSize4)
{
	rich_header header;
	header.set_dos_stub_offset((std::numeric_limits<std::size_t>::max)(), 1u);
	expect_throw_pe_error([&] {
		(void)get_built_size(header);
	}, rich_header_errc::invalid_rich_header_offset);
}

namespace
{

constexpr std::array dos_header_with_stub{
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x12 }, std::byte{ 0x34 }, std::byte{ 0x56 }, std::byte{ 0x78 }, //e_lfanew
	//Header end, DOS stub start
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
	std::byte{ 0x78u ^ static_cast<std::uint8_t>('D') },
	std::byte{ 0x56u ^ static_cast<std::uint8_t>('a') },
	std::byte{ 0x34u ^ static_cast<std::uint8_t>('n') },
	std::byte{ 0x12u ^ static_cast<std::uint8_t>('S') },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u },

	std::byte{ build_number1 ^ 0x78u }, std::byte{ 0x56u },
	std::byte{ prod_id1 ^ 0x34u }, std::byte{ 0x12u },
	std::byte{ use_count1 ^ 0x78u }, std::byte{ 0x56u },
		std::byte{ 0x34u }, std::byte{ 0x12u },

	std::byte{ build_number2[0] ^ 0x78u }, std::byte{ build_number2[1] ^ 0x56u },
	std::byte{ prod_id2[0] ^ 0x34u }, std::byte{ prod_id2[1] ^ 0x12u },
	std::byte{ use_count2[0] ^ 0x78u }, std::byte{ use_count2[1] ^ 0x56u },
		std::byte{ 0x34u }, std::byte{ 0x12u },

	std::byte{ 'R' }, std::byte{ 'i' }, std::byte{ 'c' }, std::byte{ 'h' },
	std::byte{ 0x78u }, std::byte{ 0x56u }, std::byte{ 0x34u }, std::byte{ 0x12u }
};

} //namespace

TEST(RichHeaderTests, CalculateChecksum1)
{
	auto buffer = std::make_shared<buffers::input_container_buffer>();
	static constexpr std::size_t dos_header_size = 0x40u;
	buffer->get_container().assign(std::cbegin(dos_header_with_stub),
		std::cend(dos_header_with_stub));

	auto dos_stub_buffer = buffers::reduce(buffer, dos_header_size);
	buffers::input_buffer_stateful_wrapper_ref dos_stub_ref(*dos_stub_buffer);
	auto header = load(dos_stub_ref);
	ASSERT_TRUE(header.has_value());

	buffers::input_buffer_stateful_wrapper_ref ref(*buffer);
	std::uint32_t checksum = 0x40 + 16;
	checksum += get_checksum(compid1) + get_checksum(compid2);
	EXPECT_EQ(header->calculate_checksum(ref), checksum);

	buffer->get_container()[5u] = std::byte{ 0x12u };
	buffer->get_container()[45u] = std::byte{ 0x34u };
	checksum += std::rotl(0x12u, 5u);
	checksum += std::rotl(0x34u, 45u);
	EXPECT_EQ(header->calculate_checksum(ref), checksum);
}

TEST(RichHeaderTests, CalculateChecksum2)
{
	buffers::input_memory_buffer buffer(
		dos_header_with_stub.data(), dos_header_with_stub.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buffer);

	rich_header header;
	header.set_dos_stub_offset(0x11u, 0x11u);
	expect_throw_pe_error([&] {
		(void)header.calculate_checksum(ref);
	}, rich_header_errc::unaligned_rich_header_offset);
}

TEST(RichHeaderTests, CalculateChecksum3)
{
	buffers::input_memory_buffer buffer(
		dos_header_with_stub.data(), dos_header_with_stub.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buffer);

	rich_header header;
	header.set_dos_stub_offset(0x1000u, 0x1000u);
	expect_throw_pe_error([&] {
		(void)header.calculate_checksum(ref);
	}, rich_header_errc::invalid_rich_header_offset);
}

TEST(RichHeaderTests, SerializeTest1)
{
	auto buffer = std::make_shared<buffers::input_container_buffer>();
	static constexpr std::size_t dos_header_size = 0x40u;
	static constexpr std::size_t dos_stub_excluding_rich_size = 16u;
	buffer->get_container().assign(std::cbegin(dos_header_with_stub),
		std::cbegin(dos_header_with_stub) + dos_header_size
			+ dos_stub_excluding_rich_size);

	rich_header header;
	header.set_dos_stub_offset(dos_stub_excluding_rich_size,
		dos_header_size + dos_stub_excluding_rich_size);
	header.set_checksum(dos_stub_checksum);
	header.get_compids().emplace_back(compid1);
	header.get_compids().emplace_back(compid2);

	std::vector<std::byte> serialized;
	buffers::output_memory_buffer outbuf(serialized);
	ASSERT_NO_THROW(build(header, outbuf));
	ASSERT_EQ(serialized.size(),
		dos_header_with_stub.size() - dos_header_size);
	EXPECT_TRUE(std::equal(serialized.cbegin(), serialized.cend(),
		dos_header_with_stub.cbegin() + dos_header_size));
}

TEST(RichHeaderTests, SerializeTest2)
{
	rich_header header;
	header.set_dos_stub_offset(std::numeric_limits<std::size_t>::max(),
		std::numeric_limits<std::size_t>::max());

	std::vector<std::byte> serialized;
	buffers::output_memory_buffer outbuf(serialized);
	expect_throw_pe_error([&] {
		build(header, outbuf);
	}, rich_header_builder_errc::unable_to_build_rich_header);
}
