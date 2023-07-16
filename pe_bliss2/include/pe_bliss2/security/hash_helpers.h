#pragma once

#include <cstddef>
#include <optional>
#include <system_error>
#include <type_traits>

#include "buffers/input_buffer_interface.h"

#include "cryptopp/cryptlib.h"

namespace pe_bliss::security
{

enum class hash_helpers_errc
{
	unable_to_read_data
};

class page_hash_state final
{
public:
	explicit page_hash_state(CryptoPP::HashTransformation& hash,
		std::size_t page_size);

	void update(const CryptoPP::byte* data, std::size_t offset, std::size_t size);

	void next_page();

	void add_skipped_bytes(std::size_t skipped_bytes) noexcept;

	[[nodiscard]]
	std::vector<std::byte> get_page_hashes() && noexcept;

	void reserve(std::size_t size);

private:
	std::byte* add_blank_page(std::size_t offset);

private:
	CryptoPP::HashTransformation& hash_;
	std::vector<std::byte> page_hashes_;
	const std::size_t page_size_;
	std::size_t current_size_{};
	std::size_t next_page_offset_{};
	std::size_t skipped_bytes_{};
	std::size_t last_offset_{};
};

std::error_code make_error_code(hash_helpers_errc) noexcept;

void update_hash(buffers::input_buffer_interface& buf, std::size_t from, std::size_t to,
	CryptoPP::HashTransformation& hash);

void update_hash(buffers::input_buffer_interface& buf, std::size_t from, std::size_t to,
	page_hash_state& state);

void update_hash(buffers::input_buffer_interface& buf, std::size_t from, std::size_t to,
	CryptoPP::HashTransformation& hash, std::optional<page_hash_state>& state);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::hash_helpers_errc> : true_type {};
} //namespace std
