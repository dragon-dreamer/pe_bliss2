#include "pe_bliss2/debug/debug_directory.h"

#include <cstddef>
#include <limits>
#include <string>
#include <system_error>

#include "buffers/input_buffer_section.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "utilities/generic_error.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{

struct debug_directory_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "debug_directory";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::debug::debug_directory_errc;
		switch (static_cast<pe_bliss::debug::debug_directory_errc>(ev))
		{
		case unsupported_type:
			return "Unsupported debug directory type";
		case unable_to_deserialize:
			return "Unable to deserialize debug directory";
		case too_long_hash:
			return "Too long hash";
		case unable_to_deserialize_hash:
			return "Unable to deserialize hash value";
		case unable_to_deserialize_name:
			return "Unable to deserialize name";
		case too_many_symbols:
			return "Too many debug symbols";
		case too_many_entries:
			return "Too many debug entries";
		case number_of_symbols_mismatch:
			return "Mismatching file header and COFF header number of symbols";
		case pointer_to_symbol_table_mismatch:
			return "Mismatching file header and COFF header pointer to symbol table";
		case unable_to_deserialize_symbols:
			return "Unable to deserialize COFF symbols";
		case virtual_coff_string_table:
			return "COFF string table is virtual";
		case virtual_pdb_data:
			return "Compressed PDB data is virtual";
		default:
			return {};
		}
	}
};

const debug_directory_category debug_directory_category_instance;

} //namespace

namespace pe_bliss::debug
{

namespace
{

template<typename Dir>
Dir parse_directory(buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data)
{
	Dir result;
	try
	{
		result.get_descriptor().deserialize(wrapper, allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize);
	}
	return result;
}

template<typename Dir>
Dir parse_codeview_pdb(buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data)
{
	wrapper.advance_rpos(-static_cast<std::int32_t>(
		codeview_omf_debug_directory::signature_type::packed_size));
	auto dir = parse_directory<Dir>(wrapper, allow_virtual_data);
	if (dir.has_errors())
		return dir;

	try
	{
		dir.get_pdb_file_name().deserialize(wrapper, allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		dir.add_error(debug_directory_errc::unable_to_deserialize_name);
	}

	return dir;
}

template<typename Dir>
Dir parse_omap_src_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options)
{
	Dir result;
	auto max_entries = options.max_debug_entry_count;
	try
	{
		while (wrapper.rpos() < wrapper.size())
		{
			if (!max_entries--)
			{
				result.add_error(debug_directory_errc::too_many_entries);
				break;
			}

			auto& mapping = result.get_mappings().emplace_back();
			mapping.deserialize(wrapper, options.allow_virtual_data);
		}
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize);
	}
	return result;
}

} //namespace

misc_debug_directory_details parse_misc_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data)
{
	auto result = parse_directory<misc_debug_directory_details>(
		wrapper, allow_virtual_data);
	const auto data_size = result.get_descriptor()->length;
	if (!result.has_errors() && data_size)
	{
		if (!utilities::math::is_aligned<std::uint32_t>(data_size))
			result.add_error(debug_directory_errc::unaligned_data_size);

		try
		{
			if (result.get_descriptor()->unicode)
			{
				result.get_data().emplace<packed_utf16_c_string>().deserialize(
					wrapper, allow_virtual_data, data_size);
			}
			else
			{
				result.get_data().emplace<packed_c_string>().deserialize(
					wrapper, allow_virtual_data, data_size);
			}
		}
		catch (const std::system_error&)
		{
			result.add_error(debug_directory_errc::unable_to_deserialize_name);
		}
	}

	return result;
}

repro_debug_directory_details parse_repro_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data)
{
	repro_debug_directory_details result;
	if (!wrapper.size())
		return result;

	try
	{
		result.get_descriptor().emplace().deserialize(
			wrapper, allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize);
		return result;
	}

	auto hash_size = result.get_descriptor().value()->length;
	if (hash_size > repro_debug_directory_details::max_hash_length)
	{
		result.add_error(debug_directory_errc::too_long_hash);
		return result;
	}

	try
	{
		result.get_hash().emplace().deserialize(wrapper,
			hash_size, allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize_hash);
	}

	return result;
}

spgo_debug_directory_details parse_spgo_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data)
{
	spgo_debug_directory_details result;
	try
	{
		result.get_string().deserialize(wrapper, allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize);
	}
	return result;
}

debug_directory_details::underlying_directory_type
	parse_codeview_directory(buffers::input_buffer_stateful_wrapper& wrapper,
		bool allow_virtual_data)
{
	codeview_omf_debug_directory_details result;
	try
	{
		result.get_signature().deserialize(wrapper, allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize);
		return result;
	}

	switch (result.get_codeview_signature())
	{
	case codeview_signature::pdb2:
		return parse_codeview_pdb<codeview_pdb2_debug_directory_details>(
			wrapper, allow_virtual_data);
	case codeview_signature::pdb7:
		return parse_codeview_pdb<codeview_pdb7_debug_directory_details>(
			wrapper, allow_virtual_data);
	default:
		return result;
	}
}

omap_to_src_debug_directory_details parse_omap_to_src_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options)
{
	return parse_omap_src_directory<
		omap_to_src_debug_directory_details>(wrapper, options);
}

src_to_omap_debug_directory_details parse_src_to_omap_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options)
{
	return parse_omap_src_directory<
		src_to_omap_debug_directory_details>(wrapper, options);
}

fpo_debug_directory_details parse_fpo_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options)
{
	fpo_debug_directory_details result;
	auto max_entries = options.max_debug_entry_count;
	try
	{
		while (wrapper.rpos() < wrapper.size())
		{
			if (!max_entries--)
			{
				result.add_error(debug_directory_errc::too_many_entries);
				break;
			}

			auto& entry = result.get_entries().emplace_back();
			entry.get_descriptor().deserialize(wrapper, options.allow_virtual_data);
		}
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize);
	}
	return result;
}

pogo_debug_directory_details parse_pogo_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options)
{
	auto result = parse_directory<pogo_debug_directory_details>(
		wrapper, options.allow_virtual_data);
	if (result.has_errors())
		return result;

	const auto relative_offset = wrapper.get_buffer()->relative_offset();
	auto max_entries = options.max_debug_entry_count;
	std::int32_t to_advance = 0;
	while (wrapper.rpos() + to_advance < wrapper.size())
	{
		if (!max_entries--)
		{
			result.add_error(debug_directory_errc::too_many_entries);
			break;
		}

		auto& entry = result.get_entries().emplace_back();
		try
		{
			wrapper.advance_rpos(to_advance);
			entry.get_descriptor().deserialize(wrapper, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			result.add_error(debug_directory_errc::unable_to_deserialize);
			break;
		}

		try
		{
			entry.get_name().deserialize(wrapper, options.allow_virtual_data);
			auto non_aligned_offset = relative_offset + wrapper.rpos();
			auto aligned_offset = non_aligned_offset;
			if (!utilities::math::align_up_if_safe(
				aligned_offset, sizeof(std::uint32_t)))
			{
				result.add_error(debug_directory_errc::unable_to_deserialize_name);
				break;
			}

			to_advance = static_cast<std::int32_t>(
				aligned_offset - non_aligned_offset);
		}
		catch (const std::system_error&)
		{
			result.add_error(debug_directory_errc::unable_to_deserialize_name);
			break;
		}
	}
	return result;
}

coff_debug_directory_details parse_coff_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options)
{
	auto result = parse_directory<coff_debug_directory_details>(wrapper,
		options.allow_virtual_data);
	if (result.has_errors())
		return result;

	auto number_of_symbols = result.get_descriptor()->number_of_symbols;
	if (options.file_header)
	{
		const auto& fh_descriptor = options.file_header->get_descriptor().get();
		if (fh_descriptor.number_of_symbols != number_of_symbols)
			result.add_error(debug_directory_errc::number_of_symbols_mismatch);
		if (fh_descriptor.pointer_to_symbol_table
			!= wrapper.get_buffer()->absolute_offset()
				+ result.get_descriptor()->lva_to_first_symbol)
		{
			result.add_error(debug_directory_errc::pointer_to_symbol_table_mismatch);
		}
	}

	try
	{
		if constexpr (sizeof(std::size_t) <= sizeof(number_of_symbols))
		{
			if (number_of_symbols > std::numeric_limits<std::size_t>::max()
				/ coff_symbol::descriptor_type::packed_size)
			{
				throw pe_error(utilities::generic_errc::integer_overflow);
			}
		}

		utilities::safe_uint string_table_pos = result.get_descriptor()->lva_to_first_symbol;
		string_table_pos += number_of_symbols * coff_symbol::descriptor_type::packed_size;
		wrapper.set_rpos(string_table_pos.value());
		result.get_string_table_length().deserialize(wrapper, options.allow_virtual_data);
		result.get_string_table_buffer().deserialize(
			buffers::reduce(wrapper.get_buffer(), wrapper.rpos()
				- coff_debug_directory_details::string_table_length_type::packed_size,
				result.get_string_table_length().get()),
			options.copy_coff_string_table_memory);
		if (!options.allow_virtual_data
			&& result.get_string_table_buffer().data()->virtual_size())
		{
			result.add_error(debug_directory_errc::virtual_coff_string_table);
		}

		wrapper.set_rpos(result.get_descriptor()->lva_to_first_symbol);
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize);
		return result;
	}

	if (number_of_symbols > options.max_coff_symbol_count)
	{
		number_of_symbols = options.max_coff_symbol_count;
		result.add_error(debug_directory_errc::too_many_symbols);
	}

	auto& symbols = result.get_symbols();
	while (number_of_symbols)
	{
		auto& symbol = symbols.emplace_back();
		auto& descriptor = symbol.get_descriptor();
		try
		{
			descriptor.deserialize(wrapper, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			symbols.pop_back();
			result.add_error(debug_directory_errc::unable_to_deserialize_symbols);
			break;
		}

		if (!descriptor->name[0] && !descriptor->name[1]
			&& !descriptor->name[2] && !descriptor->name[3])
		{
			try
			{
				auto& name = symbol.get_name().emplace<packed_c_string>();
				std::uint32_t offset = static_cast<std::uint32_t>(descriptor->name[4])
					| (static_cast<std::uint32_t>(descriptor->name[5]) << 8u)
					| (static_cast<std::uint32_t>(descriptor->name[6]) << 16u)
					| (static_cast<std::uint32_t>(descriptor->name[7]) << 24u);
				buffers::input_buffer_stateful_wrapper string_table_buf(
					result.get_string_table_buffer().data());
				string_table_buf.set_rpos(offset);
				name.deserialize(string_table_buf, options.allow_virtual_data);
			}
			catch (const std::system_error&)
			{
				result.add_error(debug_directory_errc::unable_to_deserialize_name);
			}
		}
		else
		{
			auto& name = symbol.get_name().emplace<std::string>(
				reinterpret_cast<const char*>(descriptor->name.data()),
				descriptor->name.size());
			while (!name.empty() && name.back() == '\0')
				name.pop_back();
		}

		try
		{
			//TODO: support COFF aux symbols
			wrapper.advance_rpos(static_cast<std::int32_t>(descriptor->aux_symbol_number)
				* coff_symbol::descriptor_type::packed_size);
		}
		catch (const std::system_error&)
		{
			result.add_error(debug_directory_errc::unable_to_deserialize_symbols);
			break;
		}

		std::uint32_t symbols_to_skip = 1;
		symbols_to_skip += descriptor->aux_symbol_number;
		if (symbols_to_skip > number_of_symbols)
		{
			if (!result.has_error(debug_directory_errc::too_many_symbols))
				result.add_error(debug_directory_errc::unable_to_deserialize_symbols);
			break;
		}
		
		number_of_symbols -= symbols_to_skip;
	}

	return result;
}

vc_feature_debug_directory_details parse_vc_feature_debug_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data)
{
	return parse_directory<vc_feature_debug_directory_details>(
		wrapper, allow_virtual_data);
}

mpx_debug_directory_details parse_mpx_debug_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data)
{
	return parse_directory<mpx_debug_directory_details>(
		wrapper, allow_virtual_data);
}

ex_dllcharacteristics_debug_directory_details
	parse_ex_dllcharacteristics_debug_directory(
		buffers::input_buffer_stateful_wrapper& wrapper,
		bool allow_virtual_data)
{
	return parse_directory<ex_dllcharacteristics_debug_directory_details>(
		wrapper, allow_virtual_data);
}

pdb_hash_debug_directory_details parse_pdbhash_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data)
{
	pdb_hash_debug_directory_details result;
	try
	{
		result.get_algorithm().deserialize(wrapper, allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize_name);
		return result;
	}

	try
	{
		result.get_hash().deserialize(wrapper, wrapper.size() - wrapper.rpos(),
			allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize_hash);
	}

	return result;
}

mpdb_debug_directory_details parse_mpdb_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options)
{
	auto result = parse_directory<mpdb_debug_directory_details>(wrapper,
		options.allow_virtual_data);
	if (result.has_errors())
		return result;

	try
	{
		result.get_compressed_pdb().deserialize(
			buffers::reduce(wrapper.get_buffer(), wrapper.rpos()),
			options.copy_mpdb_memory);
		if (!options.allow_virtual_data && result.get_compressed_pdb().virtual_size())
			result.add_error(debug_directory_errc::virtual_pdb_data);
	}
	catch (const std::system_error&)
	{
		result.add_error(debug_directory_errc::unable_to_deserialize_symbols);
	}

	return result;
}

std::error_code make_error_code(debug_directory_errc e) noexcept
{
	return { static_cast<int>(e), debug_directory_category_instance };
}

template<typename... Bases>
typename debug_directory_base<Bases...>::underlying_directory_type
	debug_directory_base<Bases...>::get_underlying_directory(
		const debug_directory_parse_options& options) const
{
	using enum debug_directory_type;
	buffers::input_buffer_stateful_wrapper wrapper(raw_data_.data());
	const bool allow_virtual_data = options.allow_virtual_data;
	switch (get_type())
	{
	case iltcg: return iltcg_debug_directory{};
	case vc_feature: return { parse_vc_feature_debug_directory(
		wrapper, allow_virtual_data) };
	case mpx: return { parse_mpx_debug_directory(
		wrapper, allow_virtual_data) };
	case ex_dllcharacteristics: return {
		parse_ex_dllcharacteristics_debug_directory(
			wrapper, allow_virtual_data) };
	case misc: return { parse_misc_directory(wrapper, allow_virtual_data) };
	case repro: return { parse_repro_directory(wrapper, allow_virtual_data) };
	case spgo: return { parse_spgo_directory(wrapper, allow_virtual_data) };
	case codeview: return parse_codeview_directory(wrapper, allow_virtual_data);
	case omap_to_src: return { parse_omap_to_src_directory(wrapper, options) };
	case omap_from_src: return { parse_src_to_omap_directory(wrapper, options) };
	case fpo: return { parse_fpo_directory(wrapper, options) };
	case pogo: return { parse_pogo_directory(wrapper, options) };
	case coff: return { parse_coff_directory(wrapper, options) };
	case pdbhash: return { parse_pdbhash_directory(wrapper,
		options.allow_virtual_data) };
	case mpdb: return { parse_mpdb_directory(wrapper, options) };
	default: break;
	}
	throw pe_error(debug_directory_errc::unsupported_type);
}

template class debug_directory_base<>;
template class debug_directory_base<error_list>;

} //namespace pe_bliss::debug
