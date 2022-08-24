#include <type_traits>

#include "gtest/gtest.h"

#include "pe_bliss2/exceptions/arm_common/arm_common_unwind_info.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"
#include "utilities/generic_error.h"

using namespace pe_bliss::exceptions::arm_common;

namespace
{
template<typename T>
class EpilogInfoTests : public testing::Test
{
public:
	static constexpr bool has_condition = T::value;
};

using epilog_info_tested_types = ::testing::Types<std::true_type, std::false_type>;
} //namespace

TYPED_TEST_SUITE(EpilogInfoTests, epilog_info_tested_types);

TYPED_TEST(EpilogInfoTests, EpilogInfoStartOffset)
{
	epilog_info<TestFixture::has_condition> info;
	EXPECT_EQ(info.get_epilog_start_offset(), 0u);

	expect_throw_pe_error([&info] {
		info.set_epilog_start_offset(3u);
	}, exception_directory_errc::invalid_epilog_start_offset);
	EXPECT_EQ(info.get_epilog_start_offset(), 0u);

	expect_throw_pe_error([&info] {
		info.set_epilog_start_offset(0xffffeu);
	}, exception_directory_errc::invalid_epilog_start_offset);
	EXPECT_EQ(info.get_epilog_start_offset(), 0u);

	EXPECT_NO_THROW(info.set_epilog_start_offset(0x3fffeu));
	EXPECT_EQ(info.get_epilog_start_offset(), 0x3fffeu);
	EXPECT_EQ(info.get_descriptor().get(), 0x3fffeu / 2u);
}

TEST(ArmCommonUnwindInfoTests, EpilogInfoStartIndexNoCondition)
{
	epilog_info<false> info;
	EXPECT_EQ(info.get_epilog_start_index(), 0u);

	expect_throw_pe_error([&info] {
		info.set_epilog_start_index(0x400u);
	}, exception_directory_errc::invalid_epilog_start_index);
	EXPECT_EQ(info.get_epilog_start_index(), 0u);

	info.get_descriptor().get() = 0xffffffffu;

	EXPECT_NO_THROW(info.set_epilog_start_index(0x2aau));
	EXPECT_EQ(info.get_epilog_start_index(), 0x2aau);
	EXPECT_EQ(info.get_descriptor().get(), 0xaabfffffu);
}

TEST(ArmCommonUnwindInfoTests, EpilogInfoStartIndexCondition)
{
	epilog_info<true> info;
	EXPECT_EQ(info.get_epilog_start_index(), 0u);

	info.get_descriptor().get() = 0xffffffffu;

	EXPECT_NO_THROW(info.set_epilog_start_index(0xaau));
	EXPECT_EQ(info.get_epilog_start_index(), 0xaau);
	EXPECT_EQ(info.get_descriptor().get(), 0xaaffffffu);
}

TEST(ArmCommonUnwindInfoTests, EpilogInfoCondition)
{
	epilog_info<true> info;
	EXPECT_EQ(info.get_epilog_condition(), 0u);

	expect_throw_pe_error([&info] {
		info.set_epilog_condition(0x10u);
	}, exception_directory_errc::invalid_epilog_condition);
	EXPECT_EQ(info.get_epilog_condition(), 0u);

	info.get_descriptor().get() = 0xffffffffu;

	EXPECT_NO_THROW(info.set_epilog_condition(0xau));
	EXPECT_EQ(info.get_epilog_condition(), 0xau);
	EXPECT_EQ(info.get_descriptor().get(), 0xffafffffu);
}

TEST(ArmCommonUnwindInfoTests, UnwindCodeCommonRequiredUintType)
{
	EXPECT_TRUE((std::is_same_v<unwind_code_common<1>::required_uint_type<1>, std::uint8_t>));
	EXPECT_TRUE((std::is_same_v<unwind_code_common<1>::required_uint_type<2>, std::uint16_t>));
	EXPECT_TRUE((std::is_same_v<unwind_code_common<1>::required_uint_type<3>, std::uint32_t>));
	EXPECT_TRUE((std::is_same_v<unwind_code_common<1>::required_uint_type<4>, std::uint32_t>));
}

namespace
{
template<typename T>
class UnwindCodeCommonTests : public testing::Test
{
public:
	static constexpr std::uint32_t width = T::value;
};

using unwind_code_common_tested_types = ::testing::Types<
	std::integral_constant<std::uint32_t, 1u>,
	std::integral_constant<std::uint32_t, 2u>,
	std::integral_constant<std::uint32_t, 3u>,
	std::integral_constant<std::uint32_t, 4u>>;
} //namespace

TYPED_TEST_SUITE(UnwindCodeCommonTests, unwind_code_common_tested_types);

TYPED_TEST(UnwindCodeCommonTests, UnwindCodeCommonValue)
{
	unwind_code_common<TestFixture::width> code;
	code.get_descriptor().value()[0] = std::byte{ 0xaau };
	EXPECT_EQ((code.get_value<0, 2>()), 0b101u);
	EXPECT_EQ((code.get_value<3, 5>()), 0b010u);
	EXPECT_EQ((code.get_value<4, 7>()), 0b1010u);

	expect_throw_pe_error([&code] {
		code.set_value<1, 5>(0x20u);
	}, utilities::generic_errc::integer_overflow);
	EXPECT_EQ(code.get_descriptor().value()[0], std::byte{ 0xaau });

	EXPECT_NO_THROW((code.set_value<1, 5>(0x1bu)));
	EXPECT_EQ((code.get_value<1, 5>()), 0x1bu);
	EXPECT_EQ(code.get_descriptor().value()[0], std::byte{ 0xeeu });

	expect_throw_pe_error([&code] {
		code.set_scaled_value<3, 1, 5, utilities::generic_errc::buffer_overrun>(0xbu);
	}, utilities::generic_errc::buffer_overrun);
	EXPECT_EQ(code.get_descriptor().value()[0], std::byte{ 0xeeu });

	expect_throw_pe_error([&code] {
		code.set_scaled_value<3, 1, 5, utilities::generic_errc::buffer_overrun>(333u);
	}, utilities::generic_errc::buffer_overrun);
	EXPECT_EQ(code.get_descriptor().value()[0], std::byte{ 0xeeu });

	code.set_scaled_value<3, 0, 7, utilities::generic_errc::buffer_overrun>(3u);
	EXPECT_EQ((code.get_value<0, 7>()), 1u);

	if constexpr (TestFixture::width >= 2u)
	{
		code.get_descriptor().value()[0] = std::byte{ 0xaau };
		code.get_descriptor().value()[1] = std::byte{ 0xaau };

		EXPECT_EQ((code.get_value<0, 9>()), 0b1010101010u);

		expect_throw_pe_error([&code] {
			code.set_value<0, 9>(0x400u);
		}, utilities::generic_errc::integer_overflow);
		EXPECT_EQ((code.get_value<0, 9>()), 0b1010101010u);

		EXPECT_NO_THROW((code.set_value<0, 9>(0x3ffu)));
		EXPECT_EQ((code.get_value<0, 9>()), 0x3ffu);
		EXPECT_EQ(code.get_descriptor().value()[0], std::byte{ 0xffu });
		EXPECT_EQ(code.get_descriptor().value()[1], std::byte{ 0xeau });
	}

	if constexpr (TestFixture::width >= 3u)
	{
		code.get_descriptor().value()[0] = std::byte{ 0xaau };
		code.get_descriptor().value()[1] = std::byte{ 0xaau };
		code.get_descriptor().value()[2] = std::byte{ 0xaau };

		EXPECT_EQ((code.get_value<5, 23>()), 0b0101010101010101010u);

		expect_throw_pe_error([&code] {
			code.set_value<5, 23>(0x80000u);
		}, utilities::generic_errc::integer_overflow);
		EXPECT_EQ((code.get_value<5, 23>()), 0b0101010101010101010u);

		EXPECT_NO_THROW((code.set_value<5, 23>(0u)));
		EXPECT_EQ((code.get_value<5, 23>()), 0u);
		EXPECT_EQ(code.get_descriptor().value()[0], std::byte{ 0xa8u });
		EXPECT_EQ(code.get_descriptor().value()[1], std::byte{});
		EXPECT_EQ(code.get_descriptor().value()[2], std::byte{});
	}

	if constexpr (TestFixture::width >= 4u)
	{
		code.get_descriptor().value()[0] = std::byte{ 0xaau };
		code.get_descriptor().value()[1] = std::byte{ 0xaau };
		code.get_descriptor().value()[2] = std::byte{ 0xaau };
		code.get_descriptor().value()[3] = std::byte{ 0xaau };

		EXPECT_EQ((code.get_value<3, 30>()), 0b101010101010101010101010101u);
		
		expect_throw_pe_error([&code] {
			code.set_value<3, 30>(0x10000000u);
		}, utilities::generic_errc::integer_overflow);
		EXPECT_EQ((code.get_value<3, 30>()), 0b101010101010101010101010101u);
		
		EXPECT_NO_THROW((code.set_value<3, 30>(0xaaaaaaau)));
		EXPECT_EQ((code.get_value<3, 30>()), 0xaaaaaaau);
		EXPECT_EQ(code.get_descriptor().value()[0], std::byte{ 0xb5u });
		EXPECT_EQ(code.get_descriptor().value()[1], std::byte{ 0x55u });
		EXPECT_EQ(code.get_descriptor().value()[2], std::byte{ 0x55u });
		EXPECT_EQ(code.get_descriptor().value()[3], std::byte{ 0x54u });
	}
}

TEST(ArmCommonUnwindInfoTests, RuntimeFunctionBase)
{
	struct descriptor {
		std::uint32_t unwind_data;
	};

	runtime_function_base<descriptor, int, int> base;
	EXPECT_TRUE(base.has_extended_unwind_record());

	base.get_descriptor()->unwind_data = 1u;
	EXPECT_FALSE(base.has_extended_unwind_record());

	base.get_descriptor()->unwind_data = 2u;
	EXPECT_FALSE(base.has_extended_unwind_record());

	base.get_descriptor()->unwind_data = 4u;
	EXPECT_TRUE(base.has_extended_unwind_record());
}

TEST(ArmCommonUnwindInfoTests, ExtendedUnwindRecordBaseVersion)
{
	extended_unwind_record_base record;
	EXPECT_EQ(record.get_version(), 0u);

	record.get_main_header().get() |= 0xc0000u;
	EXPECT_EQ(record.get_version(), 3u);

	expect_throw_pe_error([&record] {
		record.set_version(1u);
	}, exception_directory_errc::invalid_version);
	EXPECT_EQ(record.get_version(), 3u);

	EXPECT_NO_THROW(record.set_version(0u));
	EXPECT_EQ(record.get_version(), 0u);
}

TEST(ArmCommonUnwindInfoTests, ExtendedUnwindRecordBaseExceptionData)
{
	extended_unwind_record_base record;
	EXPECT_FALSE(record.has_exception_data());

	record.set_has_exception_data(true);
	EXPECT_EQ(record.get_main_header().get(), 0x100000u);
	EXPECT_TRUE(record.has_exception_data());

	record.get_main_header().get() = 0xffffffffu;
	record.set_has_exception_data(false);
	EXPECT_EQ(record.get_main_header().get(), 0xffffffffu & ~0x100000u);
	EXPECT_FALSE(record.has_exception_data());
}

TEST(ArmCommonUnwindInfoTests, ExtendedUnwindRecordBaseSingleEpilog)
{
	extended_unwind_record_base record;
	EXPECT_FALSE(record.single_epilog_info_packed());

	record.set_single_epilog_info_packed(true);
	EXPECT_EQ(record.get_main_header().get(), 0x200000u);
	EXPECT_TRUE(record.single_epilog_info_packed());

	record.get_main_header().get() = 0xffffffffu;
	record.set_single_epilog_info_packed(false);
	EXPECT_EQ(record.get_main_header().get(), 0xffffffffu & ~0x200000u);
	EXPECT_FALSE(record.single_epilog_info_packed());
}

TEST(ArmCommonUnwindInfoTests, ExtendedUnwindRecordBaseExtendedCodeWords)
{
	extended_unwind_record_base record;
	EXPECT_EQ(record.get_extended_code_words(), 0u);

	record.set_extended_code_words(0xaau);
	EXPECT_EQ(record.get_main_extended_header().get(), 0xaa0000u);
	EXPECT_EQ(record.get_extended_code_words(), 0xaau);
}

TEST(ArmCommonUnwindInfoTests, ExtendedUnwindRecordBaseExtendedEpilogCount)
{
	extended_unwind_record_base record;
	EXPECT_EQ(record.get_extended_epilog_count(), 0u);

	record.set_extended_epilog_count(0xaaaau);
	EXPECT_EQ(record.get_main_extended_header().get(), 0xaaaau);
	EXPECT_EQ(record.get_extended_epilog_count(), 0xaaaau);
}

namespace
{
struct [[nodiscard]] unwind_record_options_1
{
	using unwind_code_type = std::variant<int>;
	static constexpr std::uint32_t function_length_multiplier = 2u;
	static constexpr bool has_f_bit = true;
};
struct [[nodiscard]] unwind_record_options_2
{
	using unwind_code_type = std::variant<int>;
	static constexpr std::uint32_t function_length_multiplier = 4u;
	static constexpr bool has_f_bit = false;
};
} //namespace

TEST(ArmCommonUnwindInfoTests, ExtendedUnwindRecordFunctionLength2)
{
	extended_unwind_record<int, unwind_record_options_1> record;
	EXPECT_EQ(record.get_function_length(), 0u);

	expect_throw_pe_error([&record] {
		record.set_function_length(3u);
	}, exception_directory_errc::invalid_function_length);
	EXPECT_EQ(record.get_function_length(), 0u);

	expect_throw_pe_error([&record] {
		record.set_function_length(0x80000u);
	}, exception_directory_errc::invalid_function_length);
	EXPECT_EQ(record.get_function_length(), 0u);
	
	EXPECT_NO_THROW(record.set_function_length(0x7fffeu));
	EXPECT_EQ(record.get_function_length(), 0x7fffeu);
	EXPECT_EQ(record.get_main_header().get(), 0x3ffffu);
}

TEST(ArmCommonUnwindInfoTests, ExtendedUnwindRecordFunctionLength4)
{
	extended_unwind_record<int, unwind_record_options_2> record;
	EXPECT_EQ(record.get_function_length(), 0u);

	expect_throw_pe_error([&record] {
		record.set_function_length(3u);
	}, exception_directory_errc::invalid_function_length);
	EXPECT_EQ(record.get_function_length(), 0u);

	expect_throw_pe_error([&record] {
		record.set_function_length(0x100000u);
	}, exception_directory_errc::invalid_function_length);
	EXPECT_EQ(record.get_function_length(), 0u);

	EXPECT_NO_THROW(record.set_function_length(0xffffcu));
	EXPECT_EQ(record.get_function_length(), 0xffffcu);
	EXPECT_EQ(record.get_main_header().get(), 0x3ffffu);
}

TEST(ArmCommonUnwindInfoTests, ExtendedUnwindRecordFunctionFragment)
{
	extended_unwind_record<int, unwind_record_options_1> record;
	EXPECT_FALSE(record.is_function_fragment());

	record.set_is_function_fragment(true);
	EXPECT_TRUE(record.is_function_fragment());
	EXPECT_EQ(record.get_main_header().get(), 0x400000u);

	record.set_is_function_fragment(false);
	EXPECT_FALSE(record.is_function_fragment());
}

TEST(ArmCommonUnwindInfoTests, ExtendedUnwindRecordExtendedMainHeaderFBit)
{
	extended_unwind_record<int, unwind_record_options_1> record;
	EXPECT_TRUE(record.has_extended_main_header());

	for (int i = 0; i < 2; ++i)
	{
		record.set_epilog_count(0x1fu);
		EXPECT_EQ(record.get_epilog_count(), 0x1fu);
		EXPECT_FALSE(record.has_extended_main_header());
		record.set_code_words(0xfu);
		EXPECT_FALSE(record.has_extended_main_header());
		EXPECT_EQ(record.get_epilog_count(), 0x1fu);
		EXPECT_EQ(record.get_code_words(), 0xfu);
	}

	record.set_epilog_count(0x20u);
	EXPECT_EQ(record.get_epilog_count(), 0x20u);
	EXPECT_TRUE(record.has_extended_main_header());
	record.set_code_words(0x10u);
	EXPECT_TRUE(record.has_extended_main_header());
	EXPECT_EQ(record.get_epilog_count(), 0x20u);
	EXPECT_EQ(record.get_code_words(), 0x10u);

	record.set_epilog_count(0x1fu);
	EXPECT_EQ(record.get_epilog_count(), 0x1fu);
	EXPECT_EQ(record.get_code_words(), 0x10u);
	EXPECT_TRUE(record.has_extended_main_header());
	record.set_code_words(0xfu);
	EXPECT_FALSE(record.has_extended_main_header());
	EXPECT_EQ(record.get_epilog_count(), 0x1fu);
	EXPECT_EQ(record.get_code_words(), 0xfu);
}
