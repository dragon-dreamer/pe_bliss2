#pragma once

#include <compare>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

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
		error_context(std::error_code code)
			: code(code)
		{
		}

		error_context(std::error_code code, context_type context)
			: code(code)
			, context(std::move(context))
		{
		}

		std::error_code code;
		context_type context;

		[[nodiscard]]
		friend auto operator<=>(const error_context&, const error_context&) = default;
		[[nodiscard]]
		friend bool operator==(const error_context&, const error_context&) = default;
	};

	struct error_context_hash final
	{
		[[nodiscard]]
		std::size_t operator()(const error_context& context) const
		{
			auto hash = std::hash<std::error_code>{}(context.code);
			hash ^= std::hash<context_type>{}(context.context)
				+ 0x9e3779b9u + (hash << 6u) + (hash >> 2u);
			return hash;
		}
	};

	struct error_info
	{
		error_info() noexcept = default;
		explicit error_info(std::exception_ptr error) noexcept
			: error(std::move(error))
		{
		}

		std::exception_ptr error;

		[[nodiscard]]
		friend bool operator==(const error_info&, const error_info&) = default;
	};

public:
	using error_map_type = std::unordered_map<error_context, error_info, 
		error_context_hash>;

public:
	void add_error(std::error_code error);
	void add_error(std::error_code error, std::string context);
	void add_error(std::error_code error, std::size_t context);

	[[nodiscard]]
	const error_map_type* get_errors() const noexcept
	{
		return errors_.get();
	}

	[[nodiscard]]
	bool has_errors() const noexcept
	{
		return errors_ && !errors_->empty();
	}

	void clear_errors()
	{
		errors_.reset();
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
	void create_error_map();

private:
	std::unique_ptr<error_map_type> errors_;
};

} //namespace pe_bliss
