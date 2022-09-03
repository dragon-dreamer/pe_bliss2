#include "resources_dumper.h"

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <variant>
#include <vector>

#include "formatter.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/packed_utf16_string.h"
#include "pe_bliss2/resources/resource_directory.h"
#include "pe_bliss2/resources/resource_directory_loader.h"

namespace
{

struct name_id_dumper
{
public:
	explicit name_id_dumper(formatter& fmt) noexcept
		: fmt_(fmt)
	{
	}

	constexpr void operator()(std::monostate) const {}

	void operator()(pe_bliss::resources::resource_id_type id) const
	{
		fmt_.print_field_name("ID");
		fmt_.get_stream() << ' ';
		fmt_.print_value(id, true);
	}

	void operator()(const pe_bliss::packed_utf16_string& name) const
	{
		fmt_.print_field_name("Name");
		fmt_.get_stream() << ' ';
		fmt_.print_packed_string(name);
	}

private:
	formatter& fmt_;
};

void dump_directory(std::size_t level, formatter& fmt,
	const pe_bliss::resources::resource_directory_details& dir);

struct dir_or_data_dumper
{
public:
	explicit dir_or_data_dumper(std::size_t level, formatter& fmt) noexcept
		: level_(level), fmt_(fmt)
	{
	}

	constexpr void operator()(std::monostate) const {}

	void operator()(const pe_bliss::resources::resource_directory_details& dir) const
	{
		dump_directory(level_, fmt_, dir);
	}

	void operator()(const pe_bliss::resources::resource_data_entry_details& data) const
	{
		fmt_.get_stream() << std::string(level_, '>') << "===== ";
		fmt_.print_structure_name("Resource data entry");
		fmt_.get_stream() << " =====\n\n";
		fmt_.print_errors(data);

		fmt_.print_structure("Resource data entry descriptor",
			data.get_descriptor(), std::array{
			value_info{"offset_to_data"},
			value_info{"size"},
			value_info{"code_page"},
			value_info{"reserved"}
		});

		fmt_.print_bytes("Raw resource data", *data.get_raw_data().data(), 32u);
	}

	void operator()(pe_bliss::rva_type rva) const
	{
		fmt_.get_stream() << std::string(level_, '>') << "===== ";
		fmt_.print_structure_name("Looped");
		fmt_.get_stream() << " =====\n\n";
		fmt_.print_field_name("RVA");
		fmt_.get_stream() << ' ';
		fmt_.print_value(rva, true);
		fmt_.get_stream() << '\n';
	}

private:
	std::size_t level_{};
	formatter& fmt_;
};

void dump_name_or_id(formatter& fmt,
	const pe_bliss::resources::resource_directory_entry_details::name_or_id_type& name_or_id)
{
	std::visit(name_id_dumper{ fmt }, name_or_id);
}

void dump_entries(std::size_t level, formatter& fmt,
	const std::vector<pe_bliss::resources::resource_directory_entry_details>& entries)
{
	fmt.get_stream() << std::string(level, '>') << "===== ";
	fmt.print_structure_name("Resource directory entries");
	fmt.get_stream() << " =====\n\n";

	for (const auto& entry : entries)
	{
		fmt.print_structure("Resource directory entry descriptor",
			entry.get_descriptor(), std::array{
			value_info{"name_or_id", true, std::bind(dump_name_or_id,
				std::ref(fmt), std::ref(entry.get_name_or_id()))},
			value_info{"offset_to_data_or_directory"}
		});
		fmt.print_errors(entry);

		if (std::get_if<std::monostate>(&entry.get_data_or_directory()))
			continue;

		fmt.get_stream() << '\n';
		std::visit(dir_or_data_dumper{ level + 1, fmt }, entry.get_data_or_directory());
	}
}

void dump_directory(std::size_t level, formatter& fmt,
	const pe_bliss::resources::resource_directory_details& dir)
{
	fmt.get_stream() << std::string(level, '>') << "===== ";
	fmt.print_structure_name("Resource directory");
	fmt.get_stream() << " =====\n\n";
	fmt.print_errors(dir);

	fmt.print_structure("Resource directory descriptor", dir.get_descriptor(), std::array{
		value_info{"characteristics"},
		value_info{"time_date_stamp"},
		value_info{"major_version", false},
		value_info{"minor_version", false},
		value_info{"number_of_named_entries", false},
		value_info{"number_of_id_entries", false}
	});

	dump_entries(level, fmt, dir.get_entries());
}

} //namespace

void dump_resources(formatter& fmt, const pe_bliss::image::image& image)
{
	auto resources = pe_bliss::resources::load(image, {});
	if (!resources)
		return;

	dump_directory(1, fmt, *resources);
}
