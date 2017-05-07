//-------------------------------------------------------------------------------------------
//  bus_interface.h
//  ProgInCPP
//
//  Created by Roman Lysecky on 2/10/16.
//  Copyright © 2016 Roman Lysecky. All rights reserved.
//
//-------------------------------------------------------------------------------------------

#pragma once

#ifndef CRYPTO_H
#define CRYPTO_H

//-------------------------------------------------------------------------------------------

#include <systemc.h>
#include <openssl/ssl.h>
#include "bus_interface.h"
#include "Master.h"
#include "ecies.h"
#include "aes.h"
#include "Project.h"

//-------------------------------------------------------------------------------------------

extern sc_event encrypt_done, decrypt_done;
extern EC_KEY *ecKey;

//-------------------------------------------------------------------------------------------

// Bus Master Interface
class crypto : public sc_module, public Master
{
public:
	SC_HAS_PROCESS(crypto);

	sc_port<bus_slave_if> slvIf;

	double energy;

	char *publicKey;
	char *privateKey;
	unsigned *data;
	unsigned *bufferInt;
	unsigned mem_address_src, mem_address_dst, encrypt, length;
	unsigned char buffer[2764800];//processed block of data 720x1280x3 bytes
	unsigned char in[2764800];//block of data to be processed
	uint8_t *key;// = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	uint8_t *iv;// = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
	unsigned throughput_rate;
	sc_event encrypt_event, decrypt_event;

	crypto(sc_module_name name, unsigned id);

	void cryptography();

	void aesEncrypt();

	void aesDecrypt();

	void eccEncrypt();

	void eccDecrypt();

	void write_data(unsigned int len);

	void read_data();
};

//-------------------------------------------------------------------------------------------

#endif
