#include "gtest/gtest.h"

#include <cstddef>
#include <type_traits>

#include "pe_bliss2/exceptions/arm64/arm64_exception_directory.h"
#include "tests/pe_bliss2/directories/arm_common_exception_helpers.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::exceptions;
using namespace pe_bliss::exceptions::arm64;

TEST(Arm64ExceptionDirectoryTests, Decode)
{
	using codes = unwind_record_options::unwind_code_type;
	test_created_unwind_code<codes, opcode::alloc_s>(0u);
	test_created_unwind_code<codes, opcode::alloc_s>(0x1fu);
	test_created_unwind_code<codes, opcode::save_r19r20_x>(0x20u);
	test_created_unwind_code<codes, opcode::save_r19r20_x>(0x3fu);
	test_created_unwind_code<codes, opcode::save_fplr>(0x40u);
	test_created_unwind_code<codes, opcode::save_fplr>(0x7fu);
	test_created_unwind_code<codes, opcode::save_fplr_x>(0x80u);
	test_created_unwind_code<codes, opcode::save_fplr_x>(0xbfu);
	test_created_unwind_code<codes, opcode::alloc_m>(0xc0u);
	test_created_unwind_code<codes, opcode::alloc_m>(0xc7u);
	test_created_unwind_code<codes, opcode::save_regp>(0xc8u);
	test_created_unwind_code<codes, opcode::save_regp>(0xcbu);
	test_created_unwind_code<codes, opcode::save_regp_x>(0xccu);
	test_created_unwind_code<codes, opcode::save_regp_x>(0xcfu);
	test_created_unwind_code<codes, opcode::save_reg>(0xd0u);
	test_created_unwind_code<codes, opcode::save_reg>(0xd3u);
	test_created_unwind_code<codes, opcode::save_reg_x>(0xd4u);
	test_created_unwind_code<codes, opcode::save_reg_x>(0xd5u);
	test_created_unwind_code<codes, opcode::save_lrpair>(0xd6u);
	test_created_unwind_code<codes, opcode::save_lrpair>(0xd7u);
	test_created_unwind_code<codes, opcode::save_fregp>(0xd8u);
	test_created_unwind_code<codes, opcode::save_fregp>(0xd9u);
	test_created_unwind_code<codes, opcode::save_fregp_x>(0xdau);
	test_created_unwind_code<codes, opcode::save_fregp_x>(0xdbu);
	test_created_unwind_code<codes, opcode::save_freg>(0xdcu);
	test_created_unwind_code<codes, opcode::save_freg>(0xddu);
	test_created_unwind_code<codes, opcode::save_freg_x>(0xdeu);
	expect_throw_pe_error([] {
		std::vector<codes> vec;
		arm_common::create_unwind_code(std::byte{ 0xdfu }, vec);
	}, arm_common::exception_directory_errc::unsupported_unwind_code);
	test_created_unwind_code<codes, opcode::alloc_l>(0xe0u);
	test_created_unwind_code<codes, opcode::set_fp>(0xe1u);
	test_created_unwind_code<codes, opcode::add_fp>(0xe2u);
	test_created_unwind_code<codes, opcode::nop>(0xe3u);
	test_created_unwind_code<codes, opcode::end>(0xe4u);
	test_created_unwind_code<codes, opcode::end_c>(0xe5u);
	test_created_unwind_code<codes, opcode::save_next>(0xe6u);
	test_created_unwind_code<codes, opcode::save_reg_any>(0xe7u);
	test_created_unwind_code<codes, opcode::reserved_custom_stack>(0xe8u);
	test_created_unwind_code<codes, opcode::reserved_custom_stack>(0xefu);
	test_created_unwind_code<codes, opcode::pacibsp>(0xfcu);
}

TEST(Arm64ExceptionDirectoryTests, PackedUnwindData)
{
	packed_unwind_data data(123u);
	EXPECT_EQ(data.get_packed_value(), 123u);
	data.set_packed_value(0u);

	data.set_flag(packed_unwind_data::flag::packed_unwind_fragment);
	EXPECT_EQ(data.get_flag(), packed_unwind_data::flag::packed_unwind_fragment);
	
	EXPECT_EQ(data.get_function_length(), 0u);
	EXPECT_NO_THROW(data.set_function_length(0x1ffcu));
	expect_throw_pe_error([&data] {
		data.set_function_length(0x3u);
	}, exception_directory_errc::invalid_function_length);
	expect_throw_pe_error([&data] {
		data.set_function_length(0x2000u);
	}, exception_directory_errc::invalid_function_length);
	EXPECT_EQ(data.get_function_length(), 0x1ffcu);

	EXPECT_EQ(data.get_frame_size(), 0u);
	EXPECT_NO_THROW(data.set_frame_size(0x1ff0u));
	EXPECT_EQ(data.get_frame_size(), 0x1ff0u);
	expect_throw_pe_error([&data] {
		data.set_frame_size(17u);
	}, exception_directory_errc::invalid_frame_size);
	expect_throw_pe_error([&data] {
		data.set_frame_size(0x2000u);
	}, exception_directory_errc::invalid_frame_size);
	EXPECT_EQ(data.get_frame_size(), 0x1ff0u);

	EXPECT_EQ(data.get_cr(), packed_unwind_data::cr::unchained);
	data.set_cr(packed_unwind_data::cr::unchained_saved_lr);
	EXPECT_EQ(data.get_cr(), packed_unwind_data::cr::unchained_saved_lr);

	EXPECT_FALSE(data.homes_integer_parameter_registers());
	data.set_homes_integer_parameter_registers(true);
	EXPECT_TRUE(data.homes_integer_parameter_registers());

	EXPECT_EQ(data.get_reg_int(), 0u);
	EXPECT_NO_THROW(data.set_reg_int(0xfu));
	EXPECT_EQ(data.get_reg_int(), 0xfu);
	expect_throw_pe_error([&data] {
		data.set_reg_int(0x10u);
	}, exception_directory_errc::invalid_reg_int_value);

	EXPECT_EQ(data.get_reg_fp(), 0u);
	EXPECT_NO_THROW(data.set_reg_fp(0x8u));
	EXPECT_EQ(data.get_reg_fp(), 0x8u);
	expect_throw_pe_error([&data] {
		data.set_reg_fp(0x9u);
	}, exception_directory_errc::invalid_reg_fp_value);
	expect_throw_pe_error([&data] {
		data.set_reg_fp(0x1u);
	}, exception_directory_errc::invalid_reg_fp_value);

	EXPECT_EQ(data.get_packed_value() & 0b11u, 0b10u);
	EXPECT_EQ(data.get_packed_value() & 0b1111111111100u, 0b1111111111100u);
	EXPECT_EQ(data.get_packed_value() & 0xff800000u, 0xff800000u);
	EXPECT_EQ(data.get_packed_value() & 0x600000u, 0x200000u);
	EXPECT_EQ(data.get_packed_value() & 0x100000u, 0x100000u);
	EXPECT_EQ(data.get_packed_value() & 0xf0000u, 0xf0000u);
	EXPECT_EQ(data.get_packed_value() & 0xe000u, 0xe000u);
}

TEST(Arm64ExceptionDirectoryTests, AllocS)
{
	opcode::alloc_s code;
	EXPECT_EQ(code.get_allocation_size(), 0u);
	EXPECT_NO_THROW(code.set_allocation_size(0x1f0u));

	expect_throw_pe_error([&code] {
		code.set_allocation_size(0x200u);
	}, exception_directory_errc::invalid_allocation_size);
	expect_throw_pe_error([&code] {
		code.set_allocation_size(17u);
	}, exception_directory_errc::invalid_allocation_size);

	EXPECT_EQ(code.get_allocation_size(), 0x1f0u);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0x1fu });
}

TEST(Arm64ExceptionDirectoryTests, SaveR19R20X)
{
	opcode::save_r19r20_x code;
	EXPECT_EQ(code.get_offset(), 0u);
	EXPECT_NO_THROW(code.set_offset(0xf8u));

	expect_throw_pe_error([&code] {
		code.set_offset(0x100u);
	}, exception_directory_errc::invalid_offset);
	expect_throw_pe_error([&code] {
		code.set_offset(9u);
	}, exception_directory_errc::invalid_offset);

	EXPECT_EQ(code.get_offset(), 0xf8u);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0x1fu });
}

TEST(Arm64ExceptionDirectoryTests, SaveFpLr)
{
	opcode::save_fplr code;
	EXPECT_EQ(code.get_offset(), 0u);
	EXPECT_NO_THROW(code.set_offset(0x1f8u));

	expect_throw_pe_error([&code] {
		code.set_offset(0x200u);
	}, exception_directory_errc::invalid_offset);
	expect_throw_pe_error([&code] {
		code.set_offset(9u);
	}, exception_directory_errc::invalid_offset);

	EXPECT_EQ(code.get_offset(), 0x1f8u);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0x3fu });
}

TEST(Arm64ExceptionDirectoryTests, SaveFpLrX)
{
	opcode::save_fplr_x code;
	EXPECT_EQ(code.get_offset(), 8u);
	EXPECT_NO_THROW(code.set_offset(0x1f8u + 8u));

	expect_throw_pe_error([&code] {
		code.set_offset(0x200u + 8u);
	}, exception_directory_errc::invalid_offset);
	expect_throw_pe_error([&code] {
		code.set_offset(9u + 8u);
	}, exception_directory_errc::invalid_offset);
	expect_throw_pe_error([&code] {
		code.set_offset(0u);
	}, exception_directory_errc::invalid_offset);

	EXPECT_EQ(code.get_offset(), 0x1f8u + 8u);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0x3fu });
}

TEST(Arm64ExceptionDirectoryTests, AllocM)
{
	opcode::alloc_m code;
	EXPECT_EQ(code.get_allocation_size(), 0u);
	EXPECT_NO_THROW(code.set_allocation_size(0x7ffu * 16u));

	expect_throw_pe_error([&code] {
		code.set_allocation_size(0x7ffu * 16u + 16u);
	}, exception_directory_errc::invalid_allocation_size);
	expect_throw_pe_error([&code] {
		code.set_allocation_size(17u);
	}, exception_directory_errc::invalid_allocation_size);

	EXPECT_EQ(code.get_allocation_size(), 0x7ffu * 16u);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0x7u });
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 0xffu });
}

namespace
{
template<typename Opcode>
concept has_set_register_pair = requires (Opcode code) { code.get_register_pair(); };

template<typename Opcode>
requires (!has_set_register_pair<Opcode>)
void test_save_regp_register_pair(Opcode& code,
	std::uint8_t delta, std::uint8_t max_reg)
{
	EXPECT_EQ(code.get_register(), delta);
	EXPECT_NO_THROW(code.set_register(max_reg));

	expect_throw_pe_error([&code, delta] {
		code.set_register(delta - 1);
	}, exception_directory_errc::invalid_register);
	expect_throw_pe_error([&code, max_reg] {
		code.set_register(max_reg + 1);
	}, exception_directory_errc::invalid_register);

	EXPECT_EQ(code.get_register(), max_reg);
}

template<has_set_register_pair Opcode>
void test_save_regp_register_pair(Opcode& code,
	std::uint8_t delta, std::uint8_t max_reg)
{
	EXPECT_EQ(code.get_register_pair(), delta);
	EXPECT_NO_THROW(code.set_register_pair(max_reg));

	expect_throw_pe_error([&code, delta] {
		code.set_register_pair(delta - 1);
	}, exception_directory_errc::invalid_register);
	expect_throw_pe_error([&code, max_reg] {
		code.set_register_pair(max_reg + 1);
	}, exception_directory_errc::invalid_register);

	EXPECT_EQ(code.get_register_pair(), max_reg);
}

template<typename Opcode>
void test_save_reg(std::uint8_t reg_delta, std::uint8_t max_reg,
	std::uint8_t offset_bit_count, std::uint8_t offset_delta)
{
	Opcode code;
	test_save_regp_register_pair(code, reg_delta, max_reg);

	auto max_offset = (1u << offset_bit_count) - 1u;
	auto max_offset_scaled = static_cast<std::uint16_t>(
		max_offset * 8u + offset_delta);

	EXPECT_EQ(code.get_offset(), offset_delta);
	EXPECT_NO_THROW(code.set_offset(max_offset_scaled));
	expect_throw_pe_error([&code, offset_delta] {
		code.set_offset(9u + offset_delta);
	}, exception_directory_errc::invalid_offset);
	expect_throw_pe_error([&code, max_offset_scaled] {
		code.set_offset(max_offset_scaled + 8u);
	}, exception_directory_errc::invalid_offset);

	if constexpr (has_set_register_pair<Opcode>)
		EXPECT_EQ(code.get_register_pair(), max_reg);
	else
		EXPECT_EQ(code.get_register(), max_reg);

	EXPECT_EQ(code.get_offset(), max_offset_scaled);

	EXPECT_EQ(code.get_descriptor()[0], static_cast<std::byte>(
		(max_reg - reg_delta) >> (8u - offset_bit_count)));
	EXPECT_EQ(code.get_descriptor()[1], static_cast<std::byte>(max_offset
		| (((max_reg - reg_delta) & 0b111u) << offset_bit_count)));
}
} //namespace

TEST(Arm64ExceptionDirectoryTests, SaveRegp)
{
	test_save_reg<opcode::save_regp>(19u, 29u, 6u, 0u);
}

TEST(Arm64ExceptionDirectoryTests, SaveRegpX)
{
	test_save_reg<opcode::save_regp_x>(19u, 29u, 6u, 8u);
}

TEST(Arm64ExceptionDirectoryTests, SaveReg)
{
	test_save_reg<opcode::save_reg>(19u, 30u, 6u, 0u);
}

TEST(Arm64ExceptionDirectoryTests, SaveRegX)
{
	test_save_reg<opcode::save_reg_x>(19u, 30u, 5u, 8u);
}

TEST(Arm64ExceptionDirectoryTests, SaveLrPair)
{
	opcode::save_lrpair code;
	EXPECT_EQ(code.get_register(), 19u);
	EXPECT_NO_THROW(code.set_register(29u));

	expect_throw_pe_error([&code] {
		code.set_register(30u);
	}, exception_directory_errc::invalid_register);
	expect_throw_pe_error([&code] {
		code.set_register(31u);
	}, exception_directory_errc::invalid_register);

	EXPECT_EQ(code.get_offset(), 0u);
	EXPECT_NO_THROW(code.set_offset(0x1f8u));
	expect_throw_pe_error([&code] {
		code.set_offset(9u);
	}, exception_directory_errc::invalid_offset);
	expect_throw_pe_error([&code] {
		code.set_offset(0x1f8u + 8u);
	}, exception_directory_errc::invalid_offset);

	EXPECT_EQ(code.get_offset(), 0x1f8u);
	EXPECT_EQ(code.get_register(), 29u);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 1u });
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 0x7fu });
}

TEST(Arm64ExceptionDirectoryTests, SaveFregp)
{
	test_save_reg<opcode::save_fregp>(8u, 15u, 6u, 0u);
}

TEST(Arm64ExceptionDirectoryTests, SaveFregpX)
{
	test_save_reg<opcode::save_fregp_x>(8u, 15u, 6u, 8u);
}

TEST(Arm64ExceptionDirectoryTests, SaveFreg)
{
	test_save_reg<opcode::save_freg>(8u, 15u, 6u, 0u);
}

TEST(Arm64ExceptionDirectoryTests, SaveFregX)
{
	test_save_reg<opcode::save_freg_x>(8u, 15u, 5u, 8u);
}

TEST(Arm64ExceptionDirectoryTests, AllocL)
{
	opcode::alloc_l code;
	EXPECT_EQ(code.get_allocation_size(), 0u);
	EXPECT_NO_THROW(code.set_allocation_size(0xfafbfc0u));

	expect_throw_pe_error([&code] {
		code.set_allocation_size(0xffffff0u + 16u);
	}, exception_directory_errc::invalid_allocation_size);
	expect_throw_pe_error([&code] {
		code.set_allocation_size(17u);
	}, exception_directory_errc::invalid_allocation_size);

	EXPECT_EQ(code.get_allocation_size(), 0xfafbfc0u);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{});
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 0xfau });
	EXPECT_EQ(code.get_descriptor()[2], std::byte{ 0xfbu });
	EXPECT_EQ(code.get_descriptor()[3], std::byte{ 0xfcu });
}

TEST(Arm64ExceptionDirectoryTests, AddFp)
{
	opcode::add_fp code;
	EXPECT_EQ(code.get_delta(), 0u);
	EXPECT_NO_THROW(code.set_delta(0x7f8u));

	expect_throw_pe_error([&code] {
		code.set_delta(0x7f8u + 8u);
	}, exception_directory_errc::invalid_delta);
	expect_throw_pe_error([&code] {
		code.set_delta(9u);
	}, exception_directory_errc::invalid_delta);

	EXPECT_EQ(code.get_delta(), 0x7f8u);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{});
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 0xffu });
}

TEST(Arm64ExceptionDirectoryTests, ReservedCustomStack)
{
	opcode::reserved_custom_stack code;
	EXPECT_EQ(code.get_custom_stack_case(),
		opcode::custom_stack_case::msft_op_trap_frame);
	EXPECT_NO_THROW(code.set_custom_stack_case(
		opcode::custom_stack_case::msft_op_machine_frame));
	EXPECT_EQ(code.get_custom_stack_case(),
		opcode::custom_stack_case::msft_op_machine_frame);
	EXPECT_EQ(code.get_descriptor()[0],
		static_cast<std::byte>(opcode::custom_stack_case::msft_op_machine_frame));
}

TEST(Arm64ExceptionDirectoryTests, SaveRegAny)
{
	opcode::save_reg_any code;

	EXPECT_FALSE(code.is_reg_pair());
	code.set_reg_pair(true);
	EXPECT_TRUE(code.is_reg_pair());

	EXPECT_FALSE(code.is_negative_offset());
	code.set_negative_offset(true);
	EXPECT_TRUE(code.is_negative_offset());

	EXPECT_EQ(code.get_register_character(), opcode::register_character::x);
	EXPECT_NO_THROW(code.set_register_character(opcode::register_character::q));
	EXPECT_EQ(code.get_register_character(), opcode::register_character::q);

	EXPECT_EQ(code.get_register_or_register_pair(), 0u);
	EXPECT_NO_THROW(code.set_register_or_register_pair(31u));
	expect_throw_pe_error([&code] {
		code.set_register_or_register_pair(32u);
	}, exception_directory_errc::invalid_register);
	EXPECT_EQ(code.get_register_or_register_pair(), 31u);

	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0u });
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 0x7fu });
	EXPECT_EQ(code.get_descriptor()[2], std::byte{ 0x80u });

	EXPECT_EQ(code.get_offset(), 16u);
	EXPECT_NO_THROW(code.set_offset(0x400u));
	expect_throw_pe_error([&code] {
		code.set_offset(0x400u + 16u);
	}, exception_directory_errc::invalid_offset);
	expect_throw_pe_error([&code] {
		code.set_offset(0x400u - 15u);
	}, exception_directory_errc::invalid_offset);
	EXPECT_EQ(code.get_offset(), 0x400u);
	EXPECT_EQ(code.get_descriptor()[2], std::byte{ 0xbfu });

	EXPECT_NO_THROW(code.set_offset(16u));
	code.set_negative_offset(false);
	EXPECT_EQ(code.get_offset(), 0u);
	EXPECT_NO_THROW(code.set_offset(0x3f0u));
	expect_throw_pe_error([&code] {
		code.set_offset(0x3f0u + 16u);
	}, exception_directory_errc::invalid_offset);
	expect_throw_pe_error([&code] {
		code.set_offset(0x3f0u - 15u);
	}, exception_directory_errc::invalid_offset);
	EXPECT_EQ(code.get_offset(), 0x3f0u);
	EXPECT_EQ(code.get_descriptor()[2], std::byte{ 0xbfu });
}
