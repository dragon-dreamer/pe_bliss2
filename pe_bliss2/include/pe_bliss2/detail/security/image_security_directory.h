#pragma once

#include <cstdint>

namespace pe_bliss::detail::security
{

struct win_certificate
{
    std::uint32_t length;
    std::uint16_t revision;
    std::uint16_t certificate_type;
    //std::uint8_t certificate[];
};

constexpr std::uint16_t win_cert_revision_1_0 = 0x0100u;
constexpr std::uint16_t win_cert_revision_2_0 = 0x0200u;

//certificate contains an X.509 Certificate
constexpr std::uint16_t win_cert_type_x509 = 0x0001u;
//certificate contains a PKCS SignedData structure
constexpr std::uint16_t win_cert_type_pkcs_signed_data = 0x0002u;
//reserved
constexpr std::uint16_t win_cert_type_reserved_1 = 0x0003u;
//Terminal Server Protocol Stack Certificate signing
constexpr std::uint16_t win_cert_type_ts_stack_signed = 0x0004u;
//certificate member contains PKCS1_MODULE_SIGN fields
constexpr std::uint16_t win_cert_type_pkcs1_sign = 0x0009u;

} //namespace pe_bliss::detail::security
