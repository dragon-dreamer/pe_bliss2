#include "file_signature_dumper.h"

#include <array>

#include "formatter.h"

#include "pe_bliss2/core/image_signature.h"

void dump_file_signature(formatter& fmt,
	const pe_bliss::core::image_signature& signature)
{
	fmt.print_structure("File header signature", signature.get_descriptor(), std::array{
		value_info{"signature"},
	});
}
