#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "pe_bliss2/tls/tls_directory_loader.h"
#include "pe_bliss2/tls/tls_directory_loader.h"
#include "pe_bliss2/image/image.h"

#include "tests/pe_bliss2/image_helper.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{

class TlsLoaderTestFixture : public ::testing::TestWithParam<core::optional_header::magic>
{
public:
	TlsLoaderTestFixture()
		: instance(create_test_image({
			.is_x64 = is_x64(),
			.start_section_rva = section_rva,
			.sections = { { section_virtual_size, section_raw_size } } }))
	{
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
		instance.get_optional_header().set_raw_image_base(image_base);
	}

	bool is_x64() const
	{
		return GetParam() == core::optional_header::magic::pe64;
	}

	std::uint32_t va_size() const
	{
		return is_x64() ? sizeof(std::uint64_t) : sizeof(std::uint32_t);
	}

	void add_tls_directory()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::tls).get()
			= { .virtual_address = directory_rva, .size = 0x1000u };
	}

	void add_tls_directory_to_headers()
	{
		instance.get_full_headers_buffer().copied_data().resize(0x1000);
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::tls).get()
			= { .virtual_address = headers_directory_rva, .size = 0x1000u };
	}

	void add_tls_descriptor_with_invalid_raw_data()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		std::copy(tls_descriptor_invalid_raw_data.begin(),
			tls_descriptor_invalid_raw_data.end(),
			data.begin() + (directory_rva - section_rva));
	}

	void add_tls_data()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		if (is_x64())
		{
			std::copy(tls_descriptor64.begin(), tls_descriptor64.end(),
				data.begin() + (directory_rva - section_rva));
			std::copy(tls_callbacks64.begin(), tls_callbacks64.end(),
				data.begin() + callbacks_offset);
		}
		else
		{
			std::copy(tls_descriptor32.begin(), tls_descriptor32.end(),
				data.begin() + (directory_rva - section_rva));
			std::copy(tls_callbacks32.begin(), tls_callbacks32.end(),
				data.begin() + callbacks_offset);
		}

		std::copy(tls_raw_data.begin(), tls_raw_data.end(),
			data.begin() + raw_data_start_offset);
	}

	void cut_to_the_last_callback()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		// Cut the trailing zero callback
		data.resize(callbacks_offset + callback_count * va_size());
	}

	static auto get_expected_raw_data()
	{
		std::vector<std::byte> expected_raw_data(
			tls_raw_data.begin(), tls_raw_data.end());
		expected_raw_data.resize(raw_data_end_offset - raw_data_start_offset);
		return expected_raw_data;
	}

	template<typename Dir, typename Func>
	void with_tls(Dir&& dir, Func&& func)
	{
		bool is_64bit = !!std::get_if<tls::tls_directory_details64>(&dir);
		ASSERT_EQ(is_64bit, is_x64());
		std::visit(std::forward<Func>(func), std::forward<Dir>(dir));
	}

	template<typename Dir>
	void validate_tls_callbacks(const Dir& dir)
	{
		ASSERT_EQ(dir.get_callbacks().size(), callback_count);
		EXPECT_EQ(dir.get_callbacks()[0].get(),
			callback1_offset + section_rva + image_base);
		EXPECT_EQ(dir.get_callbacks()[1].get(),
			callback2_offset + section_rva + image_base);

		expect_contains_errors(dir.get_callbacks()[0]);
		expect_contains_errors(dir.get_callbacks()[1],
			tls::tls_directory_loader_errc::invalid_callback_va);

		EXPECT_EQ(dir.get_callbacks()[0].get_state().absolute_offset(),
			callbacks_offset + absolute_offset);
		EXPECT_EQ(dir.get_callbacks()[0].get_state().relative_offset(),
			callbacks_offset);

		EXPECT_EQ(dir.get_callbacks()[1].get_state().absolute_offset(),
			callbacks_offset + absolute_offset + va_size());
		EXPECT_EQ(dir.get_callbacks()[1].get_state().relative_offset(),
			callbacks_offset + va_size());
	}

public:
	image::image instance;

public:
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t directory_rva = 0x1000u;
	static constexpr std::uint32_t headers_directory_rva = 0x8u;
	static constexpr std::uint32_t image_base = 0x40000;
	static constexpr std::uint32_t raw_data_start_offset = 0x100u;
	static constexpr std::uint32_t raw_data_end_offset = 0x110u;
	static constexpr std::uint32_t callbacks_offset = 0x130u;
	static constexpr std::uint32_t callback1_offset = 0x50u;
	static constexpr std::uint32_t callback2_offset = 0x1150u;
	static constexpr std::uint32_t callback_count = 2u;
	static constexpr std::uint32_t section_virtual_size = 0x2000u;
	static constexpr std::uint32_t section_raw_size = 0x1000u;

	static constexpr std::array tls_descriptor_invalid_raw_data{
		std::byte{0x00}, std::byte{0x01}, std::byte{}, std::byte{}, //start_address_of_raw_data
	};

	static constexpr std::array tls_raw_data{
		std::byte{1}, std::byte{2}, std::byte{3}
	};

	static constexpr std::array tls_descriptor32{
		std::byte{0x00}, std::byte{0x11}, std::byte{0x04}, std::byte{}, //start_address_of_raw_data
		std::byte{0x10}, std::byte{0x11}, std::byte{0x04}, std::byte{}, //end_address_of_raw_data
		std::byte{0x20}, std::byte{0x11}, std::byte{0x04}, std::byte{}, //address_of_index
		std::byte{0x30}, std::byte{0x11}, std::byte{0x04}, std::byte{}, //address_of_callbacks
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size_of_zero_fill
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //characteristics
	};

	static constexpr std::array tls_descriptor64{
		std::byte{0x00}, std::byte{0x11}, std::byte{0x04}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //start_address_of_raw_data
		std::byte{0x10}, std::byte{0x11}, std::byte{0x04}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //end_address_of_raw_data
		std::byte{0x20}, std::byte{0x11}, std::byte{0x04}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //address_of_index
		std::byte{0x30}, std::byte{0x11}, std::byte{0x04}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //address_of_callbacks
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size_of_zero_fill
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //characteristics
	};

	static constexpr std::array tls_callbacks32{
		std::byte{0x50}, std::byte{0x10}, std::byte{0x04}, std::byte{}, //callback 1
		std::byte{0x50}, std::byte{0x21}, std::byte{0x04}, std::byte{}, //callback 2
		std::byte{}, std::byte{}, std::byte{}, std::byte{}
	};

	static constexpr std::array tls_callbacks64{
		std::byte{0x50}, std::byte{0x10}, std::byte{0x04}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //callback 1
		std::byte{0x50}, std::byte{0x21}, std::byte{0x04}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //callback 2
		std::byte{}, std::byte{}, std::byte{}, std::byte{}
	};
};
} //namespace

TEST_P(TlsLoaderTestFixture, LoadAbsentTlsDirectory)
{
	auto result = tls::load(instance);
	EXPECT_FALSE(result);
}

TEST_P(TlsLoaderTestFixture, LoadZeroTlsDirectory)
{
	add_tls_directory();
	auto result = tls::load(instance);
	ASSERT_TRUE(result);
	with_tls(*result, [](const auto& dir)
	{
		expect_contains_errors(dir,
			tls::tls_directory_loader_errc::invalid_index_va);
		EXPECT_EQ(dir.get_descriptor().get_state().absolute_offset(),
			absolute_offset);
		EXPECT_EQ(dir.get_descriptor().get_state().relative_offset(), 0u);
		EXPECT_EQ(dir.get_raw_data().size(), 0u);
		EXPECT_TRUE(dir.get_callbacks().empty());
	});
}

TEST_P(TlsLoaderTestFixture, LoadTlsDirectoryWithInvalidRawData)
{
	add_tls_directory();
	add_tls_descriptor_with_invalid_raw_data();
	auto result = tls::load(instance);
	ASSERT_TRUE(result);
	with_tls(*result, [](const auto& dir)
	{
		expect_contains_errors(dir,
			tls::tls_directory_loader_errc::invalid_raw_data,
			tls::tls_directory_loader_errc::invalid_index_va);
	});
}

TEST_P(TlsLoaderTestFixture, LoadTlsDirectory)
{
	add_tls_directory();
	add_tls_data();
	auto result = tls::load(instance);
	ASSERT_TRUE(result);
	with_tls(*result, [this](auto& dir)
	{
		expect_contains_errors(dir);

		EXPECT_EQ(dir.get_raw_data().data()->absolute_offset(),
			raw_data_start_offset + absolute_offset);
		EXPECT_EQ(dir.get_raw_data().data()->relative_offset(),
			raw_data_start_offset);

		EXPECT_FALSE(dir.get_raw_data().is_copied());
		EXPECT_EQ(dir.get_raw_data().copied_data(), get_expected_raw_data());

		validate_tls_callbacks(dir);
	});
}

TEST_P(TlsLoaderTestFixture, LoadZeroTlsDirectoryHeaders)
{
	add_tls_directory_to_headers();
	auto result = tls::load(instance);
	ASSERT_TRUE(result);
	with_tls(*result, [](const auto& dir)
	{
		expect_contains_errors(dir,
			tls::tls_directory_loader_errc::invalid_index_va);
		EXPECT_EQ(dir.get_descriptor().get_state().absolute_offset(),
			headers_directory_rva);
		EXPECT_EQ(dir.get_descriptor().get_state().relative_offset(),
			headers_directory_rva);
		EXPECT_EQ(dir.get_raw_data().size(), 0u);
		EXPECT_TRUE(dir.get_callbacks().empty());
	});
}

TEST_P(TlsLoaderTestFixture, LoadZeroTlsDirectoryHeadersError)
{
	add_tls_directory_to_headers();
	auto result = tls::load(instance, { .include_headers = false });
	ASSERT_TRUE(result);
	with_tls(*result, [](const auto& dir)
	{
		expect_contains_errors(dir,
			tls::tls_directory_loader_errc::invalid_directory);
	});
}

TEST_P(TlsLoaderTestFixture, LoadTlsDirectoryCopyRawData)
{
	add_tls_directory();
	add_tls_data();
	auto result = tls::load(instance, { .copy_raw_data = true });
	ASSERT_TRUE(result);
	with_tls(*result, [this](const auto& dir)
	{
		expect_contains_errors(dir);
		EXPECT_TRUE(dir.get_raw_data().is_copied());
		EXPECT_EQ(dir.get_raw_data().copied_data(), get_expected_raw_data());
		validate_tls_callbacks(dir);
	});
}

TEST_P(TlsLoaderTestFixture, LoadTlsDirectoryVirtualDataError)
{
	add_tls_directory();
	add_tls_data();
	cut_to_the_last_callback();
	auto result = tls::load(instance);
	ASSERT_TRUE(result);
	with_tls(*result, [this](const auto& dir)
	{
		expect_contains_errors(dir,
			tls::tls_directory_loader_errc::invalid_callbacks);
		validate_tls_callbacks(dir);
	});
}

TEST_P(TlsLoaderTestFixture, LoadTlsDirectoryVirtualData)
{
	add_tls_directory();
	add_tls_data();
	cut_to_the_last_callback();
	auto result = tls::load(instance, { .allow_virtual_data = true });
	ASSERT_TRUE(result);
	with_tls(*result, [this](const auto& dir)
	{
		expect_contains_errors(dir);
		validate_tls_callbacks(dir);
	});
}

INSTANTIATE_TEST_SUITE_P(
	TlsLoaderTests,
	TlsLoaderTestFixture,
	::testing::Values(
		core::optional_header::magic::pe32,
		core::optional_header::magic::pe64
	));
