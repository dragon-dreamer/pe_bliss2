#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/relocations/relocation_entry.h"

#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::relocations;

TEST(RelocationEntryTests, TypeTests)
{
	relocation_entry entry;
	entry.get_descriptor().get() = 0xd234u;
	EXPECT_EQ(entry.get_type(), static_cast<relocation_type>(0xdu));

	entry.set_type(relocation_type::high);
	EXPECT_EQ(entry.get_type(), relocation_type::high);
}

TEST(RelocationEntryTests, AddressTests)
{
	relocation_entry entry;
	entry.get_descriptor().get() = 0xd234u;
	EXPECT_EQ(entry.get_address(), 0x234u);

	EXPECT_NO_THROW(entry.set_address(0xfdeu));
	EXPECT_EQ(entry.get_address(), 0xfdeu);

	expect_throw_pe_error([&entry] {
		entry.set_address(0x1000u);
	}, relocation_entry_errc::too_large_relocation_address);
	EXPECT_EQ(entry.get_address(), 0xfdeu);
}

TEST(RelocationEntryTests, AffectedSizeTests)
{
	relocation_entry entry;
	entry.set_type(relocation_type::absolute);
	EXPECT_EQ(entry.get_affected_size_in_bytes({}), 0u);

	entry.set_type(relocation_type::highadj);
	EXPECT_EQ(entry.get_affected_size_in_bytes({}), sizeof(std::uint16_t));

	entry.set_type(relocation_type::highlow);
	EXPECT_EQ(entry.get_affected_size_in_bytes({}), sizeof(std::uint32_t));

	entry.set_type(relocation_type::dir64);
	EXPECT_EQ(entry.get_affected_size_in_bytes({}), sizeof(std::uint64_t));

	entry.set_type(relocation_type::high);
	EXPECT_EQ(entry.get_affected_size_in_bytes({}), sizeof(std::uint16_t));

	entry.set_type(relocation_type::low);
	EXPECT_EQ(entry.get_affected_size_in_bytes({}), sizeof(std::uint16_t));

	entry.set_type(relocation_type::thumb_mov32);
	expect_throw_pe_error([&entry] {
		(void)entry.get_affected_size_in_bytes({});
	}, relocation_entry_errc::unsupported_relocation_type);

	EXPECT_EQ(entry.get_affected_size_in_bytes(pe_bliss::core::file_header::machine_type::armnt),
		sizeof(std::uint64_t));
}

TEST(RelocationEntryTests, RequiresParameterTest)
{
	relocation_entry entry;
	entry.set_type(relocation_type::absolute);
	EXPECT_FALSE(entry.requires_parameter());

	entry.set_type(relocation_type::highadj);
	EXPECT_TRUE(entry.requires_parameter());
}

TEST(RelocationEntryTests, ApplyToTest)
{
	relocation_entry entry;
	static constexpr std::uint64_t value64 = 0x123456789abcdeull;
	static constexpr std::uint32_t value32 = 0x12345678u;
	static constexpr std::uint16_t value16 = 0x1234u;
	static constexpr std::uint64_t image_base_difference = 0x567u;

	entry.set_type(relocation_type::absolute);
	EXPECT_EQ(entry.apply_to(value64, image_base_difference, {}), value64);

	entry.set_type(relocation_type::highlow);
	EXPECT_EQ(entry.apply_to(value32, image_base_difference, {}),
		value32 + image_base_difference);

	entry.set_type(relocation_type::dir64);
	EXPECT_EQ(entry.apply_to(value64, image_base_difference, {}),
		value64 + image_base_difference);

	entry.set_type(relocation_type::low);
	EXPECT_EQ(entry.apply_to(value16, image_base_difference, {}),
		value16 + image_base_difference);

	entry.set_type(relocation_type::high);
	EXPECT_EQ(entry.apply_to(value16, image_base_difference, {}),
		value16 + (image_base_difference >> 16u));

	entry.set_type(relocation_type::highadj);
	expect_throw_pe_error([&entry] {
		(void)entry.apply_to(0u, 0u, {});
	}, relocation_entry_errc::relocation_param_is_absent);

	entry.get_param().emplace(0xedabu);
	static constexpr std::uint16_t value16_2 = 0xe727u;
	static constexpr std::uint64_t image_base_difference_2 = 0xFFFFFFFF21040000ull;
	EXPECT_EQ(entry.apply_to(value16_2, image_base_difference_2, {}),
		0x082cu);
	
	entry.get_param().emplace(0x5dabu);
	EXPECT_EQ(entry.apply_to(0xe727u, image_base_difference_2, {}),
		0x082bu);
}

TEST(RelocationEntryTests, ApplyToArmNtTest)
{
	relocation_entry entry;

	entry.set_type(relocation_type::thumb_mov32);
	expect_throw_pe_error([&entry] {
		(void)entry.apply_to(0u, 0u, {});
	}, relocation_entry_errc::unsupported_relocation_type);
	
	std::array instructions{
		//movw ip, #0x7989
		std::byte{0x47u}, std::byte{0xf6u}, std::byte{0x89u}, std::byte{0x1cu},
		//movt ip, #0x40
		std::byte{0xc0u}, std::byte{0xf2u}, std::byte{0x40u}, std::byte{0x0cu}
	};
	static constexpr std::uint32_t image_base_difference = 0x12345u;

	std::uint64_t instruction_data{};
	pe_bliss::detail::packed_serialization<>::deserialize(instruction_data, instructions.data());

	EXPECT_NO_THROW(instruction_data = entry.apply_to(instruction_data,
		image_base_difference, pe_bliss::core::file_header::machine_type::armnt));

	pe_bliss::detail::packed_serialization<>::serialize(instruction_data, instructions.data());

	EXPECT_EQ(instructions, (std::array{
		//movw ip, #0x9cce
		std::byte{0x49u}, std::byte{0xf6u}, std::byte{0xceu}, std::byte{0x4cu},
		//movt ip, #0x41
		std::byte{0xc0u}, std::byte{0xf2u}, std::byte{0x41u}, std::byte{0x0cu}
	}));
}
