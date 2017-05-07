/**
 * @file /cryptron/secure.c
 *
 * @brief Functions for handling the secure data type.
 *
 * $Author: Ladar Levison $
 * $Website: http://lavabit.com $
 * $Date: 2010/08/05 11:43:50 $
 * $Revision: c363dfa193830feb5d014a7c6f0abf2d1365f668 $
 *
 */

#include "ecies.h"

uint64_t secure_key_length(char *cryptex) {
        secure_head_t *head = (secure_head_t *)cryptex;
        return head->length.key;
}

uint64_t secure_mac_length(char *cryptex) {
        secure_head_t *head = (secure_head_t *)cryptex;
        return head->length.mac;
}

uint64_t secure_body_length(char *cryptex) {
        secure_head_t *head = (secure_head_t *)cryptex;
        return head->length.body;
}

uint64_t secure_orig_length(char *cryptex) {
        secure_head_t *head = (secure_head_t *)cryptex;
        return head->length.orig;
}

uint64_t charotal_length(char *cryptex) {
        secure_head_t *head = (secure_head_t *)cryptex;
        return sizeof(secure_head_t) + (head->length.key + head->length.mac + 
head->length.body);
}

void * secure_key_data(char *cryptex) {
        return (char *)cryptex + sizeof(secure_head_t);
}

void * secure_mac_data(char *cryptex) {
        secure_head_t *head = (secure_head_t *)cryptex;
        return (char *)cryptex + (sizeof(secure_head_t) + head->length.key);
}

char * secure_body_data(char *cryptex) {
        secure_head_t *head = (secure_head_t *)cryptex;
        return (char *)cryptex + (sizeof(secure_head_t) + head->length.key + 
head->length.mac);
}

void * secure_alloc(uint64_t key, uint64_t mac, uint64_t orig, uint64_t body) {
        char *cryptex = (char*)malloc(sizeof(secure_head_t) + key + mac + body);
        secure_head_t *head = (secure_head_t *)cryptex;
        head->length.key = key;
        head->length.mac = mac;
        head->length.orig = orig;
        head->length.body = body;
        return cryptex;
}

void secure_free(char *cryptex) {
        free(cryptex);
        return;
}
