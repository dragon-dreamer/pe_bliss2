#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/exceptions/x64/x64_exception_directory.h"
#include "pe_bliss2/exceptions/x64/x64_exception_directory_loader.h"
#include "pe_bliss2/exceptions/exception_directory_loader.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/pe_types.h"
#include "tests/pe_bliss2/image_helper.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::exceptions::x64;

namespace
{
class X64ExceptionLoaderTestFixture : public ::testing::Test
{
public:
	X64ExceptionLoaderTestFixture()
		: instance(create_test_image({ .is_x64 = true,
			.start_section_rva = section_rva,
			.sections = { { section_virtual_size, section_raw_size } } }))
	{
		instance.get_file_header().set_machine_type(
			core::file_header::machine_type::amd64);
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
	}

	void add_exception_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::exception).get()
			= { .virtual_address = directory_rva, .size = directory_size };
	}

	void add_exception_dir_to_headers()
	{
		instance.get_full_headers_buffer().copied_data().resize(0x1000);
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::exception).get()
			= { .virtual_address = 0x20u, .size = 0x200u };
	}

	template<typename Array>
	void add_data(rva_type rva, const Array& arr)
	{
		auto* data = instance.get_section_data_list()[0].copied_data().data()
			+ (directory_rva - section_rva)
			+ (rva - section_rva);
		std::copy(arr.begin(), arr.end(), data);
	}

	void add_exception_dir_descriptors()
	{
		add_data(section_rva, exception_dir);
		add_data(unwind_info0_rva, unwind_info0);
		add_data(unwind_info1_rva, unwind_info1);
	}

public:
	pe_bliss::exceptions::exception_directory_details load_dir(
		const loader_options& options = {}) const
	{
		pe_bliss::exceptions::exception_directory_details loaded;
		EXPECT_NO_THROW(pe_bliss::exceptions::x64::load(
			instance, options, loaded));
		return loaded;
	}

	const exception_directory_details* get_x64_dir()
	{
		expect_contains_errors(dir);
		EXPECT_EQ(dir.get_directories().size(), 1u);
		if (dir.get_directories().size() != 1u)
			return nullptr;
		const auto* x64dir = std::get_if<exception_directory_details>(
			&dir.get_directories()[0]);
		EXPECT_NE(x64dir, nullptr);
		return x64dir;
	}

	const exception_directory_details* load_and_get_x64_dir(
		const loader_options& options = {})
	{
		dir = load_dir(options);
		return get_x64_dir();
	}

public:
	static void validate_dir(const exception_directory_details* dir)
	{
		ASSERT_NE(dir, nullptr);
		expect_contains_errors(*dir);
		ASSERT_EQ(dir->get_runtime_function_list().size(),
			number_of_runtime_functions);

		const auto& function0 = dir->get_runtime_function_list()[0];
		expect_contains_errors(function0,
			exception_directory_loader_errc::push_nonvol_uwop_out_of_order);
		const auto& unwind0 = function0.get_unwind_info();
		EXPECT_EQ(unwind0.get_version(), unwind0_version);
		const auto& unwind0_codes = unwind0.get_unwind_code_list();
		ASSERT_EQ(unwind0_codes.size(), 3u);
		const auto* unwind0_code0 = std::get_if<push_nonvol>(&unwind0_codes[0]);
		ASSERT_NE(unwind0_code0, nullptr);
		EXPECT_EQ(unwind0_code0->get_descriptor().get_state().relative_offset(),
			unwind_info0_rva - section_rva + 4 /* size of unwind info header */);
		EXPECT_EQ(unwind0_code0->get_descriptor().get_state().absolute_offset(),
			absolute_offset + unwind_info0_rva - section_rva
			+ 4 /* size of unwind info header */);
		const auto* unwind0_code1 = std::get_if<alloc_large<1>>(&unwind0_codes[1]);
		ASSERT_NE(unwind0_code1, nullptr);
		EXPECT_EQ(unwind0_code1->get_descriptor()->node, unwind0_code1_node);
		const auto* unwind0_code2 = std::get_if<save_nonvol>(&unwind0_codes[2]);
		ASSERT_NE(unwind0_code2, nullptr);
		EXPECT_EQ(unwind0_code2->get_descriptor()->node, unwind0_code2_node);
		const auto* chain = std::get_if<runtime_function_details::runtime_function_ptr>(
			&function0.get_additional_info());
		ASSERT_NE(chain, nullptr);
		ASSERT_NE(chain->get(), nullptr);
		const auto& chain_function = *chain->get();
		expect_contains_errors(chain_function,
			exception_directory_loader_errc::unaligned_unwind_info,
			exception_directory_loader_errc::invalid_unwind_info);

		const auto& function1 = dir->get_runtime_function_list()[1];
		expect_contains_errors(function1,
			exception_directory_loader_errc::invalid_exception_handler_rva);
		const auto& unwind1 = function1.get_unwind_info();
		EXPECT_EQ(unwind1.get_version(), unwind1_version);
		const auto& unwind1_codes = unwind1.get_unwind_code_list();
		ASSERT_EQ(unwind1_codes.size(), 1u);
		const auto* unwind1_code0 = std::get_if<save_xmm128_far>(&unwind1_codes[0]);
		ASSERT_NE(unwind1_code0, nullptr);
		EXPECT_EQ(unwind1_code0->get_descriptor()->node, unwind1_code0_node);
		const auto* handler = std::get_if<runtime_function_details::exception_handler_rva_type>(
			&function1.get_additional_info());
		ASSERT_NE(handler, nullptr);
		EXPECT_EQ(handler->get(), exception_handler_rva_1);
	}

public:
	image::image instance;
	pe_bliss::exceptions::exception_directory_details dir;

public:
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t directory_rva = 0x1000u;
	static constexpr std::uint32_t section_virtual_size = 0x2000u;
	static constexpr std::uint32_t section_raw_size = 0x1000u;

	static constexpr std::uint32_t unwind_info0_rva = section_rva + 0x100u;
	static constexpr std::uint32_t unwind_info1_rva = section_rva + 0x300u;
	static constexpr std::array exception_dir{
		//zero entry test
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //begin_address
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //end_address
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //unwind_info_address

		//entry 0
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //begin_address
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //end_address
		//unwind_info_address
		std::byte{unwind_info0_rva & 0xffu}, std::byte{(unwind_info0_rva >> 8u) & 0xffu},
		std::byte{(unwind_info0_rva >> 16u) & 0xffu}, std::byte{(unwind_info0_rva >> 24u) & 0xffu},

		//entry 1
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //begin_address
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //end_address
		//unwind_info_address
		std::byte{unwind_info1_rva & 0xffu}, std::byte{(unwind_info1_rva >> 8u) & 0xffu},
		std::byte{(unwind_info1_rva >> 16u) & 0xffu}, std::byte{(unwind_info1_rva >> 24u) & 0xffu},
	};

	static constexpr auto directory_size = static_cast<std::uint32_t>(exception_dir.size());

	static constexpr std::uint8_t count_of_unwind_codes_0 = 5u;
	static constexpr std::uint8_t chain_begin_address_0 = 1u;
	static constexpr std::uint8_t chain_end_address_0 = 2u;
	static constexpr std::uint8_t number_of_runtime_functions = 2u;
	static constexpr std::uint8_t unwind0_version = 1u;
	static constexpr std::uint8_t unwind1_version = 2u;
	static constexpr std::uint8_t unwind0_code1_node = 123u;
	static constexpr std::uint8_t unwind0_code2_node = 50u;
	static constexpr std::array unwind_info0{
		std::byte{0b00100'000u | unwind0_version}, //flags_and_version: chaininfo, v1
		std::byte{}, //size_of_prolog
		std::byte{count_of_unwind_codes_0}, //count_of_unwind_codes
		std::byte{}, //frame_register_and_offset

		//opcodes
		//push_nonvol
		std::byte{}, //offset_in_prolog
		std::byte{}, //unwind_operation_code_and_info
		//alloc_large<1>
		std::byte{}, //offset_in_prolog
		std::byte{1}, //unwind_operation_code_and_info
		std::byte{unwind0_code1_node}, std::byte{}, //metadata
		//save_nonvol
		std::byte{}, //offset_in_prolog
		std::byte{4}, //unwind_operation_code_and_info
		std::byte{unwind0_code2_node}, std::byte{}, //metadata

		std::byte{}, std::byte{}, //padding

		//chain
		std::byte{chain_begin_address_0}, std::byte{}, std::byte{}, std::byte{}, //begin_address
		std::byte{chain_end_address_0}, std::byte{}, std::byte{}, std::byte{}, //end_address
		std::byte{0xffu}, std::byte{0xffu}, std::byte{0xffu}, std::byte{0xffu}, //unwind_info_address
	};

	static constexpr std::uint8_t count_of_unwind_codes_1 = 3u;
	static constexpr std::uint32_t exception_handler_rva_1 = 0x12345678u;
	static constexpr std::uint8_t unwind1_code0_node = 100u;
	static constexpr std::array unwind_info1{
		std::byte{0b00001'000u | unwind1_version}, //flags_and_version: ehandler, v2
		std::byte{}, //size_of_prolog
		std::byte{count_of_unwind_codes_1}, //count_of_unwind_codes
		std::byte{}, //frame_register_and_offset

		//opcodes
		//save_xmm128_far
		std::byte{}, //offset_in_prolog
		std::byte{9}, //unwind_operation_code_and_info
		std::byte{unwind1_code0_node}, std::byte{}, //metadata
		std::byte{}, std::byte{}, //metadata

		std::byte{}, std::byte{}, //padding

		//exception handler RVA
		std::byte{exception_handler_rva_1 & 0xffu},
		std::byte{(exception_handler_rva_1 >> 8u) & 0xffu},
		std::byte{(exception_handler_rva_1 >> 16u) & 0xffu},
		std::byte{(exception_handler_rva_1 >> 24u) & 0xffu},
	};
};
} //namespace

TEST_F(X64ExceptionLoaderTestFixture, AbsentDirectory)
{
	auto loaded = load_dir();
	expect_contains_errors(loaded);
	EXPECT_TRUE(loaded.get_directories().empty());
}

TEST_F(X64ExceptionLoaderTestFixture, EmptyDirectory)
{
	add_exception_dir();
	const auto* loaded = load_and_get_x64_dir();
	ASSERT_NE(loaded, nullptr);
	expect_contains_errors(*loaded);
	EXPECT_TRUE(loaded->get_runtime_function_list().empty());
}

TEST_F(X64ExceptionLoaderTestFixture, ValidDirectory)
{
	add_exception_dir();
	add_exception_dir_descriptors();
	validate_dir(load_and_get_x64_dir());
}

TEST_F(X64ExceptionLoaderTestFixture, HeaderDirectoryError)
{
	add_exception_dir_to_headers();
	const auto* loaded = load_and_get_x64_dir({ .include_headers = false });
	ASSERT_NE(loaded, nullptr);
	expect_contains_errors(*loaded,
		exception_directory_loader_errc::unmatched_directory_size);

	ASSERT_EQ(loaded->get_runtime_function_list().size(), 1u);
	const auto& function0 = loaded->get_runtime_function_list()[0];
	expect_contains_errors(function0,
		exception_directory_loader_errc::invalid_runtime_function_entry);
}

TEST_F(X64ExceptionLoaderTestFixture, HeaderDirectory)
{
	add_exception_dir_to_headers();
	const auto* loaded = load_and_get_x64_dir();
	ASSERT_NE(loaded, nullptr);
	expect_contains_errors(*loaded,
		exception_directory_loader_errc::unmatched_directory_size);
	
	ASSERT_TRUE(loaded->get_runtime_function_list().empty());
}

TEST_F(X64ExceptionLoaderTestFixture, ExceptionLoaderValidDirectory)
{
	add_exception_dir();
	add_exception_dir_descriptors();
	dir = exceptions::load(instance, {});
	validate_dir(get_x64_dir());
}
