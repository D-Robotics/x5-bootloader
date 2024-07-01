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

#ifndef MBEDTLS_SM2KEP_H
#define MBEDTLS_SM2KEP_H

#include "ecp.h"
#include "md.h"

/*
 * Use a backward compatible SM2KEP context.
 *
 * This flag is always enabled for now and future versions might add a
 * configuration option that conditionally undefines this flag.
 * The configuration option in question may have a different name.
 *
 * Features undefining this flag, must have a warning in their description in
 * config.h stating that the feature breaks backward compatibility.
 */
#define MBEDTLS_SM2KEP_LEGACY_CONTEXT

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Defines the source of key exchange.
 */
typedef enum {
    MBEDTLS_SM2KEP_INITIATOR = 0x02, /**< initiator of the key exchange. */
    MBEDTLS_SM2KEP_RESPONDER = 0x03, /**< The responder of the key exchange. */
} mbedtls_sm2kep_side;

#if !defined(MBEDTLS_SM2KEP_LEGACY_CONTEXT)
/**
 * Defines the SM2KEP implementation used.
 *
 * Later versions of the library may add new variants, therefore users should
 * not make any assumptions about them.
 */
typedef enum {
    MBEDTLS_SM2KEP_VARIANT_NONE = 0,    /*!< Implementation not defined. */
    MBEDTLS_SM2KEP_VARIANT_MBEDTLS_2_0, /*!< The default Mbed TLS implementation
                                         */
} mbedtls_sm2kep_variant;

/**
 * The context used by the default SM2KEP implementation.
 *
 * Later versions might change the structure of this context, therefore users
 * should not make any assumptions about the structure of
 * mbedtls_sm2kep_context_mbed.
 */
typedef struct mbedtls_sm2kep_context_mbed {
    mbedtls_ecp_group grp; /*!< The elliptic curve used. */
    mbedtls_mpi d;         /*!< The private key. */
    mbedtls_ecp_point Q;   /*!< The public key. */
    mbedtls_mpi r;         /*!< The temporary private key. */
    mbedtls_ecp_point R;   /*!< The temporary public key. */
    mbedtls_ecp_point Qp;  /*!< The value of the public key of the peer. */
    mbedtls_ecp_point Rp;  /*!< The value of the temporary public key of the peer. */
    mbedtls_ecp_point Z;   /*!< The shared secret. */
} mbedtls_sm2kep_context_mbed;
#endif

/**
 * \brief           The SM2KEP context structure.
 */
typedef struct mbedtls_sm2kep_context {
#if defined(MBEDTLS_SM2KEP_LEGACY_CONTEXT)
    mbedtls_ecp_group grp;   /*!< The elliptic curve used. */
    mbedtls_mpi d;           /*!< The private key. */
    mbedtls_ecp_point Q;     /*!< The public key. */
    mbedtls_mpi r;           /*!< The temporary private key. */
    mbedtls_ecp_point R;     /*!< The temporary public key. */
    mbedtls_ecp_point Qp;    /*!< The value of the public key of the peer. */
    mbedtls_ecp_point Rp;    /*!< The value of the temporary public key of the peer. */
    mbedtls_ecp_point Z;    /*!< The shared secret. */
    int point_format;        /*!< The format of point export in TLS messages. */
#else
    uint8_t point_format;        /*!< The format of point export in TLS messages
                                   as defined in RFC 4492. */
    mbedtls_ecp_group_id grp_id; /*!< The elliptic curve used. */
    mbedtls_sm2kep_variant
        var; /*!< The SM2KEP implementation/structure used. */
    union {
        mbedtls_sm2kep_context_mbed mbed_sm2kep;
    } ctx; /*!< Implementation-specific context. The
             context in use is specified by the \c var
             field. */
#endif /* MBEDTLS_SM2KEP_LEGACY_CONTEXT */
} mbedtls_sm2kep_context;

/**
 * \brief           This function generates an SM2KEP keypair on an elliptic
 *                  curve.
 *
 *                  This function performs the first of two core computations
 *                  implemented during the SM2KEP key exchange. The second core
 *                  computation is performed by mbedtls_sm2kep_compute_shared().
 *
 * \see             ecp.h
 *
 * \param d         The destination MPI (private key).
 *                  This must be initialized.
 * \param Q         The destination point (public key).
 *                  This must be initialized.
 * \param f_rng     The RNG function to use. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL in case \p f_rng doesn't need a context argument.
 *
 * \return          \c 0 on success.
 * \return          Another \c MBEDTLS_ERR_ECP_XXX or
 *                  \c MBEDTLS_MPI_XXX error code on failure.
 */
int mbedtls_sm2kep_gen_public(mbedtls_mpi *d,
                              mbedtls_ecp_point *Q,
                              int (*f_rng)(void *, unsigned char *, size_t),
                              void *p_rng);

/**
 * \brief           This function computes the shared secret.
 *
 *                  This function performs the second of two core computations
 *                  implemented during the SM2KEP key exchange. The first core
 *                  computation is performed by mbedtls_sm2kep_gen_public().
 *
 * \see             ecp.h
 *
 * \note            If \p f_rng is not NULL, it is used to implement
 *                  countermeasures against side-channel attacks.
 *                  For more information, see mbedtls_ecp_mul().
 *
 * \param K         The destination point (used to compute shared secret).
 * \param R         The temporary public key.
 * \param Rp        The value of the temporary public key of the peer.
 * \param Qp        The value of the public key of the peer.
 * \param d         Our secret exponent (private key).
 * \param r         The temporary private key.
 * \param f_rng     The RNG function. This may be \c NULL if randomization
 *                  of intermediate results during the ECP computations is
 *                  not needed (discouraged). See the documentation of
 *                  mbedtls_ecp_mul() for more.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL if \p f_rng is \c NULL or doesn't need a
 *                  context argument.
 *
 * \return          \c 0 on success.
 * \return          Another \c MBEDTLS_ERR_ECP_XXX or
 *                  \c MBEDTLS_MPI_XXX error code on failure.
 */
int mbedtls_sm2kep_compute_shared(mbedtls_ecp_point *K,
                                  const mbedtls_ecp_point *R,
                                  const mbedtls_ecp_point *Rp,
                                  const mbedtls_ecp_point *Qp,
                                  const mbedtls_mpi *d,
                                  const mbedtls_mpi *r,
                                  int (*f_rng)(void *, unsigned char *, size_t),
                                  void *p_rng);

/**
 * \brief           This function initializes an SM2KEP context.
 *
 * \param ctx       The SM2KEP context to initialize. This must not be \c NULL.
 */
void mbedtls_sm2kep_init(mbedtls_sm2kep_context *ctx);

/**
 * \brief           This function sets up the SM2KEP context with the
 * information given.
 *
 *                  This function should be called after mbedtls_sm2kep_init()
 *                  but before mbedtls_sm2kep_make_params().
 *                  There is no need to call this function before
 *                  mbedtls_sm2kep_read_params().
 *
 *                  This is the first function used by a TLS server for ECDHE
 *                  ciphersuites.
 *
 * \param ctx       The SM2KEP context to set up. This must be initialized.
 * \param grp_id    The group id of the group to set up the context for.
 *
 * \return          \c 0 on success.
 */
int mbedtls_sm2kep_setup(mbedtls_sm2kep_context *ctx);

/**
 * \brief           This function frees a context.
 *
 * \param ctx       The context to free. This may be \c NULL, in which
 *                  case this function does nothing. If it is not \c NULL,
 *                  it must point to an initialized SM2KEP context.
 */
void mbedtls_sm2kep_free(mbedtls_sm2kep_context *ctx);

/**
 * \brief           This function generates two SM2KEP keypair(d, Q) and temporary
 *                  keypair(r, R) and exports
 *                  in the format used in a TLS ServerKeyExchange handshake
 *                  message.
 *
 *                  This is the second function used by a TLS server for ECDHE
 *                  ciphersuites. (It is called after mbedtls_sm2kep_setup().)
 *
 * \see             ecp.h
 *
 * \param ctx       The SM2KEP context to use. This must be initialized
 *                  and bound to a group, for example via mbedtls_sm2kep_setup().
 * \param olen      The address at which to store the number of Bytes written.
 * \param buf       The destination buffer. This must be a writable buffer of
 *                  length \p blen Bytes.
 * \param blen      The length of the destination buffer \p buf in Bytes.
 * \param f_rng     The RNG function to use. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng.
 *                  This may be \c NULL in case \p f_rng doesn't need a context
 *                  argument.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 * \return          Another \c MBEDTLS_ERR_ECP_XXX error code on failure.
 */
int mbedtls_sm2kep_make_params(mbedtls_sm2kep_context *ctx,
                               size_t *olen,
                               unsigned char *buf,
                               size_t blen,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng);

/**
 * \brief           This function parses the ECDHE parameters in a
 *                  TLS ServerKeyExchange handshake message.
 *
 * \note            In a TLS handshake, this is the how the client
 *                  sets up its ECDHE context from the server's public
 *                  ECDHE key material.
 *
 * \see             ecp.h
 *
 * \param ctx       The ECDHE context to use. This must be initialized.
 * \param buf       On input, \c *buf must be the start of the input buffer.
 *                  On output, \c *buf is updated to point to the end of the
 *                  data that has been read. On success, this is the first byte
 *                  past the end of the ServerKeyExchange parameters.
 *                  On error, this is the point at which an error has been
 *                  detected, which is usually not useful except to debug
 *                  failures.
 * \param end       The end of the input buffer.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_ECP_XXX error code on failure.
 *
 */
int mbedtls_sm2kep_read_params(mbedtls_sm2kep_context *ctx,
                               const unsigned char **buf,
                               const unsigned char *end);

/**
 * \brief           This function sets up an SM2KEP context from an EC key.
 *
 *                  It is used by clients and servers in place of the
 *                  ServerKeyEchange for static SM2KEP, and imports SM2KEP
 *                  parameters from the EC key information of a certificate.
 *
 * \see             ecp.h
 *
 * \param ctx       The SM2KEP context to set up. This must be initialized.
 * \param key       The SM2KEP key to use. This must be initialized.
 * \param key       The temporary SM2KEP key to use. This must be initialized.
 * \param side      Defines the source of the key. Possible values are:
 *                  - #MBEDTLS_ECDH_OURS: The key is ours.
 *                  - #MBEDTLS_ECDH_THEIRS: The key is that of the peer.
 *
 * \return          \c 0 on success.
 * \return          Another \c MBEDTLS_ERR_ECP_XXX error code on failure.
 *
 */
int mbedtls_sm2kep_get_params(mbedtls_sm2kep_context *ctx,
                              const mbedtls_ecp_keypair *key,
                              const mbedtls_ecp_keypair *tmpkey,
                              mbedtls_sm2kep_side side);
/**
 * \brief           This function generates a public key and exports it
 *                  as a TLS ClientKeyExchange payload.
 *
 *                  This is the second function used by a TLS client for
 *                  SM2KEP(E) ciphersuites.
 *
 * \see             ecp.h
 *
 * \param ctx       The SM2KEP context to use. This must be initialized
 *                  and bound to a group, the latter usually by
 *                  mbedtls_sm2kep_read_params().
 * \param olen      The address at which to store the number of Bytes written.
 *                  This must not be \c NULL.
 * \param buf       The destination buffer. This must be a writable buffer
 *                  of length \p blen Bytes.
 * \param blen      The size of the destination buffer \p buf in Bytes.
 * \param f_rng     The RNG function to use. This must not be \c NULL.
 * \param p_rng     The RNG context to be passed to \p f_rng. This may be
 *                  \c NULL in case \p f_rng doesn't need a context argument.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 * \return          Another \c MBEDTLS_ERR_ECP_XXX error code on failure.
 */
int mbedtls_sm2kep_make_public(mbedtls_sm2kep_context *ctx,
                               size_t *olen,
                               unsigned char *buf,
                               size_t blen,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng);

/**
 * \brief       This function parses and processes the ECDHE payload of a
 *              TLS ClientKeyExchange message.
 *
 *              This is the third function used by a TLS server for SM2KEP(E)
 *              ciphersuites. (It is called after mbedtls_sm2kep_setup() and
 *              mbedtls_sm2kep_make_params().)
 *
 * \see         ecp.h
 *
 * \param ctx   The SM2KEP context to use. This must be initialized
 *              and bound to a group, for example via mbedtls_sm2kep_setup().
 * \param buf   The pointer to the ClientKeyExchange payload. This must
 *              be a readable buffer of length \p blen Bytes.
 * \param blen  The length of the input buffer \p buf in Bytes.
 *
 * \return      \c 0 on success.
 * \return      An \c MBEDTLS_ERR_ECP_XXX error code on failure.
 */
int mbedtls_sm2kep_read_public(mbedtls_sm2kep_context *ctx,
                               const unsigned char *buf,
                               size_t blen);

/**
 * \brief           This function derives and exports the shared secret.
 *
 *                  This is the last function used by both TLS client
 *                  and servers.
 *
 * \note            If \p f_rng is not NULL, it is used to implement
 *                  countermeasures against side-channel attacks.
 *                  For more information, see mbedtls_ecp_mul().
 *
 * \see             ecp.h

 * \param ctx       The SM2KEP context to use. This must be initialized
 *                  and have its own private key generated and the peer's
 *                  public key imported.
 * \param md_alg    The message digest that was used to do hash.
 * \param buf       The buffer to write the generated shared key to. This
 *                  must be a writable buffer of size \p blen Bytes.
 * \param blen      The length of the destination buffer \p buf in Bytes.
 * \param ZA        The hash value of user A's identifiable identifier, partial
 *                  elliptic curve system parameters, and user A's public key,
 *                  please refer to SM2 spec.
 * \param ZAlen     The length of ZA.
 * \param ZB        The hash value of user B's identifiable identifier,
 *                  partial elliptic curve system parameters,and user B's
 *                  public key,please refer to SM2 spec.
 * \param ZBlen     The length of ZB.
 * \param f_rng     The RNG function, for blinding purposes. This may
 *                  b \c NULL if blinding isn't needed.
 * \param p_rng     The RNG context. This may be \c NULL if \p f_rng
 *                  doesn't need a context argument.
 *
 * \return          \c 0 on success.
 * \return          #MBEDTLS_ERR_ECP_IN_PROGRESS if maximum number of
 *                  operations was reached: see \c mbedtls_ecp_set_max_ops().
 * \return          Another \c MBEDTLS_ERR_ECP_XXX error code on failure.
 */
int mbedtls_sm2kep_calc_secret(mbedtls_sm2kep_context *ctx,
                               mbedtls_md_type_t md_alg,
                               unsigned char *buf,
                               size_t blen,
                               unsigned char *ZA,
                               size_t ZAlen,
                               unsigned char *ZB,
                               size_t ZBlen,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng);


/**
 * \brief           This function compute the SM2KEP checksum.
 *
 *                  This is the last function used by both TLS client
 *                  and servers.
 *
 * \note            The size of  \p SI and \p SR should equal to digest size of
 *                  \p md_alg
 *
 * \see             ecp.h
 *
 * \param ctx       The SM2KEP context.
 * \param md_alg    The message digest that was used to do hash.
 * \param side      Defines the source of the key exchange,refer to enum
 *                  "mbedtls_sm2kep_side".
 * \param SI        The checksum of INITIATOR,if we are INITIATOR,we need to
 *                  keep the checksum,if we are RESPONDOR, we need to send
 *                  the checksum to INITIATOR.
 * \param SR        The checksum of RESPONDOR,if we are RESPONDOR,we need to
 *                  keep the checksum, if we are INITIATOR, we need to send
 *                  the checksum to RESPONDOR.
 * \param ZA        The hash value of user A's identifiable identifier, partial
 *                  elliptic curve system parameters, and user A's public key,
 *                  please refer to SM2 spec.
 * \param ZAlen     The length of ZA.
 * \param ZB        The hash value of user B's identifiable identifier, partial
 *                  elliptic curve system parameters, and user B's public key,
 *                  please refer to SM2 spec.
 * \param ZBlen     The length of ZB.
 *
 * \return          \c 0 on success.
 * \return          An \c MBEDTLS_ERR_ECP_XXX error code on failure.

 */
int mbedtls_sm2kep_calc_checksum(mbedtls_sm2kep_context *ctx,
                                 mbedtls_md_type_t md_alg,
                                 mbedtls_sm2kep_side side,
                                 unsigned char *SI,
                                 unsigned char *SR,
                                 unsigned char *ZA,
                                 size_t ZAlen,
                                 unsigned char *ZB,
                                 size_t ZBlen);

/**
 * \brief          The SM2PKE checkup routine.
 *
 * \return         \c 0 on success.
 * \return         \c 1 on failure.
 */
int mbedtls_sm2kep_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* sm2kep.h */
