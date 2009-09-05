/*
 * Copyright 2005-2009 SPARTA, Inc.  All rights reserved.
 * See the COPYING file distributed with this software for details.
 *
 * Author: Abhijit Hayatnagarkar
 */

/*
 * DESCRIPTION
 * This is the implementation for the DSA/SHA-1 algorithm signature
 * verification, the implementation for the RSA/MD5 algorithm signature
 * verification and the implementation for the RSA/SHA-1 algorithm signature
 * verification
 *
 * See RFC 2537, RFC 3110, RFC 4034 Appendix B.1, RFC 2536
 */
#include "validator-config.h"

#include <openssl/bn.h>
#include <openssl/sha.h>
#ifdef HAVE_CRYPTO_SHA2_H /* netbsd */
#include <crypto/sha2.h>
#endif
#include <openssl/dsa.h>
#include <openssl/md5.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/objects.h>    /* For NID_sha1 */
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <validator/resolver.h>
#include <validator/validator.h>
#include <validator/validator-internal.h>
#include "val_crypto.h"
#include "val_support.h"


/*
 * Returns VAL_NO_ERROR on success, other values on failure 
 */
static int
dsasha1_parse_public_key(const u_char *buf, size_t buflen, DSA * dsa)
{
    u_int8_t        T;
    int             index = 0;
    BIGNUM         *bn_p, *bn_q, *bn_g, *bn_y;

    if (!dsa || buflen == 0) {
        return VAL_BAD_ARGUMENT;
    }

    T = (u_int8_t) (buf[index]);
    index++;
    
    if (index+20 > buflen)
        return VAL_BAD_ARGUMENT;
    bn_q = BN_bin2bn(buf + index, 20, NULL);
    index += 20;

    if (index+64 + (T * 8) > buflen)
        return VAL_BAD_ARGUMENT;
    bn_p = BN_bin2bn(buf + index, 64 + (T * 8), NULL);
    index += (64 + (T * 8));

    if (index+64 + (T * 8) > buflen)
        return VAL_BAD_ARGUMENT;
    bn_g = BN_bin2bn(buf + index, 64 + (T * 8), NULL);
    index += (64 + (T * 8));

    if (index+64 + (T * 8) > buflen)
        return VAL_BAD_ARGUMENT;
    bn_y = BN_bin2bn(buf + index, 64 + (T * 8), NULL);
    index += (64 + (T * 8));

    dsa->p = bn_p;
    dsa->q = bn_q;
    dsa->g = bn_g;
    dsa->pub_key = bn_y;

    return VAL_NO_ERROR;        /* success */
}

void
dsasha1_sigverify(val_context_t * ctx,
                  const u_char *data,
                  size_t data_len,
                  const val_dnskey_rdata_t * dnskey,
                  const val_rrsig_rdata_t * rrsig,
                  val_astatus_t * key_status, val_astatus_t * sig_status)
{
    char            buf[1028];
    size_t          buflen = 1024;
    DSA            *dsa = NULL;
    u_char   sha1_hash[SHA_DIGEST_LENGTH];

    val_log(ctx, LOG_DEBUG,
            "dsasha1_sigverify(): parsing the public key...");
    if ((dsa = DSA_new()) == NULL) {
        val_log(ctx, LOG_INFO,
                "dsasha1_sigverify(): could not allocate dsa structure.");
        *key_status = VAL_AC_INVALID_KEY;
        return;
    };

    if (dsasha1_parse_public_key
        (dnskey->public_key, dnskey->public_key_len,
         dsa) != VAL_NO_ERROR) {
        val_log(ctx, LOG_INFO,
                "dsasha1_sigverify(): Error in parsing public key.");
        DSA_free(dsa);
        *key_status = VAL_AC_INVALID_KEY;
        return;
    }

    bzero(sha1_hash, SHA_DIGEST_LENGTH);
    SHA1(data, data_len, sha1_hash);
    val_log(ctx, LOG_DEBUG, "dsasha1_sigverify(): SHA-1 hash = %s",
            get_hex_string(sha1_hash, SHA_DIGEST_LENGTH, buf, buflen));

    val_log(ctx, LOG_DEBUG,
            "dsasha1_sigverify(): verifying DSA signature...");

    if (DSA_verify
        (NID_sha1, (u_char *) sha1_hash, SHA_DIGEST_LENGTH,
         rrsig->signature, rrsig->signature_len, dsa)) {
        val_log(ctx, LOG_INFO, "dsasha1_sigverify(): returned SUCCESS");
        DSA_free(dsa);
        *sig_status = VAL_AC_RRSIG_VERIFIED;
    } else {
        val_log(ctx, LOG_INFO, "dsasha1_sigverify(): returned FAILURE");
        DSA_free(dsa);
        *sig_status = VAL_AC_RRSIG_VERIFY_FAILED;
    }
    return;
}

/*
 * Returns VAL_NO_ERROR on success, other values on failure 
 */
static int
rsamd5_parse_public_key(const u_char *buf, size_t buflen, RSA * rsa)
{
    int             index = 0;
    const u_char   *cp;
    u_int16_t       exp_len = 0x0000;
    BIGNUM         *bn_exp;
    BIGNUM         *bn_mod;

    if (!rsa || buflen == 0)
        return VAL_BAD_ARGUMENT;

    cp = buf;

    if (buf[index] == 0) {

        if (buflen < 3)
            return VAL_BAD_ARGUMENT;

        index += 1;
        cp = (buf + index);
        VAL_GET16(exp_len, cp);
        index += 2;
    } else {
        exp_len += buf[index];
        index += 1;
    }

    if (exp_len > buflen - index) {
        return VAL_BAD_ARGUMENT;
    }
    
    /*
     * Extract the exponent 
     */
    bn_exp = BN_bin2bn(buf + index, exp_len, NULL);

    index += exp_len;

    if (buflen <= index) {
        return VAL_BAD_ARGUMENT;
    }

    /*
     * Extract the modulus 
     */
    bn_mod = BN_bin2bn(buf + index, buflen - index, NULL);

    rsa->e = bn_exp;
    rsa->n = bn_mod;

    return VAL_NO_ERROR;        /* success */
}

/*
 * See RFC 4034, Appendix B.1 :
 *
 * " For a DNSKEY RR with algorithm 1, the key tag is defined to be the most
 *   significant 16 bits of the least significant 24 bits in the public
 *   key modulus (in other words, the 4th to last and 3rd to last octets
 *   of the public key modulus)."
 */
u_int16_t
rsamd5_keytag(const u_char *pubkey, size_t pubkey_len)
{
    RSA            *rsa = NULL;
    BIGNUM         *modulus;
    u_int16_t       keytag = 0x0000;
    u_char  *modulus_bin;
    int             modulus_len;

    if ((rsa = RSA_new()) == NULL) {
        return VAL_OUT_OF_MEMORY;
    };

    if (rsamd5_parse_public_key(pubkey, pubkey_len, rsa) != VAL_NO_ERROR) {
        RSA_free(rsa);
        return VAL_BAD_ARGUMENT;
    }

    modulus = rsa->n;
    modulus_len = BN_num_bytes(modulus);
    modulus_bin =
        (u_char *) MALLOC(modulus_len * sizeof(u_char));

    BN_bn2bin(modulus, modulus_bin);

    keytag = ((0x00ff & modulus_bin[modulus_len - 3]) << 8) |
        (0x00ff & modulus_bin[modulus_len - 2]);

    FREE(modulus_bin);
    RSA_free(rsa);
    return keytag;
}

void
rsamd5_sigverify(val_context_t * ctx,
                 const u_char *data,
                 size_t data_len,
                 const val_dnskey_rdata_t * dnskey,
                 const val_rrsig_rdata_t * rrsig,
                 val_astatus_t * key_status, val_astatus_t * sig_status)
{
    char            buf[1028];
    size_t          buflen = 1024;
    RSA            *rsa = NULL;
    u_char   md5_hash[MD5_DIGEST_LENGTH];

    val_log(ctx, LOG_DEBUG,
            "rsamd5_sigverify(): parsing the public key...");
    if ((rsa = RSA_new()) == NULL) {
        val_log(ctx, LOG_INFO,
                "rsamd5_sigverify(): could not allocate rsa structure.");
        *key_status = VAL_AC_INVALID_KEY;
        return;
    };

    if (rsamd5_parse_public_key(dnskey->public_key, dnskey->public_key_len,
                                rsa) != VAL_NO_ERROR) {
        val_log(ctx, LOG_INFO,
                "rsamd5_sigverify(): Error in parsing public key.");
        RSA_free(rsa);
        *key_status = VAL_AC_INVALID_KEY;
        return;
    }

    bzero(md5_hash, MD5_DIGEST_LENGTH);
    MD5(data, data_len, (u_char *) md5_hash);
    val_log(ctx, LOG_DEBUG, "rsamd5_sigverify(): MD5 hash = %s",
            get_hex_string(md5_hash, MD5_DIGEST_LENGTH, buf, buflen));

    val_log(ctx, LOG_DEBUG,
            "rsamd5_sigverify(): verifying RSA signature...");

    if (RSA_verify(NID_md5, (u_char *) md5_hash, MD5_DIGEST_LENGTH,
                   rrsig->signature, rrsig->signature_len, rsa)) {
        val_log(ctx, LOG_INFO, "rsamd5_sigverify(): returned SUCCESS");
        RSA_free(rsa);
        *sig_status = VAL_AC_RRSIG_VERIFIED;
    } else {
        val_log(ctx, LOG_INFO, "rsamd5_sigverify(): returned FAILURE");
        RSA_free(rsa);
        *sig_status = VAL_AC_RRSIG_VERIFY_FAILED;
    }
    return;
}

/*
 * Returns VAL_NO_ERROR on success, other values on failure 
 */
static int
rsasha1_parse_public_key(const u_char *buf, size_t buflen, RSA * rsa)
{
    int             index = 0;
    const u_char   *cp;
    u_int16_t       exp_len = 0x0000;
    BIGNUM         *bn_exp;
    BIGNUM         *bn_mod;

    if (!rsa || buflen == 0)
        return VAL_BAD_ARGUMENT;

    cp = buf;

    if (buf[index] == 0) {
        if (buflen < 3)
            return VAL_BAD_ARGUMENT;
        index += 1;
        cp = (buf + index);
        VAL_GET16(exp_len, cp);
        index += 2;
    } else {
        exp_len += buf[index];
        index += 1;
    }

    if (index + exp_len > buflen) {
        return VAL_BAD_ARGUMENT;
    }
    
    /*
     * Extract the exponent 
     */
    bn_exp = BN_bin2bn(buf + index, exp_len, NULL);
    index += exp_len;

    if (buflen <= index) {
        return VAL_BAD_ARGUMENT;
    }

    /*
     * Extract the modulus 
     */
    bn_mod = BN_bin2bn(buf + index, buflen - index, NULL);

    rsa->e = bn_exp;
    rsa->n = bn_mod;

    return VAL_NO_ERROR;        /* success */
}

void
rsasha1_sigverify(val_context_t * ctx,
                  const u_char *data,
                  size_t data_len,
                  const val_dnskey_rdata_t * dnskey,
                  const val_rrsig_rdata_t * rrsig,
                  val_astatus_t * key_status, val_astatus_t * sig_status)
{
    char            buf[1028];
    int             buflen = 1024;
    RSA            *rsa = NULL;
    u_char   sha1_hash[SHA_DIGEST_LENGTH];

    val_log(ctx, LOG_DEBUG,
            "rsasha1_sigverify(): parsing the public key...");
    if ((rsa = RSA_new()) == NULL) {
        val_log(ctx, LOG_INFO,
                "rsasha1_sigverify(): could not allocate rsa structure.");
        *key_status = VAL_AC_INVALID_KEY;
        return;
    };

    if (rsasha1_parse_public_key
        (dnskey->public_key, (size_t)dnskey->public_key_len,
         rsa) != VAL_NO_ERROR) {
        val_log(ctx, LOG_INFO,
                "rsasha1_sigverify(): Error in parsing public key.");
        RSA_free(rsa);
        *key_status = VAL_AC_INVALID_KEY;
        return;
    }

    bzero(sha1_hash, SHA_DIGEST_LENGTH);
    SHA1(data, data_len, sha1_hash);
    val_log(ctx, LOG_DEBUG, "rsasha1_sigverify(): SHA-1 hash = %s",
            get_hex_string(sha1_hash, SHA_DIGEST_LENGTH, buf, buflen));

    val_log(ctx, LOG_DEBUG,
            "rsasha1_sigverify(): verifying RSA signature...");

    if (RSA_verify
        (NID_sha1, sha1_hash, SHA_DIGEST_LENGTH,
         rrsig->signature, rrsig->signature_len, rsa)) {
        val_log(ctx, LOG_INFO, "rsasha1_sigverify(): returned SUCCESS");
        RSA_free(rsa);
        *sig_status = VAL_AC_RRSIG_VERIFIED;
    } else {
        val_log(ctx, LOG_INFO, "rsasha1_sigverify(): returned FAILURE");
        RSA_free(rsa);
        *sig_status = VAL_AC_RRSIG_VERIFY_FAILED;
    }
    return;
}

int
ds_sha_hash_is_equal(u_char * name_n,
                     u_char * rrdata,
                     size_t rrdatalen, 
                     u_char * ds_hash,
                     size_t ds_hash_len)
{
    u_char        ds_digest[SHA_DIGEST_LENGTH];
    size_t        namelen;
    SHA_CTX         c;

    if (rrdata == NULL || ds_hash_len != SHA_DIGEST_LENGTH)
        return 0;

    namelen = wire_name_length(name_n);

    memset(ds_digest, SHA_DIGEST_LENGTH, 0);

    SHA1_Init(&c);
    SHA1_Update(&c, name_n, namelen);
    SHA1_Update(&c, rrdata, rrdatalen);
    SHA1_Final(ds_digest, &c);

    if (!memcmp(ds_digest, ds_hash, SHA_DIGEST_LENGTH))
        return 1;

    return 0;
}

#ifdef HAVE_SHA_256
int
ds_sha256_hash_is_equal(u_char * name_n,
                        u_char * rrdata,
                        size_t rrdatalen, 
                        u_char * ds_hash,
                        size_t ds_hash_len)
{
    u_char        ds_digest[SHA256_DIGEST_LENGTH];
    size_t        namelen;
    SHA256_CTX    c;

    if (rrdata == NULL || ds_hash_len != SHA256_DIGEST_LENGTH)
        return 0;

    namelen = wire_name_length(name_n);

    memset(ds_digest, SHA256_DIGEST_LENGTH, 0);

    SHA256_Init(&c);
    SHA256_Update(&c, name_n, namelen);
    SHA256_Update(&c, rrdata, rrdatalen);
    SHA256_Final(ds_digest, &c);

    if (!memcmp(ds_digest, ds_hash, SHA256_DIGEST_LENGTH))
        return 1;

    return 0;
}
#endif

#ifdef LIBVAL_NSEC3
u_char       *
nsec3_sha_hash_compute(u_char * qc_name_n, u_char * salt,
                       size_t saltlen, size_t iter, u_char ** hash,
                       size_t * hashlen)
{
    /*
     * Assume that the caller has already performed all sanity checks 
     */
    SHA_CTX         c;
    size_t          i;

    *hash = (u_char *) MALLOC(SHA_DIGEST_LENGTH * sizeof(u_char));
    if (*hash == NULL)
        return NULL;
    *hashlen = SHA_DIGEST_LENGTH;

    memset(*hash, 0, SHA_DIGEST_LENGTH);

    /*
     * IH(salt, x, 0) = H( x || salt) 
     */
    SHA1_Init(&c);
    SHA1_Update(&c, qc_name_n, wire_name_length(qc_name_n));
    SHA1_Update(&c, salt, saltlen);
    SHA1_Final(*hash, &c);

    /*
     * IH(salt, x, k) = H(IH(salt, x, k-1) || salt) 
     */
    for (i = 0; i < iter; i++) {
        SHA1_Init(&c);
        SHA1_Update(&c, *hash, *hashlen);
        SHA1_Update(&c, salt, saltlen);
        SHA1_Final(*hash, &c);
    }
    return *hash;
}
#endif

char           *
get_base64_string(u_char *message, size_t message_len, char *buf,
                  size_t bufsize)
{
    BIO            *b64 = BIO_new(BIO_f_base64());
    BIO            *mem = BIO_new_mem_buf(message, message_len);
    mem = BIO_push(b64, mem);

    if (-1 == BIO_write(mem, buf, bufsize))
        strcpy(buf, "");
    BIO_free_all(mem);

    return buf;
}

int
decode_base64_key(char *keyptr, u_char * public_key, size_t keysize)
{
    BIO            *b64;
    BIO            *mem;
    BIO            *bio;
    int             len;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    mem = BIO_new_mem_buf(keyptr, -1);
    bio = BIO_push(b64, mem);
    len = BIO_read(bio, public_key, keysize);
    BIO_free(mem);
    BIO_free(b64);
    return len;
}
