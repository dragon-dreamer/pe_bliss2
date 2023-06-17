#include "gtest/gtest.h"

#include <cstddef>
#include <limits>
#include <string>
#include <variant>

#include "pe_bliss2/detail/load_config/image_load_config_directory.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/load_config/load_config_directory.h"
#include "pe_bliss2/pe_error.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::load_config;

TEST(LoadConfigDirectoryTests, VersionToMinRequiredWindowsVersion)
{
	EXPECT_EQ(version_to_min_required_windows_version(version::base),
		std::string("Windows 2000"));
	EXPECT_NE(version_to_min_required_windows_version(static_cast<version>(1000)),
		nullptr);
}

TEST(LoadConfigDirectoryTests, GuardFunctionBase)
{
	guard_function_base<> base;
	EXPECT_EQ(base.get_additional_data()[0], std::byte{});
	EXPECT_EQ(base.get_flags(), 0u);
	base.set_flags(gfids_flags::export_suppressed);
	EXPECT_EQ(base.get_flags(), gfids_flags::export_suppressed);
	EXPECT_EQ(base.get_additional_data()[0],
		std::byte{ gfids_flags::export_suppressed });
}

TEST(LoadConfigDirectoryTests, ChpeArm64xCodeRangeEntry)
{
	chpe_arm64x_code_range_entry entry;
	EXPECT_EQ(entry.get_code_type(), chpe_arm64x_range_code_type::arm64);
	EXPECT_EQ(entry.get_rva(), 0u);

	entry.get_entry()->start_offset = (std::numeric_limits<std::uint32_t>::max)();
	entry.set_code_type(chpe_arm64x_range_code_type::arm64ec);
	EXPECT_EQ(entry.get_code_type(), chpe_arm64x_range_code_type::arm64ec);
	EXPECT_EQ((entry.get_entry()->start_offset & 0b11u),
		static_cast<std::uint8_t>(chpe_arm64x_range_code_type::arm64ec));
	EXPECT_EQ(entry.get_rva(), (std::numeric_limits<std::uint32_t>::max)() & ~0b11u);
}

TEST(LoadConfigDirectoryTests, ChpeX86CodeRangeEntry)
{
	chpe_x86_code_range_entry entry;
	EXPECT_EQ(entry.get_code_type(), chpe_x86_range_code_type::arm64);
	EXPECT_EQ(entry.get_rva(), 0u);

	entry.get_entry()->start_offset = (std::numeric_limits<std::uint32_t>::max)();
	entry.set_code_type(chpe_x86_range_code_type::arm64);
	EXPECT_EQ(entry.get_code_type(), chpe_x86_range_code_type::arm64);
	EXPECT_EQ((entry.get_entry()->start_offset & 0b1u),
		static_cast<std::uint8_t>(chpe_x86_range_code_type::arm64));
	EXPECT_EQ(entry.get_rva(), (std::numeric_limits<std::uint32_t>::max)() & ~0b1u);
}

TEST(LoadConfigDirectoryTests, ChpeX86MetadataBase)
{
	chpe_x86_metadata_base<> base;
	base.get_version().get() = 1u;
	EXPECT_EQ(base.get_metadata_size(), detail::packed_reflection::get_field_offset<
		&detail::load_config::image_chpe_metadata_x86::compiler_iat_pointer>());

	base.get_version().get() = 2u;
	EXPECT_EQ(base.get_metadata_size(), detail::packed_reflection::get_field_offset<
		&detail::load_config::image_chpe_metadata_x86::wow_a64_rdtsc_function_pointer>());

	base.get_version().get() = 3u;
	EXPECT_EQ(base.get_metadata_size(), detail::packed_reflection::get_type_size<
		detail::load_config::image_chpe_metadata_x86>());
}

TEST(LoadConfigDirectoryTests, DynamicRelocationBase1)
{
	dynamic_relocation_base<std::uint32_t> base;
	base.get_relocation().get() = (std::numeric_limits<std::uint32_t>::max)();
	EXPECT_EQ(base.get_page_relative_offset(), 0xfffu);
	expect_throw_pe_error([&base] {
		base.set_page_relative_offset(0x1000u);
	}, load_config_errc::invalid_page_relative_offset);
	EXPECT_NO_THROW(base.set_page_relative_offset(0x123u));
	EXPECT_EQ(base.get_page_relative_offset(), 0x123u);
}

TEST(LoadConfigDirectoryTests, DynamicRelocationBase2)
{
	struct dummy
	{
		std::uint32_t metadata{};
	};

	dynamic_relocation_base<dummy> base;
	base.get_relocation()->metadata = (std::numeric_limits<std::uint32_t>::max)();
	EXPECT_EQ(base.get_page_relative_offset(), 0xfffu);
	EXPECT_THROW(base.set_page_relative_offset(0x1000u), pe_error);
	EXPECT_NO_THROW(base.set_page_relative_offset(0x123u));
	EXPECT_EQ(base.get_page_relative_offset(), 0x123u);
}

TEST(LoadConfigDirectoryTests, ImportControlTransferDynamicRelocationIndirectCall)
{
	import_control_transfer_dynamic_relocation reloc;

	EXPECT_EQ(reloc.get_relocation()->metadata, 0u);
	EXPECT_FALSE(reloc.is_indirect_call());
	reloc.set_indirect_call(true);
	EXPECT_EQ(reloc.get_relocation()->metadata, 0x1000u);
	EXPECT_TRUE(reloc.is_indirect_call());
}

TEST(LoadConfigDirectoryTests, ImportControlTransferDynamicRelocationIatIndex)
{
	import_control_transfer_dynamic_relocation reloc;

	EXPECT_EQ(reloc.get_relocation()->metadata, 0u);
	EXPECT_EQ(reloc.get_iat_index(), 0u);
	expect_throw_pe_error([&reloc] {
		reloc.set_iat_index(import_control_transfer_dynamic_relocation::max_iat_index + 1u);
	}, load_config_errc::invalid_iat_index);
	EXPECT_EQ(reloc.get_iat_index(), 0u);

	reloc.get_relocation()->metadata = (std::numeric_limits<std::uint32_t>::max)();
	EXPECT_NO_THROW(reloc.set_iat_index(0x12345u));
	EXPECT_EQ(reloc.get_iat_index(), 0x12345u);
	EXPECT_EQ(reloc.get_relocation()->metadata, 0x2468bfffu);
}

TEST(LoadConfigDirectoryTests, IndirControlTransferDynamicRelocationIndirectCall)
{
	indir_control_transfer_dynamic_relocation reloc;

	EXPECT_EQ(reloc.get_relocation()->metadata, 0u);
	EXPECT_FALSE(reloc.is_indirect_call());
	reloc.set_indirect_call(true);
	EXPECT_EQ(reloc.get_relocation()->metadata, 0x1000u);
	EXPECT_TRUE(reloc.is_indirect_call());
}

TEST(LoadConfigDirectoryTests, IndirControlTransferDynamicRelocationRexWPrefix)
{
	indir_control_transfer_dynamic_relocation reloc;

	EXPECT_EQ(reloc.get_relocation()->metadata, 0u);
	EXPECT_FALSE(reloc.is_rex_w_prefix());
	reloc.set_rex_w_prefix(true);
	EXPECT_EQ(reloc.get_relocation()->metadata, 0x2000u);
	EXPECT_TRUE(reloc.is_rex_w_prefix());
}

TEST(LoadConfigDirectoryTests, IndirControlTransferDynamicRelocationCfgCheck)
{
	indir_control_transfer_dynamic_relocation reloc;

	EXPECT_EQ(reloc.get_relocation()->metadata, 0u);
	EXPECT_FALSE(reloc.is_cfg_check());
	reloc.set_cfg_check(true);
	EXPECT_EQ(reloc.get_relocation()->metadata, 0x4000u);
	EXPECT_TRUE(reloc.is_cfg_check());
}

TEST(LoadConfigDirectoryTests, SwitchtableBranchDynamicRelocation)
{
	switchtable_branch_dynamic_relocation reloc;

	EXPECT_EQ(reloc.get_relocation()->metadata, 0u);
	EXPECT_EQ(reloc.get_register(), switchtable_branch_dynamic_relocation::cpu_register::rax);
	reloc.get_relocation()->metadata = (std::numeric_limits<std::uint16_t>::max)();
	reloc.set_register(switchtable_branch_dynamic_relocation::cpu_register::r11);
	EXPECT_EQ(reloc.get_register(), switchtable_branch_dynamic_relocation::cpu_register::r11);
	EXPECT_EQ(reloc.get_relocation()->metadata, 0xbfffu);
}

TEST(LoadConfigDirectoryTests, Arm64xDynamicRelocation)
{
	arm64x_dynamic_relocation_base reloc;
	EXPECT_EQ(reloc.get_meta(), 0u);
	EXPECT_EQ(reloc.get_relocation()->metadata, 0u);
	EXPECT_EQ(reloc.get_type(), arm64x_dynamic_relocation_base::type::zero_fill);

	expect_throw_pe_error([&reloc] {
		reloc.set_meta(0x10u);
	}, load_config_errc::invalid_meta_value);
	EXPECT_EQ(reloc.get_meta(), 0u);
	
	EXPECT_NO_THROW(reloc.set_meta(0xau));
	EXPECT_EQ(reloc.get_meta(), 0xau);
	EXPECT_EQ(reloc.get_relocation()->metadata, 0xa000u);

	reloc.get_relocation()->metadata = 0x1000u;
	EXPECT_EQ(reloc.get_type(), arm64x_dynamic_relocation_base::type::copy_data);
}

TEST(LoadConfigDirectoryTests, Arm64xDynamicRelocationSized)
{
	arm64x_dynamic_relocation_sized_base reloc;
	EXPECT_EQ(reloc.get_size(), 1u);

	expect_throw_pe_error([&reloc] {
		reloc.set_size(10u);
	}, load_config_errc::invalid_size_value);

	expect_throw_pe_error([&reloc] {
		reloc.set_size(3u);
	}, load_config_errc::invalid_size_value);

	EXPECT_NO_THROW(reloc.set_size(8u));
	EXPECT_EQ(reloc.get_meta(), 0xcu);
	EXPECT_EQ(reloc.get_size(), 8u);

	EXPECT_NO_THROW(reloc.set_size(4u));
	EXPECT_EQ(reloc.get_meta(), 0x8u);
	EXPECT_EQ(reloc.get_size(), 4u);

	EXPECT_NO_THROW(reloc.set_size(1u));
	EXPECT_EQ(reloc.get_size(), 1u);
}

TEST(LoadConfigDirectoryTests, Arm64xDynamicRelocationAddDelta)
{
	arm64x_dynamic_relocation_add_delta_base<> reloc;
	EXPECT_EQ(reloc.get_value().get(), 0u);
	EXPECT_EQ(reloc.get_delta(), 0);
	EXPECT_EQ(reloc.get_multiplier(),
		arm64x_dynamic_relocation_add_delta_base<>::multiplier::multiplier_4);
	EXPECT_EQ(reloc.get_sign(),
		arm64x_dynamic_relocation_add_delta_base<>::sign::plus);

	reloc.set_sign(arm64x_dynamic_relocation_add_delta_base<>::sign::minus);
	EXPECT_EQ(reloc.get_sign(),
		arm64x_dynamic_relocation_add_delta_base<>::sign::minus);
	EXPECT_EQ(reloc.get_meta(), 0b100u);

	reloc.set_multiplier(
		arm64x_dynamic_relocation_add_delta_base<>::multiplier::multiplier_8);
	EXPECT_EQ(reloc.get_multiplier(),
		arm64x_dynamic_relocation_add_delta_base<>::multiplier::multiplier_8);
	EXPECT_EQ(reloc.get_meta(), 0b1100u);

	reloc.get_value().get() = 123;
	EXPECT_EQ(reloc.get_delta(), 123 * 8 * -1);
}

TEST(LoadConfigDirectoryTests, EpilogueBranchDescriptor)
{
	epilogue_branch_descriptor descr;
	EXPECT_EQ(descr.get_descriptor().get(), 0u);
	EXPECT_EQ(descr.get_instr_size(), 0u);
	EXPECT_EQ(descr.get_disp_size(), 0u);
	EXPECT_EQ(descr.get_disp_offset(), 0u);

	expect_throw_pe_error([&descr] {
		descr.set_instr_size(0x10u);
	}, load_config_errc::invalid_instr_size);
	EXPECT_EQ(descr.get_instr_size(), 0u);

	expect_throw_pe_error([&descr] {
		descr.set_disp_offset(0x10u);
	}, load_config_errc::invalid_disp_offset);
	EXPECT_EQ(descr.get_disp_offset(), 0u);

	expect_throw_pe_error([&descr] {
		descr.set_disp_size(0x10u);
	}, load_config_errc::invalid_disp_size);
	EXPECT_EQ(descr.get_disp_size(), 0u);

	EXPECT_NO_THROW(descr.set_instr_size(0xau));
	EXPECT_EQ(descr.get_instr_size(), 0xau);
	EXPECT_EQ(descr.get_descriptor().get(), 0xau);

	EXPECT_NO_THROW(descr.set_disp_offset(0xbu));
	EXPECT_EQ(descr.get_disp_offset(), 0xbu);
	EXPECT_EQ(descr.get_descriptor().get(), 0xbau);

	EXPECT_NO_THROW(descr.set_disp_size(0xcu));
	EXPECT_EQ(descr.get_disp_size(), 0xcu);
	EXPECT_EQ(descr.get_descriptor().get(), 0xcbau);
}

TEST(LoadConfigDirectoryTests, LoadConfigDirectoryBase)
{
	load_config_directory_base<> dir32(false);
	EXPECT_TRUE(std::holds_alternative<
		load_config_directory_impl<detail::load_config::image_load_config_directory32>>(
			dir32.get_value()));

	load_config_directory_base<> dir64(true);
	EXPECT_TRUE(std::holds_alternative<
		load_config_directory_impl<detail::load_config::image_load_config_directory64>>(
			dir64.get_value()));
}

namespace
{
template<typename T>
class LoadConfigDirectoryTestsFixture : public testing::Test
{
public:
	using type = T;
};

using tested_types = ::testing::Types<
	detail::load_config::image_load_config_directory32,
	detail::load_config::image_load_config_directory64>;
} //namespace

TYPED_TEST_SUITE(LoadConfigDirectoryTestsFixture, tested_types);

TYPED_TEST(LoadConfigDirectoryTestsFixture, LoadConfigDirectoryImplVersion)
{
	using type = typename TestFixture::type;
	load_config_directory_impl<type> dir;

	EXPECT_EQ(dir.get_descriptor_size(), 0u);
	EXPECT_EQ(dir.get_version(), version::base);
	EXPECT_FALSE(dir.version_exactly_matches());

	EXPECT_NO_THROW(dir.set_version(version::rf_guard_ex));
	EXPECT_EQ(dir.get_size().get(), (
		detail::packed_reflection::get_field_offset<&type::enclave>()
		+ sizeof(std::uint32_t) /* size field size */));
	EXPECT_EQ(dir.get_descriptor_size(),
		detail::packed_reflection::get_field_offset<&type::enclave>());
	EXPECT_EQ(dir.get_version(), version::rf_guard_ex);
	EXPECT_TRUE(dir.version_exactly_matches());

	expect_throw_pe_error([&dir] {
		dir.set_version(static_cast<version>(0x1000u));
	}, load_config_errc::unknown_load_config_version);
	EXPECT_EQ(dir.get_version(), version::rf_guard_ex);
}

TYPED_TEST(LoadConfigDirectoryTestsFixture,
	LoadConfigDirectoryImplGuardCfFunctionTableStride)
{
	using type = typename TestFixture::type;
	load_config_directory_impl<type> dir;
	EXPECT_EQ(dir.get_guard_cf_function_table_stride(), 0u);

	expect_throw_pe_error([&dir] {
		dir.set_guard_cf_function_table_stride(0x10u);
	}, load_config_errc::invalid_stride_value);
	EXPECT_EQ(dir.get_guard_cf_function_table_stride(), 0u);
	
	EXPECT_NO_THROW(dir.set_guard_cf_function_table_stride(0xau));
	EXPECT_EQ(dir.get_guard_cf_function_table_stride(), 0xau);
	EXPECT_EQ(dir.get_descriptor()->cf_guard.guard_flags, 0xa0000000u);
}

TEST(LoadConfigDirectoryTests, FunctionOverrideDynamicRelocation)
{
	function_override_base_relocation reloc;
	EXPECT_EQ(reloc.get_type(), function_override_base_relocation::type::invalid);
	reloc.get_relocation().get() = 0x0abcu;
	reloc.set_type(function_override_base_relocation::type::arm64_thunk);
	EXPECT_EQ(reloc.get_type(), function_override_base_relocation::type::arm64_thunk);
	EXPECT_EQ(reloc.get_relocation().get(), 0x3abcu);
}
