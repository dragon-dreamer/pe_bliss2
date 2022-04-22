#pragma once

#include <compare>

constexpr std::size_t simple_size = 15u;
struct simple
{
	std::uint8_t a;
	std::uint32_t b;
	std::uint16_t c;
	std::uint64_t d;
	friend auto operator<=>(const simple&, const simple&) = default;
};

constexpr std::size_t arrays_size = 142u;
struct arrays
{
	std::uint8_t a[2];
	std::uint32_t b[3];
	std::uint16_t c[4];
	std::uint64_t d[5][3];
	friend auto operator<=>(const arrays&, const arrays&) = default;
};

constexpr std::size_t nested_size
= simple_size * 11u + arrays_size + 15u;
struct nested
{
	std::uint32_t a;
	simple b;
	arrays c;
	std::uint8_t d;
	simple e[5][2];
	std::uint16_t f[5];
	friend auto operator<=>(const nested&, const nested&) = default;
};

struct arrays_short
{
	std::uint16_t c[3];
	friend auto operator<=>(const arrays_short&, const arrays_short&) = default;
};

struct nested_short
{
	std::uint32_t a;
	arrays_short b;
	simple c[2][3];
	std::uint8_t d[2];
	friend auto operator<=>(const nested_short&, const nested_short&) = default;
};
