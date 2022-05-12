#pragma once

#include <compare>
#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
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
	using context_type = std::variant<std::monostate,
		std::size_t, std::string>;

	struct error_context
	{
		std::error_code code;
		context_type context;

		[[nodiscard]]
		friend auto operator<=>(const error_context&, const error_context&) = default;
	};

	struct error_info
	{
		error_context context;
		std::exception_ptr error{};

		[[nodiscard]]
		friend bool operator==(const error_info&, const error_info&) = default;
	};

public:
	using error_list_type = std::vector<error_info>;

public:
	void add_error(std::error_code error);
	void add_error(std::error_code error, std::string context);
	void add_error(std::error_code error, std::size_t context);

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
	[[nodiscard]]
	bool has_error(std::error_code error,
		std::string_view context) const noexcept;
	[[nodiscard]]
	bool has_error(std::error_code error,
		std::size_t context) const noexcept;

	[[nodiscard]]
	bool has_any_error(std::error_code error) const noexcept;

	template<detail::is_convertible_to_error_code ErrorCode>
	[[nodiscard]]
	bool has_error(ErrorCode error) const noexcept
	{
		return has_error(std::make_error_code(error));
	}

	template<detail::is_convertible_to_error_code ErrorCode,
		typename Context>
	[[nodiscard]]
	bool has_error(ErrorCode error,
		Context&& context) const noexcept
	{
		return has_error(std::make_error_code(error),
			std::forward<Context>(context));
	}

	template<detail::is_convertible_to_error_code ErrorCode>
	[[nodiscard]]
	bool has_any_error(ErrorCode error) const noexcept
	{
		return has_any_error(std::make_error_code(error));
	}

private:
	error_list_type errors_;
};

} //namespace pe_bliss
