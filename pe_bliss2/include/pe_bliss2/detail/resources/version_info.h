#pragma once

#include <cstdint>

namespace pe_bliss::detail::resources
{

struct vs_fixedfileinfo
{
	uint32_t signature; //e.g. 0xfeef04bd
	uint32_t struc_version; //e.g. 0x00000042 = "0.42"
	uint32_t file_version_ms; //e.g. 0x00030075 = "3.75"
	uint32_t file_version_ls; //e.g. 0x00000031 = "0.31"
	uint32_t product_version_ms; //e.g. 0x00030010 = "3.10"
	uint32_t product_version_ls; //e.g. 0x00000031 = "0.31"
	uint32_t file_flags_mask; //= 0x3F for version "0.42"
	uint32_t file_flags; //e.g. VFF_DEBUG | VFF_PRERELEASE
	uint32_t file_os; //e.g. VOS_DOS_WINDOWS16
	uint32_t file_type; //e.g. VFT_DRIVER
	uint32_t file_subtype; //e.g. VFT2_DRV_KEYBOARD
	uint32_t file_date_ms; //e.g. 0
	uint32_t file_date_ls; //e.g. 0
};

constexpr std::uint32_t vs_ffi_fileflagsmask = 0x0000003fu;
constexpr std::uint32_t vs_ffi_signature = 0xfeef04bdu;
constexpr std::uint32_t vs_ffi_strucversion = 0x00010000u;

//file_flags
constexpr std::uint32_t vs_ff_debug = 0x00000001u;
constexpr std::uint32_t vs_ff_prerelease = 0x00000002u;
constexpr std::uint32_t vs_ff_patched = 0x00000004u;
constexpr std::uint32_t vs_ff_privatebuild = 0x00000008u;
constexpr std::uint32_t vs_ff_infoinferred = 0x00000010u;
constexpr std::uint32_t vs_ff_specialbuild = 0x00000020u;

//file_os
constexpr std::uint32_t vos_unknown = 0x00000000u;
constexpr std::uint32_t vos_dos = 0x00010000u;
constexpr std::uint32_t vos_os216 = 0x00020000u; //reserved
constexpr std::uint32_t vos_os232 = 0x00030000u; //reserved
constexpr std::uint32_t vos_nt = 0x00040000u;
constexpr std::uint32_t vos_wince = 0x00050000u;

constexpr std::uint32_t vos__base = 0x00000000u;
constexpr std::uint32_t vos__windows16 = 0x00000001u;
constexpr std::uint32_t vos__pm16 = 0x00000002u; //reserved
constexpr std::uint32_t vos__pm32 = 0x00000003u; //reserved
constexpr std::uint32_t vos__windows32 = 0x00000004u;

constexpr std::uint32_t vos_dos_windows16 = 0x00010001u;
constexpr std::uint32_t vos_dos_windows32 = 0x00010004u;
constexpr std::uint32_t vos_os216_pm16 = 0x00020002u;
constexpr std::uint32_t vos_os232_pm32 = 0x00030003u;
constexpr std::uint32_t vos_nt_windows32 = 0x00040004u;

//file_type
constexpr std::uint32_t vft_unknown = 0x00000000u;
constexpr std::uint32_t vft_app = 0x00000001u;
constexpr std::uint32_t vft_dll = 0x00000002u;
constexpr std::uint32_t vft_drv = 0x00000003u;
constexpr std::uint32_t vft_font = 0x00000004u;
constexpr std::uint32_t vft_vxd = 0x00000005u;
constexpr std::uint32_t vft_static_lib = 0x00000007u;

//file_subtype for vft_drv
constexpr std::uint32_t vft2_unknown = 0x00000000u;
constexpr std::uint32_t vft2_drv_printer = 0x00000001u;
constexpr std::uint32_t vft2_drv_keyboard = 0x00000002u;
constexpr std::uint32_t vft2_drv_language = 0x00000003u;
constexpr std::uint32_t vft2_drv_display = 0x00000004u;
constexpr std::uint32_t vft2_drv_mouse = 0x00000005u;
constexpr std::uint32_t vft2_drv_network = 0x00000006u;
constexpr std::uint32_t vft2_drv_system = 0x00000007u;
constexpr std::uint32_t vft2_drv_installable = 0x00000008u;
constexpr std::uint32_t vft2_drv_sound = 0x00000009u;
constexpr std::uint32_t vft2_drv_comm = 0x0000000au;
constexpr std::uint32_t vft2_drv_inputmethod = 0x0000000bu;
constexpr std::uint32_t vft2_drv_versioned_printer = 0x0000000cu;

//file_subtype for vft_font
constexpr std::uint32_t vft2_font_raster = 0x00000001u;
constexpr std::uint32_t vft2_font_vector = 0x00000002u;
constexpr std::uint32_t vft2_font_truetype = 0x00000003u;

constexpr std::uint16_t version_info_block_value_type_binary = 0u;
constexpr std::uint16_t version_info_block_value_type_text = 1u;

//Structure representing BLOCK in version info resource
struct version_info_block //(always aligned on 32-bit (DWORD) boundary)
{
	std::uint16_t length; //Length of this block (does not include padding)
	std::uint16_t value_length; //Value length (if any)
	std::uint16_t type; //Value type (0 = binary, 1 = text)
	//std::uint16_t key[1]; //Value name (block key) (always NULL terminated)

	//std::uint16_t padding1[]; //Padding, if any (ALIGNMENT)
	//xxxxx value[]; //Value data, if any (*ALIGNED*)
	//std::uint16_t padding2[]; //Padding, if any (ALIGNMENT)
	//xxxxx child[]; //Child block(s), if any (*ALIGNED*)
};

struct translation_block
{
	std::uint16_t lcid;
	std::uint16_t cpid;
};

} //namespace pe_bliss::detail::resources
