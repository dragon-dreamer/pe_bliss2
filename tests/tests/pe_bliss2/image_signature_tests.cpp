#include "gtest/gtest.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include "buffers/input_memory_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/output_memory_buffer.h"

#include "pe_bliss2/core/image_signature.h"
#include "pe_bliss2/core/image_signature_errc.h"
#include "pe_bliss2/core/image_signature_validator.h"
#include "pe_bliss2/pe_error.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::core;

TEST(ImageSignatureTests, EmptyTest)
{
	image_signature signature;
	EXPECT_EQ(signature.get_signature(), 0u);
	signature.set_signature(123u);
	EXPECT_EQ(signature.get_signature(), 123u);
	EXPECT_EQ(signature.get_descriptor().get(), 123u);
}

TEST(ImageSignatureTests, DeserializeSerializeTest)
{
	static constexpr std::byte data[]{
		std::byte{'P'}, std::byte{'E'}, std::byte{}
	};

	auto buf = std::make_shared<buffers::input_memory_buffer>(data, sizeof(data));
	buffers::input_virtual_buffer virtual_buf(buf, 1u);
	buffers::input_buffer_stateful_wrapper_ref ref(virtual_buf);

	image_signature signature;
	expect_throw_pe_error([&signature, &ref] {
		signature.deserialize(ref, false);
	}, image_signature_errc::unable_to_read_pe_signature);
	ref.set_rpos(0u);
	ASSERT_NO_THROW(signature.deserialize(ref, true));
	EXPECT_EQ(signature.get_signature(), image_signature::pe_signature);

	std::vector<std::byte> outdata;
	buffers::output_memory_buffer outbuf(outdata);
	ASSERT_NO_THROW(signature.serialize(outbuf, false));
	ASSERT_EQ(outdata.size(), buf->size());
	EXPECT_TRUE(std::equal(outdata.cbegin(), outdata.cend(), data));
}

TEST(ImageSignatureTests, ValidateTest)
{
	image_signature signature;
	EXPECT_EQ(validate(signature), image_signature_errc::invalid_pe_signature);
	signature.set_signature(image_signature::pe_signature);
	EXPECT_NO_THROW(validate(signature).throw_on_error());
}
