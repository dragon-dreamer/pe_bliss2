#pragma once

#include <cstdint>
#include <string_view>

namespace pe_bliss::detail::trustlet
{

constexpr std::uint8_t image_policy_metadata_version = 1U;
constexpr std::string_view image_policy_section_name = ".tPolicy";
constexpr std::string_view image_policy_metadata_name_win10_16215 = "s_IumPolicyMetadata";
constexpr std::string_view image_policy_metadata_name_win10_16193 = "__ImagePolicyMetadata";

constexpr std::uint32_t image_policy_entry_type_none = 0;
constexpr std::uint32_t image_policy_entry_type_boolean = 1;
constexpr std::uint32_t image_policy_entry_type_int8 = 2;
constexpr std::uint32_t image_policy_entry_type_uint8 = 3;
constexpr std::uint32_t image_policy_entry_type_int16 = 4;
constexpr std::uint32_t image_policy_entry_type_uint16 = 5;
constexpr std::uint32_t image_policy_entry_type_int32 = 6;
constexpr std::uint32_t image_policy_entry_type_uint32 = 7;
constexpr std::uint32_t image_policy_entry_type_int64 = 8;
constexpr std::uint32_t image_policy_entry_type_uint64 = 9;
constexpr std::uint32_t image_policy_entry_type_ansi_string = 10;
constexpr std::uint32_t image_policy_entry_type_unicode_string = 11;
constexpr std::uint32_t image_policy_entry_type_overriden = 12;

constexpr std::uint32_t image_policy_id_none = 0;
// Enables or disables ETW
constexpr std::uint32_t image_policy_id_etw = 1;
// Configures debugging
// Debug can be enabled at all times,
// only when SecureBoot is disabled,
// or using an on-demand challenge/response mechanism
constexpr std::uint32_t image_policy_id_debug = 2;
// Enables or disables Crash Dump
constexpr std::uint32_t image_policy_id_crash_dump = 3;
// Specifies public key for encrypting crash dumps
// Dumps can be submitted to Microsoft Product Team,
// which has the private key for decryption
constexpr std::uint32_t image_policy_id_crash_dump_key = 4;
// Specifies identifier for crash dump key
// This allows multiple keys to be used / identified by the product team
constexpr std::uint32_t image_policy_id_crash_dump_key_guid = 5;
// SDDL format
// This is used to validate the owner/parent process is expected
constexpr std::uint32_t image_policy_id_parent_sd = 6;
// SDDL format revision ID
// This is used to validate the owner/parent process is expected
constexpr std::uint32_t image_policy_id_parent_sd_rev = 7;
// Security version
// This is a unique number that can be used by the Trustlet (along its identity)
// when encrypting AES256/GCM messages.
constexpr std::uint32_t image_policy_id_svn = 8;
// Secure device PCI identifier
// The Trustlet can only communicate with a Secure Device whose PCI ID matches
constexpr std::uint32_t image_policy_id_device_id = 9;
// Enables powerful VTL 1 capabilities
// This enables access to the Create Secure Section API,
// DMA and user-mode MMIO access to Secure Devices, and Secure Storage API
constexpr std::uint32_t image_policy_id_capability = 10;
// Specifies the scenario ID for this binary
// Encoded as a GUID, this must be specified by Trustlet
// when creating secure image sections to ensure it is for a known scenario.
constexpr std::uint32_t image_policy_id_scenario_id = 11;

struct image_policy_entry
{
    std::uint32_t type;
    std::uint32_t policy_id;
    /*
    union {
        const VOID* None;
        BOOLEAN BoolValue;
        INT8 Int8Value;
        UINT8 UInt8Value;
        INT16 Int16Value;
        UINT16 UInt16Value;
        INT32 Int32Value;
        UINT32 UInt32Value;
        INT64 Int64Value;
        UINT64 UInt64Value;
        PCSTR AnsiStringValue;
        PCWSTR UnicodeStringValue;
    } u;
    */
    std::uint64_t value;
};

struct image_policy_metadata
{
    std::uint8_t version;
    std::uint8_t reserved0[7];
    std::uint64_t application_id;
    // image_policy_entry policies[];
};

} //namespace pe_bliss::detail::trustlet
