#include "pe_bliss2/security/buffer_hash.h"

#include "buffers/input_memory_buffer.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "cryptopp/md5.h"
#include "cryptopp/sha.h"

#include "pe_bliss2/security/hash_helpers.h"
#include "pe_bliss2/pe_error.h"

namespace
{
struct buffer_hash_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "buffer_hash";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::buffer_hash_errc;
		switch (static_cast<pe_bliss::security::buffer_hash_errc>(ev))
		{
		case unsupported_hash_algorithm:
			return "Unsupported hash algorithm";
		default:
			return {};
		}
	}
};

const buffer_hash_error_category buffer_hash_error_category_instance;
} //namespace

namespace pe_bliss::security
{

std::error_code make_error_code(buffer_hash_errc e) noexcept
{
	return { static_cast<int>(e), buffer_hash_error_category_instance };
}

namespace
{
template<typename Algorithm>
std::vector<std::byte> calculate_hash_impl(buffers::input_buffer_interface& buffer)
{
	Algorithm hash;
	update_hash(buffer, 0, buffer.physical_size(), hash);
	std::vector<std::byte> result;
	result.resize(hash.DigestSize());
	hash.Final(reinterpret_cast<CryptoPP::byte*>(result.data()));
	return result;
}

template<typename Algorithm>
std::vector<std::byte> calculate_hash_impl(std::span<const std::span<const std::byte>> buffers)
{
	Algorithm hash;
	for (const auto& part : buffers)
	{
		buffers::input_memory_buffer buf(part.data(), part.size());
		update_hash(buf, 0, buf.physical_size(), hash);
	}
	std::vector<std::byte> result;
	result.resize(hash.DigestSize());
	hash.Final(reinterpret_cast<CryptoPP::byte*>(result.data()));
	return result;
}

template<typename T>
std::vector<std::byte> calculate_hash_impl(digest_algorithm algorithm,
	T&& buffer)
{
	switch (algorithm)
	{
	case digest_algorithm::md5:
		return calculate_hash_impl<CryptoPP::Weak::MD5>(buffer);
	case digest_algorithm::sha1:
		return calculate_hash_impl<CryptoPP::SHA1>(buffer);
	case digest_algorithm::sha256:
		return calculate_hash_impl<CryptoPP::SHA256>(buffer);
	default:
		throw pe_error(buffer_hash_errc::unsupported_hash_algorithm);
	}
}
} //namespace

std::vector<std::byte> calculate_hash(digest_algorithm algorithm,
	buffers::input_buffer_interface& buffer)
{
	return calculate_hash_impl(algorithm, buffer);
}

std::vector<std::byte> calculate_hash(digest_algorithm algorithm,
	std::span<const std::span<const std::byte>> buffers)
{
	return calculate_hash_impl(algorithm, buffers);
}

} //namespace pe_bliss::security
