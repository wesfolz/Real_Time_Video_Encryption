//--------------------------------------------------------------------------//

#include "memory.h"

//--------------------------------------------------------------------------//

void memory::MemDataTransfer()
{
	unsigned addr, len;
	bool rdnwr;

	while (true)
	{
		slvIf->SlvListen(addr, rdnwr, len);

		if (addr >= MEM_START_ADDR && addr + len <= MEM_START_ADDR + MEM_SIZE)
		{
			addr = addr - MEM_START_ADDR;
			slvIf->SlvAcknowledge();
			if (rdnwr) //read
			{
				for (unsigned i = 0; i < len; i++)
				{
					//slvIf->SlvSendReadData(memData[addr + i]);
					//read
					Read(addr + i);
				}
			}
			else //write
			{
				for (unsigned i = 0; i < len; i++)
				{
					unsigned data;
					//slvIf->SlvReceiveWriteData(memData[addr+i]);
					slvIf->SlvReceiveWriteData(data);
					Write(addr + i, data);
				}
			}
		}
	}
}

memory::memory(sc_module_name name) : sc_module(name)
{
	//initializing threads
	SC_THREAD(MemDataTransfer);

	for (int i = 0; i < 16; i++)
	{
		memData[i] = i + 65;
	}
}

bool memory::Write(unsigned int addr, unsigned int data)
{
	memData[addr] = data;
	return true;
}

bool memory::Read(unsigned int addr)
{
	slvIf->SlvSendReadData(memData[addr]);
	return true;
}

//--------------------------------------------------------------------------//
