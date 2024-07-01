/*
 * Copyright (c) 2020-2021, Arm Technology (China) Co., Ltd.
 * All rights reserved.
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#ifndef MBEDTLS_SNOW3G_H
#define MBEDTLS_SNOW3G_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else /* !MBEDTLS_CONFIG_FILE */
#include MBEDTLS_CONFIG_FILE
#endif /* MBEDTLS_CONFIG_FILE */

#if !defined(CFG_MBEDTLS_TE)
#include <linux/types.h>
#else /* !CFG_MBEDTLS_TE */
#include <stddef.h>
#include <stdint.h>
#endif /* CFG_MBEDTLS_TE */

#include "klad.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MBEDTLS_ERR_SNOW3G_ALLOC_FAILED       -0x0010 /**< Failed to allocate
                                                           memory. */
#define MBEDTLS_ERR_SNOW3G_INVALID_KEY_LENGTH -0x0020 /**< Invalid key length. */
#define MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA     -0x0021 /**< Invalid input data. */
#define MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED    -0x0025 /**< hardware accelerator
                                                           failed. */
#if !defined(MBEDTLS_SNOW3G_ALT)
/**
 *  uea2 context structure
 */
typedef struct {
    void *placeholder;  /*just a placeholder meaningless, add sw impl. here */
} mbedtls_uea2_context;

/**
 *  uia2 context structure
 */
typedef struct {
    void *placeholder;  /*just a placeholder meaningless, add sw impl. here */
} mbedtls_uia2_context;
#else /* !MBEDTLS_SNOW3G_ALT */
    #include "snow3g_alt.h"
#endif /* MBEDTLS_SNOW3G_ALT */

/**
 * \brief          This function initializes the specified UEA2 context.
 *
 *                 It must be the first API called before using
 *                 the context.
 *
 * \param ctx      The UEA2 context to initialize. This must not be \c NULL.
 */
void mbedtls_uea2_init( mbedtls_uea2_context *ctx );

/**
 * \brief          This function releases and clears the specified UEA2 context.
 *
 * \param ctx      The UEA2 context to clear.
 *                 If this is \c NULL, this function does nothing.
 *                 Otherwise, the context must have been at least initialized.
 */
void mbedtls_uea2_free( mbedtls_uea2_context *ctx );

/**
 * \brief           This function sets the encryption/decryption key.
 *
 * \note            After using this function, you must also call
 *                  \c mbedtls_uea2_starts() to set count, bearer and direction
 *                  before you start encrypting/decrypting data with
 *                  \c mbedtls_uea2_update(). This function must be called
 *                  before \c mbedtls_uea2_starts().
 *
 * \param ctx       The uea2 context to which the key should be bound.
 *                  It must be initialized.
 * \param key       The encryption/decryption key. This must be \c 16 Bytes
 *                  in length.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if \p ctx or \p key is \c NULL.
 *                  \c MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED if invalid calling sequences.
 */
int mbedtls_uea2_setkey( mbedtls_uea2_context *ctx,
                         const unsigned char key[16] );

/**
 * \brief           This function sets the encryption/decryption secure key.
 *
 * \note            After using this function, you must also call
 *                  \c mbedtls_uea2_starts() to set count, bearer and direction
 *                  before you start encrypting/decrypting data with
 *                  \c mbedtls_uea2_update(). This function must be called
 *                  before \c mbedtls_uea2_starts().
 *
 * \param ctx       The uea2 context to which the key should be bound.
 *                  It must be initialized.
 * \param key       The encryption/decryption secure key. The EK3's keybits must
 *                  be \c 128 bits in length.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if  \p ctx or \p key is
 *                     \c NULL.
 *                  \c MBEDTLS_ERR_SNOW3G_INVALID_KEY_LENGTH if keybits is not
 *                     \c 128.
 *                  \c MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED if invalid calling
 *                     sequences.
 */
int mbedtls_uea2_setseckey( mbedtls_uea2_context *ctx,
                            const mbedtls_klad_seckey_t *key );

/**
 * \brief           This function sets the count, bearer and direction.
 *
 * \note            A uea2 context can be re-used with the same key by
 *                  calling this function to change the count, bearer and direction.
 *
 * \warning         You must never use the same count ,bearer and direction twice
 *                  with the same key.This would void any confidentiality guarantees
 *                  for the messages encrypted with the same count, bearer,
 *                  direction and key.
 *
 * \param ctx       The uea2 context to which the count, bearer and direction
 *                  should be bound.
 *                  It must be initialized and bound to a key.
 * \param count     The count frame dependent input.
 * \param bearer    The bearer identity, the least 5 significant bits valid.
 * \param dir       The direction of transmission, the least 1 significant bit
 *                  valid only.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if \p ctx is \c NULL,
 *                     or \p bearer > 0x1F, or \p dir > 1.
 *                  \c MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED if invalid calling
 *                     sequences.
 */
int mbedtls_uea2_starts( mbedtls_uea2_context *ctx,
                         uint32_t count, uint32_t bearer, uint32_t dir );

/**
 * \brief           This function encrypts or decrypts data.
 *
 *                  Since uea2 is a stream cipher, the same operation is
 *                  used for encrypting and decrypting data.
 *
 * \note            \c mbedtls_uea2_setkey() and
 *                  \c mbedtls_uea2_starts() must be called at least once
 *                  to setup the context before this function can be called.
 *
 * \note            This function can be called multiple times in a row in
 *                  order to encrypt or decrypt data piecewise with the same
 *                  key and count, bearer and dir.
 *
 * \param ctx       The uea2 context to use for encryption or decryption.
 *                  It must be initialized and bound to a key and count, bearer
 *                  and direction.
 * \param size      The length of the \p input data in Bytes.
 * \param input     The buffer holding the input data.
 *                  This pointer can be \c NULL if \p`size == 0`.
 * \param output    The buffer holding the output data.
 *                  This must be able to hold \p size Bytes.
 *                  This pointer can be \c NULL if \p`size == 0`.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if ctx is NULL, or
 *                     \p size > 0 and \p input or \p output is \c NULL.
 *                  \c MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED if invalid calling
 *                     sequences.
 */
int mbedtls_uea2_update( mbedtls_uea2_context *ctx,
                         size_t size, const unsigned char *input,
                         unsigned char *output );

/**
 * \brief        This function finishes the uea2 operation.
 *
 * \param ctx    The uea2 context to use and it must be initialized and
 *               have an uea2 operation started.
 *
 * \return       \c 0 on success.
 *               \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if ctx is invalid.
 *               \c MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED if invalid calling
 *                  sequences.
 */
int mbedtls_uea2_finish( mbedtls_uea2_context *ctx );

/**
 * \brief           This function encrypts or decrypts data with uea2 and
 *                  the given key and count, bearer and direction.
 *
 *                  Since uea2 is a stream cipher, the same operation is
 *                  used for encrypting and decrypting data.
 *
 * \warning         You must never use the same (key, count, bearer, dir) pair
 *                  more than once. This would void any confidentiality
 *                  guarantees for the messages encrypted with the same nonce
 *                  and key.
 *
 * \param key       The encryption/decryption key.
 *                  This must be \c 16 Bytes in length.
 * \param count     The count frame dependent input.
 * \param bearer    The bearer identify, the least 5 significant bits valid.
 * \param dir       The direction of transmission, the least 1 significant bit
 *                  valid only.
 * \param size      The length of the \p input data in Bytes.
 * \param input     The buffer holding the input data.
 *                  This pointer can be \c NULL if \p`size == 0`.
 * \param output    The buffer holding the output data.
 *                  This must be able to hold \p size Bytes.
 *                  This pointer can be \c NULL if \p`size == 0`.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA on following condictions:
 *                      1. \p key is NULL.
 *                      2. \p \p bearer > 0x1F.
 *                      3. \p dir > 1.
 *                      4. \p size > 0 and \p input or \p output is NULL.
 */
int mbedtls_uea2_crypt( const unsigned char key[16],
                        uint32_t count, uint32_t bearer, uint32_t dir,
                        size_t size, const unsigned char *input,
                        unsigned char *output );

/**
 * \brief           This function encrypts or decrypts data with uea2 and
 *                  the given key and count, bearer and direction.
 *
 *                  Since uea2 is a stream cipher, the same operation is
 *                  used for encrypting and decrypting data.
 *
 * \warning         You must never use the same (key, count, bearer, dir) pair
 *                  more than once. This would void any confidentiality
 *                  guarantees for the messages encrypted with the same nonce
 *                  and key.
 *
 * \param key       The encryption/decryption secure key.
 *                  The EK3 must be \c 128 bits in length.
 * \param count     The count frame dependent input.
 * \param bearer    The bearer identify, the least 5 significant bits valid.
 * \param dir       The direction of transmission, the least 1 significant bit
 *                  valid only.
 * \param size      The length of the \p input data in Bytes.
 * \param input     The buffer holding the input data.
 *                  This pointer can be \c NULL if \p`size == 0`.
 * \param output    The buffer holding the output data.
 *                  This must be able to hold \p size Bytes.
 *                  This pointer can be \c NULL if \p`size == 0`.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA on following condictions:
 *                      1. \p key is NULL.
 *                      2. \p bearer >  0x1F.
 *                      3. \p dir > 1.
 *                      4. \p size > 0 and \p input or \p output is NULL.
 *                  \c MBEDTLS_ERR_SNOW3G_INVALID_KEY_LENGTH if keybits is not
 *                     \c 128.
 */
int mbedtls_uea2_crypt_seckey( const mbedtls_klad_seckey_t *key,
                               uint32_t count, uint32_t bearer, uint32_t dir,
                               size_t size, const unsigned char *input,
                               unsigned char *output );

/**
 * \brief          This function initializes the specified UIA2 context.
 *
 *                 It must be the first API called before using
 *                 the context.
 *
 * \param ctx      The UIA2 context to initialize. This must not be \c NULL.
 */
void mbedtls_uia2_init( mbedtls_uia2_context *ctx );

/**
 * \brief          This function releases and clears the specified UIA2 context.
 *
 * \param ctx      The UIA2 context to clear.
 *                 If this is \c NULL, this function does nothing.
 *                 Otherwise, the context must have been at least initialized.
 */
void mbedtls_uia2_free( mbedtls_uia2_context *ctx );

/**
 * \brief           This function sets the key.
 *
 * \note            After using this function, you must also call
 *                  \c mbedtls_uia2_starts() to set count, fresh and direction
 *                  before you start authentication data with
 *                  \c mbedtls_uia2_update(). This function must be called
 *                  before \c mbedtls_uia_starts().
 *
 * \param ctx       The uia2 context to which the key should be bound.
 *                  It must be initialized.
 * \param key       The encryption/decryption key. This must be \c 16 Bytes
 *                  in length.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if \p ctx or \p key
 *                     is \c NULL.
 *                  \c MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED if invalid calling
 *                     sequences.
 */
int mbedtls_uia2_setkey( mbedtls_uia2_context *ctx,
                         const unsigned char key[16] );

/**
 * \brief           This function sets the secure key.
 *
 * \note            After using this function, you must also call
 *                  \c mbedtls_uia2_starts() to set count, fresh and direction
 *                  before you start authentication data with
 *                  \c mbedtls_uia2_update(). This function must be called
 *                  before \c mbedtls_uia_starts().
 *
 * \param ctx       The uia2 context to which the key should be bound.
 *                  It must be initialized.
 * \param key       The encryption/decryption secure key. The EK3's keybits must be
 *                  128 bits in length.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if \p ctx or \p key is
 *                     \c NULL.
 *                  \c MBEDTLS_ERR_SNOW3G_INVALID_KEY_LENGTH if keybits is not
 *                     \c 128.
 *                  \c MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED if invalid calling
 *                     sequences.
 */
int mbedtls_uia2_setseckey( mbedtls_uia2_context *ctx,
                            const mbedtls_klad_seckey_t *key );

/**
 * \brief           This function sets the count, fresh and direction.
 *
 * \note            A uia2 context can be re-used with the same key by
 *                  calling this function to change the count, fresh and direction.
 *
 * \warning         You must never use the same count ,fresh and direction twice
 *                  with the same key.This would void any confidentiality guarantees
 *                  for the messages calculated MAC with the same count,fresh,
 *                  direction and key.
 *
 * \param ctx       The uia2 context to which the count, bearer and direction
 *                  should be bound.
 *                  It must be initialized and bound to a key.
 * \param count     The count frame dependent input.
 * \param fresh     The fresh random number.
 * \param dir       The direction of transmission, the least 1 sinificant bit
 *                  valid only.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if ctx is NULL or
 *                     \dir >  1.
 *                  \c MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED if invalid calling
 *                     sequences.
 */
int mbedtls_uia2_starts( mbedtls_uia2_context *ctx,
                         uint32_t count, uint32_t fresh,
                         uint32_t dir );

/**
 * \brief           This function feeds the input data into an ongoing uia2 session.
 *
 * \note            \c mbedtls_uia2_setkey() and
 *                  \c mbedtls_uia2_starts() must be called at least once
 *                  to setup the context before this function can be called.
 *
 * \note            This function can be called multiple times in a row in
 *                  order to handle data piecewise with the same key and nonce.
 *
 * \param ctx       The uia2 context to use for MAC operation.
 *                  It must be initialized and bound to a key and count, fresh and
 *                  direction.
 * \param size      The length of the \p input data in Bytes.
 * \param input     The buffer holding the input data.
 *                  This pointer can be \c NULL if \p`size == 0`.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if \p ctx is \c NULL or,
 *                     \p size > 0 and \p input or \p output is \c NULL.
 *                  \c MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED if invalid calling
 *                     sequences.
 */
int mbedtls_uia2_update( mbedtls_uia2_context *ctx,
                         size_t size, const unsigned char *input );

/**
 * \brief               This function finishes the uia2 MAC operation, and writes
 *                      the result to the output buffer.
 *
 *                      It is called after \c mbedtls_uia2_starts() or
 *                      \c mbedtls_uia2_update().
 *
 * \param ctx           The snow3g uia2 context used for the uia2 MAC operation.
 * \param output        The output buffer for the uia2 MAC result.
 *
 * \return              \c 0 on success.
 *                      \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if ctx is \c NULL or
 *                         \p output is \c NULL.
 *                      \c MBEDTLS_ERR_SNOW3G_HW_ACCEL_FAILED if invalid
 *                         calling sequences.
 */
int mbedtls_uia2_finish( mbedtls_uia2_context *ctx,
                         unsigned char output[4] );

/**
 * \brief           This function perform an uia2 MAC operation with uia2 and
 *                  the given key and count, fresh and direction.
 *
 * \warning         You must never use the same (key, count, fresh, dir) pair
 *                  more than once. This would void any confidentiality
 *                  guarantees for the messages MAC calculation with the same
 *                  key, count, fresh and direction.
 *
 * \param key       The uia2 MAC operation key, it must be \c 128 bits in length.
 * \param count     The count frame dependent input.
 * \param fresh     The fresh random number.
 * \param dir       The direction of transmission, the least 1 significant bit
 *                  valid only.
 * \param size      The length of the \p input data in Bytes.
 * \param input     The buffer holding the input data.
 *                  This pointer can be \c NULL if \p`size == 0`.
 * \param output    The output buffer for the uia2 MAC result.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if \p key is \c NULL, or
 *                      \p output is \c NULL, or \p size > 0 and \p input is
 *                      \c NULL or \p dir > 1.
 */
int mbedtls_uia2_mac( const unsigned char key[16],
                      uint32_t count, uint32_t fresh, uint32_t dir,
                      size_t size, const unsigned char *input,
                      unsigned char output[4] );

/**
 * \brief           This function perform an uia2 MAC operation with uia2 and
 *                  the given key and count, fresh and direction.
 *
 * \warning         You must never use the same (key, count, fresh, dir) pair
 *                  more than once. This would void any confidentiality
 *                  guarantees for the messages MAC calculation with the same
 *                  key, count, fresh and direction.
 *
 * \param key       The uia2 MAC operation secure key, it's ek3 must be
 *                  \c 128 bits in length.
 * \param count     The count frame dependent input.
 * \param fresh     The fresh random number.
 * \param dir       The direction of transmission, the least 1 significant bit
 *                  valid only.
 * \param size      The length of the \p input data in Bytes.
 * \param input     The buffer holding the input data.
 *                  This pointer can be \c NULL if \p`size == 0`.
 * \param output    The output buffer for the uia2 MAC result.
 *
 * \return          \c 0 on success.
 *                  \c MBEDTLS_ERR_SNOW3G_BAD_INPUT_DATA if \p key is NULL, or
 *                      \p output is NULL, or \p size > 0 and \p input is \c NULL,
 *                      or \p dir > 1.
 *                  \c MBEDTLS_ERR_SNOW3G_INVALID_KEY_LENGTH on keybits is
 *                     not \c 128.
 */
int mbedtls_uia2_mac_seckey( const mbedtls_klad_seckey_t *key,
                             uint32_t count, uint32_t fresh, uint32_t dir,
                             size_t size, const unsigned char *input,
                             unsigned char output[4] );

/**
 * \brief          Checkup routine.
 *
 * \return         \c 0 on success.
 * \return         any other values != \c 0 on failure.
 */
int mbedtls_snow3g_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_SNOW3G_H */
