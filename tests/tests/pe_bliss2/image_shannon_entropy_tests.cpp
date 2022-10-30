#include "pe_bliss2/image/shannon_entropy.h"

#include <cstddef>
#include <cstdint>
#include <numeric>

#include "gtest/gtest.h"

#include "buffers/input_container_buffer.h"
#include "pe_bliss2/image/image.h"

TEST(ImageShannonEntropyTests, EmptyBuffer)
{
	buffers::input_container_buffer buf;
	EXPECT_EQ(pe_bliss::image::calculate_shannon_entropy(buf), 0u);
}

TEST(ImageShannonEntropyTests, Buffer)
{
	buffers::input_container_buffer buf;
	for (std::size_t i = 0; i != 256; ++i)
		buf.get_container().emplace_back(static_cast<std::byte>(i));

	EXPECT_NEAR(pe_bliss::image::calculate_shannon_entropy(buf), 8.f, 0.01f);
}

TEST(ImageShannonEntropyTests, EmptyImage)
{
	pe_bliss::image::image instance;
	EXPECT_EQ(pe_bliss::image::calculate_shannon_entropy(instance), 0u);
}

TEST(ImageShannonEntropyTests, Image)
{
	pe_bliss::image::image instance;
	instance.get_full_headers_buffer().copied_data().resize(100);
	for (std::size_t i = 0; i != 100; ++i)
		instance.get_full_headers_buffer().copied_data()[i] = static_cast<std::byte>(i);
	instance.get_section_data_list().resize(2u);
	instance.get_section_data_list()[0].copied_data().resize(50);
	for (std::size_t i = 100; i != 150; ++i)
		instance.get_section_data_list()[0].copied_data()[i - 100] = static_cast<std::byte>(i);
	instance.get_section_data_list()[1].copied_data().resize(50);
	for (std::size_t i = 150; i != 200; ++i)
		instance.get_section_data_list()[1].copied_data()[i - 150] = static_cast<std::byte>(i);
	instance.get_overlay().copied_data().resize(56);
	for (std::size_t i = 200; i != 256; ++i)
		instance.get_overlay().copied_data()[i - 200] = static_cast<std::byte>(i);
	EXPECT_NEAR(pe_bliss::image::calculate_shannon_entropy(instance), 8.f, 0.01f);
}
