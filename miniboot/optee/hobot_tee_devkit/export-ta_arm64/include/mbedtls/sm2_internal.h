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

#ifndef MBEDTLS_SM2_INTERNAL_H
#define MBEDTLS_SM2_INTERNAL_H

#include "md.h"

#ifdef __cplusplus
extern "C" {
#endif

int mbedtls_sm2_kdf( const mbedtls_md_info_t *md_info,
                     const unsigned char *cdata,
                     unsigned int datalen,
                     unsigned int keylen,
                     unsigned char *keydata );

#ifdef __cplusplus
}
#endif
#endif /* sm2_internal.h */
