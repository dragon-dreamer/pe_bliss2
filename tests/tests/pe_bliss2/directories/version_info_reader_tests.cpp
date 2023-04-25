#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>

#include "buffers/input_memory_buffer.h"
#include "buffers/input_buffer_section.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/ref_buffer.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/resources/version_info_block.h"
#include "pe_bliss2/resources/version_info_reader.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

namespace
{
std::array version_info_data{
	//root block
	std::byte{20 + 16 + 10}, std::byte{}, //length
	std::byte{2}, std::byte{}, //value_length
	std::byte{}, std::byte{}, //type: binary
	//key
	std::byte{'a'}, std::byte{}, std::byte{'b'}, std::byte{},
	std::byte{'c'}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{}, //padding
	std::byte{0xau}, std::byte{0xbu}, //binary value

	std::byte{}, std::byte{}, //padding

	//child block 1
	std::byte{16 + 10}, std::byte{}, //length
	std::byte{4}, std::byte{}, //value_length
	std::byte{1}, std::byte{}, //type: text
	std::byte{}, std::byte{}, //key
	//text value
	std::byte{'x'}, std::byte{}, std::byte{'y'}, std::byte{},
	std::byte{'z'}, std::byte{}, std::byte{}, std::byte{},

	//grandchild block 1
	std::byte{10}, std::byte{}, //length
	std::byte{}, std::byte{}, //value_length
	std::byte{}, std::byte{}, //type: binary
	std::byte{'g'}, std::byte{}, std::byte{}, std::byte{}, //key
};

auto create_version_info_buffer()
{
	return std::make_shared<buffers::input_memory_buffer>(
		version_info_data.data(), version_info_data.size());
}

void validate_version_info(version_info_block_details& root,
	bool copy_value_memory, std::uint32_t depth_limit = 0xffu,
	bool last_child_cut = false)
{
	EXPECT_EQ(root.get_descriptor()->value_length, 2u);
	ASSERT_TRUE(root.get_key().has_value());
	EXPECT_EQ(root.get_key()->value(), u"abc");
	auto* root_value_buf = std::get_if<buffers::ref_buffer>(&root.get_value());
	ASSERT_NE(root_value_buf, nullptr);
	EXPECT_EQ(root_value_buf->is_copied(), copy_value_memory);
	EXPECT_EQ(root_value_buf->copied_data(),
		(std::vector{ std::byte{0xau}, std::byte{0xbu} }));

	if (last_child_cut)
	{
		expect_contains_errors(root,
			version_info_reader_errc::child_read_error,
			version_info_reader_errc::excessive_data_in_buffer);
		ASSERT_EQ(root.get_children().size(), 0u);
		return;
	}

	if (depth_limit > 0)
	{
		expect_contains_errors(root);
		ASSERT_EQ(root.get_children().size(), 1u);
		const auto& child1 = root.get_children()[0];
		EXPECT_EQ(child1.get_descriptor()->value_length, 4u);
		ASSERT_TRUE(child1.get_key().has_value());
		EXPECT_TRUE(child1.get_key()->value().empty());
		auto* child1_value_buf = std::get_if<
			pe_bliss::packed_utf16_c_string>(&child1.get_value());
		ASSERT_NE(child1_value_buf, nullptr);
		EXPECT_EQ(child1_value_buf->value(), u"xyz");

		if (depth_limit > 1)
		{
			expect_contains_errors(child1);
			ASSERT_EQ(child1.get_children().size(), 1u);
			const auto& grandchild1 = child1.get_children()[0];
			expect_contains_errors(grandchild1);
			EXPECT_EQ(grandchild1.get_descriptor()->value_length, 0u);
			ASSERT_TRUE(grandchild1.get_key().has_value());
			EXPECT_EQ(grandchild1.get_key()->value(), u"g");
			ASSERT_TRUE(std::holds_alternative<
				buffers::ref_buffer>(grandchild1.get_value()));
			EXPECT_TRUE(grandchild1.get_children().empty());
		}
		else
		{
			ASSERT_EQ(child1.get_children().size(), 0u);
			expect_contains_errors(child1,
				version_info_reader_errc::block_tree_is_too_deep);
		}
	}
	else
	{
		ASSERT_EQ(root.get_children().size(), 0u);
		expect_contains_errors(root,
			version_info_reader_errc::block_tree_is_too_deep);
	}
}
} //namespace

TEST(VersionInfoReaderTests, ValidInfo)
{
	auto buf = create_version_info_buffer();
	buffers::input_buffer_stateful_wrapper wrapper(buf);

	auto root = version_info_from_resource(wrapper);
	validate_version_info(root, false);
}

TEST(VersionInfoReaderTests, ValidInfoCopyBuffers)
{
	auto buf = create_version_info_buffer();
	buffers::input_buffer_stateful_wrapper wrapper(buf);

	auto root = version_info_from_resource(wrapper,
		{ .copy_value_memory = true });
	validate_version_info(root, true);
}

TEST(VersionInfoReaderTests, ValidInfoDepthLimit)
{
	auto buf = create_version_info_buffer();

	{
		buffers::input_buffer_stateful_wrapper wrapper(buf);
		auto root = version_info_from_resource(wrapper,
			{ .max_depth = 1u });
		validate_version_info(root, false, 1u);
	}

	{
		buffers::input_buffer_stateful_wrapper wrapper(buf);
		auto root = version_info_from_resource(wrapper,
			{ .max_depth = 0u });
		validate_version_info(root, false, 0u);
	}
}

TEST(VersionInfoReaderTests, CutInfo)
{
	auto buf = create_version_info_buffer();
	auto cut_buf = buffers::reduce(buf, 0u, buf->size() - 1u);
	buffers::input_buffer_stateful_wrapper wrapper(cut_buf);

	auto root = version_info_from_resource(wrapper);
	validate_version_info(root, false, 0xffu, true);
}

TEST(VersionInfoReaderTests, VirtualInfo)
{
	auto buf = create_version_info_buffer();
	//Last 3 bytes are virtual
	auto cut_buf = buffers::reduce(buf, 0u, buf->size() - 3u);
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(cut_buf, 3u);
	buffers::input_buffer_stateful_wrapper wrapper(virtual_buf);

	auto root = version_info_from_resource(wrapper,
		{ .allow_virtual_data = true });
	validate_version_info(root, false);
}

TEST(VersionInfoReaderTests, VirtualInfoError)
{
	auto buf = create_version_info_buffer();
	//Last 3 bytes are virtual
	auto cut_buf = buffers::reduce(buf, 0u, buf->size() - 3u);
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(cut_buf, 3u);
	buffers::input_buffer_stateful_wrapper wrapper(virtual_buf);

	auto root = version_info_from_resource(wrapper);
	expect_contains_errors(root,
		version_info_reader_errc::excessive_data_in_buffer);
	ASSERT_EQ(root.get_children().size(), 1u);
	const auto& child1 = root.get_children()[0];
	expect_contains_errors(child1);
	ASSERT_EQ(child1.get_children().size(), 1u);
	const auto& grandchild1 = child1.get_children()[0];
	expect_contains_errors(grandchild1, version_info_reader_errc::key_read_error);
}
