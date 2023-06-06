#include "pe_bliss2/resources/accelerator_table_reader.h"

#include <string>
#include <system_error>

#include "pe_bliss2/resources/resource_reader_errc.h"
#include "pe_bliss2/detail/resources/accelerator.h"

namespace
{
struct accelerator_table_reader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "accelerator_table_reader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::accelerator_table_reader_errc;
		switch (static_cast<pe_bliss::resources::accelerator_table_reader_errc>(ev))
		{
		case too_many_accelerators:
			return "Too many accelerators";
		default:
			return {};
		}
	}
};

const accelerator_table_reader_error_category accelerator_table_reader_error_category_instance;
} // namespace

namespace pe_bliss::resources
{

std::error_code make_error_code(accelerator_table_reader_errc e) noexcept
{
	return { static_cast<int>(e), accelerator_table_reader_error_category_instance };
}

accelerator_table_details accelerator_table_from_resource(
	buffers::input_buffer_stateful_wrapper_ref& buf,
	const accelerator_table_read_options& options)
{
	accelerator_table_details table;

	auto& accels = table.get_accelerators();
	auto max_accelerator_count = options.max_accelerator_count;
	while (true)
	{
		if (!max_accelerator_count)
		{
			table.add_error(accelerator_table_reader_errc::too_many_accelerators);
			break;
		}
		--max_accelerator_count;

		auto& accel = accels.emplace_back();
		try
		{
			accel.get_descriptor().deserialize(buf, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			accels.pop_back();
			table.add_error(resource_reader_errc::buffer_read_error);
			break;
		}

		if (accel.get_descriptor()->modifier & detail::resources::modifier_last_accelerator)
			break;
	}

	return table;
}

} //namespace pe_bliss::resources
