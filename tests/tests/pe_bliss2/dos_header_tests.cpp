#include "gtest/gtest.h"

#include <iterator>

#include "buffers/input_memory_buffer.h"

#include "pe_bliss2/dos_header.h"
#include "pe_bliss2/pe_error.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

TEST(DosHeaderTests, EmptyTest)
{
	dos_header header;
	EXPECT_EQ(header.validate_magic(),
		dos_header_errc::invalid_dos_header_signature);
	EXPECT_EQ(header.validate_e_lfanew(),
		dos_header_errc::invalid_e_lfanew);
	EXPECT_THROW(header.validate({}).throw_on_error(), pe_error);
	EXPECT_NO_THROW(header.validate({
		.validate_e_lfanew = false,
		.validate_magic = false
	}).throw_on_error());
}

TEST(DosHeaderTests, ValidationTest1)
{
	dos_header header;
	static constexpr const char header_data[] =
		"MZ??????????????????????????????????????????????????????????"
		"\x04\x00\x00\x00??????";
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(header_data),
		std::size(header_data));

	header.deserialize(buf, false);
	EXPECT_NO_THROW(header.validate_magic().throw_on_error());
	EXPECT_NO_THROW(header.validate_e_lfanew().throw_on_error());
	EXPECT_NO_THROW(header.validate({}).throw_on_error());
}

TEST(DosHeaderTests, ValidationTest2)
{
	dos_header header;
	static constexpr const char header_data[] =
		"XX??????????????????????????????????????????????????????????"
		"\x05\x00\x00\x00??????";
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(header_data),
		std::size(header_data));

	header.deserialize(buf, false);
	EXPECT_EQ(header.validate_magic(),
		dos_header_errc::invalid_dos_header_signature);
	EXPECT_EQ(header.validate_e_lfanew(),
		dos_header_errc::unaligned_e_lfanew);
	EXPECT_THROW(header.validate({}).throw_on_error(), pe_error);
}

TEST(DosHeaderTests, DeserializeErrorTest)
{
	dos_header header;
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(""), 0u);
	expect_throw_pe_error(
		[&header, &buf] { header.deserialize(buf, false); },
		dos_header_errc::unable_to_read_dos_header);
}
