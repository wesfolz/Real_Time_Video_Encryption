//--------------------------------------------------------------------------//

#include "Master.h"

//--------------------------------------------------------------------------//

Master::Master(unsigned id)
{
	mstId = id;
}

unsigned* Master::MasterDataTransfer(unsigned *writeData, bool rdnwr, unsigned addr, unsigned len)
{
	bool ack = mstIf->MstBusRequest(mstId, rdnwr, addr, len);

	//cout << sc_time_stamp() << endl;

	unsigned *readData = new unsigned[len];

	if (ack)
	{
		if (rdnwr) //read
		{
			for (unsigned i = 0; i < len; i++)
			{
				mstIf->MstReadData(readData[i]);
			}
			return readData;
		}
		else //write
		{
			for (unsigned i = 0; i < len; i++)
			{
				mstIf->MstWriteData(*(writeData + i));
			//	cout << sc_time_stamp() << endl;
			}
		}
	}
	return NULL;
}

//--------------------------------------------------------------------------//
