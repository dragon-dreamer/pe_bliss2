#include "pe_bliss2/dotnet/dotnet_directory.h"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_container_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::dotnet;

namespace
{
constexpr std::array metadata_runtime_version{
	std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, std::byte{'d'},
	std::byte{'e'}, std::byte{}, std::byte{}, std::byte{},
};

constexpr std::array metadata_header_data{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //signature
	std::byte{1}, std::byte{}, //major_version
	std::byte{1}, std::byte{}, //minor_version
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //reserved
	std::byte{metadata_runtime_version.size()},
	std::byte{}, std::byte{}, std::byte{} //version_length
};

constexpr std::array metadata_runtime_version_unaligned{
	std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, std::byte{'d'},
	std::byte{'e'}, std::byte{}, std::byte{},
};

constexpr std::array metadata_header_data_unaligned{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //signature
	std::byte{1}, std::byte{}, //major_version
	std::byte{1}, std::byte{}, //minor_version
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //reserved
	std::byte{metadata_runtime_version_unaligned.size()},
	std::byte{}, std::byte{}, std::byte{} //version_length
};

constexpr std::array metadata_footer_no_streams{
	std::byte{1}, std::byte{}, //flags
	std::byte{}, std::byte{}, //stream_count
};

constexpr std::array metadata_footer{
	std::byte{1}, std::byte{}, //flags
	std::byte{3}, std::byte{}, //stream_count
};

constexpr std::array stream0{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //offset
	std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7}, //size
	std::byte{'a'}, std::byte{'b'}, std::byte{}, std::byte{}, //name
};

//maps to metadata runtime version string
constexpr std::array stream1{
	std::byte{metadata_header_data.size()},
	std::byte{}, std::byte{}, std::byte{}, //offset
	std::byte{metadata_runtime_version.size()},
	std::byte{}, std::byte{}, std::byte{}, //size
	std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, std::byte{'d'},
	std::byte{'e'}, std::byte{}, std::byte{}, std::byte{}, //name
};

constexpr std::array stream2{
	std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, //offset
};

template<typename Array>
void append_array(std::vector<std::byte>& data, const Array& arr)
{
	data.insert(data.end(), arr.begin(), arr.end());
}

template<typename... Arrays>
buffers::input_buffer_ptr create_buffer(const Arrays&... arrays)
{
	auto buf = std::make_shared<buffers::input_container_buffer>();
	(..., append_array(buf->get_container(), arrays));
	return buf;
}
} //namespace

TEST(DotnetDirectoryTests, ParseMetadataHeaderEmpty)
{
	auto buf = std::make_shared<buffers::input_container_buffer>();
	auto result = parse_metadata_header(buf);
	expect_contains_errors(result,
		dotnet_directory_errc::unable_to_deserialize_header);
}

TEST(DotnetDirectoryTests, ParseMetadataHeaderOnly)
{
	auto result = parse_metadata_header(create_buffer(metadata_header_data));
	expect_contains_errors(result,
		dotnet_directory_errc::unable_to_deserialize_runtime_version);
	EXPECT_EQ(result.get_descriptor()->version_length,
		metadata_runtime_version.size());
}

TEST(DotnetDirectoryTests, ParseMetadataHeaderAndVersion)
{
	auto result = parse_metadata_header(create_buffer(metadata_header_data,
		metadata_runtime_version));
	expect_contains_errors(result,
		dotnet_directory_errc::unable_to_deserialize_footer);
	EXPECT_EQ(result.get_runtime_version().value(), "abcde");
}

TEST(DotnetDirectoryTests, ParseMetadataNoStreams)
{
	auto result = parse_metadata_header(create_buffer(metadata_header_data,
		metadata_runtime_version, metadata_footer_no_streams));
	expect_contains_errors(result);
	EXPECT_EQ(result.get_descriptor_footer()->flags, 1u);
}

TEST(DotnetDirectoryTests, ParseMetadataNoStreamsUnaligned)
{
	auto result = parse_metadata_header(create_buffer(
		metadata_header_data_unaligned,
		metadata_runtime_version_unaligned,
		metadata_footer_no_streams));
	expect_contains_errors(result,
		dotnet_directory_errc::version_length_not_aligned);
	EXPECT_EQ(result.get_descriptor()->version_length,
		metadata_runtime_version_unaligned.size());
	EXPECT_EQ(result.get_descriptor_footer()->flags, 1u);
	EXPECT_EQ(result.get_runtime_version().value(), "abcde");
	EXPECT_TRUE(result.get_streams().empty());
}

TEST(DotnetDirectoryTests, ParseMetadataStreams)
{
	auto result = parse_metadata_header(create_buffer(metadata_header_data,
		metadata_runtime_version, metadata_footer, stream0, stream1, stream2));
	expect_contains_errors(result);

	ASSERT_EQ(result.get_streams().size(), 3u);

	const auto& s0 = result.get_streams()[0];
	expect_contains_errors(s0,
		dotnet_directory_errc::unable_to_deserialize_stream_data);
	EXPECT_EQ(s0.get_descriptor()->size, 0x07060504u);
	EXPECT_EQ(s0.get_data().size(), 0u);
	EXPECT_EQ(s0.get_name().value(), "ab");

	auto& s1 = result.get_streams()[1];
	expect_contains_errors(s1);
	EXPECT_FALSE(s1.get_data().is_copied());
	EXPECT_EQ(s1.get_data().copied_data(),
		std::vector(metadata_runtime_version.begin(),
			metadata_runtime_version.end()));
	EXPECT_EQ(s1.get_name().value(), "abcde");

	const auto& s2 = result.get_streams()[2];
	expect_contains_errors(s2,
		dotnet_directory_errc::unable_to_deserialize_stream_header);
}

TEST(DotnetDirectoryTests, ParseMetadataStreamsSkip)
{
	auto result = parse_metadata_header(create_buffer(metadata_header_data,
		metadata_runtime_version, metadata_footer, stream0, stream1, stream2),
		{ .deserialize_streams = false });
	expect_contains_errors(result);
	EXPECT_TRUE(result.get_streams().empty());
}

TEST(DotnetDirectoryTests, ParseMetadataStreamsCopyMemory)
{
	auto result = parse_metadata_header(create_buffer(metadata_header_data,
		metadata_runtime_version, metadata_footer, stream0, stream1, stream2),
		{ .copy_stream_memory = true });
	expect_contains_errors(result);

	ASSERT_EQ(result.get_streams().size(), 3u);

	const auto& s1 = result.get_streams()[1];
	expect_contains_errors(s1);
	ASSERT_TRUE(s1.get_data().is_copied());
	EXPECT_EQ(s1.get_data().copied_data(),
		std::vector(metadata_runtime_version.begin(),
			metadata_runtime_version.end()));
	EXPECT_EQ(s1.get_name().value(), "abcde");
}

TEST(DotnetDirectoryTests, ParseMetadataHeaderAndVersionLimit)
{
	auto result = parse_metadata_header(create_buffer(metadata_header_data,
		metadata_runtime_version), { .max_runtime_version_length = 3u });
	expect_contains_errors(result,
		dotnet_directory_errc::too_long_runtime_version_string);
	EXPECT_EQ(result.get_descriptor()->version_length,
		metadata_runtime_version.size());
	EXPECT_TRUE(result.get_runtime_version().value().empty());
}

TEST(DotnetDirectoryTests, ParseMetadataStreamsVirtualError)
{
	auto buf = create_buffer(metadata_header_data,
		metadata_runtime_version, metadata_footer, stream0, stream1, stream2);
	buf = std::make_shared<buffers::input_virtual_buffer>(std::move(buf), 50u);
	auto result = parse_metadata_header(buf);
	expect_contains_errors(result);

	ASSERT_EQ(result.get_streams().size(), 3u);

	const auto& s2 = result.get_streams()[2];
	expect_contains_errors(s2,
		dotnet_directory_errc::unable_to_deserialize_stream_header);
}

TEST(DotnetDirectoryTests, ParseMetadataStreamsVirtual)
{
	auto buf = create_buffer(metadata_header_data,
		metadata_runtime_version, metadata_footer, stream0, stream1, stream2);
	buf = std::make_shared<buffers::input_virtual_buffer>(std::move(buf), 50u);
	auto result = parse_metadata_header(buf, { .allow_virtual_data = true });
	expect_contains_errors(result);

	ASSERT_EQ(result.get_streams().size(), 3u);

	const auto& s2 = result.get_streams()[2];
	expect_contains_errors(s2,
		dotnet_directory_errc::unable_to_deserialize_stream_data);
	EXPECT_EQ(s2.get_descriptor()->offset, 0x05040302u);
}
