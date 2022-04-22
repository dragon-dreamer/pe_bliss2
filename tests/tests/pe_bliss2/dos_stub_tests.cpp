#include <array>
#include <utility>

#include "gtest/gtest.h"

#include "buffers/input_memory_buffer.h"
#include "pe_bliss2/dos_stub.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

TEST(DosStubTests, EmptyDosStubTest)
{
	pe_bliss::dos_stub stub;
	EXPECT_TRUE(stub.data().empty());
	EXPECT_TRUE(std::as_const(stub).data().empty());
	EXPECT_EQ(stub.buffer_pos(), 0u);
}

TEST(DosStubTests, DeserializeDosStubTest)
{
	static constexpr std::array data{
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}
	};

	buffers::input_memory_buffer buffer(data.data(), data.size());

	pe_bliss::dos_stub stub;
	expect_throw_pe_error([&] { stub.deserialize(buffer, data.size() + 1u); },
		pe_bliss::dos_stub_errc::unable_to_read_dos_stub);

	buffer.set_rpos(1u);
	EXPECT_NO_THROW(stub.deserialize(buffer, data.size() - 2u));
	EXPECT_EQ(stub.buffer_pos(), 1u);
	EXPECT_EQ(stub.data(),
		(pe_bliss::dos_stub::dos_stub_data_type{ std::byte{2}, std::byte{3} }));
	EXPECT_EQ(std::as_const(stub).data(),
		(pe_bliss::dos_stub::dos_stub_data_type{ std::byte{2}, std::byte{3} }));
}
