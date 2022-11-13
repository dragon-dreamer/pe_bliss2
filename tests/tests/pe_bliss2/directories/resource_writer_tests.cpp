#include "pe_bliss2/resources/resource_writer.h"

#include "gtest/gtest.h"

#include "pe_bliss2/resources/resource_reader.h"

using namespace pe_bliss::resources;

TEST(ResourceWriterTests, TryEmplaceById)
{
	resource_directory root;
	auto& data = try_emplace_resource_data_by_id(root, resource_type::bitmap, 123u, 456u);
	auto& data2 = try_emplace_resource_data_by_id(root, resource_type::bitmap, 123u, 456u);
	ASSERT_EQ(&data, &data2);

	auto& raw_data = get_resource_data_by_id(root, resource_type::bitmap, 123u, 456u);
	ASSERT_EQ(&raw_data, &data.get_raw_data());
}

TEST(ResourceWriterTests, TryEmplaceByName)
{
	resource_directory root;
	auto& data = try_emplace_resource_data_by_name(root, resource_type::bitmap, u"abc", 456u);
	auto& data2 = try_emplace_resource_data_by_name(root, resource_type::bitmap, u"abc", 456u);
	ASSERT_EQ(&data, &data2);

	auto& raw_data = get_resource_data_by_name(root, resource_type::bitmap, u"abc", 456u);
	ASSERT_EQ(&raw_data, &data.get_raw_data());
}
