#include <cstddef>
#include <string_view>

#include "gtest/gtest.h"

#include "buffers/input_memory_buffer.h"
#include "pe_bliss2/resources/pugixml_manifest_accessor.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::resources;

namespace
{
constexpr std::string_view empty_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly />)"
);

constexpr std::string_view manifest_no_decl(
	R"(<assembly />)"
);

constexpr std::string_view double_decl(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly />)"
);

constexpr std::string_view double_root(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly />)"
	R"(<assembly />)"
);

constexpr std::string_view xml_error(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly>)"
);

constexpr std::string_view duplicate_ns(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns:x="a" xmlns:x="b" />)"
);

constexpr std::string_view duplicate_default_ns(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="a" xmlns="b" />)"
);

constexpr std::string_view empty_ns_name(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns:="b" />)"
);

constexpr std::string_view unknown_alias(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly unknown:attr="b" />)"
);

constexpr std::string_view empty_ns_value(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns:p="" />)"
);

constexpr std::string_view nested(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="xmlns1ns" xmlns:xmlns2="xmlns2ns">)"
	R"(<xmlns2:nested xmlns="xmlns3ns">)"
	R"(<tag xmlns2:attr="val1" xmlns:xmlns2="xmlns2ovr"/>)"
	R"(</xmlns2:nested>)"
	R"(<nested>)"
	R"(<tag attr="val2"/>)"
	R"(</nested>)"
	R"(<nested>nestedValue</nested>)"
	R"(</assembly>)"
);

buffers::input_buffer_ptr buf_from_string(std::string_view str)
{
	const auto* ptr = reinterpret_cast<const std::byte*>(str.data());
	return std::make_shared<buffers::input_memory_buffer>(ptr, str.size());
}

void expect_invalid_xml(std::string_view xml)
{
	auto accessor = pugixml::parse_manifest(buf_from_string(xml));
	expect_contains_errors(accessor->get_errors(), manifest_loader_errc::invalid_xml);
	EXPECT_EQ(accessor->get_root(), nullptr) << xml;
}
} //namespace

TEST(PugixmlManifestAccessorTests, Empty)
{
	auto accessor = pugixml::parse_manifest(buf_from_string(empty_manifest));
	expect_contains_errors(accessor->get_errors());

	const auto* root = accessor->get_root();
	ASSERT_NE(root, nullptr);
	EXPECT_EQ(root->get_child_count(), 1u);
	auto assembly_it = root->get_iterator();
	const auto* assembly = assembly_it->first_child("", "assembly");
	ASSERT_NE(assembly, nullptr);
	EXPECT_EQ(assembly->get_child_count(), 0u);
	EXPECT_EQ(assembly->get_name(), "assembly");
	EXPECT_EQ(assembly->get_namespace(), "");
	EXPECT_EQ(assembly->get_node_index(), 0u);
	EXPECT_EQ(assembly->get_value(), "");
	EXPECT_EQ(assembly_it->next_child(), nullptr);
}

TEST(PugixmlManifestAccessorTests, Invalid)
{
	auto accessor = pugixml::parse_manifest(buf_from_string(manifest_no_decl));
	expect_contains_errors(accessor->get_errors(), manifest_loader_errc::absent_declaration);
	EXPECT_NE(accessor->get_root(), nullptr) << manifest_no_decl;

	expect_invalid_xml(double_decl);
	expect_invalid_xml(double_root);
	expect_invalid_xml("");
	expect_invalid_xml(xml_error);
	expect_invalid_xml(duplicate_ns);
	expect_invalid_xml(duplicate_default_ns);
	expect_invalid_xml(empty_ns_name);
	expect_invalid_xml(empty_ns_value);
	expect_invalid_xml(unknown_alias);
}

TEST(PugixmlManifestAccessorTests, Nested)
{
	auto accessor = pugixml::parse_manifest(buf_from_string(nested));
	expect_contains_errors(accessor->get_errors());

	const auto* root = accessor->get_root();
	ASSERT_NE(root, nullptr);
	const auto* assembly = root->get_iterator()->first_child("xmlns1ns", "assembly");
	ASSERT_NE(assembly, nullptr);
	EXPECT_EQ(assembly->get_child_count(), 3u);

	auto nested_it = assembly->get_iterator();
	const auto* xmlns1_nested = nested_it->first_child("xmlns1ns", "nested");
	ASSERT_NE(xmlns1_nested, nullptr);
	EXPECT_EQ(xmlns1_nested->get_child_count(), 1u);
	EXPECT_EQ(xmlns1_nested->get_value(), "");
	EXPECT_EQ(xmlns1_nested->get_name(), "nested");
	EXPECT_EQ(xmlns1_nested->get_namespace(), "xmlns1ns");

	auto xmlns1_nested1_it = xmlns1_nested->get_iterator();

	xmlns1_nested = nested_it->next_child();
	ASSERT_NE(xmlns1_nested, nullptr);
	EXPECT_EQ(xmlns1_nested->get_child_count(), 0u);
	EXPECT_EQ(xmlns1_nested->get_value(), "nestedValue");
	EXPECT_EQ(xmlns1_nested->get_name(), "nested");
	EXPECT_EQ(xmlns1_nested->get_namespace(), "xmlns1ns");

	const auto* xmlns1_tag = xmlns1_nested1_it->first_child("xmlns1ns", "tag");
	ASSERT_NE(xmlns1_tag, nullptr);
	EXPECT_EQ(xmlns1_tag->get_child_count(), 0u);
	EXPECT_EQ(xmlns1_tag->get_value(), "");
	EXPECT_EQ(xmlns1_tag->get_name(), "tag");
	EXPECT_EQ(xmlns1_tag->get_namespace(), "xmlns1ns");

	EXPECT_EQ(xmlns1_nested1_it->next_child(), nullptr);

	const auto* tag_attr = xmlns1_tag->get_attribute("xmlns1ns", "attr");
	ASSERT_NE(tag_attr, nullptr);
	EXPECT_EQ(tag_attr->get_name(), "attr");
	EXPECT_EQ(tag_attr->get_namespace(), "xmlns1ns");
	EXPECT_EQ(tag_attr->get_value(), "val2");

	EXPECT_EQ(nested_it->next_child(), nullptr);

	const auto* xmlns2_nested = nested_it->first_child("xmlns2ns", "nested");
	ASSERT_NE(xmlns2_nested, nullptr);
	EXPECT_EQ(xmlns2_nested->get_child_count(), 1u);
	EXPECT_EQ(xmlns2_nested->get_value(), "");
	EXPECT_EQ(xmlns2_nested->get_name(), "nested");
	EXPECT_EQ(xmlns2_nested->get_namespace(), "xmlns2ns");

	auto xmlns2_nested_it = xmlns2_nested->get_iterator();
	const auto* xmlns3_tag = xmlns2_nested_it->first_child("xmlns3ns", "tag");
	ASSERT_NE(xmlns3_tag, nullptr);
	EXPECT_EQ(xmlns3_tag->get_child_count(), 0u);
	EXPECT_EQ(xmlns3_tag->get_value(), "");
	EXPECT_EQ(xmlns3_tag->get_name(), "tag");
	EXPECT_EQ(xmlns3_tag->get_namespace(), "xmlns3ns");

	EXPECT_EQ(xmlns2_nested_it->next_child(), nullptr);

	tag_attr = xmlns3_tag->get_attribute("xmlns2ovr", "attr");
	ASSERT_NE(tag_attr, nullptr);
	EXPECT_EQ(tag_attr->get_name(), "attr");
	EXPECT_EQ(tag_attr->get_namespace(), "xmlns2ovr");
	EXPECT_EQ(tag_attr->get_value(), "val1");

	EXPECT_EQ(xmlns3_tag->get_attribute("xmlns3ns", "attr"), nullptr);
}
