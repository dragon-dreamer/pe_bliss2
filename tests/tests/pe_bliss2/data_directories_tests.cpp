#include "gtest/gtest.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

#include "buffers/input_memory_buffer.h"
#include "buffers/output_memory_buffer.h"

#include "pe_bliss2/data_directories.h"

#include "tests/tests/pe_bliss2/output_buffer_mock.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

using namespace pe_bliss;

TEST(DataDirectoriesTests, EmptyTest)
{
	data_directories dirs;

	EXPECT_EQ(dirs.size(), 0u);

	EXPECT_FALSE(dirs.has_directory(data_directories::directory_type::exports));
	EXPECT_FALSE(dirs.has_directory(data_directories::directory_type::com_descriptor));

	EXPECT_FALSE(dirs.has_nonempty_directory(data_directories::directory_type::exports));
	EXPECT_FALSE(dirs.has_nonempty_directory(data_directories::directory_type::com_descriptor));

	EXPECT_THROW((void)dirs
		.get_directory(data_directories::directory_type::exports),
		std::out_of_range);
	EXPECT_THROW((void)std::as_const(dirs)
		.get_directory(data_directories::directory_type::exports),
		std::out_of_range);
	EXPECT_TRUE(dirs.get_directories().empty());

	EXPECT_NO_THROW(dirs.remove_directory(data_directories::directory_type::com_descriptor));
	EXPECT_FALSE(dirs.has_directory(data_directories::directory_type::com_descriptor));

	EXPECT_FALSE(dirs.has_imports());
	EXPECT_FALSE(dirs.is_dotnet());

	EXPECT_EQ(dirs.strip_data_directories(5u), 0u);
	EXPECT_EQ(dirs.strip_data_directories(0u), 0u);

	::testing::StrictMock<output_buffer_mock> buf;
	EXPECT_NO_THROW(dirs.serialize(buf, true));
}

TEST(DataDirectoriesTests, DeserializeSerializeTest)
{
	static constexpr std::size_t dir_count = 5u;
	static constexpr std::size_t virtual_byte_count = 3;
	static constexpr const char data[]{
		"\x10\x00\x00\x00" "\x05\x00\x00\x00" //exports
		"\x20\x00\x00\x00" "\x06\x00\x00\x00" //imports
		"\x30\x00\x00\x00" "\x07\x00\x00\x00" //resource
		"\x00\x00\x00\x00" "\xff\x00\x00\x00" //exception
		"\x50\x00\x00\x00" "\x09"             //security
	};

	buffers::input_memory_buffer buffer(
		reinterpret_cast<const std::byte*>(data),
		std::size(data) - 1u);

	data_directories dirs;
	expect_throw_pe_error([&] { dirs.deserialize(buffer, dir_count, false); },
		data_directories_errc::unable_to_read_data_directory);
	dirs.get_directories().clear();

	buffer.set_rpos(0);
	EXPECT_NO_THROW(dirs.deserialize(buffer, dir_count, true));

	EXPECT_EQ(dirs.size(), dir_count);

	EXPECT_TRUE(dirs.has_directory(data_directories::directory_type::exports));
	EXPECT_TRUE(dirs.has_directory(data_directories::directory_type::security));
	EXPECT_TRUE(dirs.has_directory(data_directories::directory_type::exception));
	EXPECT_FALSE(dirs.has_directory(data_directories::directory_type::basereloc));

	EXPECT_TRUE(dirs.has_nonempty_directory(data_directories::directory_type::exports));
	EXPECT_TRUE(dirs.has_nonempty_directory(data_directories::directory_type::security));
	EXPECT_FALSE(dirs.has_nonempty_directory(data_directories::directory_type::exception));

	EXPECT_TRUE(dirs.has_exports());
	EXPECT_TRUE(dirs.has_security());
	EXPECT_FALSE(dirs.has_reloc());
	EXPECT_FALSE(dirs.is_dotnet());

	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::exports)->size, 0x05u);
	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::exports)->virtual_address,
		0x10u);
	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::exports)
		.get_state().buffer_pos(), 0u);
	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::exports)
		.get_state().relative_offset(), 0u);
	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::exports)
		.get_state().absolute_offset(), 0u);

	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::exception)->size, 0xffu);
	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::exception)->virtual_address,
		0u);
	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::exception)
		.get_state().buffer_pos(), 24u);
	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::exception)
		.get_state().relative_offset(), 24u);
	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::exception)
		.get_state().absolute_offset(), 24u);

	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::security)->size, 0x09u);
	EXPECT_EQ(dirs.get_directory(data_directories::directory_type::security)->virtual_address,
		0x50u);

	EXPECT_THROW((void)dirs
		.get_directory(data_directories::directory_type::basereloc),
		std::out_of_range);

	{
		std::vector<std::byte> outdata;
		buffers::output_memory_buffer outbuf(outdata);
		EXPECT_NO_THROW(dirs.serialize(outbuf, false));
		EXPECT_EQ(outdata.size(), std::size(data) - 1u);
		EXPECT_TRUE(std::equal(std::cbegin(outdata), std::cend(outdata),
			reinterpret_cast<const std::byte*>(data)));
	}

	{
		std::vector<std::byte> outdata;
		buffers::output_memory_buffer outbuf(outdata);
		EXPECT_NO_THROW(dirs.serialize(outbuf, true));
		EXPECT_EQ(outdata.size(), std::size(data) - 1u
			+ 3u /* remaining virtual bytes */);
		EXPECT_TRUE(std::equal(std::cbegin(outdata), std::cend(outdata) - 3u,
			reinterpret_cast<const std::byte*>(data)));
		EXPECT_EQ(std::count(std::cbegin(outdata) + std::size(data) - 1u,
			std::cend(outdata), std::byte{}), virtual_byte_count);
	}
	
	EXPECT_FALSE(dirs.get_directories().empty());
}

TEST(DataDirectoriesTests, StripTest1)
{
	data_directories dirs;
	dirs.set_size(7u);
	EXPECT_EQ(dirs.size(), 7u);
	EXPECT_EQ(dirs.strip_data_directories(8u), 7u);
	EXPECT_EQ(dirs.strip_data_directories(7u), 7u);
	EXPECT_EQ(dirs.strip_data_directories(0u), 0u);
	EXPECT_EQ(dirs.size(), 0u);
}

TEST(DataDirectoriesTests, StripTest2)
{
	data_directories dirs;
	dirs.set_size(7u);
	dirs.get_directories()[3u]->virtual_address = 123u;
	EXPECT_EQ(dirs.size(), 7u);
	EXPECT_EQ(dirs.strip_data_directories(8u), 7u);
	EXPECT_EQ(dirs.strip_data_directories(7u), 7u);
	EXPECT_EQ(dirs.strip_data_directories(6u), 6u);
	EXPECT_EQ(dirs.size(), 6u);
	EXPECT_EQ(dirs.strip_data_directories(0u), 4u);
	EXPECT_EQ(dirs.size(), 4u);
}
