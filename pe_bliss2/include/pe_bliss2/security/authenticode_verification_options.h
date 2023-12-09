#pragma once

#include "pe_bliss2/security/image_hash.h"

namespace pe_bliss::security
{

struct [[nodiscard]] authenticode_verification_options final
{
	page_hash_options page_hash_opts;
	bool verify_timestamp_signature = true;
};

} //namespace pe_bliss::security
