#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

#include "pe_bliss2/exceptions/arm/arm_exception_directory.h"
#include "tests/tests/pe_bliss2/directories/arm_common_exception_helpers.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::exceptions;
using namespace pe_bliss::exceptions::arm;


TEST(ArmExceptionDirectoryTests, Decode)
{
	using codes = unwind_record_options::unwind_code_type;
	test_created_unwind_code<codes, opcode::alloc_s>(0u);
	test_created_unwind_code<codes, opcode::alloc_s>(0x7fu);
	test_created_unwind_code<codes, opcode::save_r0r12_lr>(0x80u);
	test_created_unwind_code<codes, opcode::save_r0r12_lr>(0xbfu);
	test_created_unwind_code<codes, opcode::mov_sprx>(0xc0u);
	test_created_unwind_code<codes, opcode::mov_sprx>(0xcfu);
	test_created_unwind_code<codes, opcode::save_r4rx_lr>(0xd0u);
	test_created_unwind_code<codes, opcode::save_r4rx_lr>(0xd7u);
	test_created_unwind_code<codes, opcode::save_r4rx_lr_wide>(0xd8u);
	test_created_unwind_code<codes, opcode::save_r4rx_lr_wide>(0xdfu);
	test_created_unwind_code<codes, opcode::save_d8dx>(0xe0u);
	test_created_unwind_code<codes, opcode::save_d8dx>(0xe7u);
	test_created_unwind_code<codes, opcode::alloc_s_wide>(0xe8u);
	test_created_unwind_code<codes, opcode::alloc_s_wide>(0xebu);
	test_created_unwind_code<codes, opcode::save_r0r7_lr>(0xecu);
	test_created_unwind_code<codes, opcode::save_r0r7_lr>(0xedu);
	test_created_unwind_code<codes, opcode::ms_specific>(0xeeu);
	test_created_unwind_code<codes, opcode::ldr_lr_sp>(0xefu);
	expect_throw_pe_error([] {
		std::vector<codes> vec;
		arm_common::create_unwind_code(std::byte{ 0xf0u }, vec);
	}, arm_common::exception_directory_errc::unsupported_unwind_code);
	expect_throw_pe_error([] {
		std::vector<codes> vec;
		arm_common::create_unwind_code(std::byte{ 0xf4u }, vec);
	}, arm_common::exception_directory_errc::unsupported_unwind_code);
	test_created_unwind_code<codes, opcode::save_dsde>(0xf5u);
	test_created_unwind_code<codes, opcode::save_dsde_16>(0xf6u);
	test_created_unwind_code<codes, opcode::alloc_m>(0xf7u);
	test_created_unwind_code<codes, opcode::alloc_l>(0xf8u);
	test_created_unwind_code<codes, opcode::alloc_m_wide>(0xf9u);
	test_created_unwind_code<codes, opcode::alloc_l_wide>(0xfau);
	test_created_unwind_code<codes, opcode::nop>(0xfbu);
	test_created_unwind_code<codes, opcode::nop_wide>(0xfcu);
	test_created_unwind_code<codes, opcode::end_nop>(0xfdu);
	test_created_unwind_code<codes, opcode::end_nop_wide>(0xfeu);
	test_created_unwind_code<codes, opcode::end>(0xffu);
}

TEST(ArmExceptionDirectoryTests, AllocS)
{
	opcode::alloc_s code;
	EXPECT_EQ(code.get_allocation_size(), 0u);

	expect_throw_pe_error([&code] {
		code.set_allocation_size(0x200u);
	}, exception_directory_errc::invalid_allocation_size);
	EXPECT_EQ(code.get_allocation_size(), 0u);

	expect_throw_pe_error([&code] {
		code.set_allocation_size(0x1fbu);
	}, exception_directory_errc::invalid_allocation_size);
	EXPECT_EQ(code.get_allocation_size(), 0u);

	EXPECT_NO_THROW(code.set_allocation_size(0x1fcu));
	EXPECT_EQ(code.get_allocation_size(), 0x1fcu);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0x7fu });
}

TEST(ArmExceptionDirectoryTests, SaveR0R12Lr)
{
	opcode::save_r0r12_lr code;
	EXPECT_EQ(code.get_saved_registers(), 0u);
	EXPECT_NO_THROW(code.set_saved_registers(
		static_cast<opcode::int_registers::value>(
			opcode::int_registers::lr | opcode::int_registers::r0)));
	EXPECT_EQ(code.get_saved_registers(),
		opcode::int_registers::lr | opcode::int_registers::r0);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0x20u });
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 0x1u });
}

TEST(ArmExceptionDirectoryTests, MovSpRx)
{
	opcode::mov_sprx code;
	EXPECT_EQ(code.get_delta(), 0u);
	EXPECT_NO_THROW(code.set_delta(0xfu));
	EXPECT_EQ(code.get_delta(), 0xfu);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0xfu });

	expect_throw_pe_error([&code] {
		code.set_delta(0x10u);
	}, exception_directory_errc::invalid_delta);
	EXPECT_EQ(code.get_delta(), 0xfu);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0xfu });
}

TEST(ArmExceptionDirectoryTests, SaveR4RxLr)
{
	opcode::save_r4rx_lr code;
	EXPECT_EQ(code.get_saved_registers(), opcode::int_registers::r4);
	static constexpr auto registers = static_cast<opcode::int_registers::value>(
		opcode::int_registers::r6 | opcode::int_registers::r5
		| opcode::int_registers::r4 | opcode::int_registers::lr);
	EXPECT_NO_THROW(code.set_saved_registers(registers));
	EXPECT_EQ(code.get_saved_registers(), registers);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0x6u });

	expect_throw_pe_error([&code] {
		code.set_saved_registers(opcode::int_registers::r8);
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);

	expect_throw_pe_error([&code] {
		code.set_saved_registers(static_cast<opcode::int_registers::value>(
			opcode::int_registers::r4 | opcode::int_registers::r9));
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);

	expect_throw_pe_error([&code] {
		code.set_saved_registers(static_cast<opcode::int_registers::value>(
			opcode::int_registers::r4 | opcode::int_registers::r6));
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);

	expect_throw_pe_error([&code] {
		code.set_saved_registers(static_cast<opcode::int_registers::value>(
			opcode::int_registers::r8
			| opcode::int_registers::r7 | opcode::int_registers::r6
			| opcode::int_registers::r5 | opcode::int_registers::r4
			| opcode::int_registers::lr));
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);
}

TEST(ArmExceptionDirectoryTests, SaveR4RxLrWide)
{
	opcode::save_r4rx_lr_wide code;
	EXPECT_EQ(code.get_saved_registers(),
		opcode::int_registers::r8
		| opcode::int_registers::r7 | opcode::int_registers::r6
		| opcode::int_registers::r5 | opcode::int_registers::r4);
	static constexpr auto registers = static_cast<opcode::int_registers::value>(
		opcode::int_registers::r9 | opcode::int_registers::r8
		| opcode::int_registers::r7 | opcode::int_registers::r6
		| opcode::int_registers::r5 | opcode::int_registers::r4
		| opcode::int_registers::lr);
	EXPECT_NO_THROW(code.set_saved_registers(registers));
	EXPECT_EQ(code.get_saved_registers(), registers);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 0x5u });

	expect_throw_pe_error([&code] {
		code.set_saved_registers(opcode::int_registers::r8);
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);

	expect_throw_pe_error([&code] {
		code.set_saved_registers(static_cast<opcode::int_registers::value>(
			opcode::int_registers::r4 | opcode::int_registers::r11));
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);

	expect_throw_pe_error([&code] {
		code.set_saved_registers(static_cast<opcode::int_registers::value>(
			opcode::int_registers::r4 | opcode::int_registers::r6));
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);

	expect_throw_pe_error([&code] {
		code.set_saved_registers(static_cast<opcode::int_registers::value>(
			opcode::int_registers::r12
			| opcode::int_registers::r11 | opcode::int_registers::r10
			| opcode::int_registers::r9 | opcode::int_registers::r8
			| opcode::int_registers::r7 | opcode::int_registers::r6
			| opcode::int_registers::r5 | opcode::int_registers::r4
			| opcode::int_registers::lr));
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);
}

TEST(ArmExceptionDirectoryTests, SaveD8Dx)
{
	opcode::save_d8dx code;
	EXPECT_EQ(code.get_saved_registers(), opcode::fp_registers::d8);
	static constexpr auto registers = static_cast<opcode::fp_registers::value>(
		opcode::fp_registers::d9 | opcode::fp_registers::d8);
	EXPECT_NO_THROW(code.set_saved_registers(registers));
	EXPECT_EQ(code.get_saved_registers(), registers);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 1u });

	expect_throw_pe_error([&code] {
		code.set_saved_registers(opcode::fp_registers::d7);
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);

	expect_throw_pe_error([&code] {
		code.set_saved_registers(static_cast<opcode::fp_registers::value>(
			opcode::fp_registers::d8 | opcode::fp_registers::d10));
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);
}

TEST(ArmExceptionDirectoryTests, AllocSWide)
{
	opcode::alloc_s_wide code;
	EXPECT_EQ(code.get_allocation_size(), 0u);
	EXPECT_NO_THROW(code.set_allocation_size(32u));
	EXPECT_EQ(code.get_allocation_size(), 32u);
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 8u });

	expect_throw_pe_error([&code] {
		code.set_allocation_size(31u);
	}, exception_directory_errc::invalid_allocation_size);
	EXPECT_EQ(code.get_allocation_size(), 32u);

	expect_throw_pe_error([&code] {
		code.set_allocation_size(0x1000u);
	}, exception_directory_errc::invalid_allocation_size);
	EXPECT_EQ(code.get_allocation_size(), 32u);
}

TEST(ArmExceptionDirectoryTests, SaveR0R7Lr)
{
	opcode::save_r0r7_lr code;
	EXPECT_EQ(code.get_saved_registers(), 0u);
	static constexpr auto registers = static_cast<opcode::int_registers::value>(
		opcode::int_registers::lr | opcode::int_registers::r1
		| opcode::int_registers::r7);
	EXPECT_NO_THROW(code.set_saved_registers(registers));
	EXPECT_EQ(code.get_saved_registers(), registers);
	EXPECT_EQ(code.get_descriptor()[0], std::byte{ 1u });
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 0x82u });

	expect_throw_pe_error([&code] {
		code.set_saved_registers(opcode::int_registers::r8);
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers(), registers);

	EXPECT_NO_THROW(code.set_saved_registers({}));
	EXPECT_EQ(code.get_saved_registers(), 0u);
}

TEST(ArmExceptionDirectoryTests, MsSpecific)
{
	opcode::ms_specific code;
	EXPECT_FALSE(code.is_available_opcode());
	code.get_descriptor()[1] = std::byte{ 0xfu };
	EXPECT_FALSE(code.is_available_opcode());
	code.get_descriptor()[1] = std::byte{ 0x10u };
	EXPECT_TRUE(code.is_available_opcode());
}

TEST(ArmExceptionDirectoryTests, LdrLrSp)
{
	opcode::ldr_lr_sp code;
	EXPECT_FALSE(code.is_available_opcode());
	EXPECT_EQ(code.get_delta(), 0u);

	EXPECT_NO_THROW(code.set_delta(0x3cu));
	EXPECT_EQ(code.get_delta(), 0x3cu);
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 0xfu });

	expect_throw_pe_error([&code] {
		code.set_delta(1u);
	}, exception_directory_errc::invalid_delta);
	EXPECT_EQ(code.get_delta(), 0x3cu);

	expect_throw_pe_error([&code] {
		code.set_delta(0x40u);
	}, exception_directory_errc::invalid_delta);
	EXPECT_EQ(code.get_delta(), 0x3cu);

	code.get_descriptor()[1] = std::byte{ 0xfu };
	EXPECT_FALSE(code.is_available_opcode());
	code.get_descriptor()[1] = std::byte{ 0x10u };
	EXPECT_TRUE(code.is_available_opcode());
}

TEST(ArmExceptionDirectoryTests, SaveDsDe)
{
	opcode::save_dsde code;
	EXPECT_EQ(code.get_saved_registers_range(),
		(std::pair{ opcode::fp_register::d0, opcode::fp_register::d0 }));

	EXPECT_NO_THROW(code.set_saved_registers_range(
		{ opcode::fp_register::d1, opcode::fp_register::d15 }));
	EXPECT_EQ(code.get_saved_registers_range(),
		(std::pair{ opcode::fp_register::d1, opcode::fp_register::d15 }));

	expect_throw_pe_error([&code] {
		code.set_saved_registers_range(
			{ opcode::fp_register::d5, opcode::fp_register::d1 });
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers_range(),
		(std::pair{ opcode::fp_register::d1, opcode::fp_register::d15 }));

	expect_throw_pe_error([&code] {
		code.set_saved_registers_range(
			{ opcode::fp_register::d15, opcode::fp_register::d17 });
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers_range(),
		(std::pair{ opcode::fp_register::d1, opcode::fp_register::d15 }));
}

TEST(ArmExceptionDirectoryTests, SaveDsDe16)
{
	opcode::save_dsde_16 code;
	EXPECT_EQ(code.get_saved_registers_range(),
		(std::pair{ opcode::fp_register::d16, opcode::fp_register::d16 }));

	EXPECT_NO_THROW(code.set_saved_registers_range(
		{ opcode::fp_register::d17, opcode::fp_register::d31 }));
	EXPECT_EQ(code.get_saved_registers_range(),
		(std::pair{ opcode::fp_register::d17, opcode::fp_register::d31 }));

	expect_throw_pe_error([&code] {
		code.set_saved_registers_range(
			{ opcode::fp_register::d30, opcode::fp_register::d25 });
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers_range(),
		(std::pair{ opcode::fp_register::d17, opcode::fp_register::d31 }));

	expect_throw_pe_error([&code] {
		code.set_saved_registers_range(
			{ opcode::fp_register::d15, opcode::fp_register::d17 });
	}, exception_directory_errc::invalid_registers);
	EXPECT_EQ(code.get_saved_registers_range(),
		(std::pair{ opcode::fp_register::d17, opcode::fp_register::d31 }));
}

TEST(ArmExceptionDirectoryTests, AllocMBase)
{
	opcode::alloc_m_base<0> code;
	EXPECT_EQ(code.get_allocation_size(), 0u);

	EXPECT_NO_THROW(code.set_allocation_size(0x3ebecu));
	EXPECT_EQ(code.get_allocation_size(), 0x3ebecu);
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 0xfau });
	EXPECT_EQ(code.get_descriptor()[2], std::byte{ 0xfbu });

	expect_throw_pe_error([&code] {
		code.set_allocation_size(31u);
	}, exception_directory_errc::invalid_allocation_size);
	EXPECT_EQ(code.get_allocation_size(), 0x3ebecu);

	expect_throw_pe_error([&code] {
		code.set_allocation_size(0x40000u);
	}, exception_directory_errc::invalid_allocation_size);
	EXPECT_EQ(code.get_allocation_size(), 0x3ebecu);
}

TEST(ArmExceptionDirectoryTests, AllocLBase)
{
	opcode::alloc_l_base<0> code;
	EXPECT_EQ(code.get_allocation_size(), 0u);

	EXPECT_NO_THROW(code.set_allocation_size(0x3ebeff0u));
	EXPECT_EQ(code.get_allocation_size(), 0x3ebeff0u);
	EXPECT_EQ(code.get_descriptor()[1], std::byte{ 0xfau });
	EXPECT_EQ(code.get_descriptor()[2], std::byte{ 0xfbu });
	EXPECT_EQ(code.get_descriptor()[3], std::byte{ 0xfcu });

	expect_throw_pe_error([&code] {
		code.set_allocation_size(31u);
	}, exception_directory_errc::invalid_allocation_size);
	EXPECT_EQ(code.get_allocation_size(), 0x3ebeff0u);

	expect_throw_pe_error([&code] {
		code.set_allocation_size(0x4000000u);
	}, exception_directory_errc::invalid_allocation_size);
	EXPECT_EQ(code.get_allocation_size(), 0x3ebeff0u);
}

TEST(ArmExceptionDirectoryTests, PackedUnwundData)
{
	packed_unwind_data data(123u);
	EXPECT_EQ(data.get_packed_value(), 123u);
	data.set_packed_value(0u);

	data.set_flag(packed_unwind_data::flag::packed_unwind_fragment);
	EXPECT_EQ(data.get_flag(), packed_unwind_data::flag::packed_unwind_fragment);

	EXPECT_EQ(data.get_function_length(), 0u);
	EXPECT_NO_THROW(data.set_function_length(0xffeu));
	expect_throw_pe_error([&data] {
		data.set_function_length(0x3u);
	}, exception_directory_errc::invalid_function_length);
	expect_throw_pe_error([&data] {
		data.set_function_length(0x1000u);
	}, exception_directory_errc::invalid_function_length);
	EXPECT_EQ(data.get_function_length(), 0xffeu);

	EXPECT_EQ(data.get_ret(), packed_unwind_data::ret::pop_pc);
	data.set_ret(packed_unwind_data::ret::branch_16bit);
	EXPECT_EQ(data.get_ret(), packed_unwind_data::ret::branch_16bit);

	EXPECT_FALSE(data.homes_integer_parameter_registers());
	data.set_homes_integer_parameter_registers(true);
	EXPECT_TRUE(data.homes_integer_parameter_registers());

	EXPECT_EQ(data.get_saved_non_volatile_registers(),
		packed_unwind_data::saved_non_volatile_registers::integer);
	EXPECT_EQ(data.get_last_saved_non_volatile_register_index(), 4u);
	EXPECT_NO_THROW(data.set_last_saved_non_volatile_register_index(11u));
	expect_throw_pe_error([&data] {
		data.set_last_saved_non_volatile_register_index(12u);
	}, exception_directory_errc::invalid_non_volatile_register_count);
	expect_throw_pe_error([&data] {
		data.set_last_saved_non_volatile_register_index(3u);
	}, exception_directory_errc::invalid_non_volatile_register_count);
	EXPECT_EQ(data.get_last_saved_non_volatile_register_index(), 11u);

	expect_throw_pe_error([&data] {
		data.set_saved_non_volatile_registers(
			packed_unwind_data::saved_non_volatile_registers::floating_point);
	}, exception_directory_errc::invalid_non_volatile_register_count);
	EXPECT_NO_THROW(data.set_last_saved_non_volatile_register_index(10u));
	EXPECT_NO_THROW(data.set_saved_non_volatile_registers(
		packed_unwind_data::saved_non_volatile_registers::floating_point));
	EXPECT_EQ(data.get_saved_non_volatile_registers(),
		packed_unwind_data::saved_non_volatile_registers::floating_point);
	EXPECT_EQ(data.get_last_saved_non_volatile_register_index(), 14u);
	EXPECT_EQ(data.get_packed_value() & 0b11110000000000000000u, 0b11100000000000000000u);
	EXPECT_NO_THROW(data.set_saved_non_volatile_registers(
		packed_unwind_data::saved_non_volatile_registers::none));
	EXPECT_EQ(data.get_saved_non_volatile_registers(),
		packed_unwind_data::saved_non_volatile_registers::none);

	EXPECT_FALSE(data.save_restore_lr());
	data.set_save_restore_lr(true);
	EXPECT_TRUE(data.save_restore_lr());

	EXPECT_FALSE(data.includes_extra_instructions());
	data.set_includes_extra_instructions(true);
	EXPECT_TRUE(data.includes_extra_instructions());

	EXPECT_EQ(data.get_stack_adjust(), 0u);
	EXPECT_FALSE(data.get_stack_adjust_flags());
	EXPECT_NO_THROW(data.set_stack_adjust(0x3f3u * 4u));
	EXPECT_EQ(data.get_stack_adjust(), 0x3f3u * 4u);
	EXPECT_EQ(data.get_packed_value() & 0xffc00000u, 0xfcc00000u);
	expect_throw_pe_error([&data] {
		data.set_stack_adjust_flags({});
	}, exception_directory_errc::invalid_stack_adjust_value);

	EXPECT_NO_THROW(data.set_stack_adjust(0x3f0u * 4u));
	EXPECT_NO_THROW(data.set_stack_adjust_flags({
		.stack_adjustment_words_number = 4u, .prologue_folding = true }));
	EXPECT_EQ(data.get_stack_adjust(), 0x3f0u * 4u);
	expect_throw_pe_error([&data] {
		data.set_stack_adjust_flags({});
	}, exception_directory_errc::invalid_stack_adjust_flags);
	EXPECT_EQ(data.get_stack_adjust(), 0x3f0u * 4u);
	EXPECT_EQ(data.get_stack_adjust_flags(), (stack_adjust_flags{
		.stack_adjustment_words_number = 4u,
		.prologue_folding = true,
		.epilogue_folding = false
	}));
	EXPECT_EQ(data.get_packed_value() & 0xffc00000u, 0xfdc00000u);

	EXPECT_EQ(data.get_packed_value() & 0b11u, 0b10u);
	EXPECT_EQ(data.get_packed_value() & 0b1111111111100u, 0b1111111111100u);
	EXPECT_EQ(data.get_packed_value() & 0b110000000000000u, 0b010000000000000u);
	EXPECT_EQ(data.get_packed_value() & 0b1000000000000000u, 0b1000000000000000u);
	EXPECT_EQ(data.get_packed_value() & 0b11110000000000000000u, 0b11110000000000000000u);
	EXPECT_EQ(data.get_packed_value() & 0b100000000000000000000u, 0b100000000000000000000u);
	EXPECT_EQ(data.get_packed_value() & 0b1000000000000000000000u, 0b1000000000000000000000u);
}
