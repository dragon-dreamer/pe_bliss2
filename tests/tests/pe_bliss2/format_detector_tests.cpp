#include "pe_bliss2/image/format_detector.h"

#include <array>
#include <cstddef>
#include <memory>

#include "gtest/gtest.h"

#include "buffers/input_memory_buffer.h"

using namespace pe_bliss;
using namespace pe_bliss::image;

TEST(FormatDetectorTests, Empty)
{
	std::array<std::byte, 1> arr{};
	buffers::input_memory_buffer buf(arr.data(), 0u);
	ASSERT_FALSE(format_detector::looks_like_pe(buf));
	ASSERT_EQ(format_detector::detect_format(buf), detected_format::none);
}

TEST(FormatDetectorTests, InvalidDosHeader)
{
	static constexpr const char header_data[] =
		"MX??????????????????????????????????????????????????????????"
		"\x04\x00\x00\x00??????";
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(header_data),
		std::size(header_data));
	ASSERT_FALSE(format_detector::looks_like_pe(buf));
	ASSERT_EQ(format_detector::detect_format(buf), detected_format::none);
}

TEST(FormatDetectorTests, InvalidElfanew)
{
	static constexpr const char header_data[] =
		"MZ??????????????????????????????????????????????????????????"
		"\x64\x00\x00\x00??????";
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(header_data),
		std::size(header_data));
	ASSERT_FALSE(format_detector::looks_like_pe(buf));
	ASSERT_EQ(format_detector::detect_format(buf), detected_format::none);
}

TEST(FormatDetectorTests, LooksLikePe)
{
	static constexpr const char header_data[] =
		"MZ??????????????????????????????????????????????????????????"
		"\x50\x00\x00\x00??????XXXXXXXXXX\x50\x45\x00\x00";
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(header_data),
		std::size(header_data));
	ASSERT_TRUE(format_detector::looks_like_pe(buf));
	ASSERT_EQ(format_detector::detect_format(buf), detected_format::none);
}

TEST(FormatDetectorTests, DetectFormatInvalidMagic)
{
	static constexpr const char header_data[] =
		"MZ??????????????????????????????????????????????????????????"
		"\x50\x00\x00\x00??????XXXXXXXXXX\x50\x45\x00\x00????????????????????"
		"ab";
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(header_data),
		std::size(header_data));
	ASSERT_TRUE(format_detector::looks_like_pe(buf));
	ASSERT_EQ(format_detector::detect_format(buf), detected_format::none);
}

TEST(FormatDetectorTests, DetectFormatPe64Magic)
{
	static constexpr const char header_data[] =
		"MZ??????????????????????????????????????????????????????????"
		"\x50\x00\x00\x00??????XXXXXXXXXX\x50\x45\x00\x00????????????????????"
		"\x0b\x02";
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(header_data),
		std::size(header_data));
	ASSERT_TRUE(format_detector::looks_like_pe(buf));
	ASSERT_EQ(format_detector::detect_format(buf), detected_format::pe64);
}

TEST(FormatDetectorTests, DetectFormatPe32Magic)
{
	static constexpr const char header_data[] =
		"MZ??????????????????????????????????????????????????????????"
		"\x50\x00\x00\x00??????XXXXXXXXXX\x50\x45\x00\x00????????????????????"
		"\x0b\x01";
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(header_data),
		std::size(header_data));
	ASSERT_TRUE(format_detector::looks_like_pe(buf));
	ASSERT_EQ(format_detector::detect_format(buf), detected_format::pe32);
}
