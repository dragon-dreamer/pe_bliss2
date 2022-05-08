#pragma once

#include <system_error>
#include <vector>

namespace pe_bliss
{

class [[nodiscard]] error_list
{
public:
	using error_list_type = std::vector<std::error_code>;

public:
	void add_error(const std::error_code& error);
	
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

private:
	error_list_type errors_;
};

} //namespace pe_bliss
