/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2018, Linaro Limited */
#ifndef __MBEDTLS_CONFIG_UTA_H
#define __MBEDTLS_CONFIG_UTA_H

/*
 * When wrapping using TEE_BigInt to represent a mbedtls_mpi we can only
 * use 32-bit arithmetics.
 */
#define MBEDTLS_HAVE_INT32

#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_PKCS1_V15

#define MBEDTLS_CIPHER_C
#define MBEDTLS_DES_C
#define MBEDTLS_AES_C
#define MBEDTLS_NIST_KW_C
#define MBEDTLS_POLY1305_C
#define MBEDTLS_CHACHA20_C
#define MBEDTLS_CHACHAPOLY_C

#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA512_C
#define MBEDTLS_MD_C
#define MBEDTLS_MD5_C

#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_NNENTROPY_MAX_SOURCES   1

#define MBEDTLS_BIGNUM_C
#define MBEDTLS_GENPRIME
#define MBEDTLS_RSA_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_DHM_C
#define MBEDTLS_ECP_DP_SECP192R1_ENABLED
#define MBEDTLS_ECP_DP_SECP224R1_ENABLED
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#define MBEDTLS_ECP_DP_SECP521R1_ENABLED
#define MBEDTLS_ECP_DP_BP256R1_ENABLED
#define MBEDTLS_ECP_DP_BP384R1_ENABLED
#define MBEDTLS_ECP_DP_BP512R1_ENABLED
#define MBEDTLS_ECP_DP_SECP192K1_ENABLED
#define MBEDTLS_ECP_DP_SECP224K1_ENABLED
#define MBEDTLS_ECP_DP_SECP256K1_ENABLED
#define MBEDTLS_ECP_DP_SM2P256V1_ENABLED
#define MBEDTLS_SM2DSA_C
#define MBEDTLS_SM2DSA_DETERMINISTIC
#if defined(MBEDTLS_ECDSA_DETERMINISTIC) || \
    defined(MBEDTLS_SM2DSA_DETERMINISTIC)
#define MBEDTLS_HMAC_DRBG_C
#endif
#define MBEDTLS_SM3_C

#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_OID_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_CRT_WRITE_C
#define MBEDTLS_X509_CSR_PARSE_C
#define MBEDTLS_X509_CSR_WRITE_C
#define MBEDTLS_X509_CREATE_C
#define MBEDTLS_X509_CHECK_KEY_USAGE
#define MBEDTLS_X509_USE_C
#define MBEDTLS_BASE64_C
#define MBEDTLS_CERTS_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_PEM_WRITE_C

#define MBEDTLS_SSL_ENCRYPT_THEN_MAC
#define MBEDTLS_SSL_EXTENDED_MASTER_SECRET
#define MBEDTLS_SSL_FALLBACK_SCSV
#define MBEDTLS_SSL_CBC_RECORD_SPLITTING
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
#define MBEDTLS_SSL_PROTO_TLS1
#define MBEDTLS_SSL_PROTO_TLS1_1
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_PROTO_DTLS
#define MBEDTLS_SSL_ALPN
#define MBEDTLS_SSL_DTLS_ANTI_REPLAY
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY
#define MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE
#define MBEDTLS_SSL_DTLS_BADMAC_LIMIT
#define MBEDTLS_SSL_SESSION_TICKETS
#define MBEDTLS_SSL_EXPORT_KEYS
#define MBEDTLS_SSL_SERVER_NAME_INDICATION
#define MBEDTLS_SSL_TRUNCATED_HMAC

#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED

#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_CLI_C

#define MBEDTLS_DEBUG_C

#include <mbedtls/check_config.h>

#endif /* __MBEDTLS_CONFIG_UTA_H */
