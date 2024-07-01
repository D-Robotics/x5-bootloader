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
#ifndef MBEDTLS_KLAD_H
#define MBEDTLS_KLAD_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* Trust engine key ladder root key selection enumeration
*/
typedef enum mbedtls_klad_key_sel {
    MBEDTLS_KL_KEY_MODEL = 0,   /**< model key */
    MBEDTLS_KL_KEY_ROOT         /**< device root key */
} mbedtls_klad_key_sel_t;

/**
 * secure key structure
 */
typedef struct mbedtls_klad_seckey {
    mbedtls_klad_key_sel_t sel;       /**< key ladder root key selection */
    int ek3bits;                    /**< ek3 length in bits, 128 or 256 only */
    union {
        struct {
            unsigned char ek1[16];  /**< encrypted key1 (fixed to 128-bit) */
            unsigned char ek2[16];  /**< encrypted key2 (fixed to 128-bit) */
            unsigned char ek3[32];  /**< encrypted key3 */
        };
        unsigned char eks[64];      /**< ek1 || ek2 || ek3 */
    };
} mbedtls_klad_seckey_t;

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_KLAD_H */
