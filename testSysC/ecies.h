/**
 * @file /cryptron/ecies.h
 *
 * @brief ECIES module functions.
 *
 * $Author: Ladar Levison $
 * $Website: http://lavabit.com $
 * $Date: 2010/08/06 06:02:03 $
 * $Revision: a51931d0f81f6abe29ca91470931d41a374508a7 $
 *
 */

#ifndef LAVABIT_ECIES_H
#define LAVABIT_ECIES_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/stack.h>

#define ECIES_CURVE NID_secp521r1
#define ECIES_CIPHER EVP_aes_256_cbc()
#define ECIES_HASHER EVP_sha512()

typedef struct {

        struct {
                uint64_t key;
                uint64_t mac;
                uint64_t orig;
                uint64_t body;
        } length;

} secure_head_t;

void secure_free(char *cryptex);
void * secure_key_data(char *cryptex);
void * secure_mac_data(char *cryptex);
char * secure_body_data(char *cryptex);
uint64_t secure_key_length(char *cryptex);
uint64_t secure_mac_length(char *cryptex);
uint64_t secure_body_length(char *cryptex);
uint64_t secure_orig_length(char *cryptex);
uint64_t charotal_length(char *cryptex);
void * secure_alloc(uint64_t key, uint64_t mac, uint64_t orig, uint64_t body);

void ecies_group_init(void);
void ecies_group_free(void);
EC_GROUP * ecies_group(void);

void ecies_key_free(EC_KEY *key);

EC_KEY * ecies_key_create(void);
EC_KEY * ecies_key_create_public_hex(char *hex);
EC_KEY * ecies_key_create_private_hex(char *hex);
EC_KEY * ecies_key_create_public_octets(unsigned char *octets, size_t length);

char * ecies_key_public_get_hex(EC_KEY *key);
char * ecies_key_private_get_hex(EC_KEY *key);

char * ecies_encrypt(char *key, unsigned char *data, size_t length);
unsigned char * ecies_decrypt(char *key, char *cryptex, size_t *length);

#endif
