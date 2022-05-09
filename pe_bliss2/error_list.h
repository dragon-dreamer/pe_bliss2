#pragma once

#include <system_error>
#include <type_traits>
#include <vector>

namespace pe_bliss
{

namespace detail
{
template<typename ErrorCode>
concept is_convertible_to_error_code = requires(ErrorCode ec) {
	{ std::make_error_code(ec) } -> std::convertible_to<std::error_code>;
};
} //namespace detail

class [[nodiscard]] error_list
{
public:
	using error_list_type = std::vector<std::error_code>;

public:
	void add_error(std::error_code error);

	[[nodiscard]]
	constexpr const error_list_type& get_errors() const noexcept
	{
		return errors_;
	}

	[[nodiscard]]
	constexpr bool has_errors() const noexcept
	{
		return !errors_.empty();
	}

	void clear_errors()
	{
		errors_.clear();
	}

	[[nodiscard]]
	bool has_error(std::error_code error) const noexcept;

	template<detail::is_convertible_to_error_code ErrorCode>
	[[nodiscard]]
	bool has_error(ErrorCode error) const noexcept
	{
		return has_error(std::make_error_code(error));
	}

private:
	error_list_type errors_;
};

} //namespace pe_bliss
