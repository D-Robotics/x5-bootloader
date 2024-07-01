/*
 * Copyright (c) 2018-2020, Arm Technology (China) Co., Ltd.
 * All rights reserved.
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#ifndef MBEDTLS_DSA_H
#define MBEDTLS_DSA_H

#include "bignum.h"
#include "md.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MBEDTLS_ERR_DSA_BAD_INPUT_DATA      -0x4600 /**< Bad input parameters to function. */
#define MBEDTLS_ERR_DSA_BUFFER_TOO_SMALL    -0x4680 /**< The buffer is too small to write to. */
#define MBEDTLS_ERR_DSA_FEATURE_UNAVAILABLE -0x4700 /**< The requested feature is not available, for example, the requested curve is not supported. */
#define MBEDTLS_ERR_DSA_VERIFY_FAILED       -0x4780 /**< The signature is not valid. */
#define MBEDTLS_ERR_DSA_ALLOC_FAILED        -0x4800 /**< Memory allocation failed. */
#define MBEDTLS_ERR_DSA_RANDOM_FAILED       -0x4880 /**< Generation of random value, such as ephemeral key, failed. */
#define MBEDTLS_ERR_DSA_SIG_LEN_MISMATCH    -0x4900 /**< The buffer contains a valid signature followed by more data.*/
#define MBEDTLS_ERR_DSA_HW_FAILED           -0x4980 /**< There is HW error in calling TE driver. */

/*
 *
 *     dsa-Sig-Value ::= SEQUENCE {
 *         r       INTEGER,
 *         s       INTEGER
 *     }
 *
 * Size is at most
 *    1 (tag) + 1 (len) + 1 (initial 0) + 32 for each of r and s,
 *    twice that + 1 (tag) + 2 (len) for the sequence
 */

/* Max DSA group size in bytes (default allows 4k-bit groups) */

#ifndef MBEDTLS_DSA_MAX_GROUP_SIZE
#define MBEDTLS_DSA_MAX_GROUP_SIZE 512
#else

#if MBEDTLS_DSA_MAX_GROUP_SIZE > 512
#error                                                                         \
    "MBEDTLS_DSA_MAX_GROUP_SIZE bigger than expected, please fix MBEDTLS_DSA_MAX_GROUP_SIZE"
#endif

#endif
/** The maximal size of an DSA signature in Bytes. */
#define MBEDTLS_DSA_MAX_LEN (3 + 2 * (3 + MBEDTLS_DSA_MAX_GROUP_SIZE))

/**
 * \brief    The DSA context structure.
 */
typedef struct mbedtls_dsa_context {
    mbedtls_mpi p; /*!<  the prime modulus                 */
    mbedtls_mpi q; /*!<  the sub-prime                     */
    mbedtls_mpi g; /*!<  the generator                     */
    mbedtls_mpi x; /*!<  the private key                   */
    mbedtls_mpi y; /*!<  the public key                    */
} mbedtls_dsa_context;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief           This function computes the DSA signature of a
 *                  previously-hashed message.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the Q size, then the hash is truncated
 *                  as defined in FIPS 186-4 4.6.
 * \param p         The prime modulus parameter.
 * \param q         The sub-prime parameter.
 * \param g         The generator parameter.
 * \param r         The first output integer.
 * \param s         The second output integer.
 * \param x         The private signing key.
 * \param buf       The message's hash.
 * \param blen      The length of \p buf in Bytes. if blen is larger than DSA's
 * Q size then only maximue of the Q size message hash is read. \param f_rng The
 * RNG function. \param p_rng     The RNG context.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_DSA_XXX
 *                  or \c MBEDTLS_ERR_MPI_XXX error code on failure.
 */
int mbedtls_dsa_sign(const mbedtls_mpi *p,
                     const mbedtls_mpi *q,
                     const mbedtls_mpi *g,
                     mbedtls_mpi *r,
                     mbedtls_mpi *s,
                     const mbedtls_mpi *x,
                     const unsigned char *buf,
                     size_t blen,
                     int (*f_rng)(void *, unsigned char *, size_t),
                     void *p_rng);

/**
 * \brief           This function verifies the DSA signature of a
 *                  previously-hashed message.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the Q size, then the hash is truncated
 *                  as defined in FIPS 186-4 4.6.
 * \param p         The prime modulus parameter.
 * \param q         The sub-prime parameter.
 * \param g         The generator parameter.
 * \param buf       The message's hash.
 * \param blen      The length of \p buf in Bytes. if blen is larger than DSA's
 * Q size, then only maximue of Q size message hash is read. \param y The public
 * key to use for verification. \param r         The first integer of the
 * signature. \param s         The second integer of the signature. \return \c 0
 * on success. \return          #MBEDTLS_ERR_DSA_BAD_INPUT_DATA if the signature
 *                  is invalid.
 * \return          An \c MBEDTLS_ERR_DSA_XXX or \c MBEDTLS_MPI_XXX
 *                  error code on failure for any other reason.
 */
int mbedtls_dsa_verify(const mbedtls_mpi *p,
                       const mbedtls_mpi *q,
                       const mbedtls_mpi *g,
                       const unsigned char *buf,
                       size_t blen,
                       const mbedtls_mpi *y,
                       const mbedtls_mpi *r,
                       const mbedtls_mpi *s);

/**
 * \brief           This function computes the DSA signature and writes it
 *                  to a buffer.
 *
 * \note            The \p sig buffer must be at least twice as large as the
 *                  size of the Q size, plus 9. For example, 73 Bytes if
 *                  a Q size is 256 bit. A buffer length of
 *                  #MBEDTLS_DSA_MAX_LEN is always safe.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the Q size, then the hash is truncated as
 *                  defined in FIPS 186-4 4.6.
 *
 * \param ctx       The DSA context.
 * \param md_alg    The message digest that was used to hash the message.
 * \param buf       The message's hash.
 * \param hlen      The length of the hash in Bytes.
 * \param sig       The buffer that holds the signature.
 * \param slen      The length of the signature written.
 * \param f_rng     The RNG function.
 * \param p_rng     The RNG context.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_DSA_XXX or \c MBEDTLS_ERR_MPI_XXX or
 *                  \c MBEDTLS_ERR_ASN1_XXX error code on failure.
 */
int mbedtls_dsa_write_signature(mbedtls_dsa_context *ctx,
                                mbedtls_md_type_t md_alg,
                                const unsigned char *hash,
                                size_t hlen,
                                unsigned char *sig,
                                size_t *slen,
                                int (*f_rng)(void *, unsigned char *, size_t),
                                void *p_rng);

/**
 * \brief           This function reads and verifies an DSA signature.
 *
 * \note            If the bitlength of the message hash is larger than the
 *                  bitlength of the Q size, then the hash is truncated as
 *                  defined in FIPS 186-4 4.6.
 *
 * \param ctx       The DSA context.
 * \param hash      The message hash.
 * \param hlen      The size of the hash in Bytes.
 * \param sig       The signature to read and verify.
 * \param slen      The size of \p sig.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_DSA_BAD_INPUT_DATA if signature is invalid.
 * \return          An , \c MBEDTLS_ERR_DSA_XXX or \c MBEDTLS_ERR_ASN1_XXX or
 *                  \c MBEDTLS_ERR_MPI_XXX error code on failure for any other
 *                  reason.
 */
int mbedtls_dsa_read_signature(mbedtls_dsa_context *ctx,
                               const unsigned char *hash,
                               size_t hlen,
                               const unsigned char *sig,
                               size_t slen);
/**
 * \brief           This function export the DSA's parameter to a buffer.
 *
 * \note            The \p param buffer must be at least triple as large as the
 *                  size of the Q size, plus 12. For example, 108 Bytes if
 *                  a Q size is 256 bit.
 *
 * \param ctx       The DSA context.
 * \param param     The buffer that holds the parameters.
 * \param plen      Return the length of the parameters written in Bytes.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_MPI_XXX or
 *                  \c MBEDTLS_ERR_ASN1_XXX error code on failure.
 */
int mbedtls_dsa_write_parameter(mbedtls_dsa_context *ctx,
                                unsigned char *param,
                                size_t *plen);

/**
 * \brief           This function import the DSA's parameter from a buffer.
 *
 * \param ctx       The DSA context.
 * \param param     The buffer that holds the parameters.
 * \param plen      The length of the buffer in Bytes.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_MPI_XXX or
 *                  \c MBEDTLS_ERR_ASN1_XXX error code on failure.
 */
int mbedtls_dsa_read_parameter(mbedtls_dsa_context *ctx,
                               const unsigned char *param,
                               size_t plen);

/**
 * \brief          This function generates an DSA keypair with given parameters.
 *
 * \param ctx      The DSA context to store the keypair in.
 * \param f_rng    The RNG function.
 * \param p_rng    The RNG context.
 *
 * \return         \c 0 on success.
 * \return         An \c MBEDTLS_ERR_DSA_XXX code on failure.
 */
int mbedtls_dsa_genkey(mbedtls_dsa_context *ctx,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng);

/**
 * \brief           This function initializes an DSA context.
 *
 * \param ctx       The DSA context to initialize.
 *                  This must not be \c NULL.
 */
void mbedtls_dsa_init(mbedtls_dsa_context *ctx);

/**
 * \brief           This function frees an DSA context.
 *
 * \param ctx       The DSA context to free. This may be \c NULL,
 *                  in which case this function does nothing. If it
 *                  is not \c NULL, it must be initialized.
 */
void mbedtls_dsa_free(mbedtls_dsa_context *ctx);

/**
 * \brief          The DSA checkup routine.
 *
 * \return         \c 0 on success.
 * \return         \c 1 on failure.
 */
int mbedtls_dsa_self_test( int verbose );

#ifdef __cplusplus
}
#endif
#endif /* dsa.h */
