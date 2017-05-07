#include "network_interface_card.h"

//--------------------------------------------------------------------------//

ethernet_frame *network_frame;
sc_event receive_event, packet_received, connection_ready;
sc_time networkTime;

//--------------------------------------------------------------------------//

ethernet_frame::ethernet_frame(unsigned addr, unsigned *d, unsigned length)
{
	data = new unsigned[length];
	destination_address = addr;
	data_length = length;

	for (unsigned i = 0; i < data_length; i++)
	{
		data[i] = d[i];
	}
}

//--------------------------------------------------------------------------//

network_interface_card::network_interface_card(sc_module_name name, unsigned id, unsigned addr, unsigned dest) : sc_module(name),
	Master(id)
{
	mac_addr = addr;
	dest_addr = dest;
	nicEnergy = 0.0;
	SC_THREAD(receive_frame);
	SC_THREAD(send_frame);
	SC_THREAD(read_packet);
	SC_THREAD(wait_for_buffer_descriptor);
}

void network_interface_card::wait_for_buffer_descriptor()
{
	while (true)
	{
		unsigned addr, len;
		bool rdnwr;

		slvIf->SlvListen(addr, rdnwr, len);

		if (addr >= NIC_START_ADDR && addr + len <= NIC_END_ADDR)
		{
			slvIf->SlvAcknowledge();

			if (!rdnwr) //write
			{
				if (len < 2)
				{
					slvIf->SlvReceiveWriteData(conn);
					if (conn == 1)
						setup_tcp_connection();
					else
						tear_down_tcp_connection();
				}
				else
				{
					for (int i = 0; i < 2; i++)
					{
						if (i == 0)
							slvIf->SlvReceiveWriteData(mem_address); //get memory address of packet
						else
							slvIf->SlvReceiveWriteData(packet_length); //get length of packet
					}
					read_event.notify(); //begin reading packet from memory
				}
			}
			else
			{
				slvIf->SlvSendReadData(network_frame->data_length); //send length of packet received
			}
		}
	}
}

void network_interface_card::setup_tcp_connection()
{
	//assuming a 3 way handshake, each packet has a 20 byte IP header and a 24 byte TCP Header + data = 44 bytes * 3 = 132 Bytes
	//bandwidth including headers = A = ((L + I + T) / L) * R =  ((1460 + 20 + 20)/1460) * 2621440 Bps = 2693260 Bps
	//time = 132 / 2693260 = 49.01 us

	nicEnergy += (NIC_TRANSMIT_POWER + NIC_RECEIVE_POWER)*49.01 / 1000000;

	wait(49.01, SC_US);
	connection_ready.notify();
}

void network_interface_card::tear_down_tcp_connection()
{
	//assuming a 4 way handshake, each packet has a 20 byte IP header and a 20 byte TCP Header = 40 bytes * 4 = 160 Bytes
	//bandwidth including headers = A = ((L + I + T) / L) * R =  ((1460 + 20 + 20)/1460) * 2621440 Bps = 2693260 Bps
	//time = 160 / 2693260 = 59.41 us

	nicEnergy += (NIC_TRANSMIT_POWER + NIC_RECEIVE_POWER)*59.41 / 1000000;

	wait(59.41, SC_US);
}

void network_interface_card::read_packet()
{
	while (true)
	{
		wait(read_event);
		packet_data = new unsigned[packet_length];

		packet_data = this->MasterDataTransfer(NULL, true, mem_address, packet_length); //read packet data from memory

		#if DEMO
			cout << "packet data:" << endl;
			for (unsigned i = 0; i < packet_length; i++)
				cout << packet_data[i] << endl;
		#endif 

		send_event.notify();
	}
}

void network_interface_card::send_frame()
{
	while (true)
	{
		wait(send_event);
		networkTime = sc_time_stamp();

		network_frame = new ethernet_frame(dest_addr, packet_data, packet_length);
		//wait appropriate time
		//(54 Mbps)
		//20Mbps = 2.5MBps = 2621440 Bps
		//power = 0.2 J/MB => 0.2 J/MB * 2.5MBps = 0.5 Watts

		nicEnergy += (double)NIC_TRANSMIT_POWER*packet_length / 7077888.0;

		wait((float)packet_length / 7077888.0, SC_SEC);
		receive_event.notify();
	}
}

void network_interface_card::receive_frame()
{
	unsigned mem_num = 0;
	while (true)
	{
		wait(receive_event);
		if (network_frame->destination_address == mac_addr)
		{
			nicEnergy += (double)NIC_RECEIVE_POWER*packet_length / 7077888.0;
			if (mem_num == 0)
			{
				this->MasterDataTransfer(network_frame->data, false, MEM_START_ADDR, network_frame->data_length); //write c
				mem_num = 1;
			}
			else
			{
				this->MasterDataTransfer(network_frame->data, false, MEM_START_ADDR+MAX_FRAME_SIZE, network_frame->data_length); //write c
				mem_num = 0;
			}
			cout << "Transmit Time: " << sc_time_stamp() - networkTime << endl;
			packet_received.notify();
		}
	}

}

//--------------------------------------------------------------------------//