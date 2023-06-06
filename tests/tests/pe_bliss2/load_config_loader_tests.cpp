#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/load_config/load_config_directory.h"
#include "pe_bliss2/load_config/load_config_directory_loader.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/pe_types.h"

#include "tests/pe_bliss2/image_helper.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{

class LoadConfigLoaderTestFixture
	: public ::testing::TestWithParam<core::optional_header::magic>
{
public:
	LoadConfigLoaderTestFixture(std::optional<core::optional_header::magic> magic = {})
		: is_x64_(magic == core::optional_header::magic::pe64
			|| GetParam() == core::optional_header::magic::pe64)
		, instance(create_test_image({
			.is_x64 = is_x64(),
			.image_base = image_base,
			.start_section_rva = section_rva,
			.sections = { { section_virtual_size, section_raw_size } } }))
	{
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
	}

	bool is_x64() const
	{
		return is_x64_;
	}

	std::uint32_t va_size() const
	{
		return is_x64() ? sizeof(std::uint64_t) : sizeof(std::uint32_t);
	}

	void add_load_config_directory()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::config).get()
			= { .virtual_address = directory_rva, .size = 0x1000u };
	}
	
	template<typename Func>
	void with_load_config(Func&& func,
		const load_config::loader_options& options = {})
	{
		auto result = load_config::load(instance, options);
		ASSERT_TRUE(result);
		if (is_x64())
		{
			ASSERT_TRUE(std::holds_alternative<
				load_config::load_config_directory_details::underlying_type64>(
					result->get_value()));
		}
		else
		{
			ASSERT_TRUE(std::holds_alternative<
				load_config::load_config_directory_details::underlying_type32>(
					result->get_value()));
		}

		std::visit(std::forward<Func>(func), result->get_value());
	}

	template<typename... Pairs>
	std::uint32_t get_directory_size(const Pairs&... pairs)
	{
		std::uint32_t total_size = sizeof(std::uint32_t); //size field
		if (is_x64())
			total_size += static_cast<std::uint32_t>((... + pairs.second.size()));
		else
			total_size += static_cast<std::uint32_t>((... + pairs.first.size()));
		return total_size;
	}

	template<typename... Pairs>
	void add_directory_parts(const Pairs&... pairs)
	{
		std::uint32_t total_size = get_directory_size(pairs...);

		auto* data = instance.get_section_data_list()[0].copied_data().data()
			+ (directory_rva - section_rva);
		data = detail::packed_serialization<>::serialize(total_size, data);

		if (is_x64())
			(..., (data = std::copy(pairs.second.begin(), pairs.second.end(), data)));
		else
			(..., (data = std::copy(pairs.first.begin(), pairs.first.end(), data)));
	}

	template<typename Array>
	void add_data(std::uint64_t va, const Array& arr)
	{
		auto* data = instance.get_section_data_list()[0].copied_data().data()
			+ (directory_rva - section_rva)
			+ (va - image_base - section_rva);
		std::copy(arr.begin(), arr.end(), data);
	}

	void add_lock_prefix_table()
	{
		if (is_x64())
			add_data(lock_prefix_table_va, lock_prefixes64);
		else
			add_data(lock_prefix_table_va, lock_prefixes32);
	}

	void add_se_handlers()
	{
		add_data(se_handler_table_va, se_handlers);
	}

	void add_cf_guard_functions()
	{
		add_data(guard_cf_function_table_va, cf_guard_function_table);
		add_data(image_base + guard_cf_function_table_rva1
			- guard_cf_function_table_rva1_hash.size(),
			guard_cf_function_table_rva1_hash);
	}

	void add_chpe_tables()
	{
		if (is_x64())
		{
			add_data(chpe_metadata_va, chpe_descriptor_arm64);
			add_data(image_base + cphe_code_address_range_rva,
				chpe_range_entries_arm64);
		}
		else
		{
			add_data(chpe_metadata_va, chpe_descriptor_x86);
			add_data(image_base + cphe_code_address_range_rva,
				chpe_range_entries_x86);
		}
	}

	void add_dynamic_reloc_v2()
	{
		add_data(image_base + section_rva + dvrt_offset,
			dvrt_header_v2);
		add_data(image_base + section_rva + dvrt_offset + dvrt_header_v2.size(),
			dvrt_v2);
	}

	void add_dynamic_reloc_v1()
	{
		add_data(image_base + section_rva + dvrt_offset,
			dvrt_header_v1);
		add_data(image_base + section_rva + dvrt_offset + dvrt_header_v1.size(),
			dvrt_v1);
	}

	void add_dynamic_reloc_v0()
	{
		add_data(image_base + section_rva + dvrt_offset,
			dvrt_header_v0);
	}

	void add_enclave()
	{
		add_data(enclave_va, enclave_config_part1);
		add_data(image_base + import_list_rva, enclave_imports);
		add_data(image_base + import_name1_rva, enclave_import1_name);
	}

	void add_volatile_metadata()
	{
		add_data(volatile_metadata_va, volatile_metadata_descriptor);
		add_data(image_base + volatile_access_table_rva, volatile_access_table);
		add_data(image_base + volatile_info_range_table_rva, volatile_info_range_table);
	}

	void add_ehcont_targets()
	{
		add_data(guard_eh_continuation_table_va, ehcont_rvas);
	}

public:
	template<typename Directory>
	void validate_base(const Directory& dir, bool has_lock_prefixes = true)
	{
		if (has_lock_prefixes)
		{
			ASSERT_TRUE(dir.get_lock_prefix_table());
			ASSERT_EQ(dir.get_lock_prefix_table()->get_prefix_va_list().size(), lock_prefix_count);
			EXPECT_EQ(dir.get_lock_prefix_table()->get_prefix_va_list()[0].get(), lock_prefix1);
			EXPECT_EQ(dir.get_lock_prefix_table()->get_prefix_va_list()[1].get(), lock_prefix2);
			EXPECT_EQ(dir.get_lock_prefix_table()->get_prefix_va_list()[0]
				.get_state().absolute_offset(),
				(directory_rva - section_rva) + (lock_prefix_table_va - image_base - section_rva)
				+ absolute_offset);
			EXPECT_EQ(dir.get_lock_prefix_table()->get_prefix_va_list()[1]
				.get_state().absolute_offset(),
				(directory_rva - section_rva) + (lock_prefix_table_va - image_base - section_rva)
				+ absolute_offset + va_size());
		}
		else
		{
			EXPECT_FALSE(dir.get_lock_prefix_table());
		}

		EXPECT_EQ(dir.get_global_flags_set(), global_flags_set);
		EXPECT_EQ(dir.get_global_flags_clear(), global_flags_clear);
		EXPECT_EQ(dir.get_process_heap_flags(), process_heap_flags);
		EXPECT_EQ(dir.get_dependent_load_flags(), dependent_load_flags);
		EXPECT_TRUE(dir.version_exactly_matches());
	}

	template<typename Directory>
	void validate_safeseh(const Directory& dir, std::uint32_t count = se_handler_count)
	{
		if (is_x64())
		{
			EXPECT_FALSE(dir.get_safeseh_handler_table());
			return;
		}

		ASSERT_TRUE(dir.get_safeseh_handler_table());
		ASSERT_EQ(dir.get_safeseh_handler_table()->get_handler_list().size(), count);
		if (count)
		{
			EXPECT_EQ(dir.get_safeseh_handler_table()->get_handler_list()[0].get(),
				se_handler1);
			EXPECT_EQ(dir.get_safeseh_handler_table()->get_handler_list()[0]
				.get_state().absolute_offset(),
				(directory_rva - section_rva) + (se_handler_table_va - image_base - section_rva)
				+ absolute_offset);
		}
		if (count > 1)
		{
			EXPECT_EQ(dir.get_safeseh_handler_table()->get_handler_list()[1].get(),
				se_handler2);
			EXPECT_EQ(dir.get_safeseh_handler_table()->get_handler_list()[1]
				.get_state().absolute_offset(),
				(directory_rva - section_rva) + (se_handler_table_va - image_base - section_rva)
				+ absolute_offset + sizeof(rva_type));
		}
		if (count > 2)
		{
			EXPECT_EQ(dir.get_safeseh_handler_table()->get_handler_list()[2].get(), 0u);
			EXPECT_EQ(dir.get_safeseh_handler_table()->get_handler_list()[2]
				.get_state().absolute_offset(),
				(directory_rva - section_rva) + (se_handler_table_va - image_base - section_rva)
				+ absolute_offset + sizeof(rva_type) * 2);
		}
	}

	template<typename Table>
	static void validate_cf_guard_table(const Table& optional_table, bool has_xfg,
		std::uint32_t function_count = guard_cf_function_count)
	{
		ASSERT_TRUE(optional_table);
		const auto& table = *optional_table;
		ASSERT_EQ(table.size(), function_count);

		static constexpr std::uint32_t base_offset = (directory_rva - section_rva)
			+ (guard_cf_function_table_va - image_base - section_rva) + absolute_offset;
		static constexpr bool is_extended_table
			= std::is_same_v<std::remove_cvref_t<decltype(table[0])>,
			load_config::guard_function_base<error_list>>;

		if (function_count > 0)
		{
			EXPECT_EQ(table[0].get_rva().get(), guard_cf_function_table_rva0);
			EXPECT_TRUE(std::equal(guard_cf_stride_data0.begin(), guard_cf_stride_data0.end(),
				table[0].get_additional_data().value().begin()));
			EXPECT_EQ(table[0].get_rva().get_state().absolute_offset(), base_offset);
			EXPECT_EQ(table[0].get_additional_data().get_state().absolute_offset(),
				base_offset + sizeof(std::uint32_t));
			if constexpr (is_extended_table)
			{
				EXPECT_EQ(table[0].get_flags(),
					static_cast<load_config::gfids_flags::value>(guard_cf_stride_data0[0]));
				EXPECT_FALSE(table[0].get_type_based_hash());
				expect_contains_errors(table[0]);
			}
		}
		if (function_count > 1)
		{
			EXPECT_EQ(table[1].get_rva().get(), guard_cf_function_table_rva1);
			EXPECT_TRUE(std::equal(guard_cf_stride_data1.begin(), guard_cf_stride_data1.end(),
				table[1].get_additional_data().value().begin()));
			EXPECT_EQ(table[1].get_rva().get_state().absolute_offset(),
				base_offset + sizeof(std::uint32_t) + guard_cf_stride);
			EXPECT_EQ(table[1].get_additional_data().get_state().absolute_offset(),
				base_offset + 2u * sizeof(std::uint32_t) + guard_cf_stride);
			if constexpr (is_extended_table)
			{
				EXPECT_EQ(table[1].get_flags(),
					static_cast<load_config::gfids_flags::value>(guard_cf_stride_data1[0]));
				if (has_xfg)
				{
					ASSERT_TRUE(table[1].get_type_based_hash());
					EXPECT_EQ(table[1].get_type_based_hash()->get(),
						guard_cf_function_table_rva1_hash_value);
					EXPECT_EQ(table[1].get_type_based_hash()->get_state().absolute_offset(),
						(directory_rva - section_rva)
						+ (guard_cf_function_table_rva1 - section_rva
							- guard_cf_function_table_rva1_hash.size())
						+ absolute_offset);
				}
				else
				{
					EXPECT_FALSE(table[1].get_type_based_hash());
				}

				expect_contains_errors(table[1]);
			}
		}
		if (function_count > 2)
		{
			EXPECT_EQ(table[2].get_rva().get(), guard_cf_function_table_rva2);
			EXPECT_TRUE(std::equal(guard_cf_stride_data2.begin(), guard_cf_stride_data2.end(),
				table[2].get_additional_data().value().begin()));
			EXPECT_EQ(table[2].get_rva().get_state().absolute_offset(),
				base_offset + (sizeof(std::uint32_t) + guard_cf_stride) * 2u);
			EXPECT_EQ(table[2].get_additional_data().get_state().absolute_offset(),
				base_offset + (sizeof(std::uint32_t) + guard_cf_stride) * 2u
				+ sizeof(std::uint32_t));
			if constexpr (is_extended_table)
			{
				EXPECT_EQ(table[2].get_flags(),
					static_cast<load_config::gfids_flags::value>(guard_cf_stride_data2[0]));
				EXPECT_FALSE(table[2].get_type_based_hash());
				if (has_xfg)
				{
					expect_contains_errors(table[2],
						load_config::load_config_directory_loader_errc::invalid_xfg_type_based_hash_rva);
				}
				else
				{
					expect_contains_errors(table[2]);
				}
			}
		}
	}

	template<typename Directory>
	void validate_cf_guard(const Directory& dir, bool has_xfg = true, bool load_xfg = true,
		std::uint32_t function_count = guard_cf_function_count)
	{
		ASSERT_EQ(dir.get_guard_cf_function_table_stride(), guard_cf_stride);
		auto flags = guard_flags;
		if (!has_xfg && load_xfg)
			flags &= ~load_config::guard_flags::xfg_enabled;
		EXPECT_EQ(dir.get_guard_flags(), flags);

		validate_cf_guard_table(dir.get_guard_cf_function_table(), has_xfg,
			function_count);
	}

	template<typename Directory, typename... Pairs>
	void validate_size_and_version(const Directory& dir,
		load_config::version version, const Pairs&... pairs)
	{
		auto size = get_directory_size(pairs...);
		EXPECT_EQ(dir.get_size().get(), size);
		EXPECT_EQ(dir.get_descriptor().physical_size(), size
			- sizeof(std::uint32_t)); //size field
		EXPECT_EQ(dir.get_version(), version);
	}

	template<auto GuardFlagsPtr, typename Directory>
	void override_guard_flags(Directory& dir, std::uint32_t flags)
	{
		auto guard_flags_offset = detail::packed_reflection::get_field_offset<
			GuardFlagsPtr>();
		dir[guard_flags_offset++] = std::byte{ flags & 0xffu };
		dir[guard_flags_offset++] = std::byte{ (flags >> 8u) & 0xffu };
		dir[guard_flags_offset++] = std::byte{ (flags >> 16u) & 0xffu };
		dir[guard_flags_offset] = std::byte{ (flags >> 24u) & 0xffu };
	}

	auto override_guard_flags(std::uint32_t flags)
	{
		auto cf_guard_copy = cf_guard;
		override_guard_flags<&detail::load_config::cf_guard32::guard_flags>(
			cf_guard_copy.first, flags);
		override_guard_flags<&detail::load_config::cf_guard64::guard_flags>(
			cf_guard_copy.second, flags);
		return cf_guard_copy;
	}

	template<typename Func = int>
	void test_cf_guard(bool add_dll_characteristics, bool load_function_table,
		std::uint32_t flags = guard_flags, const Func& func = 0)
	{
		add_load_config_directory();
		auto cf_guard_copy = override_guard_flags(flags);
		add_directory_parts(load_config_base, se_handler_table, cf_guard_copy);
		add_lock_prefix_table();
		add_se_handlers();
		add_cf_guard_functions();
		if (add_dll_characteristics)
		{
			instance.get_optional_header().set_raw_dll_characteristics(
				core::optional_header::dll_characteristics::guard_cf);
		}
		with_load_config([this, &cf_guard_copy, flags, &func](const auto& dir) {
			validate_base(dir);
			validate_safeseh(dir);
			EXPECT_FALSE(dir.get_guard_cf_function_table());
			validate_size_and_version(dir, load_config::version::cf_guard,
				load_config_base, se_handler_table, cf_guard_copy);
			if constexpr (std::is_invocable_v<decltype(func), decltype(dir)>)
				func(dir);
		}, { .load_cf_guard_function_table = load_function_table });
	}

	template<typename Func>
	void test_cf_tables(
		load_config::load_config_directory_loader_errc table_error,
		std::uint32_t cf_flags, const Func& func,
		const load_config::loader_options& options = {})
	{
		add_load_config_directory();
		auto cf_guard_copy = override_guard_flags(guard_flags_stride_flag
			| load_config::guard_flags::cf_instrumented
			| cf_flags);
		add_directory_parts(load_config_base, se_handler_table,
			cf_guard_copy, code_integrity, cf_guard_ex);
		add_lock_prefix_table();
		add_se_handlers();
		add_cf_guard_functions();
		instance.get_optional_header().set_raw_dll_characteristics(
			core::optional_header::dll_characteristics::guard_cf);
		with_load_config([this, &cf_guard_copy, &func,
			table_error](const auto& dir) {
			validate_base(dir);
			validate_safeseh(dir);
			EXPECT_FALSE(dir.get_guard_cf_function_table());
			expect_contains_errors(dir,
				load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
				load_config::load_config_directory_loader_errc::invalid_guard_cf_check_function_va,
				load_config::load_config_directory_loader_errc::invalid_guard_cf_dispatch_function_va,
				table_error);
			validate_size_and_version(dir, load_config::version::cf_guard_ex,
				load_config_base, se_handler_table, cf_guard_copy, code_integrity, cf_guard_ex);
			func(dir);
		}, options);
	}

	template<typename Entries>
	void validate_chpe_entries_base(const Entries& entries,
		std::uint32_t entry_count)
	{
		static constexpr std::uint32_t base_offset = (directory_rva - section_rva)
			+ (cphe_code_address_range_rva - section_rva) + absolute_offset;
		for (std::uint32_t i = 0; i != entry_count; ++i)
		{
			EXPECT_EQ(entries[i].get_rva(), chpe_range[i]);
			EXPECT_EQ(entries[i].get_entry().get_state().absolute_offset(),
				base_offset + i * sizeof(rva_type) * 2u);
			if (i == 0)
			{
				expect_contains_errors(entries[i]);
			}
			else if (i == 1 || i == 2)
			{
				expect_contains_errors(entries[i],
					load_config::load_config_directory_loader_errc::invalid_chpe_entry_address_or_size);
			}
		}
	}

	static void validate_chpe_entries(
		const std::vector<load_config::chpe_x86_code_range_entry<error_list>>& entries,
		std::uint32_t entry_count)
	{
		for (std::uint32_t i = 0; i != entry_count; ++i)
		{
			EXPECT_EQ(entries[i].get_code_type(),
				static_cast<load_config::chpe_x86_range_code_type>(chpe_range_flags_x86[i]));
		}
	}

	static void validate_chpe_entries(
		const std::vector<load_config::chpe_arm64x_code_range_entry<error_list>>& entries,
		std::uint32_t entry_count)
	{
		for (std::uint32_t i = 0; i != entry_count; ++i)
		{
			EXPECT_EQ(entries[i].get_code_type(),
				static_cast<load_config::chpe_arm64x_range_code_type>(chpe_range_flags_arm64[i]));
		}
	}
	
	template<typename Directory>
	void validate_chpe(const Directory& dir,
		std::uint32_t entry_count = cphe_code_address_range_count)
	{
		if (is_x64())
		{
			ASSERT_TRUE(std::holds_alternative<
				load_config::chpe_arm64x_metadata_base<error_list>>(dir.get_chpe_metadata()));
		}
		else
		{
			ASSERT_TRUE(std::holds_alternative<
				load_config::chpe_x86_metadata_base<error_list>>(dir.get_chpe_metadata()));
		}

		std::visit([this, entry_count](const auto& chpe) {
			using type = std::remove_cvref_t<decltype(chpe)>;
			if constexpr (!std::is_same_v<type, std::monostate>)
			{
				if (entry_count == cphe_code_address_range_count)
				{
					expect_contains_errors(chpe);
				}
				else
				{
					expect_contains_errors(chpe,
						load_config::load_config_directory_loader_errc::invalid_chpe_range_entry_count);
				}
				ASSERT_EQ(chpe.get_range_entries().size(), entry_count);
				validate_chpe_entries_base(chpe.get_range_entries(), entry_count);
				validate_chpe_entries(chpe.get_range_entries(), entry_count);
			}

			if constexpr (std::is_same_v<type,
				load_config::chpe_x86_metadata_base<error_list>>)
			{
				EXPECT_EQ(chpe.get_metadata_size(),
					detail::packed_reflection::get_field_offset<
					&detail::load_config::image_chpe_metadata_x86::wow_a64_rdtsc_function_pointer>());
			}
			else if constexpr (std::is_same_v<type,
				load_config::chpe_arm64x_metadata_details>)
			{
				EXPECT_EQ(chpe.get_metadata()->extra_rfe_table, extra_rfe_table);
				EXPECT_EQ(chpe.get_metadata()->extra_rfe_table_size, extra_rfe_table_size);
			}
		}, dir.get_chpe_metadata());
	}

	void validate_dynamic_relocations_base(const load_config::load_config_directory_impl<
		detail::load_config::image_load_config_directory64, error_list>
		::dynamic_relocation_table_type& table_opt)
	{
		const auto& table = *table_opt;
		expect_contains_errors(table);
		EXPECT_EQ(table.get_table().get_state().absolute_offset(),
			(directory_rva - section_rva) + absolute_offset + dvrt_offset);
		EXPECT_EQ(table.get_table().get_state().relative_offset(),
			(directory_rva - section_rva) + dvrt_offset);
	}

	void validate_dynamic_relocations_v2(const load_config::load_config_directory_impl<
		detail::load_config::image_load_config_directory64, error_list>
		::dynamic_relocation_table_type& table_opt)
	{
		ASSERT_TRUE(table_opt);
		validate_dynamic_relocations_base(table_opt);

		using dvrt_type = load_config::dynamic_relocation_table_details<std::uint64_t>;
		const auto* relocs = std::get_if<dvrt_type::relocation_v2_list_type>(
			&table_opt->get_relocations());
		ASSERT_NE(relocs, nullptr);
		ASSERT_EQ(relocs->size(), dvrt_v2_relocation_count);

		const auto& reloc0 = (*relocs)[0];
		expect_contains_errors(reloc0,
			load_config::load_config_directory_loader_errc::unknown_dynamic_relocation_symbol);
		EXPECT_TRUE(reloc0.get_fixup_lists().empty());
		EXPECT_TRUE(std::holds_alternative<std::monostate>(reloc0.get_header()));

		const auto& reloc1 = (*relocs)[1];
		expect_contains_errors(reloc1);
		const auto& fixups1 = reloc1.get_fixup_lists();
		ASSERT_EQ(fixups1.size(), dvrt1_v2_base_reloc_count);
		expect_contains_errors(fixups1[0]);
		expect_contains_errors(fixups1[1]);
		ASSERT_EQ(fixups1[0].get_fixups().size(), dvrt1_v2_base_reloc0_fixup_count);
		EXPECT_EQ(fixups1[0].get_base_relocation()->virtual_address, dvrt1_v2_base_reloc0_base);
		ASSERT_EQ(fixups1[1].get_fixups().size(), dvrt1_v2_base_reloc1_fixup_count);
		EXPECT_EQ(fixups1[1].get_base_relocation()->virtual_address, dvrt1_v2_base_reloc1_base);
		EXPECT_EQ(fixups1[1].get_fixups()[0].get_relocation().get(), dvrt1_v2_base_reloc1_fixup0);

		const auto* header1 = std::get_if<load_config::prologue_dynamic_relocation_header>(
			&reloc1.get_header());
		ASSERT_NE(header1, nullptr);
		ASSERT_EQ(header1->get_data().physical_size(), 2u);
		EXPECT_EQ(header1->get_data().value()[0], dvrt2_v2_prologue_data[0]);
		EXPECT_EQ(header1->get_data().value()[1], dvrt2_v2_prologue_data[1]);

		const auto& reloc2 = (*relocs)[2];
		expect_contains_errors(reloc2);
		const auto& fixups2 = reloc2.get_fixup_lists();
		ASSERT_EQ(fixups2.size(), dvrt2_v2_base_reloc_count);
		expect_contains_errors(fixups2[0]);
		ASSERT_EQ(fixups2[0].get_fixups().size(), dvrt2_v2_base_reloc0_fixup_count);

		const auto* header2 = std::get_if<load_config::epilogue_dynamic_relocation_header_details>(
			&reloc2.get_header());
		ASSERT_NE(header2, nullptr);
		expect_contains_errors(*header2);
		ASSERT_EQ(header2->get_branch_descriptors().size(), dvrt2_v2_branch_descriptor_count);
		EXPECT_EQ(header2->get_branch_descriptors()[0].get_descriptor().get(),
			dvrt2_v2_branch_descriptor0);
		EXPECT_EQ(header2->get_branch_descriptors()[1].get_descriptor().get(),
			dvrt2_v2_branch_descriptor1);
		EXPECT_EQ(header2->get_branch_descriptors()[2].get_descriptor().get(),
			dvrt2_v2_branch_descriptor2);
		ASSERT_EQ(header2->get_branch_descriptors()[0].get_value().physical_size(),
			dvrt2_v2_branch_descriptor_data_size);
		ASSERT_EQ(header2->get_branch_descriptors()[1].get_value().physical_size(),
			dvrt2_v2_branch_descriptor_data_size);
		ASSERT_EQ(header2->get_branch_descriptors()[2].get_value().physical_size(),
			dvrt2_v2_branch_descriptor_data_size);
		EXPECT_EQ(header2->get_branch_descriptors()[0].get_value()[0],
			dvrt2_v2_branch_descriptor_data0);
		EXPECT_EQ(header2->get_branch_descriptors()[1].get_value()[0],
			dvrt2_v2_branch_descriptor_data1);
		EXPECT_EQ(header2->get_branch_descriptors()[2].get_value()[0],
			dvrt2_v2_branch_descriptor_data2);
	}

	static void validate_dynamic_relocations_v2(const load_config::load_config_directory_impl<
		detail::load_config::image_load_config_directory32, error_list>
		::dynamic_relocation_table_type& table_opt)
	{
		EXPECT_FALSE(table_opt);
	}

	void validate_dynamic_relocations_v1(const load_config::load_config_directory_impl<
		detail::load_config::image_load_config_directory64, error_list>
		::dynamic_relocation_table_type& table_opt)
	{
		ASSERT_TRUE(table_opt);
		validate_dynamic_relocations_base(table_opt);

		using dvrt_type = load_config::dynamic_relocation_table_details<std::uint64_t>;
		using table_type = load_config::dynamic_relocation_table_v1_details<std::uint64_t>;
		const auto* relocs = std::get_if<dvrt_type::relocation_v1_list_type>(
			&table_opt->get_relocations());
		ASSERT_NE(relocs, nullptr);
		ASSERT_EQ(relocs->size(), dvrt_v1_relocation_count);

		const auto& reloc0 = (*relocs)[0];
		expect_contains_errors(reloc0,
			load_config::load_config_directory_loader_errc::unknown_dynamic_relocation_symbol);

		const auto& reloc1 = (*relocs)[1];
		expect_contains_errors(reloc1);
		EXPECT_EQ(reloc1.get_symbol(),
			load_config::dynamic_relocation_symbol::guard_import_control_transfer);
		const auto* fixups1 = std::get_if<
			table_type::import_control_transfer_dynamic_relocation_list_type>(
				&reloc1.get_fixup_lists());
		ASSERT_NE(fixups1, nullptr);
		ASSERT_EQ(fixups1->size(), dvrt1_v1_base_reloc1_count);
		expect_contains_errors((*fixups1)[0],
			load_config::load_config_directory_loader_errc::unaligned_dynamic_relocation_block);
		const auto& metadata1 = (*fixups1)[0].get_fixups();
		ASSERT_EQ(metadata1.size(), dvrt1_v1_base_reloc1_metadata1_count);
		EXPECT_EQ(metadata1[0].get_relocation()->metadata, dvrt1_v1_base_reloc1_metadata1_0);
		EXPECT_EQ(metadata1[1].get_relocation()->metadata, dvrt1_v1_base_reloc1_metadata1_1);
		EXPECT_EQ(metadata1[2].get_relocation()->metadata, dvrt1_v1_base_reloc1_metadata1_2);

		const auto& reloc2 = (*relocs)[2];
		expect_contains_errors(reloc2);
		EXPECT_EQ(reloc2.get_symbol(),
			load_config::dynamic_relocation_symbol::guard_indir_control_transfer);
		const auto* fixups2 = std::get_if<
			table_type::indir_control_transfer_dynamic_relocation_list_type>(
				&reloc2.get_fixup_lists());
		ASSERT_NE(fixups2, nullptr);
		ASSERT_EQ(fixups2->size(), dvrt1_v1_base_reloc2_count);
		expect_contains_errors((*fixups2)[0],
			load_config::load_config_directory_loader_errc::unaligned_dynamic_relocation_block);
		const auto& metadata2 = (*fixups2)[0].get_fixups();
		ASSERT_EQ(metadata2.size(), dvrt1_v1_base_reloc2_metadata2_count);
		EXPECT_EQ(metadata2[0].get_relocation()->metadata, dvrt1_v1_base_reloc2_metadata2_0);
		EXPECT_EQ(metadata2[1].get_relocation()->metadata, dvrt1_v1_base_reloc2_metadata2_1);

		const auto& reloc3 = (*relocs)[3];
		expect_contains_errors(reloc3);
		EXPECT_EQ(reloc3.get_symbol(),
			load_config::dynamic_relocation_symbol::guard_switchtable_branch);
		const auto* fixups3 = std::get_if<
			table_type::switchtable_branch_dynamic_relocation_list_type>(
				&reloc3.get_fixup_lists());
		ASSERT_NE(fixups3, nullptr);
		ASSERT_EQ(fixups3->size(), dvrt1_v1_base_reloc3_count);
		expect_contains_errors((*fixups3)[0],
			load_config::load_config_directory_loader_errc::unaligned_dynamic_relocation_block);
		expect_contains_errors((*fixups3)[1],
			load_config::load_config_directory_loader_errc::unaligned_dynamic_relocation_block);
		const auto& metadata3a = (*fixups3)[0].get_fixups();
		ASSERT_EQ(metadata3a.size(), dvrt1_v1_base_reloc3_metadata3a_count);
		EXPECT_EQ(metadata3a[0].get_relocation()->metadata, dvrt1_v1_base_reloc3_metadata3a);
		const auto& metadata3b = (*fixups3)[1].get_fixups();
		ASSERT_EQ(metadata3b.size(), dvrt1_v1_base_reloc3_metadata3b_count);
		EXPECT_EQ(metadata3b[0].get_relocation()->metadata, dvrt1_v1_base_reloc3_metadata3b);

		const auto& reloc4 = (*relocs)[4];
		expect_contains_errors(reloc4);
		EXPECT_EQ(reloc4.get_symbol(),
			load_config::dynamic_relocation_symbol::guard_arm64x);
		const auto* fixups4 = std::get_if<
			table_type::arm64x_dynamic_relocation_list_type>(
				&reloc4.get_fixup_lists());
		ASSERT_NE(fixups4, nullptr);
		ASSERT_EQ(fixups4->size(), dvrt1_v1_base_reloc4_count);
		expect_contains_errors((*fixups4)[0],
			load_config::load_config_directory_loader_errc::unaligned_dynamic_relocation_block);
		const auto& metadata4 = (*fixups4)[0].get_fixups();
		ASSERT_EQ(metadata4.size(), dvrt1_v1_base_reloc4_metadata4_count);
		const auto* metadata4_0 = std::get_if<
			load_config::arm64x_dynamic_relocation_zero_fill>(&metadata4[0]);
		ASSERT_NE(metadata4_0, nullptr);
		EXPECT_EQ(metadata4_0->get_relocation()->metadata, dvrt1_v1_base_reloc4_metadata4_0);
		const auto* metadata4_1 = std::get_if<
			load_config::arm64x_dynamic_relocation_copy_data_details>(&metadata4[1]);
		ASSERT_NE(metadata4_1, nullptr);
		EXPECT_EQ(metadata4_1->get_relocation()->metadata, dvrt1_v1_base_reloc4_metadata4_1);
		ASSERT_EQ(metadata4_1->get_data().physical_size(),
			dvrt1_v1_base_reloc4_metadata4_1_extra.size());
		EXPECT_TRUE(std::equal(dvrt1_v1_base_reloc4_metadata4_1_extra.begin(),
			dvrt1_v1_base_reloc4_metadata4_1_extra.end(),
			metadata4_1->get_data().value().begin()));
		const auto* metadata4_2 = std::get_if<
			load_config::arm64x_dynamic_relocation_add_delta_details>(&metadata4[2]);
		ASSERT_NE(metadata4_2, nullptr);
		EXPECT_EQ(metadata4_2->get_relocation()->metadata, dvrt1_v1_base_reloc4_metadata4_2);
		ASSERT_EQ(metadata4_2->get_value().get(), dvrt1_v1_base_reloc4_metadata4_2_value);

		const auto& reloc5 = (*relocs)[5];
		expect_contains_errors(reloc5);
		EXPECT_EQ(reloc5.get_symbol(),
			load_config::dynamic_relocation_symbol::function_override);
		const auto* fixups5 = std::get_if<
			table_type::function_override_dynamic_relocation_list_type>(
				&reloc5.get_fixup_lists());
		ASSERT_NE(fixups5, nullptr);
		ASSERT_EQ(fixups5->size(), dvrt1_v1_base_reloc5_count);
		const auto& override0 = (*fixups5)[0];
		expect_contains_errors(override0,
			load_config::load_config_directory_loader_errc::unaligned_dynamic_relocation_block);
		EXPECT_EQ(override0.get_base_relocation()->virtual_address, dvrt1_v1_fixups5_0_rva);
		ASSERT_EQ(override0.get_relocations().size(), dvrt1_v1_fixups5_0_override_0_count);
		const auto& override0_0 = override0.get_relocations()[0];
		expect_contains_errors(override0_0);
		ASSERT_EQ(override0_0.get_rvas().size(), dvrt1_v1_fixups5_0_override_0_rvas_count);
		EXPECT_EQ(override0_0.get_rvas()[0].get(), dvrt1_v1_fixups5_0_override_0_rva0);
		EXPECT_EQ(override0_0.get_rvas()[1].get(), dvrt1_v1_fixups5_0_override_0_rva1);
		const auto& bdd_info0 = override0_0.get_bdd_info();
		expect_contains_errors(bdd_info0);
		ASSERT_EQ(bdd_info0.get_relocations().size(), bdd_info0_reloc_count);
		EXPECT_EQ(bdd_info0.get_relocations()[0]->right, bdd_info0_0_right);
		EXPECT_EQ(bdd_info0.get_relocations()[1]->left, bdd_info0_1_left);
		const auto& override0_0_relocs = override0_0.get_relocations();
		ASSERT_EQ(override0_0_relocs.size(), dvrt1_v1_fixups5_0_override0_0_relocs_count);
		expect_contains_errors(override0_0_relocs[0],
			load_config::load_config_directory_loader_errc::unaligned_dynamic_relocation_block);
		expect_contains_errors(override0_0_relocs[1],
			load_config::load_config_directory_loader_errc::unaligned_dynamic_relocation_block);
		EXPECT_EQ(override0_0_relocs[0].get_base_relocation()->virtual_address,
			dvrt1_v1_fixups5_0_override0_0_reloc0_rva);
		EXPECT_EQ(override0_0_relocs[1].get_base_relocation()->virtual_address,
			dvrt1_v1_fixups5_0_override0_0_reloc1_rva);
		ASSERT_EQ(override0_0_relocs[0].get_fixups().size(),
			dvrt1_v1_fixups5_0_override0_0_reloc0_fixup_count);
		ASSERT_EQ(override0_0_relocs[1].get_fixups().size(),
			dvrt1_v1_fixups5_0_override0_0_reloc1_fixup_count);
		EXPECT_EQ(override0_0_relocs[0].get_fixups()[0].get_relocation().get(),
			dvrt1_v1_fixups5_0_override0_0_reloc0_fixup0);
		EXPECT_EQ(override0_0_relocs[0].get_fixups()[1].get_relocation().get(),
			dvrt1_v1_fixups5_0_override0_0_reloc0_fixup1);
		const auto& override0_1 = override0.get_relocations()[1];
		expect_contains_errors(override0_1);
		ASSERT_EQ(override0_1.get_rvas().size(), dvrt1_v1_fixups5_0_override_1_rvas_count);
		const auto& bdd_info1 = override0_1.get_bdd_info();
		expect_contains_errors(bdd_info1);
		EXPECT_EQ(bdd_info1.get_descriptor().get_state(), bdd_info0.get_descriptor().get_state());
		ASSERT_EQ(bdd_info1.get_relocations().size(), bdd_info0_reloc_count);
		EXPECT_EQ(bdd_info1.get_relocations()[0]->right, bdd_info0_0_right);
		EXPECT_EQ(bdd_info1.get_relocations()[1]->left, bdd_info0_1_left);
	}

	static void validate_dynamic_relocations_v1(const load_config::load_config_directory_impl<
		detail::load_config::image_load_config_directory32, error_list>
		::dynamic_relocation_table_type& table_opt)
	{
		EXPECT_FALSE(table_opt);
	}

	template<typename Directory>
	void validate_enclave(const Directory& dir,
		std::uint32_t import_count = enclave_import_count)
	{
		ASSERT_TRUE(dir.get_enclave_config());
		const auto& config = *dir.get_enclave_config();
		EXPECT_TRUE(config.get_extra_data().value().empty());
		const auto* image_id = reinterpret_cast<const std::byte*>(
			config.get_descriptor()->image_id);
		EXPECT_EQ(std::vector(image_id,
			image_id + std::size(config.get_descriptor()->image_id)),
			std::vector(enclave_image_id.begin(), enclave_image_id.end()));

		const auto& imports = config.get_imports();
		ASSERT_EQ(imports.size(), import_count);

		EXPECT_TRUE(imports[0].get_name().value().empty());
		EXPECT_EQ(imports[0].get_extra_data().value(),
			std::vector(enclave_import0_extra.begin(), enclave_import0_extra.end()));
		const auto* family_id0 = reinterpret_cast<const std::byte*>(
			imports[0].get_descriptor()->family_id);
		EXPECT_EQ(std::vector(family_id0,
			family_id0 + std::size(imports[0].get_descriptor()->family_id)),
			std::vector(enclave_family_id.begin(), enclave_family_id.end()));

		if (import_count == enclave_import_count)
		{
			expect_contains_errors(config,
				load_config::load_config_directory_loader_errc::invalid_enclave_config_extra_data);
			EXPECT_EQ(imports[1].get_name().get_state().absolute_offset(),
				(directory_rva - section_rva)
				+ (import_name1_rva - section_rva) + absolute_offset);
			EXPECT_EQ(imports[1].get_name().value(),
				reinterpret_cast<const char*>(enclave_import1_name.data()));
			EXPECT_EQ(imports[1].get_extra_data().value(),
				std::vector(enclave_import1_extra.begin(), enclave_import1_extra.end()));
			const auto* unique_or_author_id1 = reinterpret_cast<const std::byte*>(
				imports[0].get_descriptor()->unique_or_author_id);
			EXPECT_EQ(std::vector(unique_or_author_id1,
				unique_or_author_id1 + std::size(imports[0].get_descriptor()->unique_or_author_id)),
				std::vector(enclave_import1_uid.begin(), enclave_import1_uid.end()));
		}
		else
		{
			expect_contains_errors(config,
				load_config::load_config_directory_loader_errc::invalid_enclave_config_extra_data,
				load_config::load_config_directory_loader_errc::invalid_enclave_import_array);
		}
	}

	template<typename Directory>
	void validate_volatile_metadata(const Directory& dir)
	{
		ASSERT_TRUE(dir.get_volatile_metadata());

		const auto& metadata = *dir.get_volatile_metadata();
		expect_contains_errors(metadata,
			load_config::load_config_directory_loader_errc::unaligned_volatile_metadata_range_table_size);

		const auto& access_table = metadata.get_access_rva_table();
		ASSERT_EQ(access_table.size(), volatile_metadata_access_table_size);
		EXPECT_EQ(access_table[0].get(), volatile_access_table_rva0);
		EXPECT_EQ(access_table[1].get(), volatile_access_table_rva1);

		const auto& range_table = metadata.get_range_table();
		ASSERT_EQ(range_table.size(), volatile_metadata_range_table_size);
		EXPECT_EQ(range_table[0]->rva, volatile_access_table_rva2);
	}

	template<typename Directory>
	void validate_volatile_metadata_limited(const Directory& dir)
	{
		ASSERT_TRUE(dir.get_volatile_metadata());

		const auto& metadata = *dir.get_volatile_metadata();
		expect_contains_errors(metadata,
			load_config::load_config_directory_loader_errc::unaligned_volatile_metadata_range_table_size,
			load_config::load_config_directory_loader_errc::invalid_volatile_metadata_access_rva_table_entry_count,
			load_config::load_config_directory_loader_errc::invalid_volatile_metadata_range_table_entry_count);

		EXPECT_EQ(metadata.get_access_rva_table().size(), 1u);
		EXPECT_TRUE(metadata.get_range_table().empty());
	}

	template<typename Directory>
	void validate_ehcont_targets(const Directory& dir)
	{
		ASSERT_TRUE(dir.get_eh_continuation_targets());

		const auto& targets = *dir.get_eh_continuation_targets();
		ASSERT_EQ(targets.size(), guard_eh_continuation_count);
		EXPECT_EQ(targets[0].get(), ehcont_rva0);
		EXPECT_EQ(targets[1].get(), ehcont_rva1);
	}

	template<typename Directory>
	void validate_ehcont_targets_limited(const Directory& dir)
	{
		ASSERT_TRUE(dir.get_eh_continuation_targets());

		const auto& targets = *dir.get_eh_continuation_targets();
		ASSERT_EQ(targets.size(), 1u);
		EXPECT_EQ(targets[0].get(), ehcont_rva0);
	}

public:
	bool is_x64_{};
	image::image instance;

public:
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t directory_rva = 0x1000u;
	static constexpr std::uint32_t section_virtual_size = 0x2000u;
	static constexpr std::uint32_t image_base = 0x40000u;
	static constexpr std::uint32_t section_raw_size = 0x1000u;
	static constexpr auto global_flags_clear = load_config::global_flags::application_verifier
		| load_config::global_flags::pool_enable_tagging;
	static constexpr auto global_flags_set = load_config::global_flags::stop_on_unhandled_exception;
	static constexpr std::uint64_t lock_prefix_table_va = section_rva + image_base + 0x200u;
	static constexpr auto process_heap_flags = load_config::process_heap_flags::heap_create_enable_execute
		| load_config::process_heap_flags::heap_no_serialize;
	static constexpr auto dependent_load_flags
		= load_config::dependent_load_flags::load_library_search_application_dir
		| load_config::dependent_load_flags::load_library_search_system32;
	static constexpr std::size_t lock_prefix_count = 2u;
	static constexpr std::uint32_t lock_prefix1 = 0x1020304u;
	static constexpr std::uint32_t lock_prefix2 = 0x6070809u;
	static constexpr std::uint64_t se_handler_table_va = section_rva + image_base + 0x230u;
	static constexpr std::uint32_t se_handler_count = 3u;
	static constexpr std::uint32_t se_handler1 = 0xd0c0b0au;
	static constexpr std::uint32_t se_handler2 = 0xe0d0e0fu;
	static constexpr std::uint64_t guard_cf_function_table_va = section_rva + image_base + 0x260u;
	static constexpr std::uint32_t guard_cf_function_count = 3u;
	static constexpr std::uint32_t guard_cf_stride = 3u;
	static constexpr std::uint32_t guard_flags_stride_flag = 0x30000000; //stride = 3
	static constexpr std::uint32_t guard_flags = 
		guard_flags_stride_flag
		| load_config::guard_flags::cf_instrumented
		| load_config::guard_flags::cf_function_table_present
		| load_config::guard_flags::xfg_enabled;
	static constexpr std::uint32_t guard_cf_function_table_rva0 = 0xabcdef00u;
	static constexpr std::uint32_t guard_cf_function_table_rva1 = section_rva + 0x320u;
	static constexpr std::uint64_t guard_cf_function_table_rva1_hash_value = 0x9080704030201ull;
	static constexpr std::uint32_t guard_cf_function_table_rva2 = 0u;
	static constexpr std::array guard_cf_stride_data0{
		std::byte{1}, std::byte{2}, std::byte{3} };
	static constexpr std::array guard_cf_stride_data1{
		std::byte{load_config::gfids_flags::fid_xfg}, std::byte{5}, std::byte{6} };
	static constexpr std::array guard_cf_stride_data2{
		std::byte{load_config::gfids_flags::fid_xfg | 1u}, std::byte{5}, std::byte{6} };
	static constexpr std::uint64_t guard_cf_export_suppression_table_va
		= section_rva + image_base + 0x280u;
	static constexpr std::uint64_t guard_cf_longjump_table_va
		= section_rva + image_base + 0x2a0u;
	static constexpr std::uint64_t chpe_metadata_va = section_rva + image_base + 0x320u;
	
	static constexpr std::array lock_prefixes32{
		std::byte{4}, std::byte{3}, std::byte{2}, std::byte{1},
		std::byte{9}, std::byte{8}, std::byte{7}, std::byte{6},
	};

	static constexpr std::array lock_prefixes64{
		std::byte{4}, std::byte{3}, std::byte{2}, std::byte{1},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{9}, std::byte{8}, std::byte{7}, std::byte{6},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
	};

	static constexpr std::array se_handlers{
		std::byte{0xau}, std::byte{0xbu}, std::byte{0xcu}, std::byte{0xdu},
		std::byte{0xfu}, std::byte{0xeu}, std::byte{0xdu}, std::byte{0xeu},
	};

	static constexpr std::array load_config_base32{
		std::byte{4}, std::byte{3}, std::byte{2}, std::byte{1}, //time_date_stamp
		std::byte{2}, std::byte{1}, //major_version
		std::byte{5}, std::byte{4}, //minor_version
		//global_flags_clear
		std::byte{global_flags_clear & 0xffu}, std::byte{(global_flags_clear >> 8u) & 0xffu},
		std::byte{(global_flags_clear >> 16u) & 0xffu}, std::byte{(global_flags_clear >> 24u) & 0xffu},
		//global_flags_set
		std::byte{global_flags_set & 0xffu}, std::byte{(global_flags_set >> 8u) & 0xffu},
		std::byte{(global_flags_set >> 16u) & 0xffu}, std::byte{(global_flags_set >> 24u) & 0xffu},
		std::byte{7}, std::byte{8}, std::byte{9}, std::byte{0xau}, //critical_section_default_timeout
		std::byte{9}, std::byte{8}, std::byte{7}, std::byte{6}, //de_commit_free_block_threshold
		std::byte{5}, std::byte{4}, std::byte{3}, std::byte{2}, //de_commit_total_free_threshold
		//lock_prefix_table
		std::byte{lock_prefix_table_va & 0xffu}, std::byte{(lock_prefix_table_va >> 8u) & 0xffu},
		std::byte{(lock_prefix_table_va >> 16u) & 0xffu}, std::byte{(lock_prefix_table_va >> 24u) & 0xffu},
		std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}, //maximum_allocation_size
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //virtual_memory_threshold
		//process_heap_flags
		std::byte{process_heap_flags & 0xffu}, std::byte{(process_heap_flags >> 8u) & 0xffu},
		std::byte{(process_heap_flags >> 16u) & 0xffu}, std::byte{(process_heap_flags >> 24u) & 0xffu},
		std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, //process_affinity_mask
		std::byte{6}, std::byte{7}, //csd_version
		//dependent_load_flags
		std::byte{dependent_load_flags & 0xffu}, std::byte{(dependent_load_flags >> 8u) & 0xffu},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //edit_list
		std::byte{}, std::byte{}, std::byte{}, std::byte{0xff}, //security_cookie
	};

	static constexpr std::array load_config_base64{
		std::byte{4}, std::byte{3}, std::byte{2}, std::byte{1}, //time_date_stamp
		std::byte{2}, std::byte{1}, //major_version
		std::byte{5}, std::byte{4}, //minor_version
		//global_flags_clear
		std::byte{global_flags_clear & 0xffu}, std::byte{(global_flags_clear >> 8u) & 0xffu},
		std::byte{(global_flags_clear >> 16u) & 0xffu}, std::byte{(global_flags_clear >> 24u) & 0xffu},
		//global_flags_set
		std::byte{global_flags_set & 0xffu}, std::byte{(global_flags_set >> 8u) & 0xffu},
		std::byte{(global_flags_set >> 16u) & 0xffu}, std::byte{(global_flags_set >> 24u) & 0xffu},
		std::byte{7}, std::byte{8}, std::byte{9}, std::byte{0xau}, //critical_section_default_timeout
		std::byte{9}, std::byte{8}, std::byte{7}, std::byte{6}, //de_commit_free_block_threshold
		std::byte{9}, std::byte{8}, std::byte{7}, std::byte{6},
		std::byte{5}, std::byte{4}, std::byte{3}, std::byte{2}, //de_commit_total_free_threshold
		std::byte{5}, std::byte{4}, std::byte{3}, std::byte{2},
		//lock_prefix_table
		std::byte{lock_prefix_table_va & 0xffu}, std::byte{(lock_prefix_table_va >> 8u) & 0xffu},
		std::byte{(lock_prefix_table_va >> 16u) & 0xffu}, std::byte{(lock_prefix_table_va >> 24u) & 0xffu},
		std::byte{(lock_prefix_table_va >> 32u) & 0xffu}, std::byte{(lock_prefix_table_va >> 40u) & 0xffu},
		std::byte{(lock_prefix_table_va >> 48u) & 0xffu}, std::byte{(lock_prefix_table_va >> 56u) & 0xffu},
		std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}, //maximum_allocation_size
		std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3},
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //virtual_memory_threshold
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
		std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, //process_affinity_mask
		std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5},
		//process_heap_flags
		std::byte{process_heap_flags & 0xffu}, std::byte{(process_heap_flags >> 8u) & 0xffu},
		std::byte{(process_heap_flags >> 16u) & 0xffu}, std::byte{(process_heap_flags >> 24u) & 0xffu},
		std::byte{6}, std::byte{7}, //csd_version
		//dependent_load_flags
		std::byte{dependent_load_flags & 0xffu}, std::byte{(dependent_load_flags >> 8u) & 0xffu},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //edit_list
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //security_cookie
		std::byte{}, std::byte{}, std::byte{}, std::byte{0xff},
	};

	static constexpr std::pair load_config_base{ load_config_base32, load_config_base64 };

	static constexpr std::array se_handler_table32{
		//se_handler_table
		std::byte{se_handler_table_va & 0xffu}, std::byte{(se_handler_table_va >> 8u) & 0xffu},
		std::byte{(se_handler_table_va >> 16u) & 0xffu}, std::byte{(se_handler_table_va >> 24u) & 0xffu},
		//se_handler_count
		std::byte{se_handler_count}, std::byte{}, std::byte{}, std::byte{},
	};

	static constexpr std::array se_handler_table64{
		//se_handler_table
		std::byte{se_handler_table_va & 0xffu}, std::byte{(se_handler_table_va >> 8u) & 0xffu},
		std::byte{(se_handler_table_va >> 16u) & 0xffu}, std::byte{(se_handler_table_va >> 24u) & 0xffu},
		std::byte{(se_handler_table_va >> 32u) & 0xffu}, std::byte{(se_handler_table_va >> 40u) & 0xffu},
		std::byte{(se_handler_table_va >> 48u) & 0xffu}, std::byte{(se_handler_table_va >> 56u) & 0xffu},
		//se_handler_count
		std::byte{se_handler_count}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
	};

	static constexpr std::pair se_handler_table{ se_handler_table32, se_handler_table64 };

	static constexpr std::array cf_guard32{
		//guard_cf_check_function_pointer
		std::byte{1}, std::byte{1}, std::byte{1}, std::byte{1},
		//guard_cf_dispatch_function_pointer
		std::byte{2}, std::byte{2}, std::byte{2}, std::byte{2},
		//guard_cf_function_table
		std::byte{guard_cf_function_table_va & 0xffu},
		std::byte{(guard_cf_function_table_va >> 8u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 16u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 24u) & 0xffu},
		//guard_cf_function_count
		std::byte{guard_cf_function_count}, std::byte{}, std::byte{}, std::byte{},
		//guard_flags
		std::byte{guard_flags & 0xffu}, std::byte{(guard_flags >> 8u) & 0xffu},
		std::byte{(guard_flags >> 16u) & 0xffu}, std::byte{(guard_flags >> 24u) & 0xffu},
	};

	static constexpr std::array cf_guard64{
		//guard_cf_check_function_pointer
		std::byte{1}, std::byte{1}, std::byte{1}, std::byte{1},
		std::byte{1}, std::byte{1}, std::byte{1}, std::byte{1},
		//guard_cf_dispatch_function_pointer
		std::byte{2}, std::byte{2}, std::byte{2}, std::byte{2},
		std::byte{2}, std::byte{2}, std::byte{2}, std::byte{2},
		//guard_cf_function_table
		std::byte{guard_cf_function_table_va & 0xffu},
		std::byte{(guard_cf_function_table_va >> 8u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 16u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 24u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 32u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 40u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 48u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 56u) & 0xffu},
		//guard_cf_function_count
		std::byte{guard_cf_function_count}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		//guard_flags
		std::byte{guard_flags & 0xffu}, std::byte{(guard_flags >> 8u) & 0xffu},
		std::byte{(guard_flags >> 16u) & 0xffu}, std::byte{(guard_flags >> 24u) & 0xffu},
	};

	static constexpr std::array cf_guard_ex32{
		//guard_address_taken_iat_entry_table
		std::byte{guard_cf_function_table_va & 0xffu},
		std::byte{(guard_cf_function_table_va >> 8u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 16u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 24u) & 0xffu},
		//guard_address_taken_iat_entry_count
		std::byte{guard_cf_function_count}, std::byte{}, std::byte{}, std::byte{},
		//guard_long_jump_target_table
		std::byte{guard_cf_function_table_va & 0xffu},
		std::byte{(guard_cf_function_table_va >> 8u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 16u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 24u) & 0xffu},
		//guard_long_jump_target_count
		std::byte{guard_cf_function_count}, std::byte{}, std::byte{}, std::byte{},
	};

	static constexpr std::array cf_guard_ex64{
		//guard_address_taken_iat_entry_table
		std::byte{guard_cf_function_table_va & 0xffu},
		std::byte{(guard_cf_function_table_va >> 8u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 16u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 24u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 32u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 40u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 48u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 56u) & 0xffu},
		//guard_address_taken_iat_entry_count
		std::byte{guard_cf_function_count}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		//guard_long_jump_target_table
		std::byte{guard_cf_function_table_va & 0xffu},
		std::byte{(guard_cf_function_table_va >> 8u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 16u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 24u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 32u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 40u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 48u) & 0xffu},
		std::byte{(guard_cf_function_table_va >> 56u) & 0xffu},
		//guard_long_jump_target_count
		std::byte{guard_cf_function_count}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
	};

	static constexpr std::array cf_guard_function_table{
		//rva0
		std::byte{guard_cf_function_table_rva0 & 0xffu},
		std::byte{(guard_cf_function_table_rva0 >> 8u) & 0xffu},
		std::byte{(guard_cf_function_table_rva0 >> 16u) & 0xffu},
		std::byte{(guard_cf_function_table_rva0 >> 24u) & 0xffu},
		//stride
		guard_cf_stride_data0[0], guard_cf_stride_data0[1], guard_cf_stride_data0[2],
		//rva1
		std::byte{guard_cf_function_table_rva1 & 0xffu},
		std::byte{(guard_cf_function_table_rva1 >> 8u) & 0xffu},
		std::byte{(guard_cf_function_table_rva1 >> 16u) & 0xffu},
		std::byte{(guard_cf_function_table_rva1 >> 24u) & 0xffu},
		//stride
		guard_cf_stride_data1[0], guard_cf_stride_data1[1], guard_cf_stride_data1[2],
		//rva2
		std::byte{guard_cf_function_table_rva2 & 0xffu},
		std::byte{(guard_cf_function_table_rva2 >> 8u) & 0xffu},
		std::byte{(guard_cf_function_table_rva2 >> 16u) & 0xffu},
		std::byte{(guard_cf_function_table_rva2 >> 24u) & 0xffu},
		//stride
		guard_cf_stride_data2[0], guard_cf_stride_data2[1], guard_cf_stride_data2[2],
	};

	static constexpr std::array guard_cf_function_table_rva1_hash{
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
		std::byte{7}, std::byte{8}, std::byte{9}, std::byte{0},
	};

	static constexpr std::pair cf_guard{ cf_guard32, cf_guard64 };
	static constexpr std::pair cf_guard_ex{ cf_guard_ex32, cf_guard_ex64 };

	static constexpr std::array code_integrity_arr{
		std::byte{}, std::byte{}, //flags
		std::byte{}, std::byte{}, //catalog
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //catalog_offset
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //reserved
	};

	static constexpr std::pair code_integrity{ code_integrity_arr, code_integrity_arr };

	static constexpr std::array hybrid_pe32{
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //dynamic_value_reloc_table
		//chpe_metadata_pointer
		std::byte{chpe_metadata_va & 0xffu},
		std::byte{(chpe_metadata_va >> 8u) & 0xffu},
		std::byte{(chpe_metadata_va >> 16u) & 0xffu},
		std::byte{(chpe_metadata_va >> 24u) & 0xffu},
	};

	static constexpr std::array hybrid_pe64{
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //dynamic_value_reloc_table
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		//chpe_metadata_pointer
		std::byte{chpe_metadata_va & 0xffu},
		std::byte{(chpe_metadata_va >> 8u) & 0xffu},
		std::byte{(chpe_metadata_va >> 16u) & 0xffu},
		std::byte{(chpe_metadata_va >> 24u) & 0xffu},
		std::byte{(chpe_metadata_va >> 32u) & 0xffu},
		std::byte{(chpe_metadata_va >> 40u) & 0xffu},
		std::byte{(chpe_metadata_va >> 48u) & 0xffu},
		std::byte{(chpe_metadata_va >> 56u) & 0xffu},
	};

	static constexpr std::uint32_t cphe_code_address_range_count = 3u;
	static constexpr std::uint32_t cphe_code_address_range_rva = section_rva + 0x380u;
	static constexpr std::uint32_t extra_rfe_table = 1u;
	static constexpr std::uint32_t extra_rfe_table_size = 2u;
	static constexpr std::array chpe_descriptor_arm64{
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //version
		//cphe_code_address_range_offset
		std::byte{cphe_code_address_range_rva & 0xffu},
		std::byte{(cphe_code_address_range_rva >> 8u) & 0xffu},
		std::byte{(cphe_code_address_range_rva >> 16u) & 0xffu},
		std::byte{(cphe_code_address_range_rva >> 24u) & 0xffu},
		//cphe_code_address_range_count
		std::byte{cphe_code_address_range_count}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //x64_code_ranges_to_entry_points_table
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //arm64x_redirection_metadata_table
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //dispatch_call_function_pointer_no_redirection
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //dispatch_return_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //unknown_rva1
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //dispatch_indirect_call_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //dispatch_indirect_call_function_pointer_with_cfg_check
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //alternative_entry_point
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //auxiliary_import_address_table
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //x64_code_ranges_to_entry_points_table_entry_count
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //arm64x_redirection_metadata_table_entry_count
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //unknown_rva2
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //unknown_rva3
		std::byte{extra_rfe_table}, std::byte{}, std::byte{}, std::byte{}, //extra_rfe_table
		std::byte{extra_rfe_table_size}, std::byte{}, std::byte{}, std::byte{}, //extra_rfe_table_size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //dispatch_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //copy_of_auxiliary_import_address_table
	};

	static constexpr std::array chpe_range{
		section_rva,
		0xfffffff0u,
		section_rva
	};
	static constexpr std::array chpe_range_flags_arm64{
		detail::load_config::chpe_arm64x_range_code_type_arm64,
		detail::load_config::chpe_arm64x_range_code_type_x64,
		detail::load_config::chpe_arm64x_range_code_type_arm64ec
	};
	static constexpr std::array chpe_range_flags_x86{
		detail::load_config::chpe_x86_range_code_type_x86,
		detail::load_config::chpe_x86_range_code_type_arm64,
		detail::load_config::chpe_x86_range_code_type_x86
	};
	static constexpr std::array chpe_range_entries_arm64{
		//rva
		std::byte{chpe_range_flags_arm64[0] | (chpe_range[0] & 0xffu)},
		std::byte{(chpe_range[0] >> 8u) & 0xffu},
		std::byte{(chpe_range[0] >> 16u) & 0xffu},
		std::byte{(chpe_range[0] >> 24u) & 0xffu},
		std::byte{5}, std::byte{}, std::byte{}, std::byte{}, //length
		//rva
		std::byte{chpe_range_flags_arm64[1] | (chpe_range[1] & 0xffu)},
		std::byte{(chpe_range[1] >> 8u) & 0xffu},
		std::byte{(chpe_range[1] >> 16u) & 0xffu},
		std::byte{(chpe_range[1] >> 24u) & 0xffu},
		std::byte{6}, std::byte{}, std::byte{}, std::byte{}, //length
		//rva
		std::byte{chpe_range_flags_arm64[2] | (chpe_range[2] & 0xffu)},
		std::byte{(chpe_range[2] >> 8u) & 0xffu},
		std::byte{(chpe_range[2] >> 16u) & 0xffu},
		std::byte{(chpe_range[2] >> 24u) & 0xffu},
		std::byte{7}, std::byte{}, std::byte{}, std::byte{0xfu}, //length
	};

	static constexpr std::array chpe_descriptor_x86{
		std::byte{2}, std::byte{}, std::byte{}, std::byte{}, //version
		//cphe_code_address_range_offset
		std::byte{cphe_code_address_range_rva & 0xffu},
		std::byte{(cphe_code_address_range_rva >> 8u) & 0xffu},
		std::byte{(cphe_code_address_range_rva >> 16u) & 0xffu},
		std::byte{(cphe_code_address_range_rva >> 24u) & 0xffu},
		//cphe_code_address_range_count
		std::byte{cphe_code_address_range_count}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //wow_a64_exception_handler_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //wow_a64_dispatch_call_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //wow_a64_dispatch_indirect_call_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //wow_a64_dispatch_indirect_call_cfg_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //wow_a64_dispatch_ret_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //wow_a64_dispatch_ret_leaf_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //wow_a64_dispatch_jump_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //compiler_iat_pointer
	};

	static constexpr std::array chpe_range_entries_x86{
		//rva
		std::byte{chpe_range_flags_x86[0] | (chpe_range[0] & 0xffu)},
		std::byte{(chpe_range[0] >> 8u) & 0xffu},
		std::byte{(chpe_range[0] >> 16u) & 0xffu},
		std::byte{(chpe_range[0] >> 24u) & 0xffu},
		std::byte{5}, std::byte{}, std::byte{}, std::byte{}, //length
		//rva
		std::byte{chpe_range_flags_x86[1] | (chpe_range[1] & 0xffu)},
		std::byte{(chpe_range[1] >> 8u) & 0xffu},
		std::byte{(chpe_range[1] >> 16u) & 0xffu},
		std::byte{(chpe_range[1] >> 24u) & 0xffu},
		std::byte{6}, std::byte{}, std::byte{}, std::byte{}, //length
		//rva
		std::byte{chpe_range_flags_x86[2] | (chpe_range[2] & 0xffu)},
		std::byte{(chpe_range[2] >> 8u) & 0xffu},
		std::byte{(chpe_range[2] >> 16u) & 0xffu},
		std::byte{(chpe_range[2] >> 24u) & 0xffu},
		std::byte{7}, std::byte{}, std::byte{}, std::byte{0xfu}, //length
	};

	static constexpr std::pair hybrid_pe{ hybrid_pe32, hybrid_pe64 };

	static constexpr std::uint32_t dvrt_offset = 0x500u;
	static constexpr std::array rf_guard64{
		//guard_rf_failure_routine
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		//guard_rf_failure_routine_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		//dynamic_value_reloc_table_offset
		std::byte{dvrt_offset & 0xffu},
		std::byte{(dvrt_offset >> 8u) & 0xffu},
		std::byte{(dvrt_offset >> 16u) & 0xffu},
		std::byte{(dvrt_offset >> 24u) & 0xffu},
		//dynamic_value_reloc_table_section
		std::byte{1}, std::byte{},
		//reserved2
		std::byte{}, std::byte{},
	};

	static constexpr std::size_t dvrt_v2_relocation_count = 3u;
	static constexpr std::size_t dvrt1_v2_base_reloc_count = 2u;
	static constexpr std::size_t dvrt1_v2_base_reloc0_fixup_count = 2u;
	static constexpr std::size_t dvrt1_v2_base_reloc1_fixup_count = 1u;
	static constexpr std::size_t dvrt1_v2_base_reloc0_base = 0x1100u;
	static constexpr std::size_t dvrt1_v2_base_reloc1_base = 0x1200u;
	static constexpr std::size_t dvrt1_v2_base_reloc1_fixup0 = 3u;
	static constexpr std::size_t dvrt2_v2_base_reloc_count = 1u;
	static constexpr std::size_t dvrt2_v2_base_reloc0_fixup_count = 1u;
	static constexpr std::uint16_t dvrt2_v2_branch_descriptor0 = 0x0102;
	static constexpr std::uint16_t dvrt2_v2_branch_descriptor1 = 0x0203;
	static constexpr std::uint16_t dvrt2_v2_branch_descriptor2 = 0x0304;
	static constexpr std::size_t dvrt2_v2_branch_descriptor_data_size = 1u;
	static constexpr std::size_t dvrt2_v2_branch_descriptor_count = 3u;
	static constexpr std::byte dvrt2_v2_branch_descriptor_data0{ 5u };
	static constexpr std::byte dvrt2_v2_branch_descriptor_data1{ 6u };
	static constexpr std::byte dvrt2_v2_branch_descriptor_data2{ 7u };
	static constexpr std::array dvrt2_v2_prologue_data{ std::byte{7}, std::byte{8} };
	static constexpr std::array dvrt_v2{
		//dvrt 0
		std::byte{26}, std::byte{}, std::byte{}, std::byte{}, //header_size
		std::byte{3}, std::byte{}, std::byte{}, std::byte{}, //fixup_info_size
		std::byte{3}, std::byte{}, std::byte{}, std::byte{}, //symbol - unknown
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //symbol_group
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //flags
		std::byte{1}, std::byte{2}, //header data
		std::byte{3}, std::byte{4}, std::byte{5}, //fixup_info

		//dvrt 1
		std::byte{27}, std::byte{}, std::byte{}, std::byte{}, //header_size
		std::byte{22}, std::byte{}, std::byte{}, std::byte{}, //fixup_info_size
		std::byte{1}, std::byte{}, std::byte{}, std::byte{}, //symbol
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //symbol_group
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //flags

		//prologue
		std::byte{2}, //prologue_byte_count
		std::byte{dvrt2_v2_prologue_data[0]}, std::byte{dvrt2_v2_prologue_data[1]}, //prologue

		//base relocations 0
		std::byte{0x00}, std::byte{0x11}, std::byte{}, std::byte{}, //virtual_address
		std::byte{12}, std::byte{}, std::byte{}, std::byte{}, //size_of_block
		std::byte{1}, std::byte{}, //fixup 0
		std::byte{2}, std::byte{}, //fixup 1
		//base relocations 1
		std::byte{0x00}, std::byte{0x12}, std::byte{}, std::byte{}, //virtual_address
		std::byte{10}, std::byte{}, std::byte{}, std::byte{}, //size_of_block
		std::byte{3}, std::byte{}, //fixup 0

		//dvrt 2
		std::byte{42}, std::byte{}, std::byte{}, std::byte{}, //header_size
		std::byte{12}, std::byte{}, std::byte{}, std::byte{}, //fixup_info_size
		std::byte{2}, std::byte{}, std::byte{}, std::byte{}, //symbol
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //symbol_group
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //flags

		//epilogue
		std::byte{2}, std::byte{}, std::byte{}, std::byte{}, //epilogue_count
		std::byte{}, //epilogue_byte_count
		std::byte{3}, //branch_descriptor_element_size
		std::byte{3}, std::byte{}, //branch_descriptor_count
		std::byte{dvrt2_v2_branch_descriptor0 & 0xffu},
		std::byte{(dvrt2_v2_branch_descriptor0 >> 8u) & 0xffu}, //branch descriptor 0
		std::byte{dvrt2_v2_branch_descriptor1 & 0xffu},
		std::byte{(dvrt2_v2_branch_descriptor1 >> 8u) & 0xffu}, //branch descriptor 1
		std::byte{dvrt2_v2_branch_descriptor2 & 0xffu},
		std::byte{(dvrt2_v2_branch_descriptor2 >> 8u) & 0xffu}, //branch descriptor 2
		//extra branch descriptor data
		dvrt2_v2_branch_descriptor_data0,
		dvrt2_v2_branch_descriptor_data1,
		dvrt2_v2_branch_descriptor_data2,
		std::byte{0xau}, //branch_descriptor_bit_map

		//base relocations 0
		std::byte{0x00}, std::byte{0x15}, std::byte{}, std::byte{}, //virtual_address
		std::byte{12}, std::byte{}, std::byte{}, std::byte{}, //size_of_block
		std::byte{}, std::byte{}, //fixup0
		std::byte{}, std::byte{}, //fixup1 - zero, will not be loaded
	};
	static constexpr auto dvrt_v2_size = dvrt_v2.size();
	static constexpr std::array dvrt_header_v2{
		std::byte{2}, std::byte{}, std::byte{}, std::byte{}, //version
		//size
		std::byte{dvrt_v2_size & 0xffu},
		std::byte{(dvrt_v2_size >> 8u) & 0xffu},
		std::byte{(dvrt_v2_size >> 16u) & 0xffu},
		std::byte{(dvrt_v2_size >> 24u) & 0xffu},
	};
	
	static constexpr std::size_t dvrt_v1_relocation_count = 6u;
	static constexpr std::size_t dvrt1_v1_base_reloc1_count = 1u;
	static constexpr std::size_t dvrt1_v1_base_reloc1_metadata1_count = 3u;
	static constexpr std::size_t dvrt1_v1_base_reloc2_count = 1u;
	static constexpr std::size_t dvrt1_v1_base_reloc2_metadata2_count = 2u;
	static constexpr std::size_t dvrt1_v1_base_reloc3_count = 2u;
	static constexpr std::size_t dvrt1_v1_base_reloc3_metadata3a_count = 1u;
	static constexpr std::size_t dvrt1_v1_base_reloc3_metadata3b_count = 1u;
	static constexpr std::size_t dvrt1_v1_base_reloc4_count = 1u;
	static constexpr std::size_t dvrt1_v1_base_reloc4_metadata4_count = 3u;
	static constexpr std::size_t dvrt1_v1_base_reloc5_count = 1u;
	static constexpr std::uint32_t dvrt1_v1_base_reloc1_metadata1_0 = 0x01020304u;
	static constexpr std::uint32_t dvrt1_v1_base_reloc1_metadata1_1 = 0x0a0b0c0du;
	static constexpr std::uint32_t dvrt1_v1_base_reloc1_metadata1_2 = 0xaabbccddu;
	static constexpr std::uint32_t dvrt1_v1_base_reloc2_metadata2_0 = 0xeeffu;
	static constexpr std::uint32_t dvrt1_v1_base_reloc2_metadata2_1 = 0x5577u;
	static constexpr std::uint32_t dvrt1_v1_base_reloc3_metadata3a = 0xabcdu;
	static constexpr std::uint32_t dvrt1_v1_base_reloc3_metadata3b = 0x2345u;
	static constexpr std::uint16_t dvrt1_v1_base_reloc4_metadata4_0 = 0x10u;
	static constexpr std::uint16_t dvrt1_v1_base_reloc4_metadata4_1 = 0x9020u;
	static constexpr std::array dvrt1_v1_base_reloc4_metadata4_1_extra{
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}
	};
	static constexpr std::uint16_t dvrt1_v1_base_reloc4_metadata4_2 = 0x20abu;
	static constexpr std::uint16_t dvrt1_v1_base_reloc4_metadata4_2_value = 0x0506u;
	static constexpr std::uint32_t dvrt1_v1_fixups5_0_rva = 0x123456abu;
	static constexpr std::size_t dvrt1_v1_fixups5_0_override_0_count = 2u;
	static constexpr std::size_t dvrt1_v1_fixups5_0_override_0_rvas_count = 2u;
	static constexpr std::size_t dvrt1_v1_fixups5_0_override_1_rvas_count = 0u;
	static constexpr std::uint32_t dvrt1_v1_fixups5_0_override_0_rva0 = 0xabcdef01u;
	static constexpr std::uint32_t dvrt1_v1_fixups5_0_override_0_rva1 = 0xea48a9beu;
	static constexpr std::size_t bdd_info0_reloc_count = 2u;
	static constexpr std::uint16_t bdd_info0_1_left = 0xaabbu;
	static constexpr std::uint16_t bdd_info0_0_right = 0xcdefu;
	static constexpr std::size_t dvrt1_v1_fixups5_0_override0_0_relocs_count = 2u;
	static constexpr std::uint32_t dvrt1_v1_fixups5_0_override0_0_reloc0_rva = 0x11223344u;
	static constexpr std::uint32_t dvrt1_v1_fixups5_0_override0_0_reloc1_rva = 0x55667788u;
	static constexpr std::size_t dvrt1_v1_fixups5_0_override0_0_reloc0_fixup_count = 2u;
	static constexpr std::size_t dvrt1_v1_fixups5_0_override0_0_reloc1_fixup_count = 0u;
	static constexpr std::size_t dvrt1_v1_fixups5_0_override0_0_reloc0_fixup0 = 0xabcdu;
	static constexpr std::size_t dvrt1_v1_fixups5_0_override0_0_reloc0_fixup1 = 0x7890u;
	static constexpr std::array dvrt_v1{
		//dvrt 0
		std::byte{2}, std::byte{}, std::byte{}, std::byte{}, //symbol - unknown
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{3}, std::byte{}, std::byte{}, std::byte{}, //base_reloc_size

		//dvrt 0 metadata
		std::byte{}, std::byte{}, std::byte{},

		//dvrt 1
		std::byte{3}, std::byte{}, std::byte{}, std::byte{}, //symbol
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{20}, std::byte{}, std::byte{}, std::byte{}, //base_reloc_size

		//base relocations 1
		std::byte{0x00}, std::byte{0x15}, std::byte{}, std::byte{}, //virtual_address
		std::byte{20}, std::byte{}, std::byte{}, std::byte{}, //size_of_block
		//dvrt 1 metadata
		std::byte{dvrt1_v1_base_reloc1_metadata1_0 & 0xffu},
		std::byte{(dvrt1_v1_base_reloc1_metadata1_0 >> 8u) & 0xffu},
		std::byte{(dvrt1_v1_base_reloc1_metadata1_0 >> 16u) & 0xffu},
		std::byte{(dvrt1_v1_base_reloc1_metadata1_0 >> 24u) & 0xffu},
		std::byte{dvrt1_v1_base_reloc1_metadata1_1 & 0xffu},
		std::byte{(dvrt1_v1_base_reloc1_metadata1_1 >> 8u) & 0xffu},
		std::byte{(dvrt1_v1_base_reloc1_metadata1_1 >> 16u) & 0xffu},
		std::byte{(dvrt1_v1_base_reloc1_metadata1_1 >> 24u) & 0xffu},
		std::byte{dvrt1_v1_base_reloc1_metadata1_2 & 0xffu},
		std::byte{(dvrt1_v1_base_reloc1_metadata1_2 >> 8u) & 0xffu},
		std::byte{(dvrt1_v1_base_reloc1_metadata1_2 >> 16u) & 0xffu},
		std::byte{(dvrt1_v1_base_reloc1_metadata1_2 >> 24u) & 0xffu},

		//dvrt 2
		std::byte{4}, std::byte{}, std::byte{}, std::byte{}, //symbol
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{12}, std::byte{}, std::byte{}, std::byte{}, //base_reloc_size

		//base relocations 2
		std::byte{0x00}, std::byte{0x16}, std::byte{}, std::byte{}, //virtual_address
		std::byte{12}, std::byte{}, std::byte{}, std::byte{}, //size_of_block
		//dvrt 2 metadata
		std::byte{dvrt1_v1_base_reloc2_metadata2_0 & 0xffu},
		std::byte{(dvrt1_v1_base_reloc2_metadata2_0 >> 8u) & 0xffu},
		std::byte{dvrt1_v1_base_reloc2_metadata2_1 & 0xffu},
		std::byte{(dvrt1_v1_base_reloc2_metadata2_1 >> 8u) & 0xffu},

		//dvrt 3
		std::byte{5}, std::byte{}, std::byte{}, std::byte{}, //symbol
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{20}, std::byte{}, std::byte{}, std::byte{}, //base_reloc_size

		//base relocation 3a
		std::byte{0x00}, std::byte{0x10}, std::byte{}, std::byte{}, //virtual_address
		std::byte{10}, std::byte{}, std::byte{}, std::byte{}, //size_of_block
		//dvrt 3a metadata
		std::byte{dvrt1_v1_base_reloc3_metadata3a & 0xffu},
		std::byte{(dvrt1_v1_base_reloc3_metadata3a >> 8u) & 0xffu},

		//base relocation 3b
		std::byte{0x00}, std::byte{0x20}, std::byte{}, std::byte{}, //virtual_address
		std::byte{10}, std::byte{}, std::byte{}, std::byte{}, //size_of_block
		//dvrt 3b metadata
		std::byte{dvrt1_v1_base_reloc3_metadata3b & 0xffu},
		std::byte{(dvrt1_v1_base_reloc3_metadata3b >> 8u) & 0xffu},

		//dvrt 4
		std::byte{6}, std::byte{}, std::byte{}, std::byte{}, //symbol
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{20}, std::byte{}, std::byte{}, std::byte{}, //base_reloc_size

		//base relocation 4
		std::byte{0x00}, std::byte{0x50}, std::byte{}, std::byte{}, //virtual_address
		std::byte{20}, std::byte{}, std::byte{}, std::byte{}, //size_of_block
		//dvrt 4 metadata
		//metadata 0
		std::byte{dvrt1_v1_base_reloc4_metadata4_0 & 0xffu},
		std::byte{(dvrt1_v1_base_reloc4_metadata4_0 >> 8u) & 0xffu},
		//metadata 1
		std::byte{dvrt1_v1_base_reloc4_metadata4_1 & 0xffu},
		std::byte{(dvrt1_v1_base_reloc4_metadata4_1 >> 8u) & 0xffu},
		dvrt1_v1_base_reloc4_metadata4_1_extra[0],
		dvrt1_v1_base_reloc4_metadata4_1_extra[1],
		dvrt1_v1_base_reloc4_metadata4_1_extra[2],
		dvrt1_v1_base_reloc4_metadata4_1_extra[3],
		//metadata 2
		std::byte{dvrt1_v1_base_reloc4_metadata4_2 & 0xffu},
		std::byte{(dvrt1_v1_base_reloc4_metadata4_2 >> 8u) & 0xffu},
		std::byte{dvrt1_v1_base_reloc4_metadata4_2_value & 0xffu},
		std::byte{(dvrt1_v1_base_reloc4_metadata4_2_value >> 8u) & 0xffu},

		//dvrt 5
		std::byte{7}, std::byte{}, std::byte{}, std::byte{}, //symbol
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{97}, std::byte{}, std::byte{}, std::byte{}, //base_reloc_size

		//base relocation 5
		//virtual_address
		std::byte{dvrt1_v1_fixups5_0_rva & 0xffu},
		std::byte{(dvrt1_v1_fixups5_0_rva >> 8u) & 0xffu},
		std::byte{(dvrt1_v1_fixups5_0_rva >> 16u) & 0xffu},
		std::byte{(dvrt1_v1_fixups5_0_rva >> 24u) & 0xffu},
		std::byte{97}, std::byte{}, std::byte{}, std::byte{}, //size_of_block

		//dvrt 5 metadata
		std::byte{60}, std::byte{}, std::byte{}, std::byte{}, //func_override_size
		//func 0
		std::byte{}, std::byte{0x1}, std::byte{}, std::byte{}, //original_rva
		std::byte{1}, std::byte{}, std::byte{}, std::byte{}, //bdd_offset
		std::byte{8}, std::byte{}, std::byte{}, std::byte{}, //rva_size
		std::byte{20}, std::byte{}, std::byte{}, std::byte{}, //base_reloc_size
		//rva 0
		std::byte{ dvrt1_v1_fixups5_0_override_0_rva0 & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override_0_rva0 >> 8u) & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override_0_rva0 >> 16u) & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override_0_rva0 >> 24u) & 0xffu },
		//rva 1
		std::byte{ dvrt1_v1_fixups5_0_override_0_rva1 & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override_0_rva1 >> 8u) & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override_0_rva1 >> 16u) & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override_0_rva1 >> 24u) & 0xffu },
		//base reloc virtual_address
		std::byte{ dvrt1_v1_fixups5_0_override0_0_reloc0_rva & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override0_0_reloc0_rva >> 8u) & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override0_0_reloc0_rva >> 16u) & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override0_0_reloc0_rva >> 24u) & 0xffu },
		std::byte{12}, std::byte{}, std::byte{}, std::byte{}, //base reloc size_of_block
		//fixup 0
		std::byte{ dvrt1_v1_fixups5_0_override0_0_reloc0_fixup0 & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override0_0_reloc0_fixup0 >> 8u) & 0xffu },
		//fixup 1
		std::byte{ dvrt1_v1_fixups5_0_override0_0_reloc0_fixup1 & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override0_0_reloc0_fixup1 >> 8u) & 0xffu },
		//base reloc virtual_address
		std::byte{ dvrt1_v1_fixups5_0_override0_0_reloc1_rva & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override0_0_reloc1_rva >> 8u) & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override0_0_reloc1_rva >> 16u) & 0xffu },
		std::byte{ (dvrt1_v1_fixups5_0_override0_0_reloc1_rva >> 24u) & 0xffu },
		std::byte{8}, std::byte{}, std::byte{}, std::byte{}, //base reloc size_of_block
		//func 1
		std::byte{}, std::byte{0x2}, std::byte{}, std::byte{}, //original_rva
		std::byte{1}, std::byte{}, std::byte{}, std::byte{}, //bdd_offset
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //rva_size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //base_reloc_size
		//bdd info
		std::byte{0xffu}, //dummy byte
		std::byte{1}, std::byte{}, std::byte{}, std::byte{}, //bdd version
		std::byte{16}, std::byte{}, std::byte{}, std::byte{}, //bdd size
		//bdd info reloc 0
		std::byte{ 3 }, std::byte{ 4 },
		std::byte{ bdd_info0_0_right & 0xffu },
		std::byte{ (bdd_info0_0_right >> 8u) & 0xffu },
		std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8},
		//bdd info reloc 1
		std::byte{ bdd_info0_1_left & 0xffu },
		std::byte{ (bdd_info0_1_left >> 8u) & 0xffu },
		std::byte{10}, std::byte{11},
		std::byte{14}, std::byte{15}, std::byte{16}, std::byte{17},
	};

	static constexpr std::size_t dvrt_v1_size = dvrt_v1.size();
	static constexpr std::array dvrt_header_v1{
		std::byte{1}, std::byte{}, std::byte{}, std::byte{}, //version
		//size
		std::byte{dvrt_v1_size & 0xffu},
		std::byte{(dvrt_v1_size >> 8u) & 0xffu},
		std::byte{(dvrt_v1_size >> 16u) & 0xffu},
		std::byte{(dvrt_v1_size >> 24u) & 0xffu},
	};

	static constexpr std::array dvrt_header_v0{
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //version - unsupported
		//size
		std::byte{dvrt_v1_size & 0xffu},
		std::byte{(dvrt_v1_size >> 8u) & 0xffu},
		std::byte{(dvrt_v1_size >> 16u) & 0xffu},
		std::byte{(dvrt_v1_size >> 24u) & 0xffu},
	};

	//Loader should ignore rf guard set for x86 PE version, as RF guard is not
	//available on x86 PE.
	static constexpr auto rf_guard32_size = detail::packed_reflection::get_type_size<
		detail::load_config::rf_guard32>();
	static constexpr std::pair rf_guard{ std::array<std::byte,
		rf_guard32_size>{std::byte{0xffu}}, rf_guard64 };

	static constexpr std::array rf_guard_ex32{
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //guard_rf_verify_stack_pointer_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //hot_patch_table_offset
	};
	static constexpr std::array rf_guard_ex64{
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //guard_rf_verify_stack_pointer_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //hot_patch_table_offset
	};
	static constexpr std::pair rf_guard_ex{ rf_guard_ex32, rf_guard_ex64 };

	static constexpr std::uint64_t enclave_va = section_rva + image_base + 0x700u;
	static constexpr std::array enclave32{
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //reserved
		//enclave_configuration_pointer
		std::byte{enclave_va & 0xffu},
		std::byte{(enclave_va >> 8u) & 0xffu},
		std::byte{(enclave_va >> 16u) & 0xffu},
		std::byte{(enclave_va >> 24u) & 0xffu},
	};
	static constexpr std::array enclave64{
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //reserved
		//enclave_configuration_pointer
		std::byte{enclave_va & 0xffu},
		std::byte{(enclave_va >> 8u) & 0xffu},
		std::byte{(enclave_va >> 16u) & 0xffu},
		std::byte{(enclave_va >> 24u) & 0xffu},
		std::byte{(enclave_va >> 32u) & 0xffu},
		std::byte{(enclave_va >> 40u) & 0xffu},
		std::byte{(enclave_va >> 48u) & 0xffu},
		std::byte{(enclave_va >> 56u) & 0xffu},
	};

	static constexpr std::array enclave_family_id{
		std::byte{'e'}, std::byte{'n'}, std::byte{'c'}, std::byte{'l'},
		std::byte{'a'}, std::byte{'v'}, std::byte{'e'}, std::byte{' '},
		std::byte{'f'}, std::byte{'a'}, std::byte{'m'}, std::byte{'i'},
		std::byte{'l'}, std::byte{'y'}, std::byte{}, std::byte{},
	};
	static constexpr std::array enclave_image_id{
		std::byte{'e'}, std::byte{'n'}, std::byte{'c'}, std::byte{'l'},
		std::byte{'a'}, std::byte{'v'}, std::byte{'e'}, std::byte{' '},
		std::byte{'i'}, std::byte{'m'}, std::byte{'a'}, std::byte{'g'},
		std::byte{'e'}, std::byte{}, std::byte{}, std::byte{},
	};
	//There is no part2, as all part2 bytes are zero
	//(enclave_size, number_of_threads, enclave_flags).
	static constexpr std::size_t enclave_import_count = 2u;
	static constexpr std::uint32_t import_list_rva = section_rva + 0x800u;
	static constexpr std::uint32_t import_name1_rva = section_rva + 0xa00u;
	static constexpr std::array enclave_config_part1{
		std::byte{}, std::byte{}, std::byte{}, std::byte{1}, //size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //minimum_required_config_size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //policy_flags
		std::byte{2}, std::byte{}, std::byte{}, std::byte{}, //number_of_imports
		//import_list
		std::byte{import_list_rva & 0xffu},
		std::byte{(import_list_rva >> 8u) & 0xffu},
		std::byte{(import_list_rva >> 16u) & 0xffu},
		std::byte{(import_list_rva >> 24u) & 0xffu},
		std::byte{82}, std::byte{}, std::byte{}, std::byte{}, //import_entry_size (2 extra bytes)
		//family id
		enclave_family_id[0], enclave_family_id[1],
		enclave_family_id[2], enclave_family_id[3],
		enclave_family_id[4], enclave_family_id[5],
		enclave_family_id[6], enclave_family_id[7],
		enclave_family_id[8], enclave_family_id[9],
		enclave_family_id[10], enclave_family_id[11],
		enclave_family_id[12], enclave_family_id[13],
		enclave_family_id[14], enclave_family_id[15],
		//image
		enclave_image_id[0], enclave_image_id[1],
		enclave_image_id[2], enclave_image_id[3],
		enclave_image_id[4], enclave_image_id[5],
		enclave_image_id[6], enclave_image_id[7],
		enclave_image_id[8], enclave_image_id[9],
		enclave_image_id[10], enclave_image_id[11],
		enclave_image_id[12], enclave_image_id[13],
		enclave_image_id[14], enclave_image_id[15],
		std::byte{}, std::byte{}, std::byte{}, std::byte{1}, //image_version
		std::byte{}, std::byte{}, std::byte{}, std::byte{1}, //security_version
	};

	static constexpr std::array enclave_import1_uid{
		std::byte{'e'}, std::byte{'n'}, std::byte{'c'}, std::byte{'l'},
		std::byte{'a'}, std::byte{'v'}, std::byte{'e'}, std::byte{' '},
		std::byte{'i'}, std::byte{'m'}, std::byte{'p'}, std::byte{'o'},
		std::byte{'r'}, std::byte{'t'}, std::byte{'1'}, std::byte{' '},
		std::byte{'u'}, std::byte{'i'}, std::byte{'d'}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
	};
	static constexpr std::array enclave_import2_uid{
		std::byte{'e'}, std::byte{'n'}, std::byte{'c'}, std::byte{'l'},
		std::byte{'a'}, std::byte{'v'}, std::byte{'e'}, std::byte{' '},
		std::byte{'i'}, std::byte{'m'}, std::byte{'p'}, std::byte{'o'},
		std::byte{'r'}, std::byte{'t'}, std::byte{'2'}, std::byte{' '},
		std::byte{'u'}, std::byte{'i'}, std::byte{'d'}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
	};
	static constexpr std::array enclave_import0_extra{
		std::byte{1}, std::byte{2}
	};
	static constexpr std::array enclave_import1_extra{
		std::byte{0xau}, std::byte{0xbu}
	};
	static constexpr std::array enclave_imports{
		//import 0
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //match_type = none
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //minimum_security_version
		//unique_or_author_id
		enclave_import1_uid[0], enclave_import1_uid[1],
		enclave_import1_uid[2], enclave_import1_uid[3],
		enclave_import1_uid[4], enclave_import1_uid[5],
		enclave_import1_uid[6], enclave_import1_uid[7],
		enclave_import1_uid[8], enclave_import1_uid[9],
		enclave_import1_uid[10], enclave_import1_uid[11],
		enclave_import1_uid[12], enclave_import1_uid[13],
		enclave_import1_uid[14], enclave_import1_uid[15],
		enclave_import1_uid[16], enclave_import1_uid[17],
		enclave_import1_uid[18], enclave_import1_uid[19],
		enclave_import1_uid[20], enclave_import1_uid[21],
		enclave_import1_uid[22], enclave_import1_uid[23],
		enclave_import1_uid[24], enclave_import1_uid[25],
		enclave_import1_uid[26], enclave_import1_uid[27],
		enclave_import1_uid[28], enclave_import1_uid[29],
		enclave_import1_uid[30], enclave_import1_uid[31],
		//family id
		enclave_family_id[0], enclave_family_id[1],
		enclave_family_id[2], enclave_family_id[3],
		enclave_family_id[4], enclave_family_id[5],
		enclave_family_id[6], enclave_family_id[7],
		enclave_family_id[8], enclave_family_id[9],
		enclave_family_id[10], enclave_family_id[11],
		enclave_family_id[12], enclave_family_id[13],
		enclave_family_id[14], enclave_family_id[15],
		//image
		enclave_image_id[0], enclave_image_id[1],
		enclave_image_id[2], enclave_image_id[3],
		enclave_image_id[4], enclave_image_id[5],
		enclave_image_id[6], enclave_image_id[7],
		enclave_image_id[8], enclave_image_id[9],
		enclave_image_id[10], enclave_image_id[11],
		enclave_image_id[12], enclave_image_id[13],
		enclave_image_id[14], enclave_image_id[15],
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //import_name
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //reserved
		enclave_import0_extra[0], enclave_import0_extra[1], //extra data

		//import 1
		std::byte{1}, std::byte{}, std::byte{}, std::byte{}, //match_type = unique_id
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //minimum_security_version
		//unique_or_author_id
		enclave_import2_uid[0], enclave_import2_uid[1],
		enclave_import2_uid[2], enclave_import2_uid[3],
		enclave_import2_uid[4], enclave_import2_uid[5],
		enclave_import2_uid[6], enclave_import2_uid[7],
		enclave_import2_uid[8], enclave_import2_uid[9],
		enclave_import2_uid[10], enclave_import2_uid[11],
		enclave_import2_uid[12], enclave_import2_uid[13],
		enclave_import2_uid[14], enclave_import2_uid[15],
		enclave_import2_uid[16], enclave_import2_uid[17],
		enclave_import2_uid[18], enclave_import2_uid[19],
		enclave_import2_uid[20], enclave_import2_uid[21],
		enclave_import2_uid[22], enclave_import2_uid[23],
		enclave_import2_uid[24], enclave_import2_uid[25],
		enclave_import2_uid[26], enclave_import2_uid[27],
		enclave_import2_uid[28], enclave_import2_uid[29],
		enclave_import2_uid[30], enclave_import2_uid[31],
		//family id
		enclave_family_id[0], enclave_family_id[1],
		enclave_family_id[2], enclave_family_id[3],
		enclave_family_id[4], enclave_family_id[5],
		enclave_family_id[6], enclave_family_id[7],
		enclave_family_id[8], enclave_family_id[9],
		enclave_family_id[10], enclave_family_id[11],
		enclave_family_id[12], enclave_family_id[13],
		enclave_family_id[14], enclave_family_id[15],
		//image
		enclave_image_id[0], enclave_image_id[1],
		enclave_image_id[2], enclave_image_id[3],
		enclave_image_id[4], enclave_image_id[5],
		enclave_image_id[6], enclave_image_id[7],
		enclave_image_id[8], enclave_image_id[9],
		enclave_image_id[10], enclave_image_id[11],
		enclave_image_id[12], enclave_image_id[13],
		enclave_image_id[14], enclave_image_id[15],
		//import_name
		std::byte{import_name1_rva & 0xffu},
		std::byte{(import_name1_rva >> 8u) & 0xffu},
		std::byte{(import_name1_rva >> 16u) & 0xffu},
		std::byte{(import_name1_rva >> 24u) & 0xffu},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //reserved
		enclave_import1_extra[0], enclave_import1_extra[1], //extra data
	};

	static constexpr std::array enclave_import1_name{
		std::byte{'i'}, std::byte{'m'}, std::byte{'p'}, std::byte{'o'},
		std::byte{'r'}, std::byte{'t'}, std::byte{}
	};

	static constexpr std::pair enclave{ enclave32, enclave64 };

	static constexpr std::uint64_t volatile_metadata_va = section_rva + image_base + 0xc00u;
	static constexpr std::array volatile_metadata32{
		//volatile_metadata_pointer
		std::byte{volatile_metadata_va & 0xffu},
		std::byte{(volatile_metadata_va >> 8u) & 0xffu},
		std::byte{(volatile_metadata_va >> 16u) & 0xffu},
		std::byte{(volatile_metadata_va >> 24u) & 0xffu},
	};
	static constexpr std::array volatile_metadata64{
		//volatile_metadata_pointer
		std::byte{volatile_metadata_va & 0xffu},
		std::byte{(volatile_metadata_va >> 8u) & 0xffu},
		std::byte{(volatile_metadata_va >> 16u) & 0xffu},
		std::byte{(volatile_metadata_va >> 24u) & 0xffu},
		std::byte{(volatile_metadata_va >> 32u) & 0xffu},
		std::byte{(volatile_metadata_va >> 40u) & 0xffu},
		std::byte{(volatile_metadata_va >> 48u) & 0xffu},
		std::byte{(volatile_metadata_va >> 56u) & 0xffu},
	};

	static constexpr std::uint64_t volatile_access_table_rva = section_rva + 0xc50u;
	static constexpr std::uint64_t volatile_info_range_table_rva = section_rva + 0xca0u;
	static constexpr std::array volatile_metadata_descriptor{
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //version
		//volatile_access_table_rva
		std::byte{volatile_access_table_rva & 0xffu},
		std::byte{(volatile_access_table_rva >> 8u) & 0xffu},
		std::byte{(volatile_access_table_rva >> 16u) & 0xffu},
		std::byte{(volatile_access_table_rva >> 24u) & 0xffu},
		std::byte{8}, std::byte{}, std::byte{}, std::byte{}, //volatile_access_table
		//volatile_info_range_table
		std::byte{volatile_info_range_table_rva & 0xffu},
		std::byte{(volatile_info_range_table_rva >> 8u) & 0xffu},
		std::byte{(volatile_info_range_table_rva >> 16u) & 0xffu},
		std::byte{(volatile_info_range_table_rva >> 24u) & 0xffu},
		std::byte{9}, std::byte{}, std::byte{}, std::byte{}, //volatile_info_range_table_size
	};
	static constexpr std::uint64_t volatile_access_table_rva0 = 0x12345678u;
	static constexpr std::uint64_t volatile_access_table_rva1 = 0xcdef0192u;
	static constexpr std::uint64_t volatile_access_table_rva2 = 0xffddeeaau;
	static constexpr std::size_t volatile_metadata_access_table_size = 2u;
	static constexpr std::size_t volatile_metadata_range_table_size = 1u;
	static constexpr std::array volatile_access_table{
		//rva 0
		std::byte{volatile_access_table_rva0 & 0xffu},
		std::byte{(volatile_access_table_rva0 >> 8u) & 0xffu},
		std::byte{(volatile_access_table_rva0 >> 16u) & 0xffu},
		std::byte{(volatile_access_table_rva0 >> 24u) & 0xffu},
		//rva 1
		std::byte{volatile_access_table_rva1 & 0xffu},
		std::byte{(volatile_access_table_rva1 >> 8u) & 0xffu},
		std::byte{(volatile_access_table_rva1 >> 16u) & 0xffu},
		std::byte{(volatile_access_table_rva1 >> 24u) & 0xffu},
	};
	static constexpr std::array volatile_info_range_table{
		//rva 2
		std::byte{volatile_access_table_rva2 & 0xffu},
		std::byte{(volatile_access_table_rva2 >> 8u) & 0xffu},
		std::byte{(volatile_access_table_rva2 >> 16u) & 0xffu},
		std::byte{(volatile_access_table_rva2 >> 24u) & 0xffu},
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //size 2
	};

	static constexpr std::pair volatile_metadata{ volatile_metadata32, volatile_metadata64 };

	static constexpr std::uint64_t guard_eh_continuation_table_va
		= section_rva + image_base + 0xd00u;
	static constexpr std::uint32_t guard_eh_continuation_count = 2u;
	static constexpr std::array ehcont_targets32{
		//guard_eh_continuation_table
		std::byte{guard_eh_continuation_table_va & 0xffu},
		std::byte{(guard_eh_continuation_table_va >> 8u) & 0xffu},
		std::byte{(guard_eh_continuation_table_va >> 16u) & 0xffu},
		std::byte{(guard_eh_continuation_table_va >> 24u) & 0xffu},
		//guard_eh_continuation_count
		std::byte{guard_eh_continuation_count}, std::byte{}, std::byte{}, std::byte{}
	};
	static constexpr std::array ehcont_targets64{
		//guard_eh_continuation_table
		std::byte{guard_eh_continuation_table_va & 0xffu},
		std::byte{(guard_eh_continuation_table_va >> 8u) & 0xffu},
		std::byte{(guard_eh_continuation_table_va >> 16u) & 0xffu},
		std::byte{(guard_eh_continuation_table_va >> 24u) & 0xffu},
		std::byte{(guard_eh_continuation_table_va >> 32u) & 0xffu},
		std::byte{(guard_eh_continuation_table_va >> 40u) & 0xffu},
		std::byte{(guard_eh_continuation_table_va >> 48u) & 0xffu},
		std::byte{(guard_eh_continuation_table_va >> 56u) & 0xffu},
		//guard_eh_continuation_count
		std::byte{guard_eh_continuation_count}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}
	};

	static constexpr std::uint32_t ehcont_rva0 = 0x48271837u;
	static constexpr std::uint32_t ehcont_rva1 = 0xab83d0abu;
	static constexpr std::array ehcont_rvas{
		//rva 0
		std::byte{ehcont_rva0 & 0xffu},
		std::byte{(ehcont_rva0 >> 8u) & 0xffu},
		std::byte{(ehcont_rva0 >> 16u) & 0xffu},
		std::byte{(ehcont_rva0 >> 24u) & 0xffu},
		//rva 1
		std::byte{ehcont_rva1 & 0xffu},
		std::byte{(ehcont_rva1 >> 8u) & 0xffu},
		std::byte{(ehcont_rva1 >> 16u) & 0xffu},
		std::byte{(ehcont_rva1 >> 24u) & 0xffu},
	};

	static constexpr std::pair ehcont_targets{ ehcont_targets32, ehcont_targets64 };

	static constexpr std::array xf_guard64{
		//guard_xfg_check_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{1},
		//guard_xfg_dispatch_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{2},
		//guard_xfg_table_dispatch_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{3},
	};
	static constexpr std::array xf_guard32{
		//guard_xfg_check_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{1},
		//guard_xfg_dispatch_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{2},
		//guard_xfg_table_dispatch_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{3},
	};
	static constexpr std::pair xf_guard{ xf_guard32, xf_guard64 };

	static constexpr std::array cast_guard64{
		//cast_guard_os_determined_failure_mode
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{1},
	};
	static constexpr std::array cast_guard32{
		//cast_guard_os_determined_failure_mode
		std::byte{}, std::byte{}, std::byte{}, std::byte{1},
	};
	static constexpr std::pair cast_guard{ cast_guard32, cast_guard64 };

	static constexpr std::array memcpy_guard64{
		//guard_memcpy_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{1},
	};
	static constexpr std::array memcpy_guard32{
		//guard_memcpy_function_pointer
		std::byte{}, std::byte{}, std::byte{}, std::byte{1},
	};
	static constexpr std::pair memcpy_guard{ memcpy_guard32, memcpy_guard64 };
};
} //namespace

TEST_P(LoadConfigLoaderTestFixture, LoadAbsentLoadConfigDirectory)
{
	auto result = load_config::load(instance);
	EXPECT_FALSE(result);
}

TEST_P(LoadConfigLoaderTestFixture, LoadEmptyLoadConfigDirectory)
{
	add_load_config_directory();
	with_load_config([] (const auto& dir) {
		expect_contains_errors(dir);
		EXPECT_FALSE(dir.get_lock_prefix_table());
		EXPECT_EQ(dir.get_global_flags_set(), 0u);
		EXPECT_EQ(dir.get_global_flags_clear(), 0u);
		EXPECT_EQ(dir.get_process_heap_flags(), 0u);
		EXPECT_EQ(dir.get_dependent_load_flags(), 0u);
		EXPECT_FALSE(dir.get_safeseh_handler_table());
		EXPECT_EQ(dir.get_size().get(), 0u);
		EXPECT_EQ(dir.get_version(), load_config::version::base);
		EXPECT_FALSE(dir.version_exactly_matches());
		EXPECT_EQ(dir.get_descriptor().physical_size(), 0u);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadBaseLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base);
	add_lock_prefix_table();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
		validate_size_and_version(dir, load_config::version::base, load_config_base);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadBaseLoadConfigDirectorySkipLockPrefixes)
{
	add_load_config_directory();
	add_directory_parts(load_config_base);
	add_lock_prefix_table();
	with_load_config([this](const auto& dir) {
		validate_base(dir, false);
	}, { .load_lock_prefix_table = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadBaseLoadConfigDirectoryInvalidLockPrefixes)
{
	add_load_config_directory();
	auto load_config_base_copy = load_config_base;
	{
		auto lock_prefix_offset = detail::packed_reflection::get_field_offset<
			&detail::load_config::image_load_config_directory_base32::lock_prefix_table>();
		load_config_base_copy.first[lock_prefix_offset + sizeof(std::uint32_t)
			- 1u] = std::byte{0xffu};
	}
	{
		auto lock_prefix_offset = detail::packed_reflection::get_field_offset<
			&detail::load_config::image_load_config_directory_base64::lock_prefix_table>();
		load_config_base_copy.second[lock_prefix_offset + sizeof(std::uint64_t)
			- 1u] = std::byte{ 0xffu };
	}
	add_directory_parts(load_config_base_copy);
	with_load_config([this](const auto& dir) {
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_lock_prefix_table);
		ASSERT_TRUE(dir.get_lock_prefix_table());
		EXPECT_TRUE(dir.get_lock_prefix_table()->get_prefix_va_list().empty());
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadSafeSehLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table);
	add_lock_prefix_table();
	add_se_handlers();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_safeseh(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
		validate_size_and_version(dir, load_config::version::seh,
			load_config_base, se_handler_table);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadSafeSehLoadConfigDirectorySkipSafeSeh)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table);
	add_lock_prefix_table();
	add_se_handlers();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		EXPECT_FALSE(dir.get_safeseh_handler_table());
	}, { .load_safeseh_handler_table = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadLimitedSafeSehLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table);
	add_lock_prefix_table();
	add_se_handlers();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_safeseh(dir, 1u);
		if (!is_x64())
		{
			expect_contains_errors(dir,
				load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
				load_config::load_config_directory_loader_errc::invalid_safeseh_handler_table);
		}
		validate_size_and_version(dir, load_config::version::seh,
			load_config_base, se_handler_table);
	}, { .max_safeseh_handler_count = 1u });
}

TEST_P(LoadConfigLoaderTestFixture, LoadInvalidSafeSehLoadConfigDirectory)
{
	add_load_config_directory();
	auto se_handler_table_copy = se_handler_table;
	auto se_handler_va_offset = detail::packed_reflection::get_field_offset<
		&detail::load_config::structured_exceptions32::se_handler_table>();
	se_handler_table_copy.first[se_handler_va_offset + sizeof(std::uint32_t)
		- 1u] = std::byte{ 0xffu };
	add_directory_parts(load_config_base, se_handler_table_copy);
	add_lock_prefix_table();
	add_se_handlers();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		if (!is_x64())
		{
			ASSERT_TRUE(dir.get_safeseh_handler_table());
			ASSERT_EQ(dir.get_safeseh_handler_table()->get_handler_list().size(), 0u);
			expect_contains_errors(dir,
				load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
				load_config::load_config_directory_loader_errc::invalid_safeseh_handler_table);
		}
		else
		{
			EXPECT_FALSE(dir.get_safeseh_handler_table());
		}
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard);
	add_lock_prefix_table();
	add_se_handlers();
	add_cf_guard_functions();
	instance.get_optional_header().set_raw_dll_characteristics(
		core::optional_header::dll_characteristics::guard_cf);
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_safeseh(dir);
		validate_cf_guard(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_guard_cf_check_function_va,
			load_config::load_config_directory_loader_errc::invalid_guard_cf_dispatch_function_va,
			load_config::load_config_directory_loader_errc::unsorted_cf_guard_table);
		validate_size_and_version(dir, load_config::version::cf_guard,
			load_config_base, se_handler_table, cf_guard);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLoadConfigDirectoryLimit)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard);
	add_lock_prefix_table();
	add_se_handlers();
	add_cf_guard_functions();
	instance.get_optional_header().set_raw_dll_characteristics(
		core::optional_header::dll_characteristics::guard_cf);
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_safeseh(dir);
		validate_cf_guard(dir, true, true, 1u);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_guard_cf_check_function_va,
			load_config::load_config_directory_loader_errc::invalid_guard_cf_dispatch_function_va,
			load_config::load_config_directory_loader_errc::invalid_cf_guard_table_function_count);
		validate_size_and_version(dir, load_config::version::cf_guard,
			load_config_base, se_handler_table, cf_guard);
	}, { .max_cf_function_table_functions = 1 });
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLoadConfigDirectorySkipCfGuardFunctions)
{
	test_cf_guard(true, false);
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLoadConfigDirectorySkipCfDllCharacteristics)
{
	test_cf_guard(false, true);
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLoadConfigDirectorySkipCfFunctionTableFlag)
{
	test_cf_guard(true, true, load_config::guard_flags::cf_instrumented
		| load_config::guard_flags::xfg_enabled, [](const auto& dir) {
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_guard_cf_check_function_va,
			load_config::load_config_directory_loader_errc::invalid_guard_cf_dispatch_function_va);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLoadConfigDirectorySkipCfInstrumented)
{
	test_cf_guard(true, true, load_config::guard_flags::cf_function_table_present
		| load_config::guard_flags::xfg_enabled, [](const auto& dir) {
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLoadConfigDirectorySkipXfg)
{
	add_load_config_directory();
	auto cf_guard_copy = cf_guard;
	override_guard_flags<&detail::load_config::cf_guard32::guard_flags>(
		cf_guard_copy.first, guard_flags & ~load_config::guard_flags::xfg_enabled);
	override_guard_flags<&detail::load_config::cf_guard64::guard_flags>(
		cf_guard_copy.second, guard_flags & ~load_config::guard_flags::xfg_enabled);
	add_directory_parts(load_config_base, se_handler_table, cf_guard_copy);
	add_lock_prefix_table();
	add_se_handlers();
	add_cf_guard_functions();
	instance.get_optional_header().set_raw_dll_characteristics(
		core::optional_header::dll_characteristics::guard_cf);
	with_load_config([this, &cf_guard_copy](const auto& dir) {
		validate_base(dir);
		validate_safeseh(dir);
		validate_cf_guard(dir, false);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_guard_cf_check_function_va,
			load_config::load_config_directory_loader_errc::invalid_guard_cf_dispatch_function_va,
			load_config::load_config_directory_loader_errc::unsorted_cf_guard_table);
		validate_size_and_version(dir, load_config::version::cf_guard,
			load_config_base, se_handler_table, cf_guard_copy);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLoadConfigDirectorySkipXfgLoad)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard);
	add_lock_prefix_table();
	add_se_handlers();
	add_cf_guard_functions();
	instance.get_optional_header().set_raw_dll_characteristics(
		core::optional_header::dll_characteristics::guard_cf);
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_safeseh(dir);
		validate_cf_guard(dir, false, false);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_guard_cf_check_function_va,
			load_config::load_config_directory_loader_errc::invalid_guard_cf_dispatch_function_va,
			load_config::load_config_directory_loader_errc::unsorted_cf_guard_table);
		validate_size_and_version(dir, load_config::version::cf_guard,
			load_config_base, se_handler_table, cf_guard);
	}, { .load_xfg_type_based_hashes = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardExportSuppressionLoadConfigDirectory1)
{
	test_cf_tables(
		load_config::load_config_directory_loader_errc::unsorted_guard_export_suppression_table,
		load_config::guard_flags::cf_export_suppression_info_present, [](const auto& dir)
	{
		EXPECT_FALSE(dir.get_guard_long_jump_target_table());
		validate_cf_guard_table(dir.get_guard_address_taken_iat_entry_table(), false);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardExportSuppressionLoadConfigDirectory2)
{
	test_cf_tables(
		load_config::load_config_directory_loader_errc::unsorted_guard_export_suppression_table,
		load_config::guard_flags::cf_export_suppression_info_present
		| load_config::guard_flags::cf_longjump_table_present, [](const auto& dir)
	{
		EXPECT_FALSE(dir.get_guard_long_jump_target_table());
		validate_cf_guard_table(dir.get_guard_address_taken_iat_entry_table(), false);
	}, { .load_cf_guard_longjump_table = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardExportSuppressionLoadConfigDirectoryLimit)
{
	test_cf_tables(
		load_config::load_config_directory_loader_errc::invalid_guard_export_suppression_table_function_count,
		load_config::guard_flags::cf_export_suppression_info_present, [](const auto& dir)
	{
		EXPECT_FALSE(dir.get_guard_long_jump_target_table());
		validate_cf_guard_table(dir.get_guard_address_taken_iat_entry_table(), false, 1u);
	}, { .max_guard_export_suppression_table_functions = 1u });
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLongjmpLoadConfigDirectory1)
{
	test_cf_tables(
		load_config::load_config_directory_loader_errc::unsorted_guard_longjump_table,
		load_config::guard_flags::cf_longjump_table_present, [](const auto& dir)
	{
		EXPECT_FALSE(dir.get_guard_address_taken_iat_entry_table());
		validate_cf_guard_table(dir.get_guard_long_jump_target_table(), false);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLongjmpLoadConfigDirectory2)
{
	test_cf_tables(
		load_config::load_config_directory_loader_errc::unsorted_guard_longjump_table,
		load_config::guard_flags::cf_export_suppression_info_present
		| load_config::guard_flags::cf_longjump_table_present, [](const auto& dir)
	{
		EXPECT_FALSE(dir.get_guard_address_taken_iat_entry_table());
		validate_cf_guard_table(dir.get_guard_long_jump_target_table(), false);
	}, { .load_cf_guard_export_suppression_table = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadCfGuardLongjmpLoadConfigDirectoryLimit)
{
	test_cf_tables(
		load_config::load_config_directory_loader_errc::invalid_guard_longjump_table_function_count,
		load_config::guard_flags::cf_longjump_table_present, [](const auto& dir)
	{
		EXPECT_FALSE(dir.get_guard_address_taken_iat_entry_table());
		validate_cf_guard_table(dir.get_guard_long_jump_target_table(), false, 1u);
	}, { .max_guard_longjump_table_functions = 1u });
}

TEST_P(LoadConfigLoaderTestFixture, LoadCHPELoadConfigDirectoryUnknown)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe);
	add_lock_prefix_table();
	add_chpe_tables();
	instance.get_file_header().set_machine_type(core::file_header::machine_type::pentium);
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::unknown_chpe_metadata_type);
		validate_size_and_version(dir, load_config::version::hybrid_pe,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadCHPELoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe);
	add_lock_prefix_table();
	add_chpe_tables();
	if (is_x64())
		instance.get_file_header().set_machine_type(core::file_header::machine_type::amd64);
	else
		instance.get_file_header().set_machine_type(core::file_header::machine_type::chpe_x86);
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_chpe(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
		validate_size_and_version(dir, load_config::version::hybrid_pe,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe);
	});
}

TEST_P(LoadConfigLoaderTestFixture, LoadCHPELoadConfigDirectoryLimit)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe);
	add_lock_prefix_table();
	add_chpe_tables();
	if (is_x64())
		instance.get_file_header().set_machine_type(core::file_header::machine_type::amd64);
	else
		instance.get_file_header().set_machine_type(core::file_header::machine_type::chpe_x86);
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_chpe(dir, 2u);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
		validate_size_and_version(dir, load_config::version::hybrid_pe,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe);
	}, { .max_cphe_code_address_range_count = 2u });
}

TEST_P(LoadConfigLoaderTestFixture, LoadCHPELoadConfigDirectorySkip)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe);
	add_lock_prefix_table();
	add_chpe_tables();
	if (is_x64())
		instance.get_file_header().set_machine_type(core::file_header::machine_type::amd64);
	else
		instance.get_file_header().set_machine_type(core::file_header::machine_type::chpe_x86);
	with_load_config([this](const auto& dir) {
		EXPECT_TRUE(std::holds_alternative<std::monostate>(dir.get_chpe_metadata()));
	}, { .load_chpe_metadata = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadDynRelocV2LoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard);
	add_lock_prefix_table();
	add_dynamic_reloc_v2();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_dynamic_relocations_v2(dir.get_dynamic_relocation_table());
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
		validate_size_and_version(dir, load_config::version::rf_guard,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe, rf_guard);
	}, { .load_chpe_metadata = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadDynRelocLoadConfigDirectorySkip)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard);
	add_lock_prefix_table();
	add_dynamic_reloc_v2();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		EXPECT_FALSE(dir.get_dynamic_relocation_table());
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
	}, { .load_chpe_metadata = false, .load_dynamic_relocation_table = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadDynRelocV1LoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard);
	add_lock_prefix_table();
	add_dynamic_reloc_v1();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_dynamic_relocations_v1(dir.get_dynamic_relocation_table());
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
		validate_size_and_version(dir, load_config::version::rf_guard,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe, rf_guard);
	}, { .load_chpe_metadata = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadDynRelocUnknownVerLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard);
	add_lock_prefix_table();
	add_dynamic_reloc_v0();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		if (is_x64())
		{
			ASSERT_TRUE(dir.get_dynamic_relocation_table());
			const auto& table = *dir.get_dynamic_relocation_table();
			EXPECT_TRUE(std::holds_alternative<std::monostate>(table.get_relocations()));
			expect_contains_errors(table,
				load_config::load_config_directory_loader_errc::unknown_dynamic_relocation_table_version);
		}
		else
		{
			EXPECT_FALSE(dir.get_dynamic_relocation_table());
		}
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
	}, { .load_chpe_metadata = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadEnclaveLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave);
	add_lock_prefix_table();
	add_enclave();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_enclave(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
		validate_size_and_version(dir, load_config::version::enclave,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave);
	}, { .load_chpe_metadata = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadEnclaveLoadConfigDirectoryLimit)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave);
	add_lock_prefix_table();
	add_enclave();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_enclave(dir, 1u);
	}, { .load_chpe_metadata = false, .max_enclave_number_of_imports = 1u });
}

TEST_P(LoadConfigLoaderTestFixture, LoadEnclaveLoadConfigDirectorySkip)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave);
	add_lock_prefix_table();
	add_enclave();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		EXPECT_FALSE(dir.get_enclave_config());
	}, { .load_chpe_metadata = false, .load_enclave_config = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadVolatileMetadataLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
		volatile_metadata);
	add_lock_prefix_table();
	add_volatile_metadata();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_volatile_metadata(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
		validate_size_and_version(dir, load_config::version::volatile_metadata,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
			volatile_metadata);
	}, { .load_chpe_metadata = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadVolatileMetadataLoadConfigDirectorySkip)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
		volatile_metadata);
	add_lock_prefix_table();
	add_volatile_metadata();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		EXPECT_FALSE(dir.get_volatile_metadata());
	}, { .load_chpe_metadata = false, .load_volatile_metadata = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadVolatileMetadataLoadConfigLimit)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
		volatile_metadata);
	add_lock_prefix_table();
	add_volatile_metadata();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_volatile_metadata_limited(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
		validate_size_and_version(dir, load_config::version::volatile_metadata,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
			volatile_metadata);
	}, { .load_chpe_metadata = false,
		.max_volatile_metadata_access_entries = 1u,
		.max_volatile_metadata_info_range_entries = 0u });
}

TEST_P(LoadConfigLoaderTestFixture, LoadEhContTargetsLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
		volatile_metadata, ehcont_targets);
	add_lock_prefix_table();
	add_ehcont_targets();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_ehcont_targets(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_ehcont_target_rvas);
		validate_size_and_version(dir, load_config::version::eh_guard,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
			volatile_metadata, ehcont_targets);
	}, { .load_chpe_metadata = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadEhContTargetsLoadConfigDirectorySkip)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
		volatile_metadata, ehcont_targets);
	add_lock_prefix_table();
	add_ehcont_targets();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		EXPECT_FALSE(dir.get_eh_continuation_targets());
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va);
	}, { .load_chpe_metadata = false, .load_ehcont_targets = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadEhContTargetsLoadConfigDirectoryLimit)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
		volatile_metadata, ehcont_targets);
	add_lock_prefix_table();
	add_ehcont_targets();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		validate_ehcont_targets_limited(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_ehcont_target_rvas,
			load_config::load_config_directory_loader_errc::invalid_ehcont_targets_count);
		validate_size_and_version(dir, load_config::version::eh_guard,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
			volatile_metadata, ehcont_targets);
	}, { .load_chpe_metadata = false, .max_ehcont_targets = 1u });
}

TEST_P(LoadConfigLoaderTestFixture, LoadXfGuardLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
		volatile_metadata, ehcont_targets, xf_guard);
	add_lock_prefix_table();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_guard_xfg_check_function_pointer_va,
			load_config::load_config_directory_loader_errc::invalid_guard_xfg_dispatch_function_pointer_va,
			load_config::load_config_directory_loader_errc::invalid_guard_xfg_table_dispatch_function_pointer_va);
		validate_size_and_version(dir, load_config::version::xf_guard,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
			volatile_metadata, ehcont_targets, xf_guard);
	}, { .load_chpe_metadata = false, .load_ehcont_targets = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadCastGuardLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
		volatile_metadata, ehcont_targets, xf_guard, cast_guard);
	add_lock_prefix_table();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_guard_xfg_check_function_pointer_va,
			load_config::load_config_directory_loader_errc::invalid_guard_xfg_dispatch_function_pointer_va,
			load_config::load_config_directory_loader_errc::invalid_guard_xfg_table_dispatch_function_pointer_va,
			load_config::load_config_directory_loader_errc::invalid_cast_guard_os_determined_failure_mode_va);
		validate_size_and_version(dir, load_config::version::cast_guard,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
			volatile_metadata, ehcont_targets, xf_guard, cast_guard);
	}, { .load_chpe_metadata = false, .load_ehcont_targets = false });
}

TEST_P(LoadConfigLoaderTestFixture, LoadMemcpyGuardLoadConfigDirectory)
{
	add_load_config_directory();
	add_directory_parts(load_config_base, se_handler_table, cf_guard,
		code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
		volatile_metadata, ehcont_targets, xf_guard, cast_guard, memcpy_guard);
	add_lock_prefix_table();
	with_load_config([this](const auto& dir) {
		validate_base(dir);
		expect_contains_errors(dir,
			load_config::load_config_directory_loader_errc::invalid_security_cookie_va,
			load_config::load_config_directory_loader_errc::invalid_guard_xfg_check_function_pointer_va,
			load_config::load_config_directory_loader_errc::invalid_guard_xfg_dispatch_function_pointer_va,
			load_config::load_config_directory_loader_errc::invalid_guard_xfg_table_dispatch_function_pointer_va,
			load_config::load_config_directory_loader_errc::invalid_cast_guard_os_determined_failure_mode_va,
			load_config::load_config_directory_loader_errc::invalid_guard_memcpy_function_pointer_va);
		validate_size_and_version(dir, load_config::version::memcpy_guard,
			load_config_base, se_handler_table, cf_guard,
			code_integrity, cf_guard_ex, hybrid_pe, rf_guard, rf_guard_ex, enclave,
			volatile_metadata, ehcont_targets, xf_guard, cast_guard, memcpy_guard);
	}, { .load_chpe_metadata = false, .load_ehcont_targets = false });
}

INSTANTIATE_TEST_SUITE_P(
	LoadConfigLoaderTests,
	LoadConfigLoaderTestFixture,
	::testing::Values(
		core::optional_header::magic::pe32,
		core::optional_header::magic::pe64
	));

namespace
{
struct LoadConfigLoaderTestFixtureImpl final : LoadConfigLoaderTestFixture
{
	using LoadConfigLoaderTestFixture::LoadConfigLoaderTestFixture;
	void TestBody() override {}
};
} //namespace

pe_bliss::image::image create_hybrid_exception_load_config_image()
{
	LoadConfigLoaderTestFixtureImpl fixture(core::optional_header::magic::pe64);
	fixture.add_load_config_directory();
	fixture.add_directory_parts(fixture.load_config_base,
		fixture.se_handler_table, fixture.cf_guard,
		fixture.code_integrity, fixture.cf_guard_ex,
		fixture.hybrid_pe);
	fixture.add_chpe_tables();
	return std::move(fixture.instance);
}
