//-------------------------------------------------------------------------------------------

#pragma once

#ifndef NETWORK_INTERFACE_CARD_H
#define NETWORK_INTERFACE_CARD_H

//-------------------------------------------------------------------------------------------

#include <systemc.h>
#include "bus_interface.h"
#include "Master.h"
#include "Project.h"

//-------------------------------------------------------------------------------------------

extern sc_event receive_event, packet_received, connection_ready;

//-------------------------------------------------------------------------------------------

class ethernet_frame
{
public:
	unsigned destination_address;
	unsigned *data;
	unsigned data_length;

	ethernet_frame(unsigned addr, unsigned *d, unsigned length);
};

//--------------------------------------------------------------------------//

class network_interface_card : public sc_module, public Master
{
public:
	SC_HAS_PROCESS(network_interface_card);

	sc_port<bus_slave_if> slvIf;
	sc_event read_event, send_event;

	double nicEnergy;
	unsigned mem_address, packet_length, conn;

	unsigned mac_addr, dest_addr;

	unsigned *packet_data;

	unsigned *receive_buffer;

	network_interface_card(sc_module_name name, unsigned id, unsigned addr, unsigned dest);

	void wait_for_buffer_descriptor();

	void setup_tcp_connection();

	void tear_down_tcp_connection();

	void read_packet();

	void send_frame();

	void receive_frame();
};

//-------------------------------------------------------------------------------------------

#endif
