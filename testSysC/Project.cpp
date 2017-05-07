//--------------------------------------------------------------------------//

#include <systemc.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <list>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/stack.h>
#include "bus_interface.h"
#include "ecies.h"
#include "aes.h"
#include "network_interface_card.h"
#include "crypto.h"
#include "encoder.h"
#include "memory.h"
#include "Project.h"

//--------------------------------------------------------------------------//

class BusRequest
{
public:
	unsigned id;
	bool rdnwr;
	unsigned addr;
	unsigned len;
	unsigned bytesTransferred;

	BusRequest() {}

	BusRequest(unsigned id, bool rdnwr, unsigned addr, unsigned len)
	{
		this->id = id;
		this->rdnwr = rdnwr;
		this->addr = addr;
		this->len = len;
		this->bytesTransferred = 0;
	}
};

//--------------------------------------------------------------------------//

class Bus : public sc_module, public bus_master_if, public bus_slave_if
{
public:
	sc_event bus_request, data_ready, master_ack, mst_ready, slv_request, slv_ack, slv_ready, end_of_transfer;

	bool receivedResponse;

	unsigned busData;

	BusRequest currentReq;

	list<BusRequest> queue;

	SC_HAS_PROCESS(Bus);

	Bus(sc_module_name name) : sc_module(name)
	{
		SC_THREAD(Arbiter);
	}

	void Arbiter()
	{
		while (true)
		{
			if (queue.empty())
				wait(bus_request);
			currentReq = queue.front();
			queue.pop_front();
			slv_request.notify();
			receivedResponse = false;
			wait(3 * 1000 / BUS_FREQ, SC_NS, slv_ack); //wait for acknowledge or timeout, whichever comes first
			wait(1000 / BUS_FREQ, SC_NS); //wait one cycle for master ack
			master_ack.notify();
			if (receivedResponse)
			{
				wait(end_of_transfer);
			}
		}
	}
	//Master Interface
	bool MstBusRequest(unsigned mstId, bool rdnwr, unsigned addr, unsigned len)
	{
		wait(3 * 1000 / BUS_FREQ, SC_NS); //wait 3 cycles for bus request
		BusRequest req(mstId, rdnwr, addr, len);// = new BusRequest(mstId, rdnwr, addr, len);
		queue.push_back(req);
		bus_request.notify();
		while (1)
		{
			wait(master_ack);
			if (currentReq.id == mstId)
				return receivedResponse;
		}
	}

	void MstReadData(unsigned &data)
	{
		mst_ready.notify();
		wait(data_ready);
		wait(1000 / BUS_FREQ, SC_NS); //wait one cycle for data transfer
		data = busData;
		currentReq.bytesTransferred++; //incrementing number of words transferred
		if (currentReq.bytesTransferred == currentReq.len) //if the number of words transferred is equal to the rquest length, the transfer is over
		{
			//wait one cycle for end of transaction
			wait(1000 / BUS_FREQ, SC_NS);
			end_of_transfer.notify();
		}
	}

	void MstWriteData(unsigned data)
	{
		wait(1000 / BUS_FREQ, SC_NS); //wait one cycle for data transfer
		busData = data;
		data_ready.notify();
		wait(slv_ready);
	}

	//Slave Interface
	void SlvListen(unsigned &reqAddr, bool &reqRdnwr, unsigned &reqLen)
	{
		wait(slv_request);
		reqAddr = currentReq.addr;
		reqRdnwr = currentReq.rdnwr;
		reqLen = currentReq.len;
	}

	void SlvAcknowledge()
	{
//		wait(1000 / BUS_FREQ, SC_NS); //wait one cycle for ack
		receivedResponse = true;
		slv_ack.notify();
	}

	void SlvSendReadData(unsigned data)
	{
		wait(mst_ready);
//		wait(1000 / BUS_FREQ, SC_NS); //wait one cycle for data transfer
		busData = data;
		data_ready.notify();
	}

	void SlvReceiveWriteData(unsigned &data)
	{
		wait(data_ready);
//		wait(1000 / BUS_FREQ, SC_NS); //wait one cycle for data transfer
		data = busData;
		currentReq.bytesTransferred++; //incrementing number of words transferred
		slv_ready.notify();
		if (currentReq.bytesTransferred == currentReq.len) //if the number of words transferred is equal to the rquest length, the transfer is over
		{
			wait(1000 / BUS_FREQ, SC_NS); //wait one cycle for end of transaction
			end_of_transfer.notify();
		}
	}

};

//--------------------------------------------------------------------------//

class Microprocessor : public sc_module, public Master
{
public:
	
	SC_HAS_PROCESS(Microprocessor);

	Microprocessor(sc_module_name name, unsigned id) : sc_module(name),
		Master(id)
	{
		SC_THREAD(send_packet);
		//this->mstId = 0;
	}

	void send_packet()
	{
		unsigned encod_adr[2];// = new unsigned[2];
		encod_adr[0] = MEM_START_ADDR;
		encod_adr[1] = MEM_START_ADDR + MAX_FRAME_SIZE;
		unsigned* conn = new unsigned;
		unsigned frame_num = 0;
		*conn = 1;
		this->MasterDataTransfer(conn, false, NIC_START_ADDR, 1); //setup tcp connection
		wait(connection_ready);
		//cout << "Connection Setup Time: " << sc_time_stamp() << endl;

		this->MasterDataTransfer(encod_adr, false, ENCODER_START_ADDR, 2); //encoder
		
		while (true)
		{
			wait(encode_done);

			unsigned *frameLength = this->MasterDataTransfer(NULL, true, ENCODER_START_ADDR, 1); //get length of encoded frame

#if DEMO
			*frameLength = 16;
#endif

			cout << "frame length " << *frameLength << endl;

			unsigned cryptoInfo[4];
			if (frame_num%2 == 0)
			{
				cryptoInfo[0] = MEM_START_ADDR; //src
				cryptoInfo[1] = MEM_START_ADDR + MAX_FRAME_SIZE;//dest
			}
			else
			{
				cryptoInfo[0] = MEM_START_ADDR + 3 * MAX_FRAME_SIZE;// *frameLength; //src2
				cryptoInfo[1] = MEM_START_ADDR + 4 * MAX_FRAME_SIZE;// *frameLength; //dest2
			}
			cryptoInfo[2] = 1; //encrypt
			cryptoInfo[3] = *frameLength;// *frameLength;

			this->MasterDataTransfer(cryptoInfo, false, CRYPTO_START_ADDR, 4); //encrypt

			wait(encrypt_done);

			unsigned buffer_descriptor[2];
			if (frame_num % 2 == 0)
			{
				buffer_descriptor[0] = MEM_START_ADDR + MAX_FRAME_SIZE;
			}
			else
			{
				buffer_descriptor[0] = MEM_START_ADDR + 4 * MAX_FRAME_SIZE;
			}
			buffer_descriptor[1] = *frameLength;
			if(*frameLength%16 != 0 && AES)
				buffer_descriptor[1] = *frameLength + (16 - *frameLength % 16);

			this->MasterDataTransfer(buffer_descriptor, false, NIC_START_ADDR, 2); //write packet address and length to hardware

			frame_num++;
		}
	}
};

//--------------------------------------------------------------------------//

class Microprocessor2 : public sc_module, public Master
{
public:

	SC_HAS_PROCESS(Microprocessor2);

	Microprocessor2(sc_module_name name, unsigned id) : sc_module(name),
		Master(id)
	{
		SC_THREAD(decrypt_packet);
		//this->mstId = 0;
	}

	void decrypt_packet()
	{
		unsigned *zero = new unsigned;
		*zero = MEM_START_ADDR;
		unsigned mem_num = 0;
		while (true)
		{
			wait(packet_received);

			unsigned *length = this->MasterDataTransfer(NULL, true, NIC_START_ADDR, 1); //get length of packet data

			unsigned cryptoInfo[4];
			if (mem_num == 0)
			{
				cryptoInfo[0] = MEM_START_ADDR; //src
				cryptoInfo[1] = MEM_START_ADDR + 3*MAX_FRAME_SIZE; //dest
				mem_num = 1;
			}
			else
			{
				cryptoInfo[0] = MEM_START_ADDR + 2*MAX_FRAME_SIZE; //src
				cryptoInfo[1] = MEM_START_ADDR + 4*MAX_FRAME_SIZE; //dest
				mem_num = 0;
			}
			cryptoInfo[2] = 0;
			cryptoInfo[3] = *length;

			this->MasterDataTransfer(cryptoInfo, false, CRYPTO_START_ADDR, 4); //decrypt
			wait(decrypt_done);

			//cout << "Total Time: " << sc_time_stamp() << endl;// << endl << endl;

			//this->MasterDataTransfer(conn, false, NIC_START_ADDR, 1); //teardown tcp connection
		}
	}

};

//--------------------------------------------------------------------------//

class top : public sc_module
{
public:
	Microprocessor *up;
	memory *mem1;
	Bus *bus1;
	network_interface_card *nic1;
	crypto *crypto1;
	encoder *enc;

	Microprocessor2 *up2;
	memory *mem2;
	Bus *bus2;
	network_interface_card *nic2;
	crypto *crypto2;

	top(sc_module_name name) : sc_module(name)
	{
		ecKey = ecies_key_create(); //create ecc key for both cryptos (assume exchange already done by diffie helman)
		char* filename = "720p_static.h264";
#if DEMO
		filename = "dummy_h264.avc";
#endif
		bus1 = new Bus("bus");
		mem1 = new memory("mem");
		nic1 = new network_interface_card("nic", 1, 0, 1);
		crypto1 = new crypto("crypto1", 2);
		enc = new encoder("encoder", filename, 3);
		up = new Microprocessor("up", 0);
		enc->slvIf(*bus1);
		enc->mstIf(*bus1);
		crypto1->slvIf(*bus1);
		crypto1->mstIf(*bus1);
		mem1->slvIf(*bus1);
		nic1->slvIf(*bus1);
		nic1->mstIf(*bus1);
		up->mstIf(*bus1);

		bus2 = new Bus("bus2");
		mem2 = new memory("mem2");
		nic2 = new network_interface_card("nic2", 1, 1, 0);
		crypto2 = new crypto("crypto2", 2);
		up2 = new Microprocessor2("up2", 0);
		crypto2->slvIf(*bus2);
		crypto2->mstIf(*bus2);
		mem2->slvIf(*bus2);
		nic2->slvIf(*bus2);
		nic2->mstIf(*bus2);
		up2->mstIf(*bus2);
	}
};

//--------------------------------------------------------------------------//

int sc_main(int argc, char *argv[])
{
	//instantiate memory component
	top t1("t1");

	sc_set_time_resolution(1, SC_NS);
	//start simulation
	sc_start();

#if DEMO
	cout << "Contents of memory 1 " << endl;
	for (int i = 0; i < 16; i++) {
		//if (i % 16 == 0) cout << "block" << endl;
		cout << (signed)t1.mem1->memData[i] << endl;
	}

	cout << "Contents of memory 2 " << endl;
	for (int i = 3 * MAX_FRAME_SIZE; i < 3 * MAX_FRAME_SIZE + 16; i++) {
		//if (i % 26 == 0) cout << "block" << endl;
		cout << (signed)t1.mem2->memData[i] << endl;
	}
#endif

#if !DEMO
	cout << "encrypt energy " << t1.crypto1->energy << endl;
	cout << "decrypt energy " << t1.crypto2->energy << endl;
	cout << "nic energy " << t1.nic1->nicEnergy + t1.nic2->nicEnergy << endl;
	cout << "encoder energy " << t1.enc->encoderEnergy << endl;
#endif
	return 0;
}

//--------------------------------------------------------------------------//