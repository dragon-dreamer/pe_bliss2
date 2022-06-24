#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "buffers/input_buffer_section.h"

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/pe_types.h"

#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

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
	buf1->set_rpos(0);
	buf2->set_rpos(0);
	if (buf1->size() != buf2->size())
		return false;

	std::byte byte1{}, byte2{};
	while (buf1->read(1, &byte1))
	{
		if (!buf2->read(1, &byte2))
			return false;

		if (byte1 != byte2)
			return false;
	}

	return true;
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

protected:
	pe_bliss::image::image instance;
};

class SectionDataFromRvaFullFixture : public SectionDataFromRvaFixtureBase
{
public:
	template<typename Image>
	static decltype(auto) section_data_from_address(Image& instance,
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
};

class SectionDataFromRvaWithSizeFixture : public SectionDataFromRvaFixtureBase
{
public:
	template<typename Image>
	static decltype(auto) section_data_from_address(Image& instance,
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
		(void)section_data_from_address(std::as_const(instance), 1u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	auto& full_headers_buf = instance.get_full_headers_buffer();
	EXPECT_TRUE(buffers_equal(section_data_from_address(std::as_const(instance), 1u, true),
		buffers::reduce(full_headers_buf.data(), 1u)));
}

TEST_P(SectionDataFromRvaFullFixture, SectionDataFromRvaTestNonConst2)
{
	auto& section = instance.get_section_data_list()[1];
	EXPECT_TRUE(spans_equal(section_data_from_address(instance, 0x2010u, true),
		std::span(section.copied_data().data() + 0x10u,
			section.copied_data().size() - 0x10u)));

	expect_throw_pe_error([this] {
		(void)section_data_from_address(instance, 0x4010u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(instance, 0x4000u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(SectionDataFromRvaFullFixture, SectionDataFromRvaTestConst2)
{
	const auto& section = instance.get_section_data_list()[1];
	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), 0x2010u, true),
		buffers::reduce(section.data(), 0x10u)));

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance), 0x4010u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance), 0x4000u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
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

INSTANTIATE_TEST_SUITE_P(
	SectionDataFromRvaFullTests,
	SectionDataFromRvaFullFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));

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
		(void)section_data_from_address(std::as_const(instance), 1u, 10u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance), 1u, 20000u, true);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	auto& full_headers_buf = instance.get_full_headers_buffer();
	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), 1u, 20u, true),
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
		static_cast<std::uint32_t>(section.size() - 0x10u), true),
		std::span(section.copied_data().data() + 0x10u,
			section.size() - 0x10u)));

	expect_throw_pe_error([this, &section] {
		(void)section_data_from_address(
			instance, 0x2010u,
			static_cast<std::uint32_t>(section.size() - 0x9u), true);
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
		std::as_const(instance), 0x2010u, 20u, true),
		buffers::reduce(section.data(), 0x10u, 20u)));

	EXPECT_TRUE(buffers_equal(section_data_from_address(
		std::as_const(instance), 0x2010u,
		static_cast<std::uint32_t>(section.size() - 0x10u), true),
		buffers::reduce(section.data(), 0x10u, section.size() - 0x10u)));

	expect_throw_pe_error([this, &section] {
		(void)section_data_from_address(
			std::as_const(instance), 0x2010u,
			static_cast<std::uint32_t>(section.size() - 0x9u), true);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance), 0x4010u, 10u, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		(void)section_data_from_address(std::as_const(instance), 0x4000u, 10u, false);
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
	SectionDataFromRvaWithSizeTests,
	SectionDataFromRvaWithSizeFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));