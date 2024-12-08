#pragma once

#include <cstdint>
#include <string_view>

namespace pe_bliss::detail::trustlet
{

constexpr std::uint8_t image_policy_metadata_version = 1U;
constexpr std::string_view image_policy_section_name = ".tPolicy";
constexpr std::string_view image_policy_metadata_name_win10_16215 = "s_IumPolicyMetadata";
constexpr std::string_view image_policy_metadata_name_win10_16193 = "__ImagePolicyMetadata";

enum class image_policy_entry_type : std::uint32_t
{
    none = 0,
    boolean,
    int8,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    int64,
    uint64,
    ansi_string,
    unicode_string,
    overriden,
    maximum
};

enum class image_policy_id : std::uint32_t
{
    none = 0,
    // Enables or disables ETW
    etw,
    // Configures debugging
    // Debug can be enabled at all times,
    // only when SecureBoot is disabled,
    // or using an on-demand challenge/response mechanism
    debug,
    // Enables or disables Crash Dump
    crash_dump,
    // Specifies public key for encrypting crash dumps
    // Dumps can be submitted to Microsoft Product Team,
    // which has the private key for decryption
    crash_dump_key,
    // Specifies identifier for crash dump key
    // This allows multiple keys to be used / identified by the product team
    crash_dump_key_guid,
    // SDDL format
    // This is used to validate the owner/parent process is expected
    parent_sd,
    // SDDL format revision ID
    // This is used to validate the owner/parent process is expected
    parent_sd_rev,
    // Security version
    // This is a unique number that can be used by the Trustlet (along its identity)
    // when encrypting AES256/GCM messages.
    svn,
    // Secure device PCI identifier
    // The Trustlet can only communicate with a Secure Device whose PCI ID matches
    device_id,
    // Enables powerful VTL 1 capabilities
    // This enables access to the Create Secure Section API,
    // DMA and user-mode MMIO access to Secure Devices, and Secure Storage API
    capability,
    // Specifies the scenario ID for this binary
    // Encoded as a GUID, this must be specified by Trustlet
    // when creating secure image sections to ensure it is for a known scenario.
    scenario_id,
    maximum
};

struct image_policy_entry
{
    image_policy_entry_type type;
    image_policy_id policy_id;
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
