//-------------------------------------------------------------------------------------------
//  bus_interface.h
//  ProgInCPP
//
//  Created by Roman Lysecky on 2/10/16.
//  Copyright © 2016 Roman Lysecky. All rights reserved.
//
//-------------------------------------------------------------------------------------------

#pragma once

#ifndef MASTER_H
#define MASTER_H

//-------------------------------------------------------------------------------------------

#include <systemc.h>
#include "bus_interface.h"

//-------------------------------------------------------------------------------------------

class Master
{
public:

	sc_port<bus_master_if> mstIf;
	unsigned mstId;

	Master(unsigned id);

	unsigned* MasterDataTransfer(unsigned *writeData, bool rdnwr, unsigned addr, unsigned len);
};

//-------------------------------------------------------------------------------------------

#endif
