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
#ifndef MBEDTLS_OTP_H
#define MBEDTLS_OTP_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <stddef.h>
#include <stdint.h>

/* MBEDTLS_ERR_AES_FEATURE_UNAVAILABLE is deprecated and should not be used. */
#define MBEDTLS_ERR_OTP_FEATURE_UNAVAILABLE               -0x0023  /**< Feature not available. For example, an unsupported AES key size. */

/* MBEDTLS_ERR_OTP_HW_ACCEL_FAILED is deprecated and should not be used. */
#define MBEDTLS_ERR_OTP_HW_ACCEL_FAILED                   -0x002F  /**< OTP hardware accelerator failed */

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_OTP_ALT)

/**
 * \brief          OTP context structure
 *
 * \warning        OTP is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 */
typedef struct mbedtls_otp_context
{
   void *placeholder; /**< just a place holder, meaningless, maybe a otp layout in the future */
} mbedtls_otp_context;

/**
 * \brief          OTP context structure
 *
 * \warning        OTP is considered a weak message digest and its use
 *                 constitutes a security risk. We recommend considering
 *                 stronger message digests instead.
 *
 */
typedef struct mbedtls_otp_conf
{
   void *placeholder; /**< just a place holder, meaningless.*/
}
mbedtls_otp_conf;

#else  /* MBEDTLS_OTP_ALT */
#include "otp_alt.h"
#endif /* MBEDTLS_OTP_ALT */

/**
 * \brief          Initialize OTP context
 *
 * \param ctx      OTP context to be initialized
 *
 */
void mbedtls_otp_init( mbedtls_otp_context *ctx );

/**
 * \brief          Clear OTP context
 *
 * \param ctx      OTP context to be cleared
 *
 */
void mbedtls_otp_free( mbedtls_otp_context *ctx );

/**
 * \brief          This function reads OTP data
 *
 * \param ctx      The OTP context.
 * \param off      the offset from the start address of OTP.
 * \param buf      The buffer to hold data.
 * \param len      Length expected to read.
 *
 * \return         \c 0 if successful
 */
int mbedtls_otp_read( mbedtls_otp_context *ctx,
                        size_t off,
                        uint8_t *buf,
                        size_t len );

/**
 * \brief          This function gets OTP's config.
 *
 * \param ctx      The OTP context.
 * \param conf     The config obj to hold the config data.
 *
 * \return         \c 0 if successful
 *
 */
int mbedtls_otp_get_conf(mbedtls_otp_context *ctx,
                         mbedtls_otp_conf *conf);

/**
 * \brief          write data to otp
 *
 * \param ctx      OTP context
 * \param off      the offset from the start address of OTP.
 * \param buf      buffer holding the data that to be written.
 * \param ilen     length of teh buffer to be written.
 *
 * \return         \c 0 if successful
 *
 */
int mbedtls_otp_write( mbedtls_otp_context *ctx,
                  size_t off,
                  const uint8_t *buf,
                  size_t len );

#ifdef CFG_OTP_WITH_PUF
/**
 * \brief          This function generates the device root key. When PUF enable
 *                 it's the only way to derive device root key.
 *
 * \param ctx      OTP context
 *
 * \return         \c 0 if successful
 *
 */
int mbedtls_otp_puf_enroll(mbedtls_otp_context *ctx);

/**
 * \brief          This function checks the physical status of PUF.
 *
 * \param ctx      OTP context
 *
 * \return         \c 0 if in good status.
 *
 */
int mbedtls_otp_puf_quality_check(mbedtls_otp_context *ctx);

/**
 * \brief          This function does initial margin reading when PUF enable.
 *
 * \param ctx      The OTP context.
 * \param off      the offset from the start address of OTP.
 * \param buf      The buffer to hold data.
 * \param len      Length expected to read.
 *
 * \return         \c 0 if successsful.
 *
 */
int mbedtls_otp_puf_initial_margin_read( mbedtls_otp_context *ctx,
                                            size_t off,
                                            uint8_t *buf,
                                            size_t len );

/**
 * \brief          This function does program margin reading when PUF enable.
 *
 * \param ctx      The OTP context.
 * \param off      the offset from the start address of OTP.
 * \param buf      The buffer to hold data.
 * \param len      Length expected to read.
 *
 * \return         \c 0 if successsful.
 *
 */
int mbedtls_otp_puf_pgm_margin_read( mbedtls_otp_context *ctx,
                                            size_t off,
                                            uint8_t *buf,
                                            size_t len );

#endif
#ifdef __cplusplus
}
#endif

#endif /* mbedtls_otp.h */
