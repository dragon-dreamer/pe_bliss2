#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "buffers/input_buffer_section.h"
#include "buffers/input_container_buffer.h"
#include "buffers/input_virtual_buffer.h"

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/pe_types.h"

#include "tests/pe_bliss2/image_helper.h"
#include "tests/pe_bliss2/pe_error_helper.h"

namespace
{
template<typename Span1, typename Span2>
bool spans_equal(Span1 span1, Span2 span2)
{
	return span1.size() == span2.size()
		&& span1.data() == span2.data();
}

bool buffers_equal(const buffers::input_buffer_ptr& buf1,
	const buffers::input_buffer_ptr& buf2)
{
	if (buf1->size() != buf2->size())
		return false;
	if (buf1->physical_size() != buf2->physical_size())
		return false;

	std::vector<std::byte> buf1_data;
	buf1_data.resize(buf1->physical_size());
	buf1->read(0u, buf1->physical_size(), buf1_data.data());

	std::vector<std::byte> buf2_data;
	buf2_data.resize(buf2->physical_size());
	buf2->read(0u, buf2->physical_size(), buf2_data.data());

	return buf1_data == buf2_data;
}

enum class function_type
{
	rva,
	va32,
	va64
};

class SectionDataFromRvaFixtureBase : public ::testing::TestWithParam<function_type>
{
public:
	SectionDataFromRvaFixtureBase()
		: instance(create_test_image({}))
	{
		std::uint8_t value = 0;
		for (auto& data : instance.get_section_data_list())
		{
			for (auto& byte : data.copied_data())
				byte = std::byte{ value++ };
		}
	}

public:
	static constexpr std::uint32_t last_section_last_rva = 0x7000u;
	static constexpr std::uint32_t inexistent_rva = 0x8000u;
	static constexpr std::uint32_t second_section_rva = 0x2000u;

protected:
	pe_bliss::image::image instance;
};

class SectionDataFromRvaFullFixture : public SectionDataFromRvaFixtureBase
{
public:
	static decltype(auto) section_data_from_address(pe_bliss::image::image& instance,
		pe_bliss::rva_type rva, bool include_headers)
	{
		switch (GetParam())
		{
		case function_type::va32:
			return section_data_from_va(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()), include_headers);
		case function_type::va64:
			return section_data_from_va(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()), include_headers);
		default: //rva
			return section_data_from_rva(instance, rva, include_headers);
		}
	}

	static decltype(auto) section_data_from_address(
		const pe_bliss::image::image& instance,
		pe_bliss::rva_type rva, bool include_headers, bool allow_virtual_data)
	{
		switch (GetParam())
		{
		case function_type::va32:
			return section_data_from_va(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()), include_headers,
				allow_virtual_data);
		case function_type::va64:
			return section_data_from_va(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()), include_headers,
				allow_virtual_data);
		default: //rva
			return section_data_from_rva(instance, rva, include_headers,
				allow_virtual_data);
		}
	}
};

class SectionDataFromRvaWithSizeFixture : public SectionDataFromRvaFixtureBase
{
public:
	static decltype(auto) section_data_from_address(pe_bliss::image::image& instance,
		pe_bliss::rva_type rva, std::uint32_t data_size, bool include_headers)
	{
		switch (GetParam())
		{
		case function_type::va32:
			return section_data_from_va(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				data_size, include_headers);
		case function_type::va64:
			return section_data_from_va(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				data_size, include_headers);
		default: //rva
			return section_data_from_rva(instance, rva, data_size, include_headers);
		}
	}

	static decltype(auto) section_data_from_address(const pe_bliss::image::image& instance,
		pe_bliss::rva_type rva, std::uint32_t data_size, bool include_headers,
		bool allow_virtual_data)
	{
		switch (GetParam())
		{
		case function_type::va32:
			return section_data_from_va(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				data_size, include_headers, allow_virtual_data);
		case function_type::va64:
			return section_data_from_va(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				data_size, include_headers, allow_virtual_data);
		default: //rva
			return section_data_from_rva(instance, rva, data_size, include_headers,
				allow_virtual_data);
		}
	}
};
} //namespace

TEST_P(SectionDataFromRvaFullFixture, SectionDataFromRvaTestNonConst1)
{
	instance.update_full_headers_buffer(false);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(instance, 1u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	auto& full_headers_buf = instance.get_full_headers_buffer().copied_data();
	EXPECT_TRUE(spans_equal(section_data_from_address(instance, 1u, true),
		std::span(full_headers_buf.data() + 1u, full_headers_buf.size() - 1u)));
}

TEST_P(SectionDataFromRvaFullFixture, SectionDataFromRvaTestConst1)
{
	instance.update_full_headers_buffer(false);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance), 1u, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	auto& full_headers_buf = instance.get_full_headers_buffer();
	EXPECT_TRUE(buffers_equal(section_data_from_address(std::as_const(instance),
		1u, true, false), buffers::reduce(full_headers_buf.data(), 1u)));
}

TEST_P(SectionDataFromRvaFullFixture, SectionVirtualDataFromRvaTestConst1)
{
	auto& full_headers_buf = instance.get_full_headers_buffer();
	auto buf = std::make_shared<buffers::input_container_buffer>();
	buf->get_container().resize(100u);
	for (int i = 0; i != buf->get_container().size(); ++i)
		buf->get_container()[i] = static_cast<std::byte>(i);
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(buf, 10u);
	full_headers_buf.deserialize(virtual_buf, false);

	EXPECT_TRUE(buffers_equal(section_data_from_address(std::as_const(instance),
		1u, true, false), buffers::reduce(full_headers_buf.data(),
			1u, full_headers_buf.physical_size() - 1u)));

	EXPECT_TRUE(buffers_equal(section_data_from_address(std::as_const(instance),
		1u, true, true), buffers::reduce(full_headers_buf.data(), 1u)));
}

TEST_P(SectionDataFromRvaFullFixture, SectionDataFromRvaTestNonConst2)
{
	auto& section = instance.get_section_data_list()[1];
	EXPECT_TRUE(spans_equal(section_data_from_address(instance, 0x2010u, true),
		std::span(section.copied_data().data() + 0x10u,
			section.copied_data().size() - 0x10u)));

	expect_throw_pe_error([this] {
		(void)section_data_from_address(instance, inexistent_rva, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(SectionDataFromRvaFullFixture, SectionDataFromRvaTestConst2)
{
	const auto& section = instance.get_section_data_list()[1];
	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), 0x2010u, false, false),
		buffers::reduce(section.data(), 0x10u, section.physical_size() - 0x10u)));

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance),
			inexistent_rva, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(SectionDataFromRvaFullFixture, SectionVirtualDataFromRvaTestConst2)
{
	const auto& section = instance.get_section_data_list()[1];
	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), 0x2010u, false, true),
		buffers::reduce(section.data(), 0x10u)));

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance),
			inexistent_rva, false, true);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(SectionDataFromRvaFullFixture, SectionDataFromRvaTestSectionEnd)
{
	const auto& section = instance.get_section_data_list()[2];
	for (bool allow_virtual_data : {true, false})
	{
		EXPECT_TRUE(buffers_equal(section_data_from_address(
			std::as_const(instance), last_section_last_rva, true,
			allow_virtual_data),
			std::make_shared<buffers::input_container_buffer>()));
		EXPECT_TRUE(buffers_equal(section_data_from_address(
			std::as_const(instance), last_section_last_rva, false,
			allow_virtual_data),
			std::make_shared<buffers::input_container_buffer>()));
	}
}

TEST_P(SectionDataFromRvaFullFixture, SectionDataFromRvaCutVirtualBuffer)
{
	auto& section = instance.get_section_data_list()[1];
	section.copied_data().resize(section.copied_data().size() - 0x10u);
	const std::uint32_t second_section_size = static_cast<std::uint32_t>(
		section.copied_data().size());
	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), second_section_rva + second_section_size, false, false),
		buffers::reduce(section.data(), second_section_size, 0u)));
	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), second_section_rva + second_section_size, false, true),
		buffers::reduce(section.data(), second_section_size)));
}

TEST(SectionDataFromRvaFull, SectionDataFromVaTest)
{
	auto instance = create_test_image({});

	expect_throw_pe_error([&instance] {
		(void)section_data_from_va(instance, static_cast<std::uint32_t>(0x1u), true);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		(void)section_data_from_va(instance, static_cast<std::uint64_t>(0x1u), true);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}

TEST_P(SectionDataFromRvaWithSizeFixture, SectionDataFromRvaCutBuffer)
{
	auto& section = instance.get_section_data_list()[1];
	section.copied_data().resize(section.copied_data().size() - 0x10u);
	const std::uint32_t second_section_size = static_cast<std::uint32_t>(
		section.copied_data().size());
	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), second_section_rva, second_section_size, false, false),
		buffers::reduce(section.data(), 0u, second_section_size)));
}

TEST_P(SectionDataFromRvaWithSizeFixture, SectionDataFromRvaTestNonConst1)
{
	instance.update_full_headers_buffer(false);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(instance, 1u, 10u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(instance, 1u, 20000u, true);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	auto& full_headers_buf = instance.get_full_headers_buffer().copied_data();
	EXPECT_TRUE(spans_equal(section_data_from_address(instance, 1u, 20u, true),
		std::span(full_headers_buf.data() + 1u, 20u)));
}

TEST_P(SectionDataFromRvaWithSizeFixture, SectionDataFromRvaTestConst1)
{
	instance.update_full_headers_buffer(false);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance),
			1u, 10u, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance),
			1u, 20000u, true, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	auto& full_headers_buf = instance.get_full_headers_buffer();
	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), 1u, 20u, true, false),
		buffers::reduce(full_headers_buf.data(), 1u, 20u)));
}

TEST_P(SectionDataFromRvaWithSizeFixture, SectionDataFromRvaTestNonConst2)
{
	auto& section = instance.get_section_data_list()[1];
	EXPECT_TRUE(spans_equal(section_data_from_address(
		instance, 0x2010u, 20u, true),
		std::span(section.copied_data().data() + 0x10u, 20u)));

	EXPECT_TRUE(spans_equal(section_data_from_address(
		instance, 0x2010u,
		static_cast<std::uint32_t>(section.physical_size() - 0x10u), true),
		std::span(section.copied_data().data() + 0x10u,
			section.physical_size() - 0x10u)));

	expect_throw_pe_error([this, &section] {
		(void)section_data_from_address(
			instance, 0x2010u,
			static_cast<std::uint32_t>(section.physical_size() - 0x9u), true);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(instance, 0x4010u, 10u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(instance, 0x4000u, 10u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(SectionDataFromRvaWithSizeFixture, SectionDataFromRvaTestConst2)
{
	const auto& section = instance.get_section_data_list()[1];
	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), 0x2010u, 20u, true, false),
		buffers::reduce(section.data(), 0x10u, 20u)));

	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), 0x2010u,
		static_cast<std::uint32_t>(section.physical_size() - 0x10u), true, false),
		buffers::reduce(section.data(), 0x10u, section.physical_size() - 0x10u)));

	expect_throw_pe_error([this, &section] {
		(void)section_data_from_address(
			std::as_const(instance), 0x2010u,
			static_cast<std::uint32_t>(section.physical_size() - 0x9u), true, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance),
			0x4010u, 10u, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance),
			0x4000u, 10u, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(SectionDataFromRvaWithSizeFixture, SectionDataFromRvaTestConst3)
{
	static constexpr std::size_t virtual_size = 10u;

	auto empty_buf = std::make_shared<buffers::input_container_buffer>();
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(
		empty_buf, virtual_size);

	const auto& section = instance.get_section_data_list()[1];
	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), 0x3010u, virtual_size, true, true),
		virtual_buf));

	expect_throw_pe_error([this] {
		(void)section_data_from_address(
			std::as_const(instance), 0x3010u, virtual_size, true, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(
			std::as_const(instance), last_section_last_rva, virtual_size, true, true);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(SectionDataFromRvaWithSizeFixture, SectionDataFromRvaTestConst4)
{
	static constexpr std::size_t size = 10u;

	const auto& section = instance.get_section_data_list()[1];

	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), 0x3000u - 5u, size, true, true),
		buffers::reduce(section.get_buffer().data(), 0x1000u - 5u, size)));
	
	expect_throw_pe_error([this] {
		(void)section_data_from_address(
			std::as_const(instance), 0x3000u - 5u, size, true, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST(SectionDataFromRvaWithDataSize, SectionDataFromVaTest)
{
	auto instance = create_test_image({});

	expect_throw_pe_error([&instance] {
		(void)section_data_from_va(instance,
			static_cast<std::uint32_t>(0x1u), 10u, true);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		(void)section_data_from_va(instance,
			static_cast<std::uint64_t>(0x1u), 10u, true);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}

INSTANTIATE_TEST_SUITE_P(
	SectionDataFromRvaFullTests,
	SectionDataFromRvaFullFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));

INSTANTIATE_TEST_SUITE_P(
	SectionDataFromRvaWithSizeTests,
	SectionDataFromRvaWithSizeFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));
