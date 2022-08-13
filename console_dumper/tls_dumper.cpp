#include "tls_dumper.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>

#include "formatter.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/tls/tls_directory_loader.h"
#include "pe_bliss2/tls/tls_directory.h"

namespace
{

template<typename Directory>
void dump_tls_characteristics(formatter& fmt, const Directory& dir,
	std::size_t left_padding)
{
	fmt.get_stream() << '\n';
	fmt.get_stream() << std::setw(left_padding) << std::setfill(' ') << "";

	{
		color_changer changer(fmt.get_stream(), fmt.get_color_provider(),
			fmt.flags_bg_color, fmt.flags_fg_color);
		fmt.get_stream() << "Alignment: " << std::dec
			<< dir.get_max_type_alignment();
	}
}

template<typename Directory>
void dump_tls_impl(formatter& fmt, const Directory& directory)
{
	fmt.print_errors(directory);

	fmt.print_structure("TLS descriptor", directory.get_descriptor(), std::array{
		value_info{"start_address_of_raw_data"},
		value_info{"end_address_of_raw_data"},
		value_info{"address_of_index"},
		value_info{"address_of_callbacks"},
		value_info{"size_of_zero_fill"},
		value_info{"characteristics", true, std::bind(
			dump_tls_characteristics<Directory>,
			std::ref(fmt), std::cref(directory), std::placeholders::_1)}
	});

	for (const auto& cb : directory.get_callbacks())
	{
		fmt.print_structure("TLS callback", cb, std::array{
			value_info{"VA"}
		});
		fmt.print_errors(cb);
	}

	if (directory.get_callbacks().empty())
		fmt.get_stream() << "No TLS callbacks\n\n";

	fmt.print_bytes("TLS raw data", *directory.get_raw_data().data());
}

} //namespace

void dump_tls(formatter& fmt, const pe_bliss::image::image& image) try
{
	auto tls = pe_bliss::tls::load(image, {});
	if (!tls)
		return;

	fmt.get_stream() << "===== ";
	fmt.print_structure_name("Thread local storage");
	fmt.get_stream() << " =====\n\n";

	std::visit([&fmt] (const auto& directory) { dump_tls_impl(fmt, directory); }, *tls);
}
catch (const std::system_error& e)
{
	fmt.print_error("Error loading TLS:", e);
}
catch (const std::exception& e)
{
	fmt.print_error("Error loading TLS:", e);
}
