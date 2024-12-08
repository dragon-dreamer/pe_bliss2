#pragma once

#include <cstdint>
#include <utility>
#include <system_error>
#include <type_traits>
#include <variant>
#include <vector>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/detail/trustlet/image_policy_metadata.h"
#include "pe_bliss2/packed_c_string.h"

namespace pe_bliss::trustlet
{

enum class trustlet_policy_errc
{
	unsupported_version = 1,
	invalid_string_address,
	unable_to_read_value,
	unsupported_entry_type
};

std::error_code make_error_code(trustlet_policy_errc) noexcept;

enum class image_policy_entry_type
{
    none = detail::trustlet::image_policy_entry_type_none,
    boolean = detail::trustlet::image_policy_entry_type_boolean,
    int8 = detail::trustlet::image_policy_entry_type_int8,
    uint8 = detail::trustlet::image_policy_entry_type_uint8,
    int16 = detail::trustlet::image_policy_entry_type_int16,
    uint16 = detail::trustlet::image_policy_entry_type_uint16,
    int32 = detail::trustlet::image_policy_entry_type_int32,
    uint32 = detail::trustlet::image_policy_entry_type_uint32,
    int64 = detail::trustlet::image_policy_entry_type_int64,
    uint64 = detail::trustlet::image_policy_entry_type_uint64,
    ansi_string = detail::trustlet::image_policy_entry_type_ansi_string,
    unicode_string = detail::trustlet::image_policy_entry_type_unicode_string,
    overriden = detail::trustlet::image_policy_entry_type_overriden
};

enum class image_policy_id
{
    none = detail::trustlet::image_policy_id_none,
    etw = detail::trustlet::image_policy_id_etw,
    debug = detail::trustlet::image_policy_id_debug,
    crash_dump = detail::trustlet::image_policy_id_crash_dump,
    crash_dump_key = detail::trustlet::image_policy_id_crash_dump_key,
    crash_dump_key_guid = detail::trustlet::image_policy_id_crash_dump_key_guid,
    parent_sd = detail::trustlet::image_policy_id_parent_sd,
    parent_sd_rev = detail::trustlet::image_policy_id_parent_sd_rev,
    svn = detail::trustlet::image_policy_id_svn,
    device_id = detail::trustlet::image_policy_id_device_id,
    capability = detail::trustlet::image_policy_id_capability,
    scenario_id = detail::trustlet::image_policy_id_scenario_id
};

class [[nodiscard]] trustlet_policy_entry
	: public detail::packed_struct_base<detail::trustlet::image_policy_entry>
{
public:
	using value_type = std::variant<
		std::monostate,
		bool,
		std::int8_t,
		std::uint8_t,
		std::int16_t,
		std::uint16_t,
		std::int32_t,
		std::uint32_t,
		std::int64_t,
		std::uint64_t,
		packed_c_string,
		packed_utf16_c_string>;

public:
	[[nodiscard]] const value_type& get_value() const noexcept
	{
		return value_;
	}
	[[nodiscard]] value_type& get_value() noexcept
	{
		return value_;
	}

    [[nodiscard]] image_policy_entry_type get_type() const noexcept
    {
        return static_cast<image_policy_entry_type>(get_descriptor()->type);
    }

	[[nodiscard]] image_policy_id get_policy() const noexcept
	{
		return static_cast<image_policy_id>(get_descriptor()->policy_id);
	}

private:
	value_type value_;
};

class [[nodiscard]] trustlet_policy_entry_details
	: public trustlet_policy_entry
	, public error_list
{
};

template<typename Entry>
class [[nodiscard]] trustlet_policy_metadata_base
	: public detail::packed_struct_base<detail::trustlet::image_policy_metadata>
{
public:
	using policy_entry_type = Entry;
	using policy_entry_list_type = std::vector<policy_entry_type>;

public:
	[[nodiscard]] const policy_entry_list_type& get_entries() const noexcept
	{
		return entries_;
	}
	[[nodiscard]] policy_entry_list_type& get_entries() noexcept
	{
		return entries_;
	}

private:
	policy_entry_list_type entries_;
};

using trustlet_policy_metadata = trustlet_policy_metadata_base<trustlet_policy_entry>;
class [[nodiscard]] trustlet_policy_metadata_details
	: public trustlet_policy_metadata_base<trustlet_policy_entry_details>
	, public error_list
{
};

} //namespace pe_bliss::trustlet

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::trustlet::trustlet_policy_errc> : true_type {};
} //namespace std
