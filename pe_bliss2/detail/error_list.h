#pragma once

#include <system_error>
#include <unordered_set>

namespace pe_bliss::detail
{

class error_list
{
public:
	using error_list_type = std::unordered_set<std::error_code>;

public:
	void add_error(const std::error_code& error)
	{
		errors_.insert(error);
	}
	
	[[nodiscard]]
	const error_list_type& get_errors() const noexcept
	{
		return errors_;
	}

	[[nodiscard]]
	bool has_errors() const noexcept
	{
		return !errors_.empty();
	}

private:
	error_list_type errors_;
};

} //namespace pe_bliss::detail
