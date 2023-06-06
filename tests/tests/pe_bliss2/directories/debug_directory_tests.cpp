#include "pe_bliss2/debug/debug_directory.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>

#include "gtest/gtest.h"

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_memory_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/detail/debug/image_debug_directory.h"
#include "pe_bliss2/packed_c_string.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::debug;

TEST(DebugDirectoryTest, CoffSymbolType)
{
	coff_symbol sym;
	sym.get_descriptor()->type
		= pe_bliss::detail::debug::coff_type_union;
	EXPECT_EQ(sym.get_type(), coff_symbol_type::sym_union);
	EXPECT_EQ(sym.get_type_attr(), coff_symbol_type_attr::no_derived);

	sym.get_descriptor()->type |= pe_bliss::detail::debug::coff_type_pointer;
	EXPECT_EQ(sym.get_type(), coff_symbol_type::sym_union);
	EXPECT_EQ(sym.get_type_attr(), coff_symbol_type_attr::pointer);

	sym.get_descriptor()->type = pe_bliss::detail::debug::coff_type_long_double;
	EXPECT_EQ(sym.get_type(), coff_symbol_type::sym_long_double);
	EXPECT_EQ(sym.get_type_attr(), coff_symbol_type_attr::no_derived);
}

TEST(DebugDirectoryTest, FpoEntryPrologByteCount)
{
	fpo_entry entry;
	entry.get_descriptor()->flags = 0xcabu;
	EXPECT_EQ(entry.get_prolog_byte_count(), 0xabu);
}

TEST(DebugDirectoryTest, FpoEntrySavedRegs)
{
	fpo_entry entry;
	entry.get_descriptor()->flags = 0xeabu;
	EXPECT_EQ(entry.get_saved_regs(), 6u);
}

TEST(DebugDirectoryTest, FpoEntryIsSehInFunc)
{
	fpo_entry entry;
	EXPECT_FALSE(entry.is_seh_in_func());
	entry.get_descriptor()->flags = 0xcabu;
	EXPECT_TRUE(entry.is_seh_in_func());
}

TEST(DebugDirectoryTest, FpoEntryIsEbpAllocated)
{
	fpo_entry entry;
	EXPECT_FALSE(entry.is_ebp_allocated());
	entry.get_descriptor()->flags = 0x1000u;
	EXPECT_TRUE(entry.is_ebp_allocated());
}

TEST(DebugDirectoryTest, FpoEntryFrameType)
{
	fpo_entry entry;
	EXPECT_EQ(entry.get_frame_type(), fpo_frame_type::fpo);
	entry.get_descriptor()->flags = 0xc000u;
	EXPECT_EQ(entry.get_frame_type(), fpo_frame_type::nonfpo);
}

namespace
{
template<auto Parser, typename Arr, typename Options = bool>
auto parse_debug_directory(const Arr& data, const Options& options = {})
{
	auto buf = std::make_shared<buffers::input_memory_buffer>(
		data.data(), data.size());
	buffers::input_buffer_stateful_wrapper wrapper(buf);
	return Parser(wrapper, options);
}

template<auto Parser, typename Arr, typename Options = bool>
auto parse_virtual_debug_directory(const Arr& data, std::size_t virtual_size,
	const Options& options)
{
	auto buf = std::make_shared<buffers::input_memory_buffer>(
		data.data(), data.size() - virtual_size);
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(
		std::move(buf), virtual_size);
	buffers::input_buffer_stateful_wrapper wrapper(virtual_buf);
	return Parser(wrapper, options);
}

constexpr std::array misc_directory_ascii{
	std::byte{1}, std::byte{}, std::byte{}, std::byte{}, //data_type
	std::byte{18u}, std::byte{}, std::byte{}, std::byte{}, //length
	std::byte{}, //unicode
	std::byte{}, std::byte{}, std::byte{}, //reserved
	std::byte{'a'}, std::byte{'b'}, std::byte{'c'},
	std::byte{}, std::byte{}, std::byte{}
};

constexpr std::array misc_directory_unicode{
	std::byte{1}, std::byte{}, std::byte{}, std::byte{}, //data_type
	std::byte{20u}, std::byte{}, std::byte{}, std::byte{}, //length
	std::byte{1}, //unicode
	std::byte{}, std::byte{}, std::byte{}, //reserved
	std::byte{'a'}, std::byte{}, std::byte{'b'},
	std::byte{}, std::byte{'c'}, std::byte{},
	std::byte{}, std::byte{}
};
} //namespace

TEST(DebugDirectoryTest, ParseMiscAscii)
{
	auto dir = parse_debug_directory<parse_misc_directory>(misc_directory_ascii);
	expect_contains_errors(dir, debug_directory_errc::unaligned_data_size);
	EXPECT_EQ(dir.get_data_type(), misc_data_type::exename);
	const auto* data = std::get_if<pe_bliss::packed_c_string>(&dir.get_data());
	ASSERT_NE(data, nullptr);
	EXPECT_EQ(data->value(), "abc");
	EXPECT_FALSE(data->is_virtual());
}

TEST(DebugDirectoryTest, ParseMiscUnicode)
{
	auto dir = parse_debug_directory<parse_misc_directory>(misc_directory_unicode);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_data_type(), misc_data_type::exename);
	const auto* data = std::get_if<pe_bliss::packed_utf16_c_string>(&dir.get_data());
	ASSERT_NE(data, nullptr);
	EXPECT_EQ(data->value(), u"abc");
	EXPECT_FALSE(data->is_virtual());
}

TEST(DebugDirectoryTest, ParseMiscAsciiVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_misc_directory>(
		misc_directory_ascii, 3u, false);
	expect_contains_errors(dir, debug_directory_errc::unable_to_deserialize_name,
		debug_directory_errc::unaligned_data_size);
	EXPECT_EQ(dir.get_data_type(), misc_data_type::exename);
}

TEST(DebugDirectoryTest, ParseMiscAsciiVirtual)
{
	auto dir = parse_virtual_debug_directory<parse_misc_directory>(
		misc_directory_ascii, 3u, true);
	expect_contains_errors(dir, debug_directory_errc::unaligned_data_size);
	EXPECT_EQ(dir.get_data_type(), misc_data_type::exename);
	const auto* data = std::get_if<pe_bliss::packed_c_string>(&dir.get_data());
	ASSERT_NE(data, nullptr);
	EXPECT_EQ(data->value(), "abc");
	EXPECT_TRUE(data->is_virtual());
}

namespace
{
constexpr std::array spgo_directory{
	std::byte{'a'}, std::byte{}, std::byte{'b'}, std::byte{},
	std::byte{'c'}, std::byte{}, std::byte{}, std::byte{},
};
} //namespace

TEST(DebugDirectoryTest, ParseSpgo)
{
	auto dir = parse_debug_directory<parse_spgo_directory>(spgo_directory);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_string().value(), u"abc");
}

TEST(DebugDirectoryTest, ParseSpgoVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_spgo_directory>(
		spgo_directory, 2u, false);
	expect_contains_errors(dir, debug_directory_errc::unable_to_deserialize);
}

TEST(DebugDirectoryTest, ParseSpgoVirtual)
{
	auto dir = parse_virtual_debug_directory<parse_spgo_directory>(
		spgo_directory, 2u, true);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_string().value(), u"abc");
}

namespace
{
constexpr std::array mpx_directory{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //signature
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //unknown1
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //flags
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //unknown2
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //unknown3
};
} //namespace

TEST(DebugDirectoryTest, ParseMpx)
{
	auto dir = parse_debug_directory<parse_mpx_debug_directory>(mpx_directory);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_descriptor()->signature, 0x04030201u);
}

TEST(DebugDirectoryTest, ParseMpxVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_mpx_debug_directory>(
		mpx_directory, 2u, false);
	expect_contains_errors(dir, debug_directory_errc::unable_to_deserialize);
}

TEST(DebugDirectoryTest, ParseMpxVirtual)
{
	auto dir = parse_virtual_debug_directory<parse_mpx_debug_directory>(
		mpx_directory, 2u, true);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_descriptor()->signature, 0x04030201u);
}

namespace
{
constexpr std::array vc_feature_directory{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //pre_vc_plus_plus_11_count
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //c_and_c_plus_plus_count
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //gs_count
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //sdl_count
	std::byte{5}, std::byte{}, std::byte{}, std::byte{}, //guard_n_count
};
} //namespace

TEST(DebugDirectoryTest, ParseVcFeature)
{
	auto dir = parse_debug_directory<parse_vc_feature_debug_directory>(vc_feature_directory);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_descriptor()->guard_n_count, 5u);
}

TEST(DebugDirectoryTest, ParseVcFeatureVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_vc_feature_debug_directory>(
		vc_feature_directory, 2u, false);
	expect_contains_errors(dir, debug_directory_errc::unable_to_deserialize);
}

TEST(DebugDirectoryTest, ParseVcFeatureVirtual)
{
	auto dir = parse_virtual_debug_directory<parse_vc_feature_debug_directory>(
		vc_feature_directory, 2u, true);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_descriptor()->guard_n_count, 5u);
}

namespace
{
constexpr std::array ex_dllcharacteristics_directory{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{}, //flags
};
} //namespace

TEST(DebugDirectoryTest, ParseExDllCharacteristics)
{
	auto dir = parse_debug_directory<parse_ex_dllcharacteristics_debug_directory>(
		ex_dllcharacteristics_directory);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_descriptor()->flags, 0x030201u);
}

TEST(DebugDirectoryTest, ParseExDllCharacteristicsVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_ex_dllcharacteristics_debug_directory>(
		ex_dllcharacteristics_directory, 1u, false);
	expect_contains_errors(dir, debug_directory_errc::unable_to_deserialize);
}

TEST(DebugDirectoryTest, ParseExDllCharacteristicsVirtual)
{
	auto dir = parse_virtual_debug_directory<parse_ex_dllcharacteristics_debug_directory>(
		ex_dllcharacteristics_directory, 1u, true);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_descriptor()->flags, 0x030201u);
}

namespace
{
constexpr std::array omap_src_directory{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //rva
	std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, //rva_to
	std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, //rva
	std::byte{6}, std::byte{7}, std::byte{8}, std::byte{9}, //rva_to
};

template<typename T>
class OmapSrcTest : public testing::Test
{
public:
	static constexpr auto parser = T::parser;
};

template<auto Parser>
struct debug_info_parser
{
	static constexpr auto parser = Parser;
};

using omap_src_tested_types = ::testing::Types<
	debug_info_parser<parse_omap_to_src_directory>,
	debug_info_parser<parse_src_to_omap_directory>>;
} //namespace

TYPED_TEST_SUITE(OmapSrcTest, omap_src_tested_types);

TYPED_TEST(OmapSrcTest, ParseOmapSrc)
{
	auto dir = parse_debug_directory<TestFixture::parser>(omap_src_directory,
		debug_directory_parse_options{});
	expect_contains_errors(dir);
	ASSERT_EQ(dir.get_mappings().size(), 2u);
	EXPECT_EQ(dir.get_mappings()[0]->rva, 0x04030201u);
	EXPECT_EQ(dir.get_mappings()[1]->rva, 0x05040302u);
}

TYPED_TEST(OmapSrcTest, ParseOmapSrcVirtualError)
{
	auto dir = parse_virtual_debug_directory<TestFixture::parser>(omap_src_directory,
		2u, debug_directory_parse_options{ .allow_virtual_data = false });
	expect_contains_errors(dir, debug_directory_errc::unable_to_deserialize);
	ASSERT_EQ(dir.get_mappings().size(), 2u);
	EXPECT_EQ(dir.get_mappings()[0]->rva, 0x04030201u);
}

TYPED_TEST(OmapSrcTest, ParseOmapSrcVirtual)
{
	auto dir = parse_virtual_debug_directory<TestFixture::parser>(omap_src_directory,
		2u, debug_directory_parse_options{ .allow_virtual_data = true });
	expect_contains_errors(dir);
	ASSERT_EQ(dir.get_mappings().size(), 2u);
	EXPECT_EQ(dir.get_mappings()[0]->rva, 0x04030201u);
	EXPECT_EQ(dir.get_mappings()[1]->rva, 0x05040302u);
}

TYPED_TEST(OmapSrcTest, ParseOmapSrcLimit)
{
	auto dir = parse_debug_directory<TestFixture::parser>(omap_src_directory,
		debug_directory_parse_options{ .max_debug_entry_count = 1u });
	expect_contains_errors(dir, debug_directory_errc::too_many_entries);
	ASSERT_EQ(dir.get_mappings().size(), 1u);
	EXPECT_EQ(dir.get_mappings()[0]->rva, 0x04030201u);
}

namespace
{
constexpr std::array fpo_directory{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //ul_off_start
	std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, //cb_proc_size
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //cdw_locals
	std::byte{}, std::byte{}, //cdw_params
	std::byte{9}, std::byte{10}, //flags

	std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, //ul_off_start
	std::byte{6}, std::byte{7}, std::byte{8}, std::byte{9}, //cb_proc_size
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //cdw_locals
	std::byte{}, std::byte{}, //cdw_params
	std::byte{10}, std::byte{11} //flags
};
} //namespace

TEST(DebugDirectoryTest, ParseFpo)
{
	auto dir = parse_debug_directory<parse_fpo_directory>(fpo_directory,
		debug_directory_parse_options{});
	expect_contains_errors(dir);
	ASSERT_EQ(dir.get_entries().size(), 2u);
	EXPECT_EQ(dir.get_entries()[0].get_descriptor()->cb_proc_size, 0x08070605u);
	EXPECT_EQ(dir.get_entries()[1].get_descriptor()->cb_proc_size, 0x09080706u);
}

TEST(DebugDirectoryTest, ParseFpoVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_fpo_directory>(fpo_directory,
		2u, debug_directory_parse_options{ .allow_virtual_data = false });
	expect_contains_errors(dir, debug_directory_errc::unable_to_deserialize);
	ASSERT_EQ(dir.get_entries().size(), 2u);
	EXPECT_EQ(dir.get_entries()[0].get_descriptor()->cb_proc_size, 0x08070605u);
}

TEST(DebugDirectoryTest, ParseFpoVirtual)
{
	auto dir = parse_virtual_debug_directory<parse_fpo_directory>(fpo_directory,
		2u, debug_directory_parse_options{ .allow_virtual_data = true });
	expect_contains_errors(dir);
	ASSERT_EQ(dir.get_entries().size(), 2u);
	EXPECT_EQ(dir.get_entries()[0].get_descriptor()->cb_proc_size, 0x08070605u);
	EXPECT_EQ(dir.get_entries()[1].get_descriptor()->cb_proc_size, 0x09080706u);
}

TEST(DebugDirectoryTest, ParseFpoLimit)
{
	auto dir = parse_debug_directory<parse_fpo_directory>(fpo_directory,
		debug_directory_parse_options{ .max_debug_entry_count = 1u });
	expect_contains_errors(dir, debug_directory_errc::too_many_entries);
	ASSERT_EQ(dir.get_entries().size(), 1u);
	EXPECT_EQ(dir.get_entries()[0].get_descriptor()->cb_proc_size, 0x08070605u);
}

namespace
{
constexpr std::array pogo_directory{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //signature
	
	std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, //start_rva
	std::byte{6}, std::byte{7}, std::byte{8}, std::byte{9}, //size
	std::byte{'a'}, std::byte{'b'}, std::byte{}, std::byte{},

	std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, //start_rva
	std::byte{7}, std::byte{8}, std::byte{9}, std::byte{10}, //size
	std::byte{'c'}, std::byte{},
};

constexpr std::array pogo_directory_aligned{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //signature

	std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, //start_rva
	std::byte{6}, std::byte{7}, std::byte{8}, std::byte{9}, //size
	std::byte{'a'}, std::byte{'b'}, std::byte{}, std::byte{},
};

void check_pogo_directory(const pogo_debug_directory_details& dir)
{
	EXPECT_EQ(dir.get_descriptor()->signature, 0x04030201u);
	ASSERT_EQ(dir.get_entries().size(), 2u);
	EXPECT_EQ(dir.get_entries()[0].get_descriptor()->size, 0x09080706u);
	EXPECT_EQ(dir.get_entries()[0].get_name().value(), "ab");
	EXPECT_FALSE(dir.get_entries()[0].get_name().is_virtual());
	EXPECT_EQ(dir.get_entries()[1].get_descriptor()->size, 0x0a090807u);
}
} //namespace

TEST(DebugDirectoryTest, ParsePogo)
{
	auto dir = parse_debug_directory<parse_pogo_directory>(pogo_directory,
		debug_directory_parse_options{});
	expect_contains_errors(dir);
	check_pogo_directory(dir);
	EXPECT_EQ(dir.get_entries()[1].get_name().value(), "c");
	EXPECT_FALSE(dir.get_entries()[1].get_name().is_virtual());
}

TEST(DebugDirectoryTest, ParsePogoAligned)
{
	auto dir = parse_debug_directory<parse_pogo_directory>(pogo_directory_aligned,
		debug_directory_parse_options{});
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_descriptor()->signature, 0x04030201u);
	ASSERT_EQ(dir.get_entries().size(), 1u);
	EXPECT_EQ(dir.get_entries()[0].get_descriptor()->size, 0x09080706u);
	EXPECT_EQ(dir.get_entries()[0].get_name().value(), "ab");
}

TEST(DebugDirectoryTest, ParsePogoVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_pogo_directory>(pogo_directory,
		1u, debug_directory_parse_options{ .allow_virtual_data = false });
	expect_contains_errors(dir, debug_directory_errc::unable_to_deserialize_name);
	check_pogo_directory(dir);
}

TEST(DebugDirectoryTest, ParsePogoVirtual)
{
	auto dir = parse_virtual_debug_directory<parse_pogo_directory>(pogo_directory,
		1u, debug_directory_parse_options{ .allow_virtual_data = true });
	expect_contains_errors(dir);
	check_pogo_directory(dir);
	EXPECT_EQ(dir.get_entries()[1].get_name().value(), "c");
	EXPECT_TRUE(dir.get_entries()[1].get_name().is_virtual());
}

TEST(DebugDirectoryTest, ParsePogoLimit)
{
	auto dir = parse_debug_directory<parse_pogo_directory>(pogo_directory,
		debug_directory_parse_options{ .max_debug_entry_count = 1u });
	expect_contains_errors(dir, debug_directory_errc::too_many_entries);
	EXPECT_EQ(dir.get_descriptor()->signature, 0x04030201u);
	ASSERT_EQ(dir.get_entries().size(), 1u);
	EXPECT_EQ(dir.get_entries()[0].get_descriptor()->size, 0x09080706u);
	EXPECT_EQ(dir.get_entries()[0].get_name().value(), "ab");
	EXPECT_FALSE(dir.get_entries()[0].get_name().is_virtual());
}

namespace
{
constexpr std::array pdb2_directory{
	//cv_signature
	std::byte{pe_bliss::detail::debug::debug_signature_pdb2 & 0xffu},
	std::byte{(pe_bliss::detail::debug::debug_signature_pdb2 >> 8u) & 0xffu},
	std::byte{(pe_bliss::detail::debug::debug_signature_pdb2 >> 16u) & 0xffu},
	std::byte{(pe_bliss::detail::debug::debug_signature_pdb2 >> 24u) & 0xffu},
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //offset
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //signature
	std::byte{5}, std::byte{}, std::byte{}, std::byte{}, //age
	std::byte{'x'}, std::byte{'y'}, std::byte{'z'}, std::byte{} //pdb_file_name
};

constexpr std::array pdb7_directory{
	//cv_signature
	std::byte{pe_bliss::detail::debug::debug_signature_pdb7 & 0xffu},
	std::byte{(pe_bliss::detail::debug::debug_signature_pdb7 >> 8u) & 0xffu},
	std::byte{(pe_bliss::detail::debug::debug_signature_pdb7 >> 16u) & 0xffu},
	std::byte{(pe_bliss::detail::debug::debug_signature_pdb7 >> 24u) & 0xffu},
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //signature
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{5}, std::byte{}, std::byte{}, std::byte{}, //age
	std::byte{'x'}, std::byte{'y'}, std::byte{'z'}, std::byte{} //pdb_file_name
};

constexpr std::array nb11_directory{
	//cv_signature
	std::byte{pe_bliss::detail::debug::debug_signature_nb11 & 0xffu},
	std::byte{(pe_bliss::detail::debug::debug_signature_nb11 >> 8u) & 0xffu},
	std::byte{(pe_bliss::detail::debug::debug_signature_nb11 >> 16u) & 0xffu},
	std::byte{(pe_bliss::detail::debug::debug_signature_nb11 >> 24u) & 0xffu},
};

constexpr std::array codeview_unknown_directory{
	//cv_signature
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
};
} // namespace

TEST(DebugDirectoryTest, ParsePdb2)
{
	auto dir = parse_debug_directory<parse_codeview_directory>(pdb2_directory);
	const auto* pdb2 = std::get_if<codeview_pdb2_debug_directory_details>(&dir);
	ASSERT_NE(pdb2, nullptr);
	expect_contains_errors(*pdb2);
	EXPECT_EQ(pdb2->get_pdb_file_name().value(), "xyz");
	EXPECT_EQ(pdb2->get_descriptor()->age, 5u);
	EXPECT_FALSE(pdb2->get_pdb_file_name().is_virtual());
}

TEST(DebugDirectoryTest, ParsePdb2VirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_codeview_directory>(
		pdb2_directory, 1u, false);
	const auto* pdb2 = std::get_if<codeview_pdb2_debug_directory_details>(&dir);
	ASSERT_NE(pdb2, nullptr);
	expect_contains_errors(*pdb2, debug_directory_errc::unable_to_deserialize_name);
}

TEST(DebugDirectoryTest, ParsePdb2Virtual)
{
	auto dir = parse_virtual_debug_directory<parse_codeview_directory>(
		pdb2_directory, 1u, true);
	const auto* pdb2 = std::get_if<codeview_pdb2_debug_directory_details>(&dir);
	ASSERT_NE(pdb2, nullptr);
	EXPECT_EQ(pdb2->get_pdb_file_name().value(), "xyz");
	EXPECT_EQ(pdb2->get_descriptor()->age, 5u);
	EXPECT_TRUE(pdb2->get_pdb_file_name().is_virtual());
}

TEST(DebugDirectoryTest, ParsePdb7)
{
	auto dir = parse_debug_directory<parse_codeview_directory>(pdb7_directory);
	const auto* pdb7 = std::get_if<codeview_pdb7_debug_directory_details>(&dir);
	ASSERT_NE(pdb7, nullptr);
	expect_contains_errors(*pdb7);
	EXPECT_EQ(pdb7->get_pdb_file_name().value(), "xyz");
	EXPECT_EQ(pdb7->get_descriptor()->age, 5u);
	EXPECT_FALSE(pdb7->get_pdb_file_name().is_virtual());
}

TEST(DebugDirectoryTest, ParsePdb7VirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_codeview_directory>(
		pdb7_directory, 1u, false);
	const auto* pdb7 = std::get_if<codeview_pdb7_debug_directory_details>(&dir);
	ASSERT_NE(pdb7, nullptr);
	expect_contains_errors(*pdb7, debug_directory_errc::unable_to_deserialize_name);
}

TEST(DebugDirectoryTest, ParsePdb7Virtual)
{
	auto dir = parse_virtual_debug_directory<parse_codeview_directory>(
		pdb7_directory, 1u, true);
	const auto* pdb7 = std::get_if<codeview_pdb7_debug_directory_details>(&dir);
	ASSERT_NE(pdb7, nullptr);
	EXPECT_EQ(pdb7->get_pdb_file_name().value(), "xyz");
	EXPECT_EQ(pdb7->get_descriptor()->age, 5u);
	EXPECT_TRUE(pdb7->get_pdb_file_name().is_virtual());
}

TEST(DebugDirectoryTest, ParseNb11)
{
	auto dir = parse_debug_directory<parse_codeview_directory>(nb11_directory);
	const auto* nb11 = std::get_if<codeview_omf_debug_directory_details>(&dir);
	ASSERT_NE(nb11, nullptr);
	expect_contains_errors(*nb11);
	EXPECT_EQ(nb11->get_signature().get(), pe_bliss::detail::debug::debug_signature_nb11);
}

TEST(DebugDirectoryTest, ParseCodeviewUnknown)
{
	auto dir = parse_debug_directory<parse_codeview_directory>(codeview_unknown_directory);
	const auto* unknown_dir = std::get_if<codeview_omf_debug_directory_details>(&dir);
	ASSERT_NE(unknown_dir, nullptr);
	expect_contains_errors(*unknown_dir);
}

TEST(DebugDirectoryTest, ParseCodeviewUnknownVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_codeview_directory>(
		codeview_unknown_directory, 1u, false);
	const auto* unknown_dir = std::get_if<codeview_omf_debug_directory_details>(&dir);
	ASSERT_NE(unknown_dir, nullptr);
	expect_contains_errors(*unknown_dir, debug_directory_errc::unable_to_deserialize);
}

namespace
{
constexpr std::uint8_t number_of_coff_symbols = 5u;
constexpr std::uint8_t lva_to_first_symbol = 34u;
constexpr std::uint8_t coff_string_table_length = 12u;
constexpr std::array coff_directory{
	std::byte{number_of_coff_symbols}, std::byte{}, std::byte{}, std::byte{}, //number_of_symbols
	std::byte{lva_to_first_symbol}, std::byte{}, std::byte{}, std::byte{}, //lva_to_first_symbol
	std::byte{5}, std::byte{}, std::byte{}, std::byte{}, //number_of_linenumbers
	std::byte{100}, std::byte{}, std::byte{}, std::byte{}, //lva_to_first_linenumber
	std::byte{1}, std::byte{}, std::byte{}, std::byte{}, //rva_to_first_byte_of_code
	std::byte{2}, std::byte{}, std::byte{}, std::byte{}, //rva_to_last_byte_of_code
	std::byte{3}, std::byte{}, std::byte{}, std::byte{}, //rva_to_first_byte_of_data
	std::byte{4}, std::byte{}, std::byte{}, std::byte{}, //rva_to_last_byte_of_data
	std::byte{}, std::byte{}, //unused bytes
	//symbol 1
	std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, std::byte{'d'}, //name
	std::byte{'e'}, std::byte{'f'}, std::byte{'g'}, std::byte{'h'},
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //value
	std::byte{}, std::byte{}, //section_number
	std::byte{}, std::byte{}, //type
	std::byte{}, //storage_class
	std::byte{2}, //aux_symbol_number
	// aux 1
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{},
	// aux 2
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	std::byte{}, std::byte{},
	//symbol 2
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //e_zeroes
	std::byte{5}, std::byte{}, std::byte{}, std::byte{}, //e_offset
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //value
	std::byte{}, std::byte{}, //section_number
	std::byte{}, std::byte{}, //type
	std::byte{}, //storage_class
	std::byte{}, //aux_symbol_number
	//symbol 3
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //e_zeroes
	std::byte{9}, std::byte{}, std::byte{}, std::byte{}, //e_offset
	std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, //value
	std::byte{}, std::byte{}, //section_number
	std::byte{}, std::byte{}, //type
	std::byte{}, //storage_class
	std::byte{}, //aux_symbol_number
	// string table
	std::byte{coff_string_table_length}, std::byte{}, std::byte{}, std::byte{}, //size
	std::byte{}, std::byte{'a'}, std::byte{'b'}, std::byte{'c'},
	std::byte{}, std::byte{'x'}, std::byte{'y'}, std::byte{}
};

void validate_coff_directory(coff_debug_directory_details& dir,
	std::size_t symbol_count, bool string_table_copied, bool virtual_string_table = false)
{
	EXPECT_EQ(dir.get_descriptor()->lva_to_first_symbol, lva_to_first_symbol);
	EXPECT_EQ(dir.get_string_table_length().get(), coff_string_table_length);
	EXPECT_EQ(dir.get_string_table_buffer().size(), coff_string_table_length);
	EXPECT_EQ(dir.get_string_table_buffer().is_copied(), string_table_copied);
	EXPECT_EQ(dir.get_string_table_buffer().copied_data(), std::vector(
		coff_directory.end() - coff_string_table_length, coff_directory.end()
		- virtual_string_table
	));

	ASSERT_EQ(dir.get_symbols().size(), symbol_count);
	if (symbol_count)
	{
		EXPECT_EQ(dir.get_symbols()[0].get_name(),
			coff_symbol::name_type(std::string("abcdefgh")));
		EXPECT_EQ(dir.get_symbols()[0].get_descriptor()->aux_symbol_number, 2u);
	}
	if (symbol_count > 1)
	{
		EXPECT_EQ(dir.get_symbols()[1].get_name(),
			coff_symbol::name_type(pe_bliss::packed_c_string("abc")));
		EXPECT_EQ(dir.get_symbols()[1].get_descriptor()->value, 0x04030201u);
	}
	if (symbol_count > 2)
	{
		if (!virtual_string_table)
		{
			auto name = dir.get_symbols()[2].get_name();
			const auto* str = std::get_if<pe_bliss::packed_c_string>(&name);
			ASSERT_NE(str, nullptr);
			EXPECT_FALSE(str->is_virtual());
			EXPECT_EQ(dir.get_symbols()[2].get_name(),
				coff_symbol::name_type(pe_bliss::packed_c_string("xy")));
		}
		EXPECT_EQ(dir.get_symbols()[2].get_descriptor()->value, 0x05040302u);
	}
}
} // namespace

TEST(DebugDirectoryTest, ParseCoff)
{
	auto dir = parse_debug_directory<parse_coff_directory>(coff_directory,
		debug_directory_parse_options{});
	expect_contains_errors(dir);
	validate_coff_directory(dir, 3u, false);
}

TEST(DebugDirectoryTest, ParseCoffLimitSymbols)
{
	auto dir = parse_debug_directory<parse_coff_directory>(coff_directory,
		debug_directory_parse_options{ .max_coff_symbol_count = 1u });
	expect_contains_errors(dir, debug_directory_errc::too_many_symbols);
	validate_coff_directory(dir, 1u, false);
}

TEST(DebugDirectoryTest, ParseCoffCopyStringTable)
{
	auto dir = parse_debug_directory<parse_coff_directory>(coff_directory,
		debug_directory_parse_options{ .copy_coff_string_table_memory = true });
	expect_contains_errors(dir);
	validate_coff_directory(dir, 3u, true);
}

TEST(DebugDirectoryTest, ParseCoffVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_coff_directory>(coff_directory,
		1u, debug_directory_parse_options{ .allow_virtual_data = false });
	expect_contains_errors(dir, debug_directory_errc::unable_to_deserialize_name,
		debug_directory_errc::virtual_coff_string_table);
	validate_coff_directory(dir, 3u, false, true);
}

TEST(DebugDirectoryTest, ParseCoffVirtual)
{
	auto dir = parse_virtual_debug_directory<parse_coff_directory>(coff_directory,
		1u, debug_directory_parse_options{ .allow_virtual_data = true });
	expect_contains_errors(dir);
	validate_coff_directory(dir, 3u, false, true);
	EXPECT_EQ(dir.get_symbols()[2].get_name(),
		coff_symbol::name_type(pe_bliss::packed_c_string("xy")));
	auto name = dir.get_symbols()[2].get_name();
	const auto* str = std::get_if<pe_bliss::packed_c_string>(&name);
	ASSERT_NE(str, nullptr);
	EXPECT_TRUE(str->is_virtual());
}

TEST(DebugDirectoryTest, ParseCoffValidateFileHeader)
{
	pe_bliss::core::file_header fh;
	fh.get_descriptor()->number_of_symbols = number_of_coff_symbols;
	fh.get_descriptor()->pointer_to_symbol_table = lva_to_first_symbol;
	auto dir = parse_debug_directory<parse_coff_directory>(coff_directory,
		debug_directory_parse_options{ .file_header = &fh });
	expect_contains_errors(dir);
	validate_coff_directory(dir, 3u, false);
}

TEST(DebugDirectoryTest, ParseCoffValidateFileHeaderNumberOfSymbolsError)
{
	pe_bliss::core::file_header fh;
	fh.get_descriptor()->number_of_symbols = number_of_coff_symbols + 1u;
	fh.get_descriptor()->pointer_to_symbol_table = lva_to_first_symbol;
	auto dir = parse_debug_directory<parse_coff_directory>(coff_directory,
		debug_directory_parse_options{ .file_header = &fh });
	expect_contains_errors(dir, debug_directory_errc::number_of_symbols_mismatch);
	validate_coff_directory(dir, 3u, false);
}

TEST(DebugDirectoryTest, ParseCoffValidateFileHeaderPointerToSymbolTableError)
{
	pe_bliss::core::file_header fh;
	fh.get_descriptor()->number_of_symbols = number_of_coff_symbols;
	fh.get_descriptor()->pointer_to_symbol_table = lva_to_first_symbol + 1u;
	auto dir = parse_debug_directory<parse_coff_directory>(coff_directory,
		debug_directory_parse_options{ .file_header = &fh });
	expect_contains_errors(dir, debug_directory_errc::pointer_to_symbol_table_mismatch);
	validate_coff_directory(dir, 3u, false);
}

namespace
{
template<typename T>
void check_debug_type(std::uint32_t type)
{
	debug_directory dir;
	dir.get_descriptor()->type = type;
	auto underlying = dir.get_underlying_directory();
	EXPECT_TRUE(std::holds_alternative<T>(underlying)) << type;
}
} //namespace

TEST(DebugDirectoryTest, DebugDirectory)
{
	using namespace pe_bliss::detail::debug::image_debug_type;
	check_debug_type<misc_debug_directory_details>(misc);
	check_debug_type<iltcg_debug_directory>(iltcg);
	check_debug_type<vc_feature_debug_directory_details>(vc_feature);
	check_debug_type<mpx_debug_directory_details>(mpx);
	check_debug_type<ex_dllcharacteristics_debug_directory_details>(
		ex_dllcharacteristics);
	check_debug_type<repro_debug_directory_details>(repro);
	check_debug_type<spgo_debug_directory_details>(spgo);
	check_debug_type<codeview_omf_debug_directory_details>(codeview);
	check_debug_type<omap_to_src_debug_directory_details>(omap_to_src);
	check_debug_type<src_to_omap_debug_directory_details>(omap_from_src);
	check_debug_type<fpo_debug_directory_details>(fpo);
	check_debug_type<pogo_debug_directory_details>(pogo);
	check_debug_type<coff_debug_directory_details>(coff);
}

TEST(DebugDirectoryTest, UnsupportedDebugDirectory)
{
	debug_directory dir;
	dir.get_descriptor()->type = 12345678u;
	expect_throw_pe_error([&dir]() {
		(void)dir.get_underlying_directory();
	}, debug_directory_errc::unsupported_type);
}

namespace
{
constexpr std::array pdb_hash_directory{
	std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, std::byte{}, //algorithm
	std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, std::byte{0} //hash
};
} //namespace

TEST(DebugDirectoryTest, ParsePdbHash)
{
	auto dir = parse_debug_directory<parse_pdbhash_directory>(
		pdb_hash_directory);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_algorithm().value(), "abc");
	EXPECT_EQ(dir.get_hash().value(), std::vector(
		pdb_hash_directory.end() - 5, pdb_hash_directory.end()));
	EXPECT_EQ(dir.get_hash().physical_size(), 5u);
}

TEST(DebugDirectoryTest, ParsePdbHashVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_pdbhash_directory>(
		pdb_hash_directory, 1u, false);
	EXPECT_EQ(dir.get_algorithm().value(), "abc");
	expect_contains_errors(dir, debug_directory_errc::unable_to_deserialize_hash);
}

TEST(DebugDirectoryTest, ParsePdbHashVirtual)
{
	auto dir = parse_virtual_debug_directory<parse_pdbhash_directory>(
		pdb_hash_directory, 1u, true);
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_algorithm().value(), "abc");
	EXPECT_EQ(dir.get_hash().value(), std::vector(
		pdb_hash_directory.end() - 5, pdb_hash_directory.end() - 1));
	EXPECT_EQ(dir.get_hash().data_size(), 5u);
	EXPECT_EQ(dir.get_hash().physical_size(), 4u);
}

namespace
{
constexpr std::array mpdb_directory{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //signature
	std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, //uncompressed_size
	std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, //compressed data
};
} //namespace

TEST(DebugDirectoryTest, ParseMpdb)
{
	auto dir = parse_debug_directory<parse_mpdb_directory>(
		mpdb_directory, debug_directory_parse_options{});
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_descriptor()->signature, 0x04030201u);
	EXPECT_FALSE(dir.get_compressed_pdb().is_copied());
	EXPECT_EQ(dir.get_compressed_pdb().copied_data(), std::vector(
		mpdb_directory.end() - 3, mpdb_directory.end()));
}

TEST(DebugDirectoryTest, ParseMpdbCopy)
{
	auto dir = parse_debug_directory<parse_mpdb_directory>(
		mpdb_directory, debug_directory_parse_options{ .copy_mpdb_memory = true });
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_descriptor()->signature, 0x04030201u);
	EXPECT_TRUE(dir.get_compressed_pdb().is_copied());
	EXPECT_EQ(dir.get_compressed_pdb().copied_data(), std::vector(
		mpdb_directory.end() - 3, mpdb_directory.end()));
}

TEST(DebugDirectoryTest, ParseMpdbVirtualError)
{
	auto dir = parse_virtual_debug_directory<parse_mpdb_directory>(
		mpdb_directory, 1u, debug_directory_parse_options{ .allow_virtual_data = false });
	expect_contains_errors(dir, debug_directory_errc::virtual_pdb_data);
	EXPECT_EQ(dir.get_descriptor()->signature, 0x04030201u);
	EXPECT_FALSE(dir.get_compressed_pdb().is_copied());
	EXPECT_EQ(dir.get_compressed_pdb().copied_data(), std::vector(
		mpdb_directory.end() - 3, mpdb_directory.end() - 1));
}

TEST(DebugDirectoryTest, ParseMpdbVirtual)
{
	auto dir = parse_virtual_debug_directory<parse_mpdb_directory>(
		mpdb_directory, 1u, debug_directory_parse_options{ .allow_virtual_data = true });
	expect_contains_errors(dir);
	EXPECT_EQ(dir.get_descriptor()->signature, 0x04030201u);
	EXPECT_FALSE(dir.get_compressed_pdb().is_copied());
	EXPECT_EQ(dir.get_compressed_pdb().copied_data(), std::vector(
		mpdb_directory.end() - 3, mpdb_directory.end() - 1));
}
