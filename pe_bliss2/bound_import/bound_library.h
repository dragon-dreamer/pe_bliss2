#pragma once

#include <cstdint>
#include <list>

#include "pe_bliss2/detail/bound_import/image_bound_import_descriptor.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/packed_struct.h"

namespace pe_bliss::detail::bound_import
{

template<typename Descriptor>
class bound_library_base
{
public:
	using packed_descriptor_type = packed_struct<Descriptor>;

public:
	[[nodiscard]]
	const packed_descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	packed_descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const packed_c_string& get_library_name() const noexcept
	{
		return library_name_;
	}

	[[nodiscard]]
	packed_c_string& get_library_name() noexcept
	{
		return library_name_;
	}

private:
	packed_descriptor_type descriptor_;
	packed_c_string library_name_;
};

} //namespace pe_bliss::detail::bound_import

namespace pe_bliss::bound_import
{

using bound_library_reference = detail::bound_import::bound_library_base<
	detail::bound_import::image_bound_forwarder_ref>;

class bound_library_reference_details
	: public bound_library_reference
	, public error_list
{
};

template<typename Reference>
class bound_library_base : public detail::bound_import::bound_library_base<
	detail::bound_import::image_bound_import_descriptor>
{
public:
	using reference_list = std::list<Reference>;

public:
	[[nodiscard]]
	const reference_list& get_references() const noexcept
	{
		return references_;
	}

	[[nodiscard]]
	reference_list& get_references() noexcept
	{
		return references_;
	}

private:
	reference_list references_;
};

using bound_library = bound_library_base<bound_library_reference>;
class bound_library_details
	: public bound_library_base<bound_library_reference_details>
	, public error_list
{
};

using bound_library_list = std::list<bound_library>;
using bound_library_details_list = std::list<bound_library_details>;

} //namespace pe_bliss::bound_import
