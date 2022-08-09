#include <array>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_memory_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/dos/dos_stub.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::dos;

TEST(DosStubTests, EmptyDosStubTest)
{
	dos_stub stub;
	EXPECT_EQ(stub.size(), 0u);
	EXPECT_EQ(stub.data()->absolute_offset(), 0u);
	EXPECT_EQ(stub.data()->relative_offset(), 0u);
}

TEST(DosStubTests, DeserializeDosStubTest)
{
	static constexpr std::array data{
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}
	};

	auto buffer = std::make_shared<buffers::input_memory_buffer>(data.data(), data.size());
	buffers::input_buffer_stateful_wrapper wrapper(buffer);

	dos_stub stub;
	expect_throw_pe_error([&] { stub.deserialize(wrapper, {
		.e_lfanew = static_cast<std::uint32_t>(data.size() + 1u) }); },
		dos_stub_errc::unable_to_read_dos_stub);

	for (bool copy_buffer : {true, false})
	{
		wrapper.set_rpos(1u);
		ASSERT_NO_THROW(stub.deserialize(wrapper, {
			.copy_memory = copy_buffer,
			.e_lfanew = static_cast<std::uint32_t>(data.size() - 2u) }));
		EXPECT_EQ(stub.data()->absolute_offset(), 1u);
		EXPECT_EQ(stub.data()->relative_offset(), 0u);
		EXPECT_EQ(stub.copied_data(),
			(std::vector{ std::byte{2}, std::byte{3} }));
		EXPECT_EQ(wrapper.rpos(), data.size() - 2u);
	}
}
