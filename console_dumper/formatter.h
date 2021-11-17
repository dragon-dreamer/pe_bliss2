#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <iomanip>
#include <map>
#include <ostream>
#include <string>
#include <system_error>
#include <type_traits>

#include <boost/pfr/core.hpp>

#include "color_provider.h"

#include "buffers/input_buffer_interface.h"
#include "buffers/input_memory_buffer.h"
#include "pe_bliss2/detail/error_list.h"
#include "pe_bliss2/detail/packed_byte_array.h"
#include "pe_bliss2/detail/packed_byte_vector.h"
#include "pe_bliss2/detail/packed_c_string.h"

struct value_info
{
	const char* name = nullptr;
	bool is_hex = true;
	std::function<void(std::size_t)> formatter;
	bool output_value = true;
};

class formatter
{
public:
	formatter(color_provider_interface& color_provider,
		std::ostream& stream, std::ostream& error_stream) noexcept
		: color_provider_(color_provider)
		, stream_(stream)
		, error_stream_(error_stream)
	{
	}

	template<typename Array>
	static void format_array_extents(std::string& value)
	{
		if constexpr (std::is_array_v<Array>)
		{
			value += '[';
			value += std::to_string(std::extent_v<Array, 0>);
			value += ']';

			format_array_extents<std::remove_extent_t<Array>>(value);
		}
	}

	template<typename T>
	void print_value(const T& value)
	{
		if constexpr (std::is_same_v<T, std::uint8_t>)
			stream_ << static_cast<std::uint32_t>(value);
		else if constexpr (std::is_same_v<T, std::int8_t>)
			stream_ << static_cast<std::int32_t>(value);
		else
			stream_ << value;
	}

	template<typename T>
	void print_value(const T& value, bool is_hex)
	{
		color_changer changer(stream_, color_provider_, value_fg_color, value_bg_color);
		if (is_hex)
		{
			stream_ << std::hex;

			if constexpr (std::is_integral_v<T>)
				stream_ << std::setw(sizeof(T) * 2) << std::setfill('0');
		}
		else
		{
			stream_ << std::dec;
		}

		print_value(value);
	}

	template<typename T, std::size_t N>
	void print_value(const value_info& info, T(&arr)[N], std::size_t left_padding = 0)
	{
		if (!info.output_value)
		{
			if (info.formatter)
				info.formatter(left_padding);
			stream_ << '\n';
			return;
		}

		for (std::size_t i = 0; i != N; ++i)
		{
			if (i && left_padding)
				stream_ << std::setfill(' ') << std::setw(left_padding) << "";
			print_value(info, arr[i], i ? left_padding : 0);
		}
	}

	template<typename T>
	void print_value(const value_info& info, const T& value, std::size_t left_padding = 0)
	{
		if (info.output_value)
		{
			print_value(value, info.is_hex);
			stream_ << ' ';
		}

		if (info.formatter)
			info.formatter(left_padding);

		stream_ << '\n';
	}

	void print_absolute_offset(std::size_t absolute_offset)
	{
		stream_ << '@';

		{
			color_changer changer(stream_, color_provider_,
				absolute_fg_offset_color, absolute_bg_offset_color);
			stream_ << std::hex << std::setw(8) << std::setfill('0') << absolute_offset;
		}
	}

	void print_offsets(std::size_t absolute_offset, std::size_t relative_offset)
	{
		print_absolute_offset(absolute_offset);

		stream_ << '/';

		{
			color_changer changer(stream_, color_provider_,
				relative_fg_offset_color, relative_bg_offset_color);
			stream_ << std::hex << std::setw(8) << std::setfill('0') << relative_offset;
		}
	}

	template<typename PackedValue>
	void print_offsets(const PackedValue& value)
	{
		print_offsets(value.absolute_offset(), value.relative_offset());
	}
	
	template<typename PackedValue>
	void print_offsets_and_value(const PackedValue& value, bool is_hex)
	{
		print_offsets(value);
		stream_ << '=';
		print_value(value.get(), is_hex);
	}

	template<typename T>
	void print_flags(T flags, std::size_t left_padding,
		const std::map<T, const char*>& mapping)
	{
		std::string pad(left_padding, ' ');
		pad = "\n" + pad;
		for (const auto& [key, name] : mapping)
		{
			if (flags & key)
			{
				stream_ << pad;

				color_changer changer(stream_, color_provider_,
					flags_fg_color, flags_bg_color);
				stream_ << name;
			}
		}
	}

	template<typename T>
	void print_comma_separated_flags(T flags,
		const std::map<T, const char*>& mapping)
	{
		const char* separator = "\0\0";
		for (const auto& [key, name] : mapping)
		{
			if (flags & key)
			{
				stream_ << separator;

				color_changer changer(stream_, color_provider_,
					flags_fg_color, flags_bg_color);
				stream_ << name;

				separator = ", ";
			}
		}
	}

	void print_packed_string(const pe_bliss::detail::packed_c_string& str)
	{
		if (str.value().empty() && !str.absolute_offset())
			return;

		print_offsets(str);
		stream_ << " (";
		print_string(str.value().c_str());
		stream_ << ')';
	}

	void print_string(const char* str)
	{
		color_changer changer(stream_, color_provider_,
			string_fg_color, string_bg_color);
		stream_ << str;
	}

	void print_error(const char* prefix, const std::system_error& e)
	{
		color_changer changer(stream_, color_provider_,
			error_fg_color, error_bg_color);
		error_stream_ << prefix << e.code() << ", " << e.what() << "\n\n";
	}

	void print_error(const char* prefix, const std::exception& e)
	{
		color_changer changer(stream_, color_provider_,
			error_fg_color, error_bg_color);
		error_stream_ << prefix << e.what() << "\n\n";
	}

	void print_with_color(const char* text, color foreground, color background, std::size_t width)
	{
		{
			color_changer changer(stream_, color_provider_, foreground, background);
			stream_ << text;
		}

		auto len = std::strlen(text);
		while (len < width)
		{
			stream_ << ' ';
			++len;
		}
	}

	template<typename T>
	void print_type()
	{
		using type = std::remove_cvref_t<T>;
		using base_type = std::remove_all_extents_t<type>;
		static constexpr std::size_t width = 6;
		if constexpr (std::is_same_v<base_type, std::uint8_t>)
			print_with_color("BYTE", type_name_fg_color, type_name_bg_color, width);
		else if constexpr (std::is_same_v<base_type, std::uint16_t>)
			print_with_color("WORD", type_name_fg_color, type_name_bg_color, width);
		else if constexpr (std::is_same_v<base_type, std::uint32_t>)
			print_with_color("DWORD", type_name_fg_color, type_name_bg_color, width);
		else if constexpr (std::is_same_v<base_type, std::uint64_t>)
			print_with_color("QWORD", type_name_fg_color, type_name_bg_color, width);
		else if constexpr (std::is_same_v<base_type, std::int8_t>)
			print_with_color("INT8", type_name_fg_color, type_name_bg_color, width);
		else if constexpr (std::is_same_v<base_type, std::int16_t>)
			print_with_color("INT16", type_name_fg_color, type_name_bg_color, width);
		else if constexpr (std::is_same_v<base_type, std::int32_t>)
			print_with_color("INT32", type_name_fg_color, type_name_bg_color, width);
		else if constexpr (std::is_same_v<base_type, std::int64_t>)
			print_with_color("INT64", type_name_fg_color, type_name_bg_color, width);
		else
			print_with_color("", type_name_fg_color, type_name_bg_color, width);
	}

	void print_structure_name(const char* name)
	{
		color_changer changer(stream_, color_provider_,
			struct_name_fg_color, struct_name_bg_color);
		stream_ << name;
	}

	void print_field_name(const char* name)
	{
		if (!name)
			return;

		color_changer changer(stream_, color_provider_,
			field_name_fg_color, field_name_bg_color);
		stream_ << name;
	}

	template<typename Struct, std::size_t N>
	void print_structure(const char* name,
		const Struct& obj, const std::array<value_info, N>& infos,
		std::size_t max_field_offset = (std::numeric_limits<std::size_t>::max)())
	{
		if (name)
		{
			print_structure_name(name);
			stream_ << '\n';
		}

		std::size_t max_name_length = 0;
		std::size_t field_index = 0;
		std::array<std::string, N> formatted_names;
		format_array_names(obj.get(), infos, max_name_length, formatted_names, field_index);

		field_index = 0;
		std::size_t abs_offset = obj.absolute_offset();
		std::size_t rel_offset = obj.relative_offset();
		print_structure_impl(obj.get(), infos, formatted_names, max_name_length,
			max_field_offset, field_index, abs_offset, rel_offset);
		stream_ << '\n';
	}

	void print_errors(const pe_bliss::detail::error_list& errors)
	{
		if (errors.get_errors().empty())
			return;

		error_stream_ << "Errors:";
		for (auto error : errors.get_errors())
		{
			error_stream_ << ' ';

			color_changer changer(stream_, color_provider_,
				error_fg_color, error_bg_color);
			error_stream_ << error.category().name() << ": " << error.message();
		}

		error_stream_ << "\n\n";
	}

	template<std::size_t MaxSize>
	void print_bytes(const char* structure_name,
		const pe_bliss::detail::packed_byte_array<MaxSize>& data,
		std::size_t max_length = 16 * 10)
	{
		if (!data.physical_size())
			return;

		buffers::input_memory_buffer buf(data.value().data(), data.physical_size());
		buf.set_absolute_offset(data.absolute_offset());
		buf.set_relative_offset(data.relative_offset());
		print_bytes(structure_name, buf, max_length);
	}

	void print_bytes(const char* structure_name,
		const pe_bliss::detail::packed_byte_vector& data,
		std::size_t max_length = 16 * 10)
	{
		if (!data.physical_size())
			return;

		buffers::input_memory_buffer buf(data.value().data(), data.physical_size());
		buf.set_absolute_offset(data.absolute_offset());
		buf.set_relative_offset(data.relative_offset());
		print_bytes(structure_name, buf, max_length);
	}

	void print_bytes(const char* name,
		buffers::input_buffer_interface& buf, std::size_t max_length = 16 * 10)
	{
		if (name)
		{
			print_structure_name(name);
			stream_ << '\n';
		}

		auto buf_length = buf.size() - buf.rpos();
		auto length = (std::min)(max_length, buf_length);
		while (length)
		{
			print_offsets(buf);
			stream_ << "    ";

			color_changer changer(stream_, color_provider_,
				bytes_fg_color, bytes_bg_color);

			while (length)
			{
				std::byte data{};
				buf.read(1u, &data);
				stream_ << std::hex << std::setw(2) << std::setfill('0')
					<< std::to_integer<std::uint32_t>(data) << ' ';
				--length;
			}

			stream_ << '\n';
		}

		if (buf_length > max_length)
			stream_ << "...\n";
	}

public:
	[[nodiscard]]
	std::ostream& get_stream() noexcept
	{
		return stream_;
	}

	[[nodiscard]]
	std::ostream& get_error_stream() noexcept
	{
		return error_stream_;
	}

	[[nodiscard]]
	color_provider_interface& get_color_provider() noexcept
	{
		return color_provider_;
	}

public:
	static constexpr std::size_t fixed_left_padding = 28;

public:
	color value_fg_color = color::yellow;
	color value_bg_color = color::unchanged;
	color absolute_fg_offset_color = color::lightcyan;
	color absolute_bg_offset_color = color::unchanged;
	color relative_fg_offset_color = color::lightmagenta;
	color relative_bg_offset_color = color::unchanged;
	color flags_fg_color = color::lightblue;
	color flags_bg_color = color::unchanged;
	color string_fg_color = color::white;
	color string_bg_color = color::unchanged;
	color error_fg_color = color::yellow;
	color error_bg_color = color::red;
	color struct_name_fg_color = color::white;
	color struct_name_bg_color = color::unchanged;
	color field_name_fg_color = color::lightyellow;
	color field_name_bg_color = color::unchanged;
	color type_name_fg_color = color::white;
	color type_name_bg_color = color::unchanged;
	color bytes_fg_color = color::lightgreen;
	color bytes_bg_color = color::unchanged;

private:
	template<typename Struct, typename NameInfo, typename FormattedNames>
	void print_structure_impl(const Struct& obj,
		const NameInfo& infos, const FormattedNames& formatted_names,
		std::size_t max_name_length, std::size_t& max_field_offset,
		std::size_t& field_index, std::size_t& abs_offset, std::size_t& rel_offset)
	{
		boost::pfr::for_each_field(obj, [this, &infos, &field_index, &formatted_names,
			&abs_offset, &rel_offset, &obj, max_name_length, &max_field_offset] (const auto& value) {
			if constexpr (std::is_class_v<std::remove_cvref_t<decltype(value)>>)
			{
				print_structure_impl(value, infos, formatted_names, max_name_length,
					max_field_offset, field_index, abs_offset, rel_offset);
			}
			else
			{
				if (max_field_offset < sizeof(value))
					max_field_offset = 0u;

				if (!max_field_offset)
					return;

				while (!infos[field_index].name)
					++field_index;

				const auto& info = infos.at(field_index);

				print_type<decltype(value)>();

				print_offsets(abs_offset, rel_offset);

				stream_ << std::setw(0);
				stream_ << "  ";
				print_field_name(formatted_names.at(field_index).c_str());
				stream_ << "  ";

				auto remaining_spaces = max_name_length - formatted_names.at(field_index).size();
				while (remaining_spaces--)
					stream_ << ' ';

				print_value(info, value, max_name_length + fixed_left_padding);
				abs_offset += sizeof(value);
				rel_offset += sizeof(value);
				max_field_offset -= sizeof(value);
				++field_index;
			}
		});
	}

	template<typename Struct, std::size_t N>
	static void format_array_names(const Struct& obj,
		const std::array<value_info, N>& infos, std::size_t& max_name_length,
		std::array<std::string, N>& formatted_names, std::size_t& field_index)
	{
		boost::pfr::for_each_field(obj, [&infos, &field_index,
			&max_name_length, &formatted_names] (const auto& value) {
			if constexpr (std::is_class_v<std::remove_cvref_t<decltype(value)>>)
			{
				format_array_names(value, infos, max_name_length, formatted_names, field_index);
			}
			else
			{
				while (!infos.at(field_index).name)
					++field_index;

				const auto& info = infos.at(field_index);
				formatted_names.at(field_index) = info.name;

				using type = std::remove_cvref_t<decltype(value)>;
				format_array_extents<type>(formatted_names.at(field_index));

				max_name_length = (std::max)(max_name_length, formatted_names.at(field_index).size());
				++field_index;
			}
		});
	}

private:
	color_provider_interface& color_provider_;
	std::ostream& stream_;
	std::ostream& error_stream_;
};
