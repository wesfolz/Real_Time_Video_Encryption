//-------------------------------------------------------------------------------------------
//  bus_interface.h
//  ProgInCPP
//
//  Created by Roman Lysecky on 2/10/16.
//  Copyright © 2016 Roman Lysecky. All rights reserved.
//
//-------------------------------------------------------------------------------------------

#ifndef ENCODER_H
#define ENCODER_H

//-------------------------------------------------------------------------------------------

#include <systemc.h>
#include "bus_interface.h"
#include "Master.h"
#include "Project.h"

//-------------------------------------------------------------------------------------------

extern sc_event encode_done;

//-------------------------------------------------------------------------------------------

// Bus Master Interface
class encoder : public sc_module, public Master
{
public:
	SC_HAS_PROCESS(encoder);

	sc_port<bus_slave_if> slvIf;
	char path[200];
	unsigned bufferInt[1280 * 720 + 400];
	char buffer[1280 * 720 + 400];//worst case scenario, full 720p frame plus extra info packets
	FILE *file;

	double encoderEnergy;
	unsigned rand_num;
	bool ack;
	unsigned temp;
	int len_delayed;
	unsigned mem_address_dst0;
	unsigned mem_address_dst1;
	unsigned buffer_ptr = 0;
	unsigned frame_length;

	unsigned delay_ns;
	sc_event read;

	encoder(sc_module_name name, char *path, unsigned id);

	void encode();

	void read_file();

	void new_frame();
};

//-------------------------------------------------------------------------------------------

#endif
