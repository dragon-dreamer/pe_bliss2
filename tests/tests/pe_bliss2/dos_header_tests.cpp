#include "gtest/gtest.h"

#include <iterator>

#include "buffers/input_memory_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"

#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/dos/dos_header_errc.h"
#include "pe_bliss2/dos/dos_header_validator.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/pe_error.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::dos;

TEST(DosHeaderTests, EmptyTest)
{
	dos_header header;
	EXPECT_EQ(validate_magic(header),
		dos_header_errc::invalid_dos_header_signature);
	EXPECT_EQ(validate_e_lfanew(header),
		dos_header_errc::invalid_e_lfanew);
	error_list errs;
	EXPECT_FALSE(validate(header, {}, errs));
	expect_contains_errors(errs,
		dos_header_errc::invalid_dos_header_signature,
		dos_header_errc::invalid_e_lfanew);

	errs.clear_errors();
	EXPECT_TRUE(validate(header, {
		.validate_e_lfanew = false,
		.validate_magic = false
	}, errs));
	EXPECT_FALSE(errs.has_errors());
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

	buffers::input_buffer_stateful_wrapper_ref wrapper(buf);
	header.deserialize(wrapper, false);
	EXPECT_NO_THROW(validate_magic(header).throw_on_error());
	EXPECT_NO_THROW(validate_e_lfanew(header).throw_on_error());
	error_list errs;
	EXPECT_TRUE(validate(header, {}, errs));
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

	buffers::input_buffer_stateful_wrapper_ref wrapper(buf);
	header.deserialize(wrapper, false);
	EXPECT_EQ(validate_magic(header),
		dos_header_errc::invalid_dos_header_signature);
	EXPECT_EQ(validate_e_lfanew(header),
		dos_header_errc::unaligned_e_lfanew);
	error_list errs;
	EXPECT_FALSE(validate(header, {}, errs));
	expect_contains_errors(errs,
		dos_header_errc::invalid_dos_header_signature,
		dos_header_errc::unaligned_e_lfanew);
}

TEST(DosHeaderTests, DeserializeErrorTest)
{
	dos_header header;
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(""), 0u);
	buffers::input_buffer_stateful_wrapper_ref wrapper(buf);
	expect_throw_pe_error(
		[&header, &wrapper] { header.deserialize(wrapper, false); },
		dos_header_errc::unable_to_read_dos_header);
}
