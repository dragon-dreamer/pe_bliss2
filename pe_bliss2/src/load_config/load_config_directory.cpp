#include "pe_bliss2/load_config/load_config_directory.h"

#include <array>
#include <bit>
#include <system_error>
#include <tuple>

#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/pe_error.h"

namespace
{

struct load_config_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "load_config";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::load_config::load_config_errc;

		switch (static_cast<pe_bliss::load_config::load_config_errc>(ev))
		{
		case unknown_load_config_version:
			return "Unknown load config version";
		case invalid_stride_value:
			return "Invalid guard CF stride value";
		case invalid_page_relative_offset:
			return "Invalid page relative offset";
		case invalid_iat_index:
			return "Invalid IAT index";
		case invalid_size_value:
			return "Invalid size value for ARM64X dynamic relocation";
		case invalid_meta_value:
			return "Invalid meta value for ARM64X dynamic relocation";
		case invalid_instr_size:
			return "Invalid instr_size";
		case invalid_disp_offset:
			return "Invalid disp_offset";
		case invalid_disp_size:
			return "Invalid disp_size";
		default:
			return {};
		}
	}
};

const load_config_error_category load_config_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::load_config;

template<typename Descriptor>
constexpr std::array field_offsets{
	detail::packed_reflection::get_field_offset<&Descriptor::structured_exceptions>(),
	detail::packed_reflection::get_field_offset<&Descriptor::cf_guard>(),
	detail::packed_reflection::get_field_offset<&Descriptor::code_integrity>(),
	detail::packed_reflection::get_field_offset<&Descriptor::cf_guard_ex>(),
	detail::packed_reflection::get_field_offset<&Descriptor::hybrid_pe>(),
	detail::packed_reflection::get_field_offset<&Descriptor::rf_guard>(),
	detail::packed_reflection::get_field_offset<&Descriptor::rf_guard_ex>(),
	detail::packed_reflection::get_field_offset<&Descriptor::enclave>(),
	detail::packed_reflection::get_field_offset<&Descriptor::volatile_metadata>(),
	detail::packed_reflection::get_field_offset<&Descriptor::guard_exception_handling>(),
	detail::packed_reflection::get_field_offset<&Descriptor::extended_flow_guard>(),
	detail::packed_reflection::get_field_offset<&Descriptor::mode>(),
	detail::packed_reflection::get_field_offset<&Descriptor::memcpy_function_pointer>()
};

constexpr std::array versions{
	version::base,
	version::seh,
	version::cf_guard,
	version::code_integrity,
	version::cf_guard_ex,
	version::hybrid_pe,
	version::rf_guard,
	version::rf_guard_ex,
	version::enclave,
	version::volatile_metadata,
	version::eh_guard,
	version::xf_guard,
	version::cast_guard
};

static_assert(std::tuple_size_v<decltype(field_offsets<detail::load_config::image_load_config_directory32>)>
	== std::tuple_size_v<decltype(versions)>);

} //namespace

namespace pe_bliss::load_config
{

std::error_code make_error_code(load_config_errc e) noexcept
{
	return { static_cast<int>(e), load_config_error_category_instance };
}

template<typename Descriptor, typename... Bases>
version load_config_directory_impl<Descriptor, Bases...>::get_version() const noexcept
{
	auto size = get_descriptor_size();
	for (std::size_t i = 0; i != field_offsets<Descriptor>.size(); ++i)
	{
		if (size <= field_offsets<Descriptor>[i])
			return versions[i];
	}
	return version::memcpy_guard;
}

template<typename Descriptor, typename... Bases>
bool load_config_directory_impl<Descriptor, Bases...>::is_version_at_least(
	version ver) const noexcept
{
	return static_cast<std::size_t>(get_version()) >= static_cast<std::size_t>(ver);
}

template<typename Descriptor, typename... Bases>
bool load_config_directory_impl<Descriptor, Bases...>::version_exactly_matches() const noexcept
{
	auto size = get_descriptor_size();
	if (size == load_config_directory_impl<Descriptor, Bases...>::descriptor_type::packed_size)
		return true;
	for (std::size_t i = 0; i != field_offsets<Descriptor>.size(); ++i)
	{
		if (size == field_offsets<Descriptor>[i])
			return true;
	}
	return false;
}

template<typename Descriptor, typename... Bases>
void load_config_directory_impl<Descriptor, Bases...>::set_version(version ver)
{
	if (ver == version::memcpy_guard)
	{
		size_ = load_config_directory_impl<Descriptor, Bases...>::descriptor_type::packed_size
			+ size_.packed_size;
		return;
	}

	for (std::size_t i = 0; i != versions.size(); ++i)
	{
		if (ver == versions[i])
		{
			size_ = static_cast<std::uint32_t>(field_offsets<Descriptor>[i])
				+ size_.packed_size;
			return;
		}
	}

	throw pe_error(load_config_errc::unknown_load_config_version);
}

template<typename Descriptor, typename... Bases>
std::uint8_t load_config_directory_impl<Descriptor,
	Bases...>::get_guard_cf_function_table_stride() const noexcept
{
	return static_cast<std::uint8_t>((this->descriptor_->cf_guard.guard_flags
		& detail::load_config::guard_flags::cf_function_table_size_mask)
		>> detail::load_config::guard_flags::cf_function_table_size_shift);
}

template<typename Descriptor, typename... Bases>
void load_config_directory_impl<Descriptor,
	Bases...>::set_guard_cf_function_table_stride(std::uint8_t stride)
{
	if (stride > (detail::load_config::guard_flags::cf_function_table_size_mask
		>> detail::load_config::guard_flags::cf_function_table_size_shift))
	{
		throw pe_error(load_config_errc::invalid_stride_value);
	}

	this->descriptor_->cf_guard.guard_flags |= (stride
		<< detail::load_config::guard_flags::cf_function_table_size_shift);
}

std::uint8_t arm64x_dynamic_relocation_sized_base::get_size() const noexcept
{
	return static_cast<std::uint8_t>(1u << (get_meta() >> 2u));
}

void arm64x_dynamic_relocation_sized_base::set_size(std::uint8_t size)
{
	if (!std::has_single_bit(size) || size > 8u)
		throw pe_error(load_config_errc::invalid_size_value);

	get_relocation()->metadata &= ~0xc000u;
	get_relocation()->metadata |= std::bit_width(size - 1u) << 14u;
}

template<typename... Bases>
typename arm64x_dynamic_relocation_add_delta_base<Bases...>::multiplier
	arm64x_dynamic_relocation_add_delta_base<Bases...>::get_multiplier() const noexcept
{
	return (get_relocation()->metadata & 0x8000u)
		? multiplier::multiplier_8 : multiplier::multiplier_4;
}

template<typename... Bases>
typename arm64x_dynamic_relocation_add_delta_base<Bases...>::sign
	arm64x_dynamic_relocation_add_delta_base<Bases...>::get_sign() const noexcept
{
	return (get_relocation()->metadata & 0x4000u) ? sign::minus : sign::plus;
}

template<typename... Bases>
void arm64x_dynamic_relocation_add_delta_base<Bases...>::set_multiplier(
	multiplier value) noexcept
{
	if (value == multiplier::multiplier_4)
		get_relocation()->metadata &= ~0x8000u;
	else
		get_relocation()->metadata |= 0x8000u;
}

template<typename... Bases>
void arm64x_dynamic_relocation_add_delta_base<Bases...>::set_sign(
	sign value) noexcept
{
	if (value == sign::plus)
		get_relocation()->metadata &= ~0x4000u;
	else
		get_relocation()->metadata |= 0x4000u;
}

template<typename... Bases>
std::int32_t arm64x_dynamic_relocation_add_delta_base<Bases...>::get_delta() const noexcept
{
	return static_cast<std::int32_t>(get_value().get())
		* (get_multiplier() == multiplier::multiplier_8 ? 8 : 4)
		* (get_sign() == sign::minus ? -1 : 1);
}

template class load_config_directory_impl<detail::load_config::image_load_config_directory32>;
template class load_config_directory_impl<detail::load_config::image_load_config_directory64>;
template class load_config_directory_impl<detail::load_config::image_load_config_directory32, error_list>;
template class load_config_directory_impl<detail::load_config::image_load_config_directory64, error_list>;

const char* version_to_min_required_windows_version(version value) noexcept
{
	using enum version;
	switch (value)
	{
	case base: return "Windows 2000";
	case seh: return "Windows XP";
	case cf_guard: return "Windows 10 Threshold 1 (1507)";
	case code_integrity: return "Windows 10 Threshold 2 (1511) (preview 9879)";
	case cf_guard_ex: return "Windows 10 Redstone 1 (1607) (build 14286)";
	case hybrid_pe: return "Windows 10 Redstone 1 (1607) (build 14383)";
	case rf_guard: return "Windows 10 Redstone 2 (1703) (build 14901)";
	case rf_guard_ex: return "Windows 10 Redstone 2 (1703) (build 15002)";
	case enclave: return "Windows 10 Redstone 3 (1709) (build 16237)";
	case volatile_metadata: return "Windows 10 Redstone 4 (1803)";
	case eh_guard: return "Windows 10 Redstone 5 (1809)";
	case xf_guard: return "Windows 10 Vibranium 3 (21H1)";
	case cast_guard:
		return "Windows 10 Vibranium 4 (21H2)";
	case memcpy_guard: [[fallthrough]];
	default: return "Windows 10 Vibranium 4 (21H2) or later";
	}
}

template<typename... Bases>
std::uint32_t chpe_x86_metadata_base<Bases...>::get_metadata_size() const noexcept
{
	auto version = version_.get();

	if (version <= 1u)
	{
		return detail::packed_reflection::get_field_offset<
			&detail::load_config::image_chpe_metadata_x86::compiler_iat_pointer>();
	}

	if (version == 2u)
	{
		return detail::packed_reflection::get_field_offset<
			&detail::load_config::image_chpe_metadata_x86::wow_a64_rdtsc_function_pointer>();
	}

	return detail::packed_reflection::get_type_size<
		detail::load_config::image_chpe_metadata_x86>();
}

void arm64x_dynamic_relocation_base::set_meta(std::uint8_t meta)
{
	if (meta > 0xfu)
		throw pe_error(load_config_errc::invalid_meta_value);

	get_relocation()->metadata &= ~0xf000u;
	get_relocation()->metadata |= meta << 12u;
}

void epilogue_branch_descriptor::set_instr_size(std::uint8_t instr_size)
{
	if (instr_size > 0xfu)
		throw pe_error(load_config_errc::invalid_instr_size);

	descriptor_.get() &= ~0xfu;
	descriptor_.get() |= instr_size;
}

void epilogue_branch_descriptor::set_disp_offset(std::uint8_t disp_offset)
{
	if (disp_offset > 0xfu)
		throw pe_error(load_config_errc::invalid_disp_offset);

	descriptor_.get() &= ~0xf0u;
	descriptor_.get() |= (static_cast<std::uint16_t>(disp_offset) << 4u);
}

void epilogue_branch_descriptor::set_disp_size(std::uint8_t disp_size)
{
	if (disp_size > 0xfu)
		throw pe_error(load_config_errc::invalid_disp_size);

	descriptor_.get() &= ~0xf00u;
	descriptor_.get() |= (static_cast<std::uint16_t>(disp_size) << 8u);
}

template class chpe_x86_metadata_base<>;
template class chpe_x86_metadata_base<error_list>;
template class arm64x_dynamic_relocation_add_delta_base<>;
template class arm64x_dynamic_relocation_add_delta_base<error_list>;

} //namespace pe_bliss::load_config
