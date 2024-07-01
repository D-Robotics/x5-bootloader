/*
 * Copyright (c) 2020, Arm Technology (China) Co., Ltd.
 * All rights reserved.
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */
#ifndef MBEDTLS_SM3_H
#define MBEDTLS_SM3_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <stddef.h>
#include <stdint.h>

#define MBEDTLS_ERR_SM3_BAD_INPUT_DATA                    -0x0075  /**< SM3 input data was malformed. */
/* MBEDTLS_ERR_SM3_HW_ACCEL_FAILED is deprecated and should not be used. */
#define MBEDTLS_ERR_SM3_HW_ACCEL_FAILED                   -0x0039  /**< SM3 hardware accelerator failed */
#define MBEDTLS_ERR_SM3_FEATURE_UNAVAILABLE               -0x0040  /**< SM3 feature not available */

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_SM3_ALT)
// Regular implementation
//

/**
 * \brief          SM3 context structure
 *
 * \warning        SM3 is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 */
typedef struct mbedtls_sm3_context
{
    uint32_t total[2];          /*!< The number of Bytes processed.  */
    uint32_t state[8];          /*!< The intermediate digest state.  */
    unsigned char buffer[64];   /*!< The data block being processed. */
}
mbedtls_sm3_context;

#else  /* MBEDTLS_SM3_ALT */
#include "sm3_alt.h"
#endif /* MBEDTLS_SM3_ALT */

/**
 * \brief          Initialize SM3 context
 *
 * \param ctx      SM3 context to be initialized
 *
 */
void mbedtls_sm3_init( mbedtls_sm3_context *ctx );

/**
 * \brief          Clear SM3 context
 *
 * \param ctx      SM3 context to be cleared
 *
 */
void mbedtls_sm3_free( mbedtls_sm3_context *ctx );

/**
 * \brief          Clone (the state of) an SM3 context
 *
 * \param dst      The destination context
 * \param src      The context to be cloned
 *
 */
void mbedtls_sm3_clone( mbedtls_sm3_context *dst,
                        const mbedtls_sm3_context *src );

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
#if defined(MBEDTLS_DEPRECATED_WARNING)
#define MBEDTLS_DEPRECATED      __attribute__((deprecated))
#else
#define MBEDTLS_DEPRECATED
#endif

/**
 * \brief          SM3 context setup
 *
 * \param ctx      context to be initialized
 *
 */
MBEDTLS_DEPRECATED void mbedtls_sm3_starts( mbedtls_sm3_context *ctx );

/**
 * \brief          SM3 process buffer
 *
 * \param ctx      SM3 context
 * \param input    buffer holding the data
 * \param ilen     length of the input data
 *
 */
MBEDTLS_DEPRECATED void mbedtls_sm3_update( mbedtls_sm3_context *ctx,
                                            const unsigned char *input,
                                            size_t ilen );

/**
 * \brief          SM3 final digest
 *
 * \param ctx      SM3 context
 * \param output   SM3 checksum result
 *
 */
MBEDTLS_DEPRECATED void mbedtls_sm3_finish( mbedtls_sm3_context *ctx,
                                            unsigned char output[32] );

/**
 * \brief          This function processes a single data block within
 *                 the ongoing SM3 computation. This function is for
 *                 internal use only.
 *
 * \param ctx      The SM3 context.
 * \param data     The buffer holding one block of data. This must be
*                  a readable buffer of size \c 64 Bytes.
 *
 */
MBEDTLS_DEPRECATED void mbedtls_sm3_process( mbedtls_sm3_context *ctx,
                                             const unsigned char data[64] );

/**
 * \brief          Output = SM3( input buffer )
 *
 * \param input    buffer holding the data
 * \param ilen     length of the input data
 * \param output   SM3 checksum result
 *
 */
MBEDTLS_DEPRECATED void mbedtls_sm3( const unsigned char *input,
                                     size_t ilen,
                                     unsigned char output[32] );

#undef MBEDTLS_DEPRECATED
#endif /* !MBEDTLS_DEPRECATED_REMOVED */

/**
 * \brief          SM3 context setup
 *
 * \param ctx      context to be initialized
 *
 * \return         \c 0 if successful
 *
 */
int mbedtls_sm3_starts_ret( mbedtls_sm3_context *ctx );

/**
 * \brief          SM3 process buffer
 *
 * \param ctx      SM3 context
 * \param input    buffer holding the data
 * \param ilen     length of the input data
 *
 * \return         \c 0 if successful
 *
 */
int mbedtls_sm3_update_ret( mbedtls_sm3_context *ctx,
                            const unsigned char *input,
                            size_t ilen );

/**
 * \brief          SM3 final digest
 *
 * \param ctx      SM3 context
 * \param output   SM3 checksum result
 *
 * \return         \c 0 if successful
 *
 */
int mbedtls_sm3_finish_ret( mbedtls_sm3_context *ctx,
                            unsigned char output[32] );

/**
 * \brief          SM3 process data block (internal use only)
 *
 * \param ctx      SM3 context
 * \param data     buffer holding one block of data
 *
 * \return         \c 0 if successful
 *
 */
int mbedtls_internal_sm3_process( mbedtls_sm3_context *ctx,
                                  const unsigned char data[64] );

/**
 * \brief          Output = SM3( input buffer )
 *
 * \param input    buffer holding the data
 * \param ilen     length of the input data
 * \param output   SM3 checksum result
 *
 * \return         \c 0 if successful
 *
 */
int mbedtls_sm3_ret( const unsigned char *input,
                     size_t ilen,
                     unsigned char output[32] );

/**
 * \brief          Checkup routine
 *
 * \return         \c 0 if successful, or 1 if the test failed
 *
 */
int mbedtls_sm3_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* mbedtls_sm3.h */
