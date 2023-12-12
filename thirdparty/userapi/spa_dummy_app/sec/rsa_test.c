/*
 *  RSA simple encryption/decryption program
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "mbedtls/build_info.h"

#if defined(MBEDTLS_RSA_C) && defined(CMSE_CRYPT_TEST_DEMO)

#include <stdlib.h>
#include <string.h>
#include "hal_trace.h"
#include "hal_timer.h"

#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/platform.h"

#include "mbedtls/memory_buffer_alloc.h"
#include "heap_api.h"
#if defined(MBEDTLS_SHA1_C)
#include "sha1.h"
#ifndef PT_LEN
#define PT_LEN (24)
#endif
#endif


#define KEY_SIZE 2048
#define EXPONENT 65537
#define BUF_SIZE 16384

static unsigned char alloc_buf[BUF_SIZE];

#ifndef MBEDTLS_MEMORY_BUFFER_ALLOC_C
static multi_heap_handle_t g_heap_handle;

void *test_calloc(size_t n, size_t size)
{
    void *ptr = NULL;
    size_t size_real = size * n;

    ptr = heap_malloc(g_heap_handle, size_real);
    ASSERT(ptr != NULL, "%s:%d, malloc failed, size=%d", __FUNCTION__, __LINE__, (int)size_real);
    mbedtls_printf("malloc done, ptr=0x%x, size=%d\n", (int)ptr, (int)size_real);

    return ptr;
}

void test_free(void *ptr)
{
    heap_free(g_heap_handle, ptr);
    mbedtls_printf("%s:%d, ptr=0x%x, free done\n", __FUNCTION__, __LINE__, (int)ptr);
}

static void mbedtls_calloc_init(void)
{
    mbedtls_printf("%s:%d\n", __FUNCTION__, __LINE__);

    g_heap_handle = heap_register((void *)alloc_buf, sizeof(alloc_buf));

    mbedtls_printf("%s:%d, g_heap_handle=0x%x, start=0x%x, size=%d\n",
                   __FUNCTION__, __LINE__, (int)g_heap_handle, (int)alloc_buf, (int)sizeof(alloc_buf));

}
#endif

static int platform_entropy_poll(void *ctx, unsigned char *output,
                                 size_t len, size_t *olen)
{
    size_t i;
    for (i = 0; i < len; i++) {
        output[i] = rand() & 0xFF;
    }

    *olen = len;
    return 0;
}

static void dump_rsa_key(mbedtls_rsa_context *rsa)
{
    size_t olen;
    char buf[516];

    mbedtls_printf("\n  +++++++++++++++++ rsa keypair +++++++++++++++++\n");
    mbedtls_mpi_write_string(&rsa->N, 16, buf, sizeof(buf), &olen);
    mbedtls_printf("N: %s\n", buf);

    mbedtls_mpi_write_string(&rsa->E, 16, buf, sizeof(buf), &olen);
    mbedtls_printf("E: %s\n", buf);

    mbedtls_mpi_write_string(&rsa->D, 16, buf, sizeof(buf), &olen);
    mbedtls_printf("D: %s\n", buf);

    mbedtls_mpi_write_string(&rsa->P, 16, buf, sizeof(buf), &olen);
    mbedtls_printf("P: %s\n", buf);

    mbedtls_mpi_write_string(&rsa->Q, 16, buf, sizeof(buf), &olen);
    mbedtls_printf("Q: %s\n", buf);

    mbedtls_mpi_write_string(&rsa->DP, 16, buf, sizeof(buf), &olen);
    mbedtls_printf("DP: %s\n", buf);

    mbedtls_mpi_write_string(&rsa->DQ, 16, buf, sizeof(buf), &olen);
    mbedtls_printf("DQ: %s\n", buf);

    mbedtls_mpi_write_string(&rsa->QP, 16, buf, sizeof(buf), &olen);
    mbedtls_printf("QP: %s\n", buf);
    mbedtls_printf("  +++++++++++++++++++++++++++++++++++++++++++++++\n");
}

#if defined(MBEDTLS_SHA1_C)
static int myrand( void *rng_state, unsigned char *output, size_t len )
{
#if !defined(__OpenBSD__) && !defined(__NetBSD__)
    size_t i;

    if( rng_state != NULL )
        rng_state  = NULL;

    for( i = 0; i < len; ++i )
        output[i] = rand();
#else
    if( rng_state != NULL )
        rng_state = NULL;

    arc4random_buf( output, len );
#endif /* !OpenBSD && !NetBSD */

    return( 0 );
}
#endif

void rsa_test(void)
{
    int ret = 1;
    size_t i;
    mbedtls_rsa_context rsa;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_mpi N, P, Q, D, E, DP, DQ, QP;
    static unsigned char buf[512];
    static unsigned char result[512];
    const char *pers = "rsa_test";
    const char *plaintext = "rsa encrypt test, 2022-08-19";
    uint32_t time_start, time[6];

    time_start = hal_fast_sys_timer_get();
    //memory init
#ifdef MBEDTLS_MEMORY_BUFFER_ALLOC_C
    mbedtls_memory_buffer_alloc_init(alloc_buf, sizeof(alloc_buf));
#else
    mbedtls_calloc_init();
    mbedtls_platform_set_calloc_free(test_calloc, test_free);
#endif

    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_rsa_init(&rsa);
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
    mbedtls_mpi_init(&D);
    mbedtls_mpi_init(&E);
    mbedtls_mpi_init(&DP);
    mbedtls_mpi_init(&DQ);
    mbedtls_mpi_init(&QP);

    mbedtls_entropy_init(&entropy);
    srand(hal_sys_timer_get() * hal_fast_sys_timer_get());
    mbedtls_entropy_add_source(&entropy, platform_entropy_poll, NULL,
                               32,//The min threshold of random numbers
                               MBEDTLS_ENTROPY_SOURCE_STRONG);

    time[0] = FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time_start);
    mbedtls_printf("\n\n  . Seeding the random number generator...");
    time_start = hal_fast_sys_timer_get();
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d(-0x%X)\n", ret, -ret);
        goto exit;
    }
    time[1] = FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time_start);

    //===========================generate RSA key===============================
    mbedtls_printf(" ok\n  . Generating the RSA key [ %d-bit ]...", KEY_SIZE);
    time_start = hal_fast_sys_timer_get();
    if ((ret = mbedtls_rsa_gen_key(&rsa, mbedtls_ctr_drbg_random, &ctr_drbg, KEY_SIZE,
                                   EXPONENT)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_rsa_gen_key returned %d(-0x%X)\n", ret, -ret);
        goto exit;
    }
    time[2] = FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time_start);

    mbedtls_printf(" ok\n  . Exporting the public key ....");
    time_start = hal_fast_sys_timer_get();
    if ((ret = mbedtls_rsa_export(&rsa, &N, &P, &Q, &D, &E)) != 0 ||
        (ret = mbedtls_rsa_export_crt(&rsa, &DP, &DQ, &QP))      != 0) {
        mbedtls_printf(" failed\n  ! could not export RSA parameters\n\n");
        goto exit;
    }
    time[3] = FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time_start);
    mbedtls_printf(" ok\n\n");
    dump_rsa_key(&rsa);

    //============================encrypt data==================================
    mbedtls_printf("\n  . Generating the RSA encrypted value...");
    time_start = hal_fast_sys_timer_get();
    ret = mbedtls_rsa_pkcs1_encrypt(&rsa, mbedtls_ctr_drbg_random,
                                    &ctr_drbg, strlen(plaintext),
                                    (const unsigned char *)plaintext, buf);
    if (ret != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_rsa_pkcs1_encrypt returned %d(-0x%X)\n", ret, -ret);
        goto exit;
    }
    time[4] = FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time_start);
    mbedtls_printf(" ok\n");

    mbedtls_printf("  . plaintext: %s\n", plaintext);
    mbedtls_printf("  . ciphertext:\n");
    for (i = 0; i < rsa.MBEDTLS_PRIVATE(len); i++)
        mbedtls_printf("%02X%s", buf[i], (i + 1) % 16 == 0 ? "\r\n" : " ");

    //============================decrypt data==================================
    memset(result, 0, sizeof(result));
    mbedtls_printf("\n\n  . Decrypting the encrypted data...");
    time_start = hal_fast_sys_timer_get();
    ret = mbedtls_rsa_pkcs1_decrypt(&rsa, mbedtls_ctr_drbg_random,
                                    &ctr_drbg, &i,
                                    buf, result, sizeof(result));
    if (ret != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_rsa_pkcs1_decrypt returned %d(-0x%X)\n", ret, -ret);
        goto exit;
    }
    time[5] = FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time_start);
    mbedtls_printf("  ok\n");
    mbedtls_printf("  . The decrypted result is: %s\n\n", result);

    if (strcmp((char *)result, plaintext) == 0) {
        mbedtls_printf("  =========time-consuming statistics========\n");
        mbedtls_printf("  ******************************************\n");
        mbedtls_printf("  init      :%uus\n", time[0]);
        mbedtls_printf("  drbg seed :%uus\n", time[1]);
        mbedtls_printf("  gen key   :%uus=%us\n", time[2], time[2] / 1000 / 1000);
        mbedtls_printf("  export crt:%uus\n", time[3]);
        mbedtls_printf("  encrypt   :%uus\n", time[4]);
        mbedtls_printf("  decrypt   :%uus\n", time[5]);
        mbedtls_printf("  ******************************************\n");
    } else {
        mbedtls_printf("  ***compare fail\n");
    }
    mbedtls_printf("\n");

#if defined(MBEDTLS_SHA1_C)
    unsigned char sha1sum[20];

    ret = mbedtls_sha1((const unsigned char *)plaintext, PT_LEN, sha1sum );
    if( ret != 0 )
    {
        mbedtls_printf(" failed\n  ! mbedtls_sha1 returned %d(-0x%X)\n", ret, -ret);
        goto exit;
    }

    time[6] = FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time_start);

    ret = mbedtls_rsa_pkcs1_sign( &rsa, myrand, NULL,
                                MBEDTLS_MD_SHA1, 20,
                                sha1sum, buf );
    if( ret != 0 )
    {
        mbedtls_printf(" failed\n  ! mbedtls_rsa_pkcs1_sign returned %d(-0x%X)\n", ret, -ret);

        goto exit;
    }

    time[7] = FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time_start);
    mbedtls_printf( "passed\n  PKCS#1 sig. verify: " );

    ret = mbedtls_rsa_pkcs1_verify( &rsa, MBEDTLS_MD_SHA1, 20,
                                  sha1sum, buf );
    if( ret != 0 )
    {
        mbedtls_printf( "failed\n" );
        goto exit;
    }

    time[8] = FAST_TICKS_TO_US(hal_fast_sys_timer_get() - time_start);

    mbedtls_printf( "passed\n" );

    mbedtls_printf("  =========time-consuming statistics========\n");
    mbedtls_printf("  **************RSA verify****************\n");
    mbedtls_printf("  mbedtls_sha1:%uus\n", time[6]);
    mbedtls_printf("  mbedtls_rsa_pkcs1_sign   :%uus\n", time[7]);
    mbedtls_printf("  mbedtls_rsa_pkcs1_verify   :%uus\n", time[8]);

    mbedtls_printf("  ******************************************\n");

#endif /* MBEDTLS_SHA1_C */


exit:
    mbedtls_mpi_free(&N);
    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&Q);
    mbedtls_mpi_free(&D);
    mbedtls_mpi_free(&E);
    mbedtls_mpi_free(&DP);
    mbedtls_mpi_free(&DQ);
    mbedtls_mpi_free(&QP);
    mbedtls_rsa_free(&rsa);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

#endif /* MBEDTLS_RSA_C */

