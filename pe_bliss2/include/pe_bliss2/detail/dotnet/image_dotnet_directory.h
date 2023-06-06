#pragma once

#include <cstdint>

#include "pe_bliss2/detail/image_data_directory.h"

namespace pe_bliss::detail::dotnet
{

struct image_cor20_header
{
	std::uint32_t cb; //struct size
	std::uint16_t major_runtime_version;
	std::uint16_t minor_runtime_version;

	//Symbol table and startup information
	image_data_directory meta_data;
	std::uint32_t flags;

	//If COMIMAGE_FLAGS_NATIVE_ENTRYPOINT is not set,
	//EntryPointToken represents a managed entrypoint.
	//If COMIMAGE_FLAGS_NATIVE_ENTRYPOINT is set,
	//EntryPointRVA represents an RVA to a native entrypoint.
	std::uint32_t entry_point_token_or_rva;

	//Binding information
	image_data_directory resources;
	image_data_directory strong_name_signature;

	//Regular fixup and binding information
	image_data_directory code_manager_table; //Deprecated, not used
	image_data_directory vtable_fixups;
	image_data_directory export_address_table_jumps; //Always 0

	//Precompiled image info (internal use only - set to zero for ordinary images)
	//NGEN images point at a CORCOMPILE_HEADER structure
	image_data_directory managed_native_header;
};

namespace comimage_flags
{
constexpr std::uint32_t ilonly = 0x00000001u;
constexpr std::uint32_t x32bitrequired = 0x00000002u;
constexpr std::uint32_t il_library = 0x00000004u;
constexpr std::uint32_t strongnamesigned = 0x00000008u;
constexpr std::uint32_t native_entrypoint = 0x00000010u;
constexpr std::uint32_t trackdebugdata = 0x00010000u;
constexpr std::uint32_t x32bitpreferred = 0x00020000u;
} // namespace comimage_flags

struct metadata_header
{
	std::uint32_t signature; //always 0x424a5342
	std::uint16_t major_version; //always 1
	std::uint16_t minor_version; //always 1
	std::uint32_t reserved; //always 0
	std::uint32_t version_length; //has to be rounded up to a multiple of 4
	//char[] version_string;
};

struct metadata_header_footer
{
	std::uint16_t flags; //always 0
	std::uint16_t stream_count;
};

struct stream_header
{
	std::uint32_t offset;
	std::uint32_t size;
	//char[] name; //aligned to the next 4-byte boundary
};

} // namespace pe_bliss::detail::dotnet
