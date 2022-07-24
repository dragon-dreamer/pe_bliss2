#include "gtest/gtest.h"


#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/imports/import_directory.h"
#include "pe_bliss2/imports/import_directory_loader.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/pe_types.h"

#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{

struct ilt_validation_options
{
	uint32_t iat_offset{};
	bool is_bound = false;
	bool with_iat = false;
	bool with_name = false;
	bool has_ilt = true;
	bool virtual_name_error = false;
};

class ImportLoaderTestFixture : public ::testing::TestWithParam<core::optional_header::magic>
{
public:
	ImportLoaderTestFixture()
		: instance(create_test_image({
			.is_x64 = is_x64(),
			.start_section_rva = section_rva,
			.sections = { { 0x1000u, 0x1000u } } }))
	{
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
	}

	bool is_x64() const
	{
		return GetParam() == core::optional_header::magic::pe64;
	}

	std::uint32_t va_size() const
	{
		return is_x64() ? sizeof(std::uint64_t) : sizeof(std::uint32_t);
	}

	void add_import_directory(core::data_directories::directory_type dir
		= core::data_directories::directory_type::imports)
	{
		instance.get_data_directories().get_directory(dir).get()
			= { .virtual_address = directory_rva, .size = 0x1000u };
	}

	void add_import_directory_to_headers()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::imports).get()
			= { .virtual_address = directory_header_rva, .size = 0x1000u };
		instance.get_full_headers_buffer().copied_data().resize(0x1000u);
	}

	template<typename Func>
	void with_imports(const imports::import_directory_details& dir, Func&& func)
	{
		bool is_32bit = !!std::get_if<std::vector<
			imports::imported_library_details<std::uint32_t>>>(&dir.get_list());
		ASSERT_EQ(is_32bit, GetParam() == core::optional_header::magic::pe32);
		std::visit(std::forward<Func>(func), dir.get_list());
	}

	void add_import_directory_data()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		std::copy(library_descriptors.begin(), library_descriptors.end(),
			data.begin() + (directory_rva - section_rva));
	}

	void add_library_names()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		auto lib13_name_ptr = reinterpret_cast<const std::byte*>(lib13_name);
		std::copy(lib13_name_ptr, lib13_name_ptr + sizeof(lib13_name),
			data.begin() + lib13_name_offset);
		auto lib24_name_ptr = reinterpret_cast<const std::byte*>(lib24_name);
		std::copy(lib24_name_ptr, lib24_name_ptr + sizeof(lib24_name),
			data.begin() + lib24_name_offset);
	}

	void add_ilt(std::uint32_t offset = ilt13_offset)
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		if (is_x64())
			std::copy(ilt64.begin(), ilt64.end(), data.begin() + offset);
		else
			std::copy(ilt32.begin(), ilt32.end(), data.begin() + offset);
	}

	void add_hint_name()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		std::copy(hint_name.begin(), hint_name.end(), data.begin() + import2_offset);
	}

	template<typename Import1, typename Import2>
	void validate_import_addresses(const Import1* import1, const Import2* import2,
		const ilt_validation_options& opts) const
	{
		ASSERT_TRUE(import1);
		ASSERT_TRUE(import2);

		ASSERT_TRUE(import1->get_imported_va());
		ASSERT_TRUE(import2->get_imported_va());
		EXPECT_EQ(import1->get_imported_va()->get(), ordinal1
			| (is_x64() ? 0x8000000000000000ull : 0x80000000ull));
		EXPECT_EQ(import2->get_imported_va()->get(),
			import2_offset + absolute_offset);

		EXPECT_EQ(import1->get_imported_va()->get_state().absolute_offset(),
			absolute_offset + opts.iat_offset);
		EXPECT_EQ(import1->get_imported_va()->get_state().relative_offset(),
			opts.iat_offset);

		EXPECT_EQ(import2->get_imported_va()->get_state().absolute_offset(),
			absolute_offset + opts.iat_offset + va_size());
		EXPECT_EQ(import2->get_imported_va()->get_state().relative_offset(),
			opts.iat_offset + va_size());
	}

	template<typename Import1, typename Import2>
	void validate_imports(const Import1* import1, const Import2* import2,
		const ilt_validation_options& opts) const
	{
		ASSERT_TRUE(import1);
		ASSERT_TRUE(import2);

		EXPECT_EQ(import1->get_ordinal(), ordinal1);
		EXPECT_EQ(import1->to_thunk(), ordinal1
			| (is_x64() ? 0x8000000000000000ull : 0x80000000ull));

		if (opts.with_name)
		{
			EXPECT_EQ(import2->get_hint().get(), imported_hint);
			if (!opts.virtual_name_error)
			{
				EXPECT_EQ(import2->get_name().value(), imported_name);
				EXPECT_EQ(import2->get_name().value().size(),
					sizeof(imported_name) - 1);
			}
		}
		else
		{
			EXPECT_EQ(import2->get_hint().get(), 0u);
			EXPECT_TRUE(import2->get_name().value().empty());
		}

		EXPECT_EQ(import2->get_hint().get_state().absolute_offset(),
			absolute_offset + import2_offset);
		EXPECT_EQ(import2->get_hint().get_state().relative_offset(),
			import2_offset);
		if (!opts.virtual_name_error)
		{
			EXPECT_EQ(import2->get_name().get_state().absolute_offset(),
				absolute_offset + import2_offset + sizeof(std::uint16_t));
			EXPECT_EQ(import2->get_name().get_state().relative_offset(),
				import2_offset + sizeof(std::uint16_t));
		}

		if (opts.is_bound && opts.has_ilt)
		{
			ASSERT_TRUE(import1->get_imported_va());
			ASSERT_TRUE(import2->get_imported_va());
			if (opts.with_iat)
			{
				EXPECT_EQ(import1->get_imported_va()->get(),
					import1->to_thunk());
				EXPECT_EQ(import2->get_imported_va()->get(),
					import2_offset + absolute_offset);
			}
			else
			{
				EXPECT_EQ(import1->get_imported_va()->get(), 0u);
				EXPECT_EQ(import2->get_imported_va()->get(), 0u);
			}
			EXPECT_EQ(import1->get_imported_va()->get_state().absolute_offset(),
				absolute_offset + opts.iat_offset);
			EXPECT_EQ(import1->get_imported_va()->get_state().relative_offset(),
				opts.iat_offset);
			EXPECT_EQ(import2->get_imported_va()->get_state().absolute_offset(),
				absolute_offset + opts.iat_offset + va_size());
			EXPECT_EQ(import2->get_imported_va()->get_state().relative_offset(),
				opts.iat_offset + va_size());
		}
		else if (instance.is_loaded_to_memory())
		{
			validate_import_addresses(import1, import2, opts);
		}
		else
		{
			EXPECT_FALSE(import1->get_imported_va());
			EXPECT_FALSE(import2->get_imported_va());
		}
	}

	template<typename Import1>
	static void validate_import1_errors(const Import1& import1,
		const ilt_validation_options& opts)
	{
		if (opts.with_name)
		{
			if (opts.virtual_name_error)
			{
				expect_contains_errors(import1,
					imports::import_directory_loader_errc::invalid_import_name);
			}
			else
			{
				expect_contains_errors(import1);
			}
		}
		else
		{
			expect_contains_errors(import1,
				imports::import_directory_loader_errc::empty_import_name);
		}
	}

	template<typename T>
	void validate_ilt13(const T& library,
		ilt_validation_options opts = {}) const
	{
		opts.iat_offset = iat13_offset;
		const auto& list = library.get_imports();
		ASSERT_EQ(list.size(), 2u);
		ASSERT_TRUE(list[0].get_lookup());
		ASSERT_TRUE(list[1].get_lookup());

		if constexpr (std::is_same_v<std::uint64_t,
			typename std::remove_cvref_t<decltype(list[0])>::va_type>)
		{
			EXPECT_EQ(list[0].get_lookup()->get(), ordinal1 | 0x80000000'00000000ull);
			if (opts.with_iat)
			{
				EXPECT_EQ(list[0].get_address().get(), ordinal1 | 0x80000000'00000000ull);
			}
			validate_imports(
				std::get_if<imports::imported_function_ordinal<std::uint64_t>>(
					&(list[0].get_import_info())),
				std::get_if<imports::imported_function_hint_and_name<std::uint64_t>>(
					&(list[1].get_import_info())), opts);
		}
		else
		{
			EXPECT_EQ(list[0].get_lookup()->get(), ordinal1 | 0x80000000u);
			if (opts.with_iat)
			{
				EXPECT_EQ(list[0].get_address().get(), ordinal1 | 0x80000000u);
			}
			validate_imports(
				std::get_if<imports::imported_function_ordinal<std::uint32_t>>(
					&(list[0].get_import_info())),
				std::get_if<imports::imported_function_hint_and_name<std::uint32_t>>(
					&(list[1].get_import_info())), opts);
		}

		EXPECT_EQ(list[1].get_lookup()->get(), import2_offset + section_rva);
		EXPECT_EQ(list[0].get_lookup()->get_state().absolute_offset(),
			absolute_offset + ilt13_offset);
		EXPECT_EQ(list[0].get_lookup()->get_state().relative_offset(),
			ilt13_offset);
		EXPECT_EQ(list[1].get_lookup()->get_state().absolute_offset(),
			absolute_offset + ilt13_offset + va_size());
		EXPECT_EQ(list[1].get_lookup()->get_state().relative_offset(),
			ilt13_offset + va_size());
		if (opts.with_iat)
		{
			EXPECT_EQ(list[1].get_address().get(), import2_offset + section_rva);
			EXPECT_EQ(list[0].get_address().get_state().absolute_offset(),
				absolute_offset + iat13_offset);
			EXPECT_EQ(list[0].get_address().get_state().relative_offset(),
				iat13_offset);
			EXPECT_EQ(list[1].get_address().get_state().absolute_offset(),
				absolute_offset + iat13_offset + va_size());
			EXPECT_EQ(list[1].get_address().get_state().relative_offset(),
				iat13_offset + va_size());
		}

		if (opts.is_bound || opts.with_iat)
		{
			expect_contains_errors(list[0]);
			validate_import1_errors(list[1], opts);
		}
		else
		{
			expect_contains_errors(list[0],
				imports::import_directory_loader_errc::lookup_and_address_table_thunks_differ);
			expect_contains_errors(list[1],
				imports::import_directory_loader_errc::lookup_and_address_table_thunks_differ,
				imports::import_directory_loader_errc::empty_import_name);
		}
	}

	template<typename T>
	void validate_iat24(const T& library,
		ilt_validation_options opts = {})
	{
		opts.iat_offset = iat24_offset;
		const auto& list = library.get_imports();
		ASSERT_EQ(list.size(), 2u);
		ASSERT_FALSE(list[0].get_lookup());
		ASSERT_FALSE(list[1].get_lookup());

		if constexpr (std::is_same_v<std::uint64_t,
			typename std::remove_cvref_t<decltype(list[0])>::va_type>)
		{
			EXPECT_EQ(list[0].get_address().get(), ordinal1 | 0x80000000'00000000ull);
			if (instance.is_loaded_to_memory())
			{
				validate_import_addresses(
					std::get_if<imports::imported_function_address<std::uint64_t>>(
						&(list[0].get_import_info())),
					std::get_if<imports::imported_function_address<std::uint64_t>>(
						&(list[1].get_import_info())), opts);
			}
			else
			{
				validate_imports(
					std::get_if<imports::imported_function_ordinal<std::uint64_t>>(
						&(list[0].get_import_info())),
					std::get_if<imports::imported_function_hint_and_name<std::uint64_t>>(
						&(list[1].get_import_info())), opts);
			}
		}
		else
		{
			EXPECT_EQ(list[0].get_address().get(), ordinal1 | 0x80000000u);
			if (instance.is_loaded_to_memory())
			{
				validate_import_addresses(
					std::get_if<imports::imported_function_address<std::uint32_t>>(
						&(list[0].get_import_info())),
					std::get_if<imports::imported_function_address<std::uint32_t>>(
						&(list[1].get_import_info())), opts);
			}
			else
			{
				validate_imports(
					std::get_if<imports::imported_function_ordinal<std::uint32_t>>(
						&(list[0].get_import_info())),
					std::get_if<imports::imported_function_hint_and_name<std::uint32_t>>(
						&(list[1].get_import_info())), opts);
			}
		}

		EXPECT_EQ(list[1].get_address().get(), import2_offset + section_rva);
		EXPECT_EQ(list[0].get_address().get_state().absolute_offset(),
			absolute_offset + iat24_offset);
		EXPECT_EQ(list[0].get_address().get_state().relative_offset(),
			iat24_offset);
		EXPECT_EQ(list[1].get_address().get_state().absolute_offset(),
			absolute_offset + iat24_offset + va_size());
		EXPECT_EQ(list[1].get_address().get_state().relative_offset(),
			iat24_offset + va_size());

		expect_contains_errors(list[0]);
		validate_import1_errors(list[1], opts);
	}

public:
	image::image instance;

public:
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t directory_header_rva = 0x8u;
	static constexpr std::uint32_t directory_rva = 0x1000u;
	static constexpr std::uint32_t ilt13_offset = 0x100u;
	static constexpr std::uint32_t iat13_offset = 0x180u;
	static constexpr std::uint32_t lib13_name_offset = 0x200u;
	static constexpr std::uint32_t iat24_offset = 0x300u;
	static constexpr std::uint32_t lib24_name_offset = 0x400u;
	static constexpr const char lib13_name[] = "library1_3";
	static constexpr const char lib24_name[] = "library2";
	static constexpr std::uint8_t ordinal1 = 0xabu;
	static constexpr std::uint32_t import2_offset = 0x500;
	static constexpr std::uint16_t imported_hint = 0x1234u;
	static constexpr const char imported_name[] = "abcdef";
	static constexpr std::uint32_t library_count = 5u;

	static constexpr std::array library_descriptors
	{
		//library 0 - Invalid
		std::byte{0x00}, std::byte{0x11}, std::byte{0x11}, std::byte{0x11}, //lookup table
		std::byte{0xffu}, std::byte{0xffu}, std::byte{0xffu}, std::byte{}, //time date stamp
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //forwarder chain
		std::byte{0x00}, std::byte{0x12}, std::byte{}, std::byte{0x11}, //name
		std::byte{0x80}, std::byte{0x11}, std::byte{}, std::byte{0x11}, //address table

		//library 1 - IAT + ILT
		std::byte{0x00}, std::byte{0x11}, std::byte{}, std::byte{}, //lookup table
		std::byte{0x34}, std::byte{0x12}, std::byte{}, std::byte{}, //time date stamp
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //forwarder chain
		std::byte{0x00}, std::byte{0x12}, std::byte{}, std::byte{}, //name
		std::byte{0x80}, std::byte{0x11}, std::byte{}, std::byte{}, //address table

		//library 2 - IAT only
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //lookup table
		std::byte{0xddu}, std::byte{0xccu}, std::byte{0xbbu}, std::byte{0xaau}, //time date stamp
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //forwarder chain
		std::byte{0x00}, std::byte{0x14}, std::byte{}, std::byte{}, //name
		std::byte{0x00}, std::byte{0x13}, std::byte{}, std::byte{}, //address table

		//library 3 - Bound
		std::byte{0x00}, std::byte{0x11}, std::byte{}, std::byte{}, //lookup table
		std::byte{0xffu}, std::byte{0xffu}, std::byte{0xffu}, std::byte{0xffu}, //time date stamp
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //forwarder chain
		std::byte{0x00}, std::byte{0x12}, std::byte{}, std::byte{}, //name
		std::byte{0x80}, std::byte{0x11}, std::byte{}, std::byte{}, //address table

		//library 4 - IAT only + bound
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //lookup table
		std::byte{0xffu}, std::byte{0xffu}, std::byte{0xffu}, std::byte{0xffu}, //time date stamp
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //forwarder chain
		std::byte{0x00}, std::byte{0x14}, std::byte{}, std::byte{}, //name
		std::byte{0x00}, std::byte{0x13}, std::byte{}, std::byte{}, //address table
	};

	static constexpr std::array ilt32
	{
		std::byte{ordinal1}, std::byte{}, std::byte{}, std::byte{0x80}, //import1
		std::byte{}, std::byte{0x15}, std::byte{}, std::byte{}, //import2
	};

	static constexpr std::array ilt64
	{
		std::byte{ordinal1}, std::byte{}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{0x80}, //import1
		std::byte{}, std::byte{0x15}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //import2
	};

	static constexpr std::array hint_name
	{
		std::byte{0x34}, std::byte{0x12}, //hint
		std::byte{'a'}, std::byte{'b'},
		std::byte{'c'}, std::byte{'d'},
		std::byte{'e'}, std::byte{'f'},
		std::byte{} //name
	};
};
} //namespace

TEST_P(ImportLoaderTestFixture, LoadAbsentImportDirectory)
{
	auto result = imports::load(instance);
	EXPECT_FALSE(result);
}

TEST_P(ImportLoaderTestFixture, LoadInvalidImportDirectory)
{
	instance.get_data_directories().get_directory(
		core::data_directories::directory_type::imports).get()
		= { .virtual_address = 0xffffffu, .size = 0x1000u };
	auto result = imports::load(instance);
	ASSERT_TRUE(result);
	expect_contains_errors(*result,
		imports::import_directory_loader_errc::invalid_import_directory);
}

TEST_P(ImportLoaderTestFixture, LoadAbsentDelayedImportDirectory)
{
	auto result = imports::load(instance, { .target_directory
		= core::data_directories::directory_type::delay_import });
	EXPECT_FALSE(result);
}

TEST_P(ImportLoaderTestFixture, LoadZeroImportDirectory)
{
	for (auto dir : { core::data_directories::directory_type::imports,
		core::data_directories::directory_type::delay_import })
	{
		add_import_directory(dir);
		auto result = imports::load(instance, { .target_directory = dir });
		ASSERT_TRUE(result);
		expect_contains_errors(*result);
		with_imports(*result, [](const auto& list)
		{
			EXPECT_TRUE(list.empty());
		});
	}
}

TEST_P(ImportLoaderTestFixture, LoadZeroAddressesImportDirectory)
{
	add_import_directory();
	add_import_directory_data();
	auto result = imports::load(instance);
	ASSERT_TRUE(result);
	expect_contains_errors(*result);
	with_imports(*result, [](const auto& list)
	{
		ASSERT_EQ(list.size(), library_count);
		expect_contains_errors(list[0],
			imports::import_directory_loader_errc::invalid_library_name,
			imports::import_directory_loader_errc::invalid_imported_library_iat_ilt);
		expect_contains_errors(list[1],
			imports::import_directory_loader_errc::empty_library_name);
		expect_contains_errors(list[2],
			imports::import_directory_loader_errc::empty_library_name);
		expect_contains_errors(list[3],
			imports::import_directory_loader_errc::empty_library_name);
		expect_contains_errors(list[4],
			imports::import_directory_loader_errc::empty_library_name);
		EXPECT_TRUE(list[0].get_library_name().value().empty());
		EXPECT_TRUE(list[1].get_library_name().value().empty());
		EXPECT_TRUE(list[2].get_library_name().value().empty());
		EXPECT_TRUE(list[3].get_library_name().value().empty());
		EXPECT_TRUE(list[4].get_library_name().value().empty());
		EXPECT_FALSE(list[1].is_bound());
		EXPECT_FALSE(list[2].is_bound());
		EXPECT_TRUE(list[3].is_bound());
		EXPECT_TRUE(list[4].is_bound());
	});
}

TEST_P(ImportLoaderTestFixture, LoadImportDirectoryLibraryNames)
{
	add_import_directory();
	add_import_directory_data();
	add_library_names();
	auto result = imports::load(instance);
	ASSERT_TRUE(result);
	expect_contains_errors(*result);
	with_imports(*result, [](const auto& list)
	{
		ASSERT_EQ(list.size(), library_count);
		expect_contains_errors(list[1]);
		expect_contains_errors(list[2]);
		expect_contains_errors(list[3]);
		expect_contains_errors(list[4]);
		EXPECT_EQ(list[1].get_library_name().value(), lib13_name);
		EXPECT_EQ(list[2].get_library_name().value(), lib24_name);
		EXPECT_EQ(list[3].get_library_name().value(), lib13_name);
		EXPECT_EQ(list[4].get_library_name().value(), lib24_name);
	});
}

TEST_P(ImportLoaderTestFixture, LoadIlt13)
{
	add_import_directory();
	add_import_directory_data();
	add_library_names();
	add_ilt();
	auto result = imports::load(instance);
	ASSERT_TRUE(result);
	expect_contains_errors(*result);
	with_imports(*result, [this](const auto& list)
	{
		ASSERT_EQ(list.size(), library_count);
		expect_contains_errors(list[1]);
		expect_contains_errors(list[2]);
		expect_contains_errors(list[3]);
		expect_contains_errors(list[4]);
		validate_ilt13(list[1]);
		validate_ilt13(list[3], { .is_bound = true });
	});
}

TEST_P(ImportLoaderTestFixture, LoadIltIat13)
{
	add_import_directory();
	add_import_directory_data();
	add_library_names();
	add_ilt(ilt13_offset);
	add_ilt(iat13_offset);
	auto result = imports::load(instance);
	ASSERT_TRUE(result);
	expect_contains_errors(*result);
	with_imports(*result, [this](const auto& list)
	{
		ASSERT_EQ(list.size(), library_count);
		expect_contains_errors(list[1]);
		expect_contains_errors(list[2]);
		expect_contains_errors(list[3]);
		expect_contains_errors(list[4]);
		validate_ilt13(list[1], { .with_iat = true });
		validate_ilt13(list[3], { .is_bound = true,
			.with_iat = true });
	});
}

TEST_P(ImportLoaderTestFixture, LoadIltIat13WithHintName)
{
	add_import_directory();
	add_import_directory_data();
	add_library_names();
	add_ilt(ilt13_offset);
	add_ilt(iat13_offset);
	add_hint_name();
	auto result = imports::load(instance);
	ASSERT_TRUE(result);
	expect_contains_errors(*result);
	with_imports(*result, [this](const auto& list)
	{
		ASSERT_EQ(list.size(), library_count);
		expect_contains_errors(list[1]);
		expect_contains_errors(list[2]);
		expect_contains_errors(list[3]);
		expect_contains_errors(list[4]);
		validate_ilt13(list[1], { .with_iat = true,
			.with_name = true });
		validate_ilt13(list[3], { .is_bound = true,
			.with_iat = true, .with_name = true });
	});
}

TEST_P(ImportLoaderTestFixture, LoadIltIat24)
{
	add_import_directory();
	add_import_directory_data();
	add_library_names();
	add_ilt(iat24_offset);
	auto result = imports::load(instance);
	ASSERT_TRUE(result);
	expect_contains_errors(*result);
	with_imports(*result, [this](const auto& list)
	{
		ASSERT_EQ(list.size(), library_count);
		expect_contains_errors(list[2]);
		expect_contains_errors(list[4]);
		validate_iat24(list[2], { .has_ilt = false });
		validate_iat24(list[4], { .is_bound = true,
			.has_ilt = false });
	});
}

TEST_P(ImportLoaderTestFixture, LoadIltIat2WithHintName)
{
	add_import_directory();
	add_import_directory_data();
	add_library_names();
	add_ilt(iat24_offset);
	add_hint_name();
	auto result = imports::load(instance);
	ASSERT_TRUE(result);
	expect_contains_errors(*result);
	with_imports(*result, [this](const auto& list)
	{
		ASSERT_EQ(list.size(), library_count);
		expect_contains_errors(list[2]);
		expect_contains_errors(list[4]);
		validate_iat24(list[2], { .with_name = true,
			.has_ilt = false });
		validate_iat24(list[4], { .is_bound = true,
			.with_name = true, .has_ilt = false });
	});
}

TEST_P(ImportLoaderTestFixture, ImageLoadedIntoMemory)
{
	add_import_directory();
	add_import_directory_data();
	add_library_names();
	add_ilt(iat24_offset);
	add_ilt(ilt13_offset);
	add_ilt(iat13_offset);
	add_hint_name();
	instance.set_loaded_to_memory(true);
	auto result = imports::load(instance);
	ASSERT_TRUE(result);
	expect_contains_errors(*result);
	with_imports(*result, [this](const auto& list)
	{
		ASSERT_EQ(list.size(), library_count);
		expect_contains_errors(list[1]);
		expect_contains_errors(list[2]);
		expect_contains_errors(list[3]);
		expect_contains_errors(list[4]);
		validate_ilt13(list[1], { .with_iat = true,
			.with_name = true });
		validate_ilt13(list[3], { .is_bound = true,
			.with_iat = true, .with_name = true });
		validate_iat24(list[2], { .with_name = true,
			.has_ilt = false });
		validate_iat24(list[4], { .is_bound = true,
			.with_name = true, .has_ilt = false });
	});
}

TEST_P(ImportLoaderTestFixture, VirtualPart)
{
	add_import_directory();
	add_import_directory_data();
	add_library_names();
	add_ilt(iat24_offset);
	add_ilt(ilt13_offset);
	add_ilt(iat13_offset);
	add_hint_name();
	instance.get_section_data_list()[0].copied_data().resize(
		import2_offset
		+ sizeof(std::uint16_t) //size of hint
		+ sizeof(imported_name) - 1u); //nullbyte is virtual

	for (bool allow_virtual_memory : {false, true})
	{
		auto result = imports::load(instance,
			{ .allow_virtual_data = allow_virtual_memory });
		ASSERT_TRUE(result);
		expect_contains_errors(*result);
		with_imports(*result, [this, allow_virtual_memory](const auto& list)
		{
			ASSERT_EQ(list.size(), library_count);
			expect_contains_errors(list[1]);
			expect_contains_errors(list[2]);
			expect_contains_errors(list[3]);
			expect_contains_errors(list[4]);
			validate_ilt13(list[1], { .with_iat = true,
				.with_name = true, .virtual_name_error = !allow_virtual_memory });
			validate_ilt13(list[3], { .is_bound = true,
				.with_iat = true, .with_name = true,
				.virtual_name_error = !allow_virtual_memory });
			validate_iat24(list[2], { .with_name = true,
				.has_ilt = false, .virtual_name_error = !allow_virtual_memory });
			validate_iat24(list[4], { .is_bound = true,
				.with_name = true, .has_ilt = false,
				.virtual_name_error = !allow_virtual_memory });
		});
	}
}

TEST_P(ImportLoaderTestFixture, LoadZeroImportDirectoryFromHeadersError)
{
	add_import_directory_to_headers();
	auto result = imports::load(instance, { .include_headers = false });
	ASSERT_TRUE(result);
	expect_contains_errors(*result,
		imports::import_directory_loader_errc::invalid_import_directory);
}

TEST_P(ImportLoaderTestFixture, LoadZeroImportDirectoryFromHeaders)
{
	add_import_directory_to_headers();
	auto result = imports::load(instance);
	ASSERT_TRUE(result);
	expect_contains_errors(*result);
	with_imports(*result, [](const auto& list)
	{
		EXPECT_TRUE(list.empty());
	});
}

INSTANTIATE_TEST_SUITE_P(
	ImportLoaderTests,
	ImportLoaderTestFixture,
	::testing::Values(
		core::optional_header::magic::pe32,
		core::optional_header::magic::pe64
	));
