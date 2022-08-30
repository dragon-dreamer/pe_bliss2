#include "gtest/gtest.h"

#include <type_traits>

#include "pe_bliss2/exceptions/x64/x64_exception_directory.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::exceptions::x64;

namespace
{
template<typename T>
class X64ExceptionDirectoryTests : public testing::Test
{
public:
	static constexpr std::uint8_t node_size = T::value;
};

using tested_types = ::testing::Types<
	std::integral_constant<std::uint8_t, 0u>,
	std::integral_constant<std::uint8_t, 1u>,
	std::integral_constant<std::uint8_t, 2u>>;
} //namespace

TYPED_TEST_SUITE(X64ExceptionDirectoryTests, tested_types);

TYPED_TEST(X64ExceptionDirectoryTests, OpcodeBase)
{
	opcode_base<TestFixture::node_size> base;
	EXPECT_EQ(base.get_uwop_code(), opcode_id::push_nonvol);
	base.set_uwop_code(opcode_id::set_fpreg_large);
	EXPECT_EQ(base.get_uwop_code(), opcode_id::set_fpreg_large);
	EXPECT_EQ(base.get_descriptor()->unwind_operation_code_and_info,
		static_cast<std::uint8_t>(opcode_id::set_fpreg_large));

	EXPECT_EQ(base.get_operation_info(), 0u);
	EXPECT_NO_THROW(base.set_operation_info(0xdu));
	expect_throw_pe_error([&base] {
		base.set_operation_info(0x10u);
	}, exception_directory_errc::invalid_operation_info);
	EXPECT_EQ(base.get_operation_info(), 0xdu);
	EXPECT_EQ(base.get_descriptor()->unwind_operation_code_and_info,
		static_cast<std::uint8_t>(opcode_id::set_fpreg_large) | (0xdu << 4u));
}

TYPED_TEST(X64ExceptionDirectoryTests, OpcodeBaseWithRegister)
{
	opcode_base_with_register<TestFixture::node_size> base;
	EXPECT_EQ(base.get_register(), register_id::rax);
	EXPECT_NO_THROW(base.set_register(register_id::r15));
	EXPECT_EQ(base.get_register(), register_id::r15);
	EXPECT_EQ(base.get_descriptor()->unwind_operation_code_and_info,
		static_cast<std::uint8_t>(register_id::r15) << 4u);
}

TEST(X64ExceptionDirectoryTests, AllocLarge1)
{
	alloc_large<1u> code;
	EXPECT_EQ(code.get_allocation_size(), 0u);
	EXPECT_NO_THROW(code.set_allocation_size(0x7fff8u));
	expect_throw_pe_error([&code] {
		code.set_allocation_size(0x9u);
	}, exception_directory_errc::invalid_allocation_size);
	expect_throw_pe_error([&code] {
		code.set_allocation_size(0x7fff8u + 8u);
	}, exception_directory_errc::invalid_allocation_size);
	EXPECT_EQ(code.get_allocation_size(), 0x7fff8u);
	EXPECT_EQ(code.get_descriptor()->node, 0xffffu);
}

TEST(X64ExceptionDirectoryTests, AllocLarge2)
{
	alloc_large<2u> code;
	EXPECT_EQ(code.get_allocation_size(), 0u);
	code.set_allocation_size(0xffffffffu);
	EXPECT_EQ(code.get_allocation_size(), 0xffffffffu);
	EXPECT_EQ(code.get_descriptor()->node, 0xffffffffu);
}

TEST(X64ExceptionDirectoryTests, AllocSmall)
{
	alloc_small code;
	EXPECT_EQ(code.get_allocation_size(), 8u);
	EXPECT_NO_THROW(code.set_allocation_size(0x80u));
	expect_throw_pe_error([&code] {
		code.set_allocation_size(23u);
	}, exception_directory_errc::invalid_allocation_size);
	expect_throw_pe_error([&code] {
		code.set_allocation_size(0x81u);
	}, exception_directory_errc::invalid_allocation_size);
	EXPECT_EQ(code.get_allocation_size(), 0x80u);
	EXPECT_EQ(code.get_descriptor()->unwind_operation_code_and_info, 0xfu << 4u);
}

TEST(X64ExceptionDirectoryTests, SaveNonvol)
{
	save_nonvol code;
	EXPECT_EQ(code.get_stack_offset(), 0u);
	EXPECT_NO_THROW(code.set_stack_offset(0x7fff8u));
	expect_throw_pe_error([&code] {
		code.set_stack_offset(0x9u);
	}, exception_directory_errc::invalid_stack_offset);
	expect_throw_pe_error([&code] {
		code.set_stack_offset(0x7fff8u + 8u);
	}, exception_directory_errc::invalid_stack_offset);
	EXPECT_EQ(code.get_stack_offset(), 0x7fff8u);
	EXPECT_EQ(code.get_descriptor()->node, 0xffffu);
}

TEST(X64ExceptionDirectoryTests, SaveNonvolFar)
{
	save_nonvol_far code;
	EXPECT_EQ(code.get_stack_offset(), 0u);
	code.set_stack_offset(0xffffffffu);
	EXPECT_EQ(code.get_stack_offset(), 0xffffffffu);
	EXPECT_EQ(code.get_descriptor()->node, 0xffffffffu);
}

TEST(X64ExceptionDirectoryTests, Epilog)
{
	epilog code;
	EXPECT_EQ(code.get_size(), 0u);
	code.set_size(0xfu);
	expect_throw_pe_error([&code] {
		code.set_size(0x10u);
	}, exception_directory_errc::invalid_operation_info);
	EXPECT_EQ(code.get_size(), 0xfu);
	EXPECT_EQ(code.get_descriptor()->unwind_operation_code_and_info, 0xfu << 4u);
}

TEST(X64ExceptionDirectoryTests, SaveXmm128)
{
	save_xmm128 code;
	EXPECT_EQ(code.get_stack_offset(), 0u);
	EXPECT_NO_THROW(code.set_stack_offset(0xffff0u));
	expect_throw_pe_error([&code] {
		code.set_stack_offset(0x17u);
	}, exception_directory_errc::invalid_stack_offset);
	expect_throw_pe_error([&code] {
		code.set_stack_offset(0xffff0u + 16u);
	}, exception_directory_errc::invalid_stack_offset);
	EXPECT_EQ(code.get_stack_offset(), 0xffff0u);
	EXPECT_EQ(code.get_descriptor()->node, 0xffffu);
}

TEST(X64ExceptionDirectoryTests, SaveXmm128Far)
{
	save_xmm128_far code;
	EXPECT_EQ(code.get_stack_offset(), 0u);
	code.set_stack_offset(0xffffffffu);
	EXPECT_EQ(code.get_stack_offset(), 0xffffffffu);
	EXPECT_EQ(code.get_descriptor()->node, 0xffffffffu);
}

TEST(X64ExceptionDirectoryTests, PushMachframe)
{
	push_machframe code;
	EXPECT_FALSE(code.push_error_code());
	EXPECT_EQ(code.get_rsp_decrement(), 40u);
	code.set_push_error_code(true);
	EXPECT_TRUE(code.push_error_code());
	EXPECT_EQ(code.get_rsp_decrement(), 48u);
	EXPECT_EQ(code.get_descriptor()->unwind_operation_code_and_info, 1u << 4u);
}

TEST(X64ExceptionDirectoryTests, SetFpregLarge)
{
	set_fpreg_large code;
	EXPECT_EQ(code.get_offset(), 0u);
	EXPECT_NO_THROW(code.set_offset(0xffffffff0ull));
	expect_throw_pe_error([&code] {
		code.set_offset(0x17u);
	}, exception_directory_errc::invalid_scaled_frame_register_offset);
	expect_throw_pe_error([&code] {
		code.set_offset(0xffffffff0ull + 16u);
	}, exception_directory_errc::invalid_scaled_frame_register_offset);
	EXPECT_EQ(code.get_offset(), 0xffffffff0ull);
	EXPECT_EQ(code.get_descriptor()->node, 0xffffffffu);
}

TEST(X64ExceptionDirectoryTests, UnwindInfo)
{
	unwind_info info;
	EXPECT_EQ(info.get_unwind_flags(), 0u);
	EXPECT_NO_THROW(info.set_unwind_flags(unwind_flags::chaininfo));
	EXPECT_EQ(info.get_unwind_flags(), unwind_flags::chaininfo);
	EXPECT_NO_THROW(info.set_unwind_flags(static_cast<unwind_flags::value>(
		unwind_flags::ehandler | unwind_flags::uhandler)));
	expect_throw_pe_error([&info] {
		info.set_unwind_flags(static_cast<unwind_flags::value>(
			unwind_flags::chaininfo | unwind_flags::uhandler));
	}, exception_directory_errc::invalid_unwind_flags);
	EXPECT_EQ(info.get_unwind_flags(), unwind_flags::ehandler | unwind_flags::uhandler);

	EXPECT_EQ(info.get_version(), 0u);
	EXPECT_NO_THROW(info.set_version(7u));
	expect_throw_pe_error([&info] {
		info.set_version(8u);
	}, exception_directory_errc::invalid_unwind_info_version);
	EXPECT_EQ(info.get_version(), 7u);
	EXPECT_EQ(info.get_descriptor()->flags_and_version, 7u | (3u << 3u));

	EXPECT_FALSE(info.get_frame_register());
	EXPECT_NO_THROW(info.set_frame_register(register_id::r15));
	EXPECT_EQ(info.get_descriptor()->frame_register_and_offset, 15u);
	expect_throw_pe_error([&info] {
		info.set_frame_register(register_id::rax);
	}, exception_directory_errc::invalid_frame_register);
	EXPECT_EQ(info.get_frame_register(), register_id::r15);
	info.clear_frame_register();
	EXPECT_FALSE(info.get_frame_register());
	EXPECT_EQ(info.get_descriptor()->frame_register_and_offset, 0u);

	EXPECT_EQ(info.get_scaled_frame_register_offset(), 0u);
	EXPECT_NO_THROW(info.set_scaled_frame_register_offset(0xf0u));
	expect_throw_pe_error([&info] {
		info.set_scaled_frame_register_offset(0xdfu);
	}, exception_directory_errc::invalid_scaled_frame_register_offset);
	EXPECT_EQ(info.get_scaled_frame_register_offset(), 0xf0u);
	EXPECT_EQ(info.get_descriptor()->frame_register_and_offset, 0xf0u);
}
