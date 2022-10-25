#include <bit>

#include "pe_bliss2/pe_error.h"

namespace pe_bliss::tls
{
template<typename Directory, typename TlsCallback>
const buffers::ref_buffer& tls_directory_base<Directory,
	TlsCallback>::get_raw_data() const noexcept
{
	return raw_data_;
}

template<typename Directory, typename TlsCallback>
buffers::ref_buffer& tls_directory_base<Directory, TlsCallback>::get_raw_data() noexcept
{
	return raw_data_;
}

template<typename Directory, typename TlsCallback>
const typename tls_directory_base<Directory, TlsCallback>::callback_list_type&
	tls_directory_base<Directory, TlsCallback>::get_callbacks() const& noexcept
{
	return callbacks_;
}

template<typename Directory, typename TlsCallback>
typename tls_directory_base<Directory, TlsCallback>::callback_list_type&
	tls_directory_base<Directory, TlsCallback>::get_callbacks() & noexcept
{
	return callbacks_;
}

template<typename Directory, typename TlsCallback>
typename tls_directory_base<Directory, TlsCallback>::callback_list_type
	tls_directory_base<Directory, TlsCallback>::get_callbacks() && noexcept
{
	return std::move(callbacks_);
}

template<typename Directory, typename TlsCallback>
std::uint32_t tls_directory_base<Directory, TlsCallback>
	::get_max_type_alignment() const noexcept
{
	std::uint32_t alignment = this->descriptor_->characteristics & 0xf00000u;
	if (!alignment)
		return 0;

	return static_cast<std::uint8_t>(1u << ((alignment >> 20u) - 1u));
}

template<typename Directory, typename TlsCallback>
void tls_directory_base<Directory, TlsCallback>
	::set_max_type_alignment(std::uint32_t alignment)
{
	if (alignment > (1u << 0xeu))
		throw pe_error(tls_directory_errc::too_big_alignment_value);

	if (!std::has_single_bit(alignment))
		throw pe_error(tls_directory_errc::invalid_alignment_value);

	this->descriptor_->characteristics &= ~0xf00000u;
	if (!alignment)
		return;

	this->descriptor_->characteristics |= std::bit_width(alignment) << 20u;
}
} //namespace pe_bliss::tls
