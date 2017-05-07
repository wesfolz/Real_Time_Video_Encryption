//-------------------------------------------------------------------------------------------
//  bus_interface.h
//  ProgInCPP
//
//  Created by Roman Lysecky on 2/10/16.
//  Copyright © 2016 Roman Lysecky. All rights reserved.
//
//-------------------------------------------------------------------------------------------


#ifndef PROJECT_H
#define PROJECT_H

#include <systemc.h>
#include "ecies.h"

//-------------------------------------------------------------------------------------------

#define NIC_START_ADDR 0
#define NIC_END_ADDR 2

#define CRYPTO_START_ADDR 3
#define CRYPTO_END_ADDR 7

#define ENCODER_START_ADDR 8
#define ENCODER_END_ADDR 10

#define MEM_START_ADDR 11
#define MEM_SIZE 3000000
#define MAX_FRAME_SIZE 300000

#define MAX_PACKET_SIZE 1460
#define READ 1
#define WRITE 0

#define ENCODE_CLOCK_LEN	2.5 //ns
#define FRAME_PERIOD	33000 //ns

#define BUS_FREQ 72 //MHz

#define AES_THROUGHPUT 8000 / (1.27 * BUS_FREQ)//190//29 //ns per byte
#define ECC_THROUGHPUT 8000 / (0.02575 * BUS_FREQ) //9414 //ns per byte

//mW
#define AES_POWER 9.6 * BUS_FREQ / 33
#define ECC_POWER 56.8 * BUS_FREQ / 33
#define NIC_TRANSMIT_POWER 498
#define NIC_RECEIVE_POWER 153
#define ENCODER_POWER 490
#define ENCODER_LOW_POWER 160

extern sc_time lastTime;

#define DEMO 0
#define AES 1
#define ECC 0

using namespace std;

//-------------------------------------------------------------------------------------------

#endif