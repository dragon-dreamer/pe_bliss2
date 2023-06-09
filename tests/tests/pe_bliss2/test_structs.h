#pragma once

#include <array>
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
	std::array<std::uint8_t, 2> a;
	std::array<std::uint32_t, 3> b;
	std::array<std::uint16_t, 4> c;
	std::array<std::array<std::uint64_t, 3>, 5> d;
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
	std::array<std::array<simple, 2>, 5> e;
	std::array<std::uint16_t, 5> f;
	friend auto operator<=>(const nested&, const nested&) = default;
};

struct arrays_short
{
	std::array<std::uint16_t, 3> c;
	friend auto operator<=>(const arrays_short&, const arrays_short&) = default;
};

struct nested_short
{
	std::uint32_t a;
	arrays_short b;
	std::array<std::array<simple, 3>, 2> c;
	std::array<std::uint8_t, 2> d;
	friend auto operator<=>(const nested_short&, const nested_short&) = default;
};
