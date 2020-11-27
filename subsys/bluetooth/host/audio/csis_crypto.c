/*
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "csis_crypto.h"
#include <bluetooth/crypto.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/cmac_mode.h>
#include <tinycrypt/ccm_mode.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_CSIS_CRYPTO)
#define LOG_MODULE_NAME bt_csis_crypto
#include "common/log.h"

static int aes_cmac(const uint8_t key[16], const uint8_t *in, size_t in_len,
		    uint8_t *out)
{
	struct tc_aes_key_sched_struct sched;
	struct tc_cmac_struct state;

	/* TODO: Copy of the aes_cmac from smp.c: Can we merge them? */

	if (tc_cmac_setup(&state, key, &sched) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_cmac_update(&state, in, in_len) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_cmac_final(out, &state) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	return 0;
}

int bt_csis_sih(const uint8_t sirk[16], const uint32_t r, uint32_t *out)
{
	uint8_t res[16];
	int err;

	if ((r & BIT(23)) || ((r & BIT(22)) == 0)) {
		BT_DBG("Invalid r %0x06x", r & 0xffffff);
	}

	BT_DBG("sirk %s", bt_hex(sirk, 16));
	BT_DBG("r 0x%06x", r);

	/* r' = padding || r */
	memcpy(res, &r, 3);
	(void)memset(res + 3, 0, 13);

	BT_DBG("r' %s", bt_hex(res, 16));

	err = bt_encrypt_le(sirk, res, res);
	if (err) {
		return err;
	}

	/* The output of the function sih is:
	 *      sih(k, r) = e(k, r') mod 2^24
	 * The output of the security function e is then truncated to 24 bits
	 * by taking the least significant 24 bits of the output of e as the
	 * result of sih.
	 */

	BT_DBG("res %s", bt_hex(res, 16));
	BT_DBG("sih %s", bt_hex(res, 3));
	memcpy(out, res, 3);

	return 0;
}

/**
 * @brief k1 deriviation function
 *
 * The key derivation function k1 is used to derive a key. The derived key is
 * used to encrypt and decrypt the value of the Set Identity Resolving Key
 * characteristic.
 *
 * @param n      n is 0 or more bytes.
 * @param n_size Number of bytes in @p n.
 * @param salt   A 16-byte salt.
 * @param p      p is 0 or more bytes.
 * @param p_size Number of bytes in @p p.
 * @param out    A 16-byte output buffer.
 * @return int 0 on success, any other value indicates a failure.
 */
static int k1(const uint8_t *n, size_t n_size, const uint8_t salt[16],
	      const uint8_t *p, size_t p_size, uint8_t out[16])
{
	/* TODO: This is basically a duplicate of bt_mesh_k1 - Perhaps they can
	 * be merged
	 */
	uint8_t t[16];
	int err;

	/*
	 * T = AES_CMAC_SALT(N)
	 *
	 * k1(N, SALT, P) = AES-CMAC_T(P)
	 */

	BT_DBG("n %s", bt_hex(n, n_size));
	BT_DBG("salt %s", bt_hex(salt, 16));
	BT_DBG("p %s", bt_hex(p, p_size));

	err = aes_cmac(salt, n, n_size, t);

	BT_DBG("t %s", bt_hex(t, sizeof(t)));

	if (err) {
		return err;
	}

	err = aes_cmac(t, p, p_size, out);

	BT_DBG("out %s", bt_hex(out, 16));

	return err;
}

/**
 * @brief s1 SALT generation function
 *
 * @param m      A non-zero length octet array or ASCII encoded string
 * @param m_size Size of @p m.
 * @param out    16-byte output buffer.
 * @return int 0 on success, any other value indicates a failure.
 */
static int s1(const uint8_t *m, size_t m_size, uint8_t out[16])
{
	uint8_t zero[16];
	int err;

	/*
	 * s1(M) = AES-CMAC_zero(M)
	 */

	BT_DBG("m %s", bt_hex(m, m_size));

	memset(zero, 0, sizeof(zero));

	err = aes_cmac(zero, m, m_size, out);
	BT_DBG("out %s", bt_hex(out, 16));

	return err;
}
