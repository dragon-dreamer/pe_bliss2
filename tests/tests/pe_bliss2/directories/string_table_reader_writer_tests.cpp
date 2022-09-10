#include "gtest/gtest.h"

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_container_buffer.h"
#include "buffers/output_memory_buffer.h"
#include "pe_bliss2/resources/resource_reader_errc.h"
#include "pe_bliss2/resources/string_table.h"
#include "pe_bliss2/resources/string_table_reader.h"
#include "pe_bliss2/resources/string_table_writer.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::resources;

TEST(StringTableReaderTests, ReadEmpty)
{
	buffers::input_container_buffer buf;
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	string_table table;
	expect_throw_pe_error([&ref] {
		(void)string_table_from_resource(ref, { .string_table_id = 5u });
	}, resource_reader_errc::buffer_read_error);
}

TEST(StringTableReaderTests, ReadValid)
{
	buffers::input_container_buffer buf;
	buf.get_container() = {
		std::byte{}, std::byte{}, // string 0
		std::byte{}, std::byte{}, // string 1
		std::byte{}, std::byte{}, // string 2
		std::byte{}, std::byte{}, // string 3
		std::byte{}, std::byte{}, // string 4
		std::byte{}, std::byte{}, // string 5
		std::byte{}, std::byte{}, // string 6
		std::byte{}, std::byte{}, // string 7
		std::byte{}, std::byte{}, // string 8
		std::byte{}, std::byte{}, // string 9
		std::byte{}, std::byte{}, // string 10
		std::byte{}, std::byte{}, // string 11
		std::byte{}, std::byte{}, // string 12
		std::byte{2u}, std::byte{}, // string 13
		std::byte{'a'}, std::byte{}, std::byte{'b'}, std::byte{},
		std::byte{1u}, std::byte{}, // string 14
		std::byte{'x'}, std::byte{},
		std::byte{}, std::byte{}, // string 15
	};

	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	string_table table;
	EXPECT_NO_THROW(table = string_table_from_resource(ref, { .string_table_id = 5u }));
	EXPECT_EQ(table.get_id(), 5u);
	EXPECT_TRUE(table.is_valid_id());
	int index = 0;
	for (const auto& str : table.get_list())
	{
		if (index == 13)
		{
			EXPECT_EQ(str.value(), u"ab");
		}
		else if (index == 14)
		{
			EXPECT_EQ(str.value(), u"x");
		}
		else
		{
			EXPECT_TRUE(str.value().empty());
		}
		++index;
	}

	buffers::output_memory_buffer::buffer_type vec;
	buffers::output_memory_buffer out(vec);
	EXPECT_NO_THROW(write_string_table(table, out));
	EXPECT_EQ(buf.get_container(), vec);
}
