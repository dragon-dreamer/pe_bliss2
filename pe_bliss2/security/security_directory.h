#pragma once

#include <utility>
#include <vector>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/detail/security/image_security_directory.h"
#include "pe_bliss2/error_list.h"

namespace pe_bliss::security
{

enum class directory_revision
{
	revision_1_0 = detail::security::win_cert_revision_1_0,
	revision_2_0 = detail::security::win_cert_revision_2_0
};

enum class certificate_type
{
	x509 = detail::security::win_cert_type_x509,
	pkcs_signed_data = detail::security::win_cert_type_pkcs_signed_data,
	ts_stack_signed = detail::security::win_cert_type_ts_stack_signed,
	pkcs1_sign = detail::security::win_cert_type_pkcs1_sign
};

template<typename... Bases>
class [[nodiscard]] certificate_entry_base
	: public detail::packed_struct_base<detail::security::win_certificate>
	, public Bases...
{
public:
	[[nodiscard]]
	directory_revision get_revision() const noexcept
	{
		return static_cast<directory_revision>(descriptor_->revision);
	}
	
	[[nodiscard]]
	certificate_type get_type() const noexcept
	{
		return static_cast<certificate_type>(descriptor_->certificate_type);
	}

	[[nodiscard]]
	buffers::ref_buffer get_certificate() && noexcept
	{
		return std::move(certificate_);
	}

	[[nodiscard]]
	const buffers::ref_buffer& get_certificate() const& noexcept
	{
		return certificate_;
	}

	[[nodiscard]]
	buffers::ref_buffer& get_certificate() & noexcept
	{
		return certificate_;
	}

private:
	buffers::ref_buffer certificate_;
};

using certificate_entry = certificate_entry_base<>;
using certificate_entry_details = certificate_entry_base<error_list>;

template<typename... Bases>
class [[nodiscard]] security_directory_base
	: public Bases...
{
public:
	using certificate_entry_list_type = std::vector<certificate_entry_base<Bases...>>;
	
public:
	[[nodiscard]]
	certificate_entry_list_type get_entries() && noexcept
	{
		return std::move(certificate_entries_);
	}

	[[nodiscard]]
	const certificate_entry_list_type& get_entries() const& noexcept
	{
		return certificate_entries_;
	}

	[[nodiscard]]
	certificate_entry_list_type& get_entries() & noexcept
	{
		return certificate_entries_;
	}

private:
	certificate_entry_list_type certificate_entries_;
};

using security_directory = security_directory_base<>;
using security_directory_details = security_directory_base<error_list>;

} //namespace pe_bliss::security
