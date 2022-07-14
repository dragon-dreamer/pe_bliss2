#include "pe_bliss2/error_list.h"

#include <algorithm>
#include <utility>

namespace pe_bliss
{

void error_list::create_error_map()
{
	if (!errors_)
		errors_ = std::make_unique<error_map_type>();
}

void error_list::add_error(std::error_code error)
{
	create_error_map();
	auto it = errors_->find({ error });
	if (it == errors_->end())
	{
		errors_->emplace(error_context{ error },
			std::current_exception());
	}
}

void error_list::add_error(std::error_code error, std::string context)
{
	create_error_map();
	auto ctx = error_context{ error, std::move(context) };
	auto it = errors_->find(ctx);
	if (it == errors_->end())
	{
		errors_->emplace(std::move(ctx),
			std::current_exception());
	}
}

void error_list::add_error(std::error_code error, std::size_t context)
{
	create_error_map();
	auto ctx = error_context{ error, context };
	auto it = errors_->find(ctx);
	if (it == errors_->end())
	{
		errors_->emplace(std::move(ctx),
			std::current_exception());
	}
}

bool error_list::has_error(std::error_code error) const noexcept
{
	return errors_ && errors_->contains({ error });
}

bool error_list::has_error(std::error_code error,
	std::string_view context) const noexcept
{
	return errors_ && errors_->contains({ error, std::string(context) });
}

bool error_list::has_error(std::error_code error,
	std::size_t context) const noexcept
{
	return errors_ && errors_->contains({ error, context });
}

bool error_list::has_any_error(std::error_code error) const noexcept
{
	if (!errors_)
		return false;

	for (const auto& [key, val] : *errors_)
	{
		if (key.code == error)
			return true;
	}

	return false;
}

} //namespace pe_bliss
