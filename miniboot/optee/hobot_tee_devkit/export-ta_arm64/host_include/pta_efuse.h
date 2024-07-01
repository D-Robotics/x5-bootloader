/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023 VeriSilicon Holdings Co., Ltd.
 */

#ifndef __PTA_EFUSE_H
#define __PTA_EFUSE_H

#define PTA_EFUSE_UUID                                     \
    {                                                      \
        0x16c83a2b, 0xaae3, 0x4542,                        \
        {                                                  \
            0x9d, 0xdd, 0x40, 0x46, 0x51, 0xe0, 0x1e, 0xa2 \
        }                                                  \
    }

/*
 * in       params[0].value.a = offset
 * out      params[1].memref = buffer
 */
#define PTA_EFUSE_CMD_READ 0
/*
 * in       params[0].value.a = offset
 * in       params[1].memref = buffer
 */
#define PTA_EFUSE_CMD_WRITE 1
#define OTP_SHADOW_OFFSET   256
#endif /* __PTA_EFUSE_H */
