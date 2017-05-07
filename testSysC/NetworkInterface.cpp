#include "NetworkInterface.h"

class network_interface_card : public sc_module, public Master
{
public:
	sc_port<bus_slave_if> slvIf;
	sc_event read_event, send_event;

	unsigned mem_address, packet_length, conn;

	unsigned mac_addr, dest_addr;

	unsigned *packet_data;

	unsigned receive_buffer[MAX_PACKET_SIZE];

	SC_HAS_PROCESS(network_interface_card);

	network_interface_card(sc_module_name name, unsigned id, unsigned addr, unsigned dest) : sc_module(name),
		Master(id)
	{
		mac_addr = addr;
		dest_addr = dest;
		SC_THREAD(receive_frame);
		SC_THREAD(send_frame);
		SC_THREAD(read_packet);
		SC_THREAD(wait_for_buffer_descriptor);
	}

	void wait_for_buffer_descriptor()
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
			}
		}
	}

	void setup_tcp_connection()
	{
		//assuming a 3 way handshake, each packet has a 20 byte IP header and a 24 byte TCP Header + data = 44 bytes * 3 = 132 Bytes
		//bandwidth including headers = A = ((L + I + T) / L) * R =  ((1460 + 20 + 20)/1460) * 2621440 Bps = 2693260 Bps
		//time = 132 / 2693260 = 49.01 us
		wait(49.01, SC_US);
		connection_ready.notify();
	}

	void tear_down_tcp_connection()
	{
		//assuming a 4 way handshake, each packet has a 20 byte IP header and a 20 byte TCP Header = 40 bytes * 4 = 160 Bytes
		//bandwidth including headers = A = ((L + I + T) / L) * R =  ((1460 + 20 + 20)/1460) * 2621440 Bps = 2693260 Bps
		//time = 160 / 2693260 = 59.41 us
		wait(59.41, SC_US);
	}

	void read_packet()
	{
		while (true)
		{
			wait(read_event);
			packet_data = new unsigned[packet_length];

			packet_data = this->MasterDataTransfer(NULL, true, mem_address, packet_length); //read packet data from memory

			cout << "packet data:" << endl;
			for (unsigned i = 0; i < packet_length; i++)
				cout << packet_data[i] << endl;

			send_event.notify();
		}
	}

	void send_frame()
	{
		while (true)
		{
			wait(send_event);
			network_frame = new ethernet_frame(dest_addr, packet_data, packet_length);
			//wait appropriate time
			//20Mbps = 2.5MBps = 2621440 Bps
			//power = 0.2 J/MB => 0.2 J/MB * 2.5MBps = 0.5 Watts
			wait((float)packet_length / 2621440.0, SC_SEC);
			receive_event.notify();
		}
	}

	void receive_frame()
	{
		while (true)
		{
			wait(receive_event);
			if (network_frame->destination_address == mac_addr)
			{
				for (unsigned i = 0; i < network_frame->data_length; i++)
				{
					receive_buffer[i] = network_frame->data[i];
				}
				this->MasterDataTransfer(receive_buffer, false, MEM_START_ADDR + 10, network_frame->data_length); //write c
				packet_received.notify();
			}
		}

	}
};