#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <variant>
#include <vector>

#include "pe_bliss2/exceptions/arm_common/arm_common_unwind_info.h"
#include "pe_bliss2/exceptions/arm_common/arm_common_exception_directory_loader.h"

#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::exceptions::arm_common;

namespace
{
struct packed_unwind_data
{
	explicit packed_unwind_data(std::uint32_t unwind_data) noexcept
		: unwind_data(unwind_data)
	{
	}

	std::uint32_t unwind_data{};
};

struct unwind_record_options
{
	using unwind_code_type = std::variant<
		unwind_code_common<1>,
		unwind_code_common<2>,
		unwind_code_common<3>,
		unwind_code_common<4>
	>;

	static constexpr std::uint32_t function_length_multiplier = 1u;
	static constexpr bool has_f_bit = false;
};

using epilog_info_type = epilog_info<false>;
using extended_unwind_record_type = extended_unwind_record<
	epilog_info_type, unwind_record_options>;

struct runtime_function_entry
{
	std::uint32_t begin_address;
	std::uint32_t unwind_data;
};

template<typename... Bases>
using runtime_function_base_type = runtime_function_base<
	runtime_function_entry,
	packed_unwind_data, extended_unwind_record_type,
	Bases...>;

using exception_directory_details = exception_directory_base<
	runtime_function_base_type, error_list>;

class exception_directory_container
	: public error_list
{
public:
	using exception_directory_type = std::variant<exception_directory_details>;
	using exception_directory_list_type = std::vector<exception_directory_type>;

public:
	[[nodiscard]]
	exception_directory_list_type& get_directories() & noexcept
	{
		return directories_;
	}

	[[nodiscard]]
	const exception_directory_list_type& get_directories() const& noexcept
	{
		return directories_;
	}

	[[nodiscard]]
	exception_directory_list_type get_directories() && noexcept
	{
		return std::move(directories_);
	}

private:
	exception_directory_list_type directories_;
};

class ArmCommonExceptionsLoaderTestFixture : public ::testing::Test
{
public:
	using unwind_code_type = std::variant<unwind_code_common<1>,
		unwind_code_common<2>, unwind_code_common<3>, unwind_code_common<4>>;

public:
	ArmCommonExceptionsLoaderTestFixture()
		: instance(create_test_image({ .start_section_rva = section_rva,
			.sections = { { section_virtual_size, section_raw_size } } }))
	{
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
	}

	static exception_directory_info get_exception_directory(
		const image::image& /* instance */, const ArmCommonExceptionsLoaderTestFixture& options)
	{
		return { .rva = options.directory_rva, .size = options.directory_size };
	}

	static std::byte decode_unwind_code(std::byte code) noexcept
	{
		return code;
	}

	static void create_uwop_code(std::vector<unwind_code_type>& codes, std::byte code)
	{
		if (code == std::byte{ 1 })
			codes.emplace_back(std::in_place_type<unwind_code_common<1>>);
		else if (code == std::byte{ 2 })
			codes.emplace_back(std::in_place_type<unwind_code_common<2>>);
		else if (code == std::byte{ 3 })
			codes.emplace_back(std::in_place_type<unwind_code_common<3>>);
		else
			codes.emplace_back(std::in_place_type<unwind_code_common<4>>);
	}

	void load_dir()
	{
		EXPECT_NO_THROW((load<ArmCommonExceptionsLoaderTestFixture,
			ArmCommonExceptionsLoaderTestFixture, packed_unwind_data,
			extended_unwind_record_type, exception_directory_details>(
				instance, *this, container)));
	}

	void add_directory()
	{
		auto it = instance.get_section_data_list()[0].copied_data().begin();
		it = std::copy(directory_part1.begin(), directory_part1.end(), it);
		std::copy(directory_part2.begin(), directory_part2.end(), it);
	}

	void add_invalid_directory()
	{
		auto it = instance.get_section_data_list()[0].copied_data().begin();
		it = std::copy(directory_part1.begin(), directory_part1.end(), it);
		std::copy(directory_part2_invalid.begin(), directory_part2_invalid.end(), it);
	}

	void add_directory_to_headers()
	{
		//Part 2 to headers
		auto& data = instance.get_full_headers_buffer().copied_data();
		data.resize(directory_part2.size() + directory_part1.size());
		std::copy(directory_part2.begin(), directory_part2.end(), data.begin()
			+ directory_part1.size());

		//Part 1 to section
		auto it = instance.get_section_data_list()[0].copied_data().begin();
		std::copy(directory_part1.begin(), directory_part1.end(), it);
	}

	exception_directory_details& get_directory()
	{
		return std::get<exception_directory_details>(
			container.get_directories().at(0));
	}

public:
	void validate_directory()
	{
		expect_contains_errors(container);
		ASSERT_EQ(container.get_directories().size(), 1u);
		const auto& func_list = get_directory().get_runtime_function_list();
		ASSERT_EQ(func_list.size(), runtime_function_count);

		ASSERT_TRUE(func_list[0].has_extended_unwind_record());
		expect_contains_errors(func_list[0],
			exception_directory_loader_errc::unordered_epilog_scopes,
			exception_directory_loader_errc::invalid_exception_handler_rva);
		const auto* unwind0_ptr = std::get_if<extended_unwind_record_type>(
			&func_list[0].get_unwind_info());
		ASSERT_NE(unwind0_ptr, nullptr);
		const auto& unwind0 = *unwind0_ptr;
		EXPECT_TRUE(unwind0.has_extended_main_header());
		const auto& epilogs0 = unwind0.get_epilog_info_list();
		ASSERT_EQ(epilogs0.size(), extended_epilog_count);
		EXPECT_EQ(epilogs0[0].get_descriptor().get(), epilog0);
		EXPECT_EQ(epilogs0[1].get_descriptor().get(), epilog1);
		const auto& codes0 = unwind0.get_unwind_code_list();
		ASSERT_EQ(codes0.size(), extended_code_words_count);
		const auto* code0 = std::get_if<unwind_code_common<1>>(&codes0[0]);
		ASSERT_NE(code0, nullptr);
		EXPECT_EQ(code0->get_descriptor()[0], std::byte{ 1 });
		EXPECT_EQ(code0->get_descriptor().data_size(), 1u);
		EXPECT_EQ(code0->get_descriptor().physical_size(), 1u);
		EXPECT_EQ(code0->get_descriptor().get_state().relative_offset(), code0_offset);
		const auto* code1 = std::get_if<unwind_code_common<4>>(&codes0[1]);
		ASSERT_NE(code1, nullptr);
		EXPECT_EQ(code1->get_descriptor()[0], std::byte{ 4 });
		EXPECT_EQ(code1->get_descriptor()[1], std::byte{ 5 });
		EXPECT_EQ(code1->get_descriptor()[2], std::byte{ 6 });
		EXPECT_EQ(code1->get_descriptor()[3], std::byte{ 7 });
		EXPECT_EQ(code1->get_descriptor().data_size(), 4u);
		EXPECT_EQ(code1->get_descriptor().physical_size(), 4u);
		EXPECT_EQ(code1->get_descriptor().get_state().relative_offset(), code1_offset);
		ASSERT_TRUE(unwind0.has_exception_data());
		EXPECT_EQ(unwind0.get_exception_handler_rva().get(), exception_handler_rva);

		ASSERT_FALSE(func_list[1].has_extended_unwind_record());
		expect_contains_errors(func_list[1]);
		const auto* unwind1_ptr = std::get_if<packed_unwind_data>(
			&func_list[1].get_unwind_info());
		ASSERT_NE(unwind1_ptr, nullptr);
		EXPECT_EQ(unwind1_ptr->unwind_data, 1u);
	}

public:
	image::image instance;
	exception_directory_container container;
	std::uint32_t directory_rva = 0u;
	std::uint32_t directory_size = 0x100u;
	bool include_headers = false;
	bool allow_virtual_data = false;

public:
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t section_virtual_size = 0x2000u;
	static constexpr std::uint32_t section_raw_size = 0x1000u;

	static constexpr std::uint32_t extended_record_rva = section_rva;
	static constexpr std::size_t extended_epilog_count = 2u;
	static constexpr std::size_t extended_code_words_count = 2u;
	static constexpr std::size_t runtime_function_count = 2u;
	static constexpr std::uint32_t exception_handler_rva = 0xfefafdfcu;
	static constexpr std::uint32_t epilog0 = 2u;
	static constexpr std::uint32_t epilog1 = 1u;
	static constexpr std::uint32_t code0_offset = 16u;
	static constexpr std::uint32_t code1_offset = 17u;
	static_assert((extended_record_rva & 0b11u) == 0);
	static constexpr std::array directory_part1{
		//extended record for the first entry
		std::byte{}, std::byte{}, std::byte{0x10u}, std::byte{}, //main header
		//extended main header
		std::byte{extended_epilog_count}, std::byte{},
		std::byte{extended_code_words_count}, std::byte{},
		std::byte{epilog0}, std::byte{}, std::byte{}, std::byte{}, //epilog 0
		std::byte{epilog1}, std::byte{}, std::byte{}, std::byte{}, //epilog 1
		//code words
		std::byte{1}, //opcode of length 1
		std::byte{4}, //opcode of length 4
		std::byte{5}, std::byte{6}, std::byte{7}, //bytes of opcode
		std::byte{}, //end of code words
		std::byte{}, std::byte{}, //padding
		//exception handler RVA
		std::byte{exception_handler_rva & 0xffu},
		std::byte{(exception_handler_rva >> 8u) & 0xffu},
		std::byte{(exception_handler_rva >> 16u) & 0xffu},
		std::byte{(exception_handler_rva >> 24u) & 0xffu},
	};
	static constexpr auto runtime_function_rva = static_cast<std::uint32_t>(
		section_rva + directory_part1.size());
	static constexpr std::array directory_part2{
		//runtime function entries
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //begin_address
		//unwind_data - has extended record
		std::byte{extended_record_rva & 0xffu},
		std::byte{(extended_record_rva >> 8u) & 0xffu},
		std::byte{(extended_record_rva >> 16u) & 0xffu},
		std::byte{(extended_record_rva >> 24u) & 0xffu},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //begin_address
		std::byte{1}, std::byte{}, std::byte{}, std::byte{}, //unwind_data - no extended record
	};
	static constexpr std::uint32_t extended_record_invalid_rva = 0xffffff00u;
	static_assert((extended_record_invalid_rva & 0b11u) == 0);
	static constexpr std::array directory_part2_invalid{
		//runtime function entries
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //begin_address
		//unwind_data - has extended record
		std::byte{extended_record_invalid_rva & 0xffu},
		std::byte{(extended_record_invalid_rva >> 8u) & 0xffu},
		std::byte{(extended_record_invalid_rva >> 16u) & 0xffu},
		std::byte{(extended_record_invalid_rva >> 24u) & 0xffu},
	};
};
} //namespace

TEST_F(ArmCommonExceptionsLoaderTestFixture, AbsentDirectory)
{
	load_dir();
	EXPECT_TRUE(container.get_directories().empty());
}

TEST_F(ArmCommonExceptionsLoaderTestFixture, InvalidDirectorySize)
{
	directory_rva = 1u;
	directory_size = (std::numeric_limits<std::uint32_t>::max)();
	load_dir();
	expect_contains_errors(container);
	EXPECT_EQ(container.get_directories().size(), 1u);
	EXPECT_TRUE(get_directory().get_runtime_function_list().empty());
	expect_contains_errors(get_directory(),
		exception_directory_loader_errc::invalid_directory_size);
}

TEST_F(ArmCommonExceptionsLoaderTestFixture, UnmatchedDirectorySize)
{
	directory_rva = 1u;
	directory_size = 1u;
	load_dir();
	expect_contains_errors(container);
	ASSERT_EQ(container.get_directories().size(), 1u);
	EXPECT_TRUE(get_directory().get_runtime_function_list().empty());
	expect_contains_errors(get_directory(),
		exception_directory_loader_errc::unmatched_directory_size);
}

TEST_F(ArmCommonExceptionsLoaderTestFixture, ValidDirectory)
{
	directory_rva = runtime_function_rva;
	directory_size = static_cast<std::uint32_t>(directory_part2.size());
	add_directory();
	load_dir();
	validate_directory();
}

TEST_F(ArmCommonExceptionsLoaderTestFixture, InvalidDirectory)
{
	directory_rva = runtime_function_rva;
	directory_size = static_cast<std::uint32_t>(directory_part2_invalid.size());
	add_invalid_directory();
	load_dir();
	expect_contains_errors(container);
	ASSERT_EQ(container.get_directories().size(), 1u);
	const auto& func_list = get_directory().get_runtime_function_list();
	ASSERT_EQ(func_list.size(), 1u);

	ASSERT_TRUE(func_list[0].has_extended_unwind_record());
	expect_contains_errors(func_list[0],
		exception_directory_loader_errc::invalid_extended_unwind_info);
}

TEST_F(ArmCommonExceptionsLoaderTestFixture, LoadHeadersError)
{
	directory_rva = static_cast<std::uint32_t>(directory_part1.size());
	directory_size = static_cast<std::uint32_t>(directory_part2.size());
	add_directory_to_headers();
	load_dir();
	expect_contains_errors(container);
	ASSERT_EQ(container.get_directories().size(), 1u);
	const auto& func_list = get_directory().get_runtime_function_list();
	ASSERT_EQ(func_list.size(), 1u);
	expect_contains_errors(func_list[0],
		exception_directory_loader_errc::invalid_runtime_function_entry);
}

TEST_F(ArmCommonExceptionsLoaderTestFixture, LoadHeaders)
{
	directory_rva = static_cast<std::uint32_t>(directory_part1.size());
	directory_size = static_cast<std::uint32_t>(directory_part2.size());
	include_headers = true;
	add_directory_to_headers();
	load_dir();
	validate_directory();
}

TEST_F(ArmCommonExceptionsLoaderTestFixture, VirtualPart)
{
	directory_rva = runtime_function_rva;
	directory_size = static_cast<std::uint32_t>(directory_part2.size());
	add_directory();
	//3 last bytes are virtual
	auto& data = instance.get_section_data_list()[0].copied_data();
	data.resize(directory_part1.size() + directory_part2.size() - 3u);
	allow_virtual_data = true;
	load_dir();
	validate_directory();

	ASSERT_EQ(container.get_directories().size(), 1u);
	const auto& func_list = get_directory().get_runtime_function_list();
	ASSERT_EQ(func_list.size(), runtime_function_count);
	EXPECT_EQ(func_list[1].get_descriptor().physical_size(),
		func_list[1].get_descriptor().data_size() - 3u);
}

TEST_F(ArmCommonExceptionsLoaderTestFixture, VirtualPartError)
{
	directory_rva = runtime_function_rva;
	directory_size = static_cast<std::uint32_t>(directory_part2.size());
	add_directory();
	//3 last bytes are virtual
	auto& data = instance.get_section_data_list()[0].copied_data();
	data.resize(directory_part1.size() + directory_part2.size() - 3u);
	load_dir();

	ASSERT_EQ(container.get_directories().size(), 1u);
	const auto& func_list = get_directory().get_runtime_function_list();
	ASSERT_EQ(func_list.size(), runtime_function_count);
	expect_contains_errors(func_list[0],
		exception_directory_loader_errc::unordered_epilog_scopes,
		exception_directory_loader_errc::invalid_exception_handler_rva);
	expect_contains_errors(func_list[1],
		exception_directory_loader_errc::invalid_runtime_function_entry);
}
