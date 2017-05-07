//-------------------------------------------------------------------------------------------
//  bus_interface.h
//  ProgInCPP
//
//  Created by Roman Lysecky on 2/10/16.
//  Copyright © 2016 Roman Lysecky. All rights reserved.
//
//-------------------------------------------------------------------------------------------

#pragma once

#ifndef BUS_INTERFACE_H
#define BUS_INTERFACE_H

//-------------------------------------------------------------------------------------------

#include <systemc.h>

//-------------------------------------------------------------------------------------------

// Bus Master Interface
class bus_master_if : virtual public sc_interface
{
public:
	virtual bool MstBusRequest(unsigned mstId, bool rdnwr, unsigned addr, unsigned len) = 0;
	virtual void MstReadData(unsigned &data) = 0;
	virtual void MstWriteData(unsigned data) = 0;
};

// Bus Master Interface
class bus_slave_if : virtual public sc_interface
{
public:
	virtual void SlvListen(unsigned &reqAddr, bool &reqRdnwr, unsigned &reqLen) = 0;
	virtual void SlvAcknowledge() = 0;
	virtual void SlvSendReadData(unsigned data) = 0;
	virtual void SlvReceiveWriteData(unsigned &data) = 0;
};

//-------------------------------------------------------------------------------------------

#endif 
