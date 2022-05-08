#include <array>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_memory_buffer.h"
#include "pe_bliss2/dos/dos_stub.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::dos;

TEST(DosStubTests, EmptyDosStubTest)
{
	dos_stub stub;
	EXPECT_TRUE(stub.empty());
	EXPECT_EQ(stub.data()->absolute_offset(), 0u);
	EXPECT_EQ(stub.data()->relative_offset(), 0u);
}

TEST(DosStubTests, DeserializeDosStubTest)
{
	static constexpr std::array data{
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}
	};

	auto buffer = std::make_shared<buffers::input_memory_buffer>(data.data(), data.size());

	dos_stub stub;
	expect_throw_pe_error([&] { stub.deserialize(buffer, {
		.e_lfanew = static_cast<std::uint32_t>(data.size() + 1u) }); },
		dos_stub_errc::unable_to_read_dos_stub);

	buffer->set_rpos(1u);
	EXPECT_NO_THROW(stub.deserialize(buffer, {
		.e_lfanew = static_cast<std::uint32_t>(data.size() - 2u) }));
	EXPECT_EQ(stub.data()->absolute_offset(), 1u);
	EXPECT_EQ(stub.data()->relative_offset(), 0u);
	EXPECT_EQ(stub.copied_data(),
		(std::vector{ std::byte{2}, std::byte{3} }));
}
