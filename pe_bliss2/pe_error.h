#pragma once

#include <system_error>
#include <type_traits>

namespace pe_bliss
{

namespace detail
{
template<typename ErrorCode>
concept is_comparable_with_error_code = requires(ErrorCode errc) {
	{ errc == std::error_code{} } -> std::convertible_to<bool>;
};
} //namespace detail

class [[nodiscard]] pe_error : public std::system_error
{
public:
	using std::system_error::system_error;
};

class [[nodiscard]] pe_error_wrapper
{
public:
	template<typename ErrorCode>
		requires(std::is_error_code_enum_v<ErrorCode>)
	pe_error_wrapper(ErrorCode ec) noexcept
		: ec_(ec)
	{
	}

	pe_error_wrapper() noexcept = default;

	void throw_on_error()
	{
		if (ec_)
			throw pe_error(ec_);
	}

	[[nodiscard]]
	operator std::error_code() const noexcept
	{
		return ec_;
	}

	template<detail::is_comparable_with_error_code ErrorCode>
	[[nodiscard]]
	bool operator==(ErrorCode ec) const noexcept
	{
		return ec_ == ec;
	}

	[[nodiscard]]
	explicit operator bool() const noexcept
	{
		return static_cast<bool>(ec_);
	}

private:
	std::error_code ec_;
};

} //namespace pe_bliss
