//-------------------------------------------------------------------------------------------
//  bus_interface.h
//  ProgInCPP
//
//  Created by Roman Lysecky on 2/10/16.
//  Copyright © 2016 Roman Lysecky. All rights reserved.
//
//-------------------------------------------------------------------------------------------

#pragma once

#ifndef MEMORY_H
#define MEMORY_H

//-------------------------------------------------------------------------------------------

#include <systemc.h>
#include "bus_interface.h"
#include "Master.h"
#include "Project.h"

//-------------------------------------------------------------------------------------------

// Bus Master Interface
class memory : public sc_module
{
public:
	sc_port<bus_slave_if> slvIf;

	sc_signal<sc_logic> Clk; //clock
	sc_signal<sc_logic> Ren; //read enable
	sc_signal<sc_logic> Wen; //write enable
	sc_signal<int> Addr;
	sc_signal<int> DataIn;
	sc_signal<int> DataOut;
	sc_signal<sc_logic> Ack;

	unsigned int memData[MEM_SIZE];

	SC_HAS_PROCESS(memory);

	memory(sc_module_name name);

	void MemDataTransfer();

	bool Write(unsigned int addr, unsigned int data);

	bool Read(unsigned int addr);
};

//-------------------------------------------------------------------------------------------

#endif
