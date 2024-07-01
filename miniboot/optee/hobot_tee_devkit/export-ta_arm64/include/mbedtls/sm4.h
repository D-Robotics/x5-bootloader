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

#ifndef MBEDTLS_SM4_H
#define MBEDTLS_SM4_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <stddef.h>
#include <stdint.h>

/* padlock.c and sm4ni.c rely on these values! */
#define MBEDTLS_SM4_ENCRYPT     1 /**< SM4 encryption. */
#define MBEDTLS_SM4_DECRYPT     0 /**< SM4 decryption. */

#define MBEDTLS_ERR_SM4_ALLOC_FAILED                      -0x0010  /**< Failed to allocate memory. */

/* Error codes in range 0x0020-0x0022 */
#define MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH                -0x0020  /**< Invalid key length. */
#define MBEDTLS_ERR_SM4_INVALID_INPUT_LENGTH              -0x0022  /**< Invalid data input length. */

/* Error codes in range 0x0021-0x0025 */
#define MBEDTLS_ERR_SM4_BAD_INPUT_DATA                    -0x0021  /**< Invalid input data. */

/* MBEDTLS_ERR_SM4_FEATURE_UNAVAILABLE is deprecated and should not be used. */
#define MBEDTLS_ERR_SM4_FEATURE_UNAVAILABLE               -0x0023  /**< Feature not available. For example, an unsupported SM4 key size. */

/* MBEDTLS_ERR_SM4_HW_ACCEL_FAILED is deprecated and should not be used. */
#define MBEDTLS_ERR_SM4_HW_ACCEL_FAILED                   -0x0025  /**< SM4 hardware accelerator failed. */

#if ( defined(__ARMCC_VERSION) || defined(_MSC_VER) ) && \
    !defined(inline) && !defined(__cplusplus)
#define inline __inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_SM4_ALT)

/**
 * \brief The SM4 context-type definition.
 */
typedef struct mbedtls_sm4_context
{
    uint32_t rk[32];            /*!< SM4 round keys. */
}
mbedtls_sm4_context;

#if defined(MBEDTLS_CIPHER_MODE_XTS)
/**
 * \brief The SM4 XTS context-type definition.
 */
typedef struct mbedtls_sm4_xts_context
{
    mbedtls_sm4_context crypt; /*!< The SM4 context to use for SM4 block
                                        encryption or decryption. */
    mbedtls_sm4_context tweak; /*!< The SM4 context used for tweak
                                        computation. */
} mbedtls_sm4_xts_context;
#endif /* MBEDTLS_CIPHER_MODE_XTS */

#else  /* MBEDTLS_SM4_ALT */
#include "sm4_alt.h"
#endif /* MBEDTLS_SM4_ALT */
/**
* Trust engine key ladder root key selection enumeration
*/
typedef enum mbedtls_sm4_key_sel {
    MBEDTLS_SM4_KL_KEY_MODEL = 0,         /**< model key */
    MBEDTLS_SM4_KL_KEY_ROOT               /**< device root key */
} mbedtls_sm4_key_sel_t;

/**
 * secure key structure
 */
typedef struct mbedtls_sm4_sec_key {
    mbedtls_sm4_key_sel_t sel;   /**< key ladder root key selection */
    uint32_t ek3bits;            /**< ek3 length in bits, fixed to 128 */
    union {
        struct {
            uint8_t ek1[16];     /**< encrypted key1 (fixed to 128-bit) */
            uint8_t ek2[16];     /**< encrypted key2 (fixed to 128-bit) */
            uint8_t ek3[32];     /**< encrypted key3 */
        };
        uint8_t eks[64];         /**< ek1 || ek2 || ek3 */
    };
} mbedtls_sm4_sec_key_t;

/**
 * \brief          This function initializes the specified SM4 context.
 *
 *                 It must be the first API called before using
 *                 the context.
 *
 * \param ctx      The SM4 context to initialize. This must not be \c NULL.
 */
void mbedtls_sm4_init( mbedtls_sm4_context *ctx );

/**
 * \brief          This function clones the state of the specified SM4 context.
 *
 * \param dst      The SM4 context to clone to.
 * \param src      The SM4 context to clone from.
 * \return          \c 0 on success.
 */
int mbedtls_sm4_clone( mbedtls_sm4_context *dst,
                       const mbedtls_sm4_context *src );

/**
 * \brief          This function releases and clears the specified SM4 context.
 *
 * \param ctx      The SM4 context to clear.
 *                 If this is \c NULL, this function does nothing.
 *                 Otherwise, the context must have been at least initialized.
 */
void mbedtls_sm4_free( mbedtls_sm4_context *ctx );

#if defined(MBEDTLS_CIPHER_MODE_XTS)
/**
 * \brief          This function initializes the specified SM4 XTS context.
 *
 *                 It must be the first API called before using
 *                 the context.
 *
 * \param ctx      The SM4 XTS context to initialize. This must not be \c NULL.
 */
void mbedtls_sm4_xts_init( mbedtls_sm4_xts_context *ctx );

/**
 * \brief          This function clones the state of the specified SM4 XTS context.
 *
 * \param dst      The SM4 XTS context to clone to.
 * \param src      The SM4 XTS context to clone from.
 * \return          \c 0 on success.
 */
int mbedtls_sm4_xts_clone( mbedtls_sm4_xts_context *dst,
                           const mbedtls_sm4_xts_context *src );

/**
 * \brief          This function releases and clears the specified SM4 XTS context.
 *
 * \param ctx      The SM4 XTS context to clear.
 *                 If this is \c NULL, this function does nothing.
 *                 Otherwise, the context must have been at least initialized.
 */
void mbedtls_sm4_xts_free( mbedtls_sm4_xts_context *ctx );
#endif /* MBEDTLS_CIPHER_MODE_XTS */

/**
 * \brief          This function sets the encryption key.
 *
 * \param ctx      The SM4 context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The encryption key.
 *                 This must be a readable buffer of size \p keybits bits.
 * \param keybits  The size of data passed in bits. Valid options are:
 *                 <ul><li>128 bits</li>
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on failure.
 */
int mbedtls_sm4_setkey_enc( mbedtls_sm4_context *ctx, const unsigned char *key,
                    unsigned int keybits );

/**
 * \brief          This function sets the encryption key.
 *
 * \param ctx      The SM4 context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The encryption secure key.
 *                 including ek1 ek2 ek3.
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on failure.
 */
int mbedtls_sm4_setseckey_enc( mbedtls_sm4_context *ctx,
                               mbedtls_sm4_sec_key_t *key );

/**
 * \brief          This function sets the decryption key.
 *
 * \param ctx      The SM4 context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The decryption key.
 *                 This must be a readable buffer of size \p keybits bits.
 * \param keybits  The size of data passed. Valid options are:
 *                 <ul><li>128 bits</li>
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on failure.
 */
int mbedtls_sm4_setkey_dec( mbedtls_sm4_context *ctx, const unsigned char *key,
                    unsigned int keybits );

/**
 * \brief          This function sets the decryption key.
 *
 * \param ctx      The SM4 context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The decryption secure key.
 *                 including ek1 ek2 ek3.
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on failure.
 */

int mbedtls_sm4_setseckey_dec( mbedtls_sm4_context *ctx,
                               mbedtls_sm4_sec_key_t *key );
#if defined(MBEDTLS_CIPHER_MODE_XTS)
/**
 * \brief          This function prepares an XTS context for encryption and
 *                 sets the encryption key.
 *
 * \param ctx      The SM4 XTS context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The encryption key. This is comprised of the XTS key1
 *                 concatenated with the XTS key2.
 *                 This must be a readable buffer of size \p keybits bits.
 * \param keybits  The size of \p key passed in bits. Valid options are:
 *                 <ul><li>256 bits (each of key1 and key2 is a 128-bit key)</li>
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on failure.
 */
int mbedtls_sm4_xts_setkey_enc( mbedtls_sm4_xts_context *ctx,
                                const unsigned char *key,
                                unsigned int keybits );

/**
 * \brief          This function prepares an XTS context for encryption and
 *                 sets the encryption key.
 *
 * \param ctx      The SM4 XTS context to which the key should be bound.
 *                 It must be initialized.
 * \param key1      The encryption secure key. This is the XTS key1.
 * \param key2      The encryption secure key. This is the XTS key2.
 *                 This must be a readable buffer of size \p keybits bits.
 * \param keybits  The size of \p key passed in bits. Valid options are:
 *                 <ul><li>256 bits (each of key1 and key2 is a 128-bit key)</li>
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on failure.
 */
int mbedtls_sm4_xts_setseckey_enc( mbedtls_sm4_xts_context *ctx,
                                       mbedtls_sm4_sec_key_t *key1,
                                       mbedtls_sm4_sec_key_t *key2);

/**
 * \brief          This function prepares an XTS context for decryption and
 *                 sets the decryption key.
 *
 * \param ctx      The SM4 XTS context to which the key should be bound.
 *                 It must be initialized.
 * \param key      The decryption key. This is comprised of the XTS key1
 *                 concatenated with the XTS key2.
 *                 This must be a readable buffer of size \p keybits bits.
 * \param keybits  The size of \p key passed in bits. Valid options are:
 *                 <ul><li>256 bits (each of key1 and key2 is a 128-bit key)</li>
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on failure.
 */
int mbedtls_sm4_xts_setkey_dec( mbedtls_sm4_xts_context *ctx,
                                const unsigned char *key,
                                unsigned int keybits );

/**
 * \brief          This function prepares an XTS context for decryption and
 *                 sets the decryption key.
 *
 * \param ctx      The SM4 XTS context to which the key should be bound.
 *                 It must be initialized.
 * \param key1     The decryption secure key. This is the XTS key1.
 * \param key2     The decryption secure key. This is the XTS key2.
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_SM4_INVALID_KEY_LENGTH on failure.
 */
int mbedtls_sm4_xts_setseckey_dec( mbedtls_sm4_xts_context *ctx,
                                       mbedtls_sm4_sec_key_t *key1,
                                       mbedtls_sm4_sec_key_t *key2);
#endif /* MBEDTLS_CIPHER_MODE_XTS */

/**
 * \brief          This function performs an SM4 single-block encryption or
 *                 decryption operation.
 *
 *                 It performs the operation defined in the \p mode parameter
 *                 (encrypt or decrypt), on the input data buffer defined in
 *                 the \p input parameter.
 *
 *                 mbedtls_sm4_init(), and either mbedtls_sm4_setkey_enc() or
 *                 mbedtls_sm4_setkey_dec() must be called before the first
 *                 call to this API with the same context.
 *
 * \param ctx      The SM4 context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param mode     The SM4 operation: #MBEDTLS_SM4_ENCRYPT or
 *                 #MBEDTLS_SM4_DECRYPT.
 * \param input    The buffer holding the input data.
 *                 It must be readable and at least \c 16 Bytes long.
 * \param output   The buffer where the output data will be written.
 *                 It must be writeable and at least \c 16 Bytes long.

 * \return         \c 0 on success.
 */
int mbedtls_sm4_crypt_ecb( mbedtls_sm4_context *ctx,
                    int mode,
                    const unsigned char input[16],
                    unsigned char output[16] );

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/**
 * \brief  This function performs an SM4-CBC encryption or decryption operation
 *         on full blocks.
 *
 *         It performs the operation defined in the \p mode
 *         parameter (encrypt/decrypt), on the input data buffer defined in
 *         the \p input parameter.
 *
 *         It can be called as many times as needed, until all the input
 *         data is processed. mbedtls_sm4_init(), and either
 *         mbedtls_sm4_setkey_enc() or mbedtls_sm4_setkey_dec() must be called
 *         before the first call to this API with the same context.
 *
 * \note   This function operates on full blocks, that is, the input size
 *         must be a multiple of the SM4 block size of \c 16 Bytes.
 *
 * \note   Upon exit, the content of the IV is updated so that you can
 *         call the same function again on the next
 *         block(s) of data and get the same result as if it was
 *         encrypted in one call. This allows a "streaming" usage.
 *         If you need to retain the contents of the IV, you should
 *         either save it manually or use the cipher module instead.
 *
 *
 * \param ctx      The SM4 context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param mode     The SM4 operation: #MBEDTLS_SM4_ENCRYPT or
 *                 #MBEDTLS_SM4_DECRYPT.
 * \param length   The length of the input data in Bytes. This must be a
 *                 multiple of the block size (\c 16 Bytes).
 * \param iv       Initialization vector (updated after use).
 *                 It must be a readable and writeable buffer of \c 16 Bytes.
 * \param input    The buffer holding the input data.
 *                 It must be readable and of size \p length Bytes.
 * \param output   The buffer holding the output data.
 *                 It must be writeable and of size \p length Bytes.
 *
 * \return         \c 0 on success.
 * \return         #MBEDTLS_ERR_SM4_INVALID_INPUT_LENGTH
 *                 on failure.
 */
int mbedtls_sm4_crypt_cbc( mbedtls_sm4_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output );
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_XTS)
/**
 * \brief      This function performs an SM4-XTS encryption or decryption
 *             operation for an entire XTS data unit.
 *
 *             SM4-XTS encrypts or decrypts blocks based on their location as
 *             defined by a data unit number. The data unit number must be
 *             provided by \p data_unit.
 *
 *             NIST SP 800-38E limits the maximum size of a data unit to 2^20
 *             SM4 blocks. If the data unit is larger than this, this function
 *             returns #MBEDTLS_ERR_SM4_INVALID_INPUT_LENGTH.
 *
 * \param ctx          The SM4 XTS context to use for SM4 XTS operations.
 *                     It must be initialized and bound to a key.
 * \param mode         The SM4 operation: #MBEDTLS_SM4_ENCRYPT or
 *                     #MBEDTLS_SM4_DECRYPT.
 * \param length       The length of a data unit in Bytes. This can be any
 *                     length between 16 bytes and 2^24 bytes inclusive
 *                     (between 1 and 2^20 block cipher blocks).
 * \param data_unit    The address of the data unit encoded as an array of 16
 *                     bytes in little-endian format. For disk encryption, this
 *                     is typically the index of the block device sector that
 *                     contains the data. Updated after used.
 * \param input        The buffer holding the input data (which is an entire
 *                     data unit). This function reads \p length Bytes from \p
 *                     input.
 * \param output       The buffer holding the output data (which is an entire
 *                     data unit). This function writes \p length Bytes to \p
 *                     output.
 *
 * \return             \c 0 on success.
 * \return             #MBEDTLS_ERR_SM4_INVALID_INPUT_LENGTH if \p length is
 *                     smaller than an SM4 block in size (16 Bytes) or if \p
 *                     length is larger than 2^20 blocks (16 MiB).
 */
int mbedtls_sm4_crypt_xts( mbedtls_sm4_xts_context *ctx,
                           int mode,
                           size_t length,
                           unsigned char data_unit[16],
                           const unsigned char *input,
                           unsigned char *output );
#endif /* MBEDTLS_CIPHER_MODE_XTS */

#if defined(MBEDTLS_CIPHER_MODE_CFB)
/**
 * \brief This function performs an SM4-CFB128 encryption or decryption
 *        operation.
 *
 *        It performs the operation defined in the \p mode
 *        parameter (encrypt or decrypt), on the input data buffer
 *        defined in the \p input parameter.
 *
 *        For CFB, you must set up the context with mbedtls_sm4_setkey_enc(),
 *        regardless of whether you are performing an encryption or decryption
 *        operation, that is, regardless of the \p mode parameter. This is
 *        because CFB mode uses the same key schedule for encryption and
 *        decryption.
 *
 * \note  Upon exit, the content of the IV is updated so that you can
 *        call the same function again on the next
 *        block(s) of data and get the same result as if it was
 *        encrypted in one call. This allows a "streaming" usage.
 *        If you need to retain the contents of the
 *        IV, you must either save it manually or use the cipher
 *        module instead.
 *
 *
 * \param ctx      The SM4 context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param mode     The SM4 operation: #MBEDTLS_SM4_ENCRYPT or
 *                 #MBEDTLS_SM4_DECRYPT.
 * \param length   The length of the input data in Bytes.
 * \param iv_off   The offset in IV (updated after use).
 *                 It must point to a valid \c size_t.
 * \param iv       The initialization vector (updated after use).
 *                 It must be a readable and writeable buffer of \c 16 Bytes.
 * \param input    The buffer holding the input data.
 *                 It must be readable and of size \p length Bytes.
 * \param output   The buffer holding the output data.
 *                 It must be writeable and of size \p length Bytes.
 *
 * \return         \c 0 on success.
 */
int mbedtls_sm4_crypt_cfb128( mbedtls_sm4_context *ctx,
                       int mode,
                       size_t length,
                       size_t *iv_off,
                       unsigned char iv[16],
                       const unsigned char *input,
                       unsigned char *output );

/**
 * \brief This function performs an SM4-CFB8 encryption or decryption
 *        operation.
 *
 *        It performs the operation defined in the \p mode
 *        parameter (encrypt/decrypt), on the input data buffer defined
 *        in the \p input parameter.
 *
 *        Due to the nature of CFB, you must use the same key schedule for
 *        both encryption and decryption operations. Therefore, you must
 *        use the context initialized with mbedtls_sm4_setkey_enc() for
 *        both #MBEDTLS_SM4_ENCRYPT and #MBEDTLS_SM4_DECRYPT.
 *
 * \note  Upon exit, the content of the IV is updated so that you can
 *        call the same function again on the next
 *        block(s) of data and get the same result as if it was
 *        encrypted in one call. This allows a "streaming" usage.
 *        If you need to retain the contents of the
 *        IV, you should either save it manually or use the cipher
 *        module instead.
 *
 *
 * \param ctx      The SM4 context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param mode     The SM4 operation: #MBEDTLS_SM4_ENCRYPT or
 *                 #MBEDTLS_SM4_DECRYPT
 * \param length   The length of the input data.
 * \param iv       The initialization vector (updated after use).
 *                 It must be a readable and writeable buffer of \c 16 Bytes.
 * \param input    The buffer holding the input data.
 *                 It must be readable and of size \p length Bytes.
 * \param output   The buffer holding the output data.
 *                 It must be writeable and of size \p length Bytes.
 *
 * \return         \c 0 on success.
 */
int mbedtls_sm4_crypt_cfb8( mbedtls_sm4_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output );
#endif /*MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_OFB)
/**
 * \brief       This function performs an SM4-OFB (Output Feedback Mode)
 *              encryption or decryption operation.
 *
 *              For OFB, you must set up the context with
 *              mbedtls_sm4_setkey_enc(), regardless of whether you are
 *              performing an encryption or decryption operation. This is
 *              because OFB mode uses the same key schedule for encryption and
 *              decryption.
 *
 *              The OFB operation is identical for encryption or decryption,
 *              therefore no operation mode needs to be specified.
 *
 * \note        Upon exit, the content of iv, the Initialisation Vector, is
 *              updated so that you can call the same function again on the next
 *              block(s) of data and get the same result as if it was encrypted
 *              in one call. This allows a "streaming" usage, by initialising
 *              iv_off to 0 before the first call, and preserving its value
 *              between calls.
 *
 *              For non-streaming use, the iv should be initialised on each call
 *              to a unique value, and iv_off set to 0 on each call.
 *
 *              If you need to retain the contents of the initialisation vector,
 *              you must either save it manually or use the cipher module
 *              instead.
 *
 * \warning     For the OFB mode, the initialisation vector must be unique
 *              every encryption operation. Reuse of an initialisation vector
 *              will compromise security.
 *
 * \param ctx      The SM4 context to use for encryption or decryption.
 *                 It must be initialized and bound to a key.
 * \param length   The length of the input data.
 * \param iv_off   The offset in IV (updated after use).
 *                 It must point to a valid \c size_t.
 * \param iv       The initialization vector (updated after use).
 *                 It must be a readable and writeable buffer of \c 16 Bytes.
 * \param input    The buffer holding the input data.
 *                 It must be readable and of size \p length Bytes.
 * \param output   The buffer holding the output data.
 *                 It must be writeable and of size \p length Bytes.
 *
 * \return         \c 0 on success.
 */
int mbedtls_sm4_crypt_ofb( mbedtls_sm4_context *ctx,
                       size_t length,
                       size_t *iv_off,
                       unsigned char iv[16],
                       const unsigned char *input,
                       unsigned char *output );

#endif /* MBEDTLS_CIPHER_MODE_OFB */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
/**
 * \brief      This function performs an SM4-CTR encryption or decryption
 *             operation.
 *
 *             This function performs the operation defined in the \p mode
 *             parameter (encrypt/decrypt), on the input data buffer
 *             defined in the \p input parameter.
 *
 *             Due to the nature of CTR, you must use the same key schedule
 *             for both encryption and decryption operations. Therefore, you
 *             must use the context initialized with mbedtls_sm4_setkey_enc()
 *             for both #MBEDTLS_SM4_ENCRYPT and #MBEDTLS_SM4_DECRYPT.
 *
 * \warning    You must never reuse a nonce value with the same key. Doing so
 *             would void the encryption for the two messages encrypted with
 *             the same nonce and key.
 *
 *             There are two common strategies for managing nonces with CTR:
 *
 *             1. You can handle everything as a single message processed over
 *             successive calls to this function. In that case, you want to
 *             set \p nonce_counter and \p nc_off to 0 for the first call, and
 *             then preserve the values of \p nonce_counter, \p nc_off and \p
 *             stream_block across calls to this function as they will be
 *             updated by this function.
 *
 *             With this strategy, you must not encrypt more than 2**128
 *             blocks of data with the same key.
 *
 *             2. You can encrypt separate messages by dividing the \p
 *             nonce_counter buffer in two areas: the first one used for a
 *             per-message nonce, handled by yourself, and the second one
 *             updated by this function internally.
 *
 *             For example, you might reserve the first 12 bytes for the
 *             per-message nonce, and the last 4 bytes for internal use. In that
 *             case, before calling this function on a new message you need to
 *             set the first 12 bytes of \p nonce_counter to your chosen nonce
 *             value, the last 4 to 0, and \p nc_off to 0 (which will cause \p
 *             stream_block to be ignored). That way, you can encrypt at most
 *             2**96 messages of up to 2**32 blocks each with the same key.
 *
 *             The per-message nonce (or information sufficient to reconstruct
 *             it) needs to be communicated with the ciphertext and must be unique.
 *             The recommended way to ensure uniqueness is to use a message
 *             counter. An alternative is to generate random nonces, but this
 *             limits the number of messages that can be securely encrypted:
 *             for example, with 96-bit random nonces, you should not encrypt
 *             more than 2**32 messages with the same key.
 *
 *             Note that for both stategies, sizes are measured in blocks and
 *             that an SM4 block is 16 bytes.
 *
 * \warning    Upon return, \p stream_block contains sensitive data. Its
 *             content must not be written to insecure storage and should be
 *             securely discarded as soon as it's no longer needed.
 *
 * \param ctx              The SM4 context to use for encryption or decryption.
 *                         It must be initialized and bound to a key.
 * \param length           The length of the input data.
 * \param nc_off           The offset in the current \p stream_block, for
 *                         resuming within the current cipher stream. The
 *                         offset pointer should be 0 at the start of a stream.
 *                         It must point to a valid \c size_t.
 * \param nonce_counter    The 128-bit nonce and counter.
 *                         It must be a readable-writeable buffer of \c 16 Bytes.
 * \param stream_block     The saved stream block for resuming. This is
 *                         overwritten by the function.
 *                         It must be a readable-writeable buffer of \c 16 Bytes.
 * \param input            The buffer holding the input data.
 *                         It must be readable and of size \p length Bytes.
 * \param output           The buffer holding the output data.
 *                         It must be writeable and of size \p length Bytes.
 *
 * \return                 \c 0 on success.
 */
int mbedtls_sm4_crypt_ctr( mbedtls_sm4_context *ctx,
                       size_t length,
                       size_t *nc_off,
                       unsigned char nonce_counter[16],
                       unsigned char stream_block[16],
                       const unsigned char *input,
                       unsigned char *output );
#endif /* MBEDTLS_CIPHER_MODE_CTR */

/**
 * \brief           Internal SM4 block encryption function. This is only
 *                  exposed to allow overriding it using
 *                  \c MBEDTLS_SM4_ENCRYPT_ALT.
 *
 * \param ctx       The SM4 context to use for encryption.
 * \param input     The plaintext block.
 * \param output    The output (ciphertext) block.
 *
 * \return          \c 0 on success.
 */
int mbedtls_internal_sm4_encrypt( mbedtls_sm4_context *ctx,
                                  const unsigned char input[16],
                                  unsigned char output[16] );

/**
 * \brief           Internal SM4 block decryption function. This is only
 *                  exposed to allow overriding it using see
 *                  \c MBEDTLS_SM4_DECRYPT_ALT.
 *
 * \param ctx       The SM4 context to use for decryption.
 * \param input     The ciphertext block.
 * \param output    The output (plaintext) block.
 *
 * \return          \c 0 on success.
 */
int mbedtls_internal_sm4_decrypt( mbedtls_sm4_context *ctx,
                                  const unsigned char input[16],
                                  unsigned char output[16] );

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
#if defined(MBEDTLS_DEPRECATED_WARNING)
#define MBEDTLS_DEPRECATED      __attribute__((deprecated))
#else
#define MBEDTLS_DEPRECATED
#endif
/**
 * \brief           Deprecated internal SM4 block encryption function
 *                  without return value.
 *
 * \deprecated      Superseded by mbedtls_internal_sm4_encrypt()
 *
 * \param ctx       The SM4 context to use for encryption.
 * \param input     Plaintext block.
 * \param output    Output (ciphertext) block.
 */
MBEDTLS_DEPRECATED void mbedtls_sm4_encrypt( mbedtls_sm4_context *ctx,
                                             const unsigned char input[16],
                                             unsigned char output[16] );

/**
 * \brief           Deprecated internal SM4 block decryption function
 *                  without return value.
 *
 * \deprecated      Superseded by mbedtls_internal_sm4_decrypt()
 *
 * \param ctx       The SM4 context to use for decryption.
 * \param input     Ciphertext block.
 * \param output    Output (plaintext) block.
 */
MBEDTLS_DEPRECATED void mbedtls_sm4_decrypt( mbedtls_sm4_context *ctx,
                                             const unsigned char input[16],
                                             unsigned char output[16] );

#undef MBEDTLS_DEPRECATED
#endif /* !MBEDTLS_DEPRECATED_REMOVED */

/**
 * \brief          Checkup routine.
 *
 * \return         \c 0 on success.
 * \return         \c 1 on failure.
 */
int mbedtls_sm4_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* sm4.h */
