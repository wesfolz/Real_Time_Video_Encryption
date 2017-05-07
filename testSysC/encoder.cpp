//--------------------------------------------------------------------------//

#include "encoder.h"

//--------------------------------------------------------------------------//

sc_event encode_done;

//--------------------------------------------------------------------------//

//VideoCore IV GPU

encoder::encoder(sc_module_name name, char *path, unsigned id) : sc_module(name),
	Master(id)
{
	file = fopen(path, "rb");
	if (!file)
	{
		printf("Unable to open file!");
		return;
	}
	encoderEnergy = 0;
	SC_THREAD(encode);
	SC_THREAD(read_file);
}

void encoder::encode()
{
	unsigned addr, len;
	bool rdnwr;

	while (true)
	{
		//wait for master request
		slvIf->SlvListen(addr, rdnwr, len);

		if (addr >= ENCODER_START_ADDR && addr + len <= ENCODER_END_ADDR)
		{
			slvIf->SlvAcknowledge();

			if (!rdnwr) //write
			{
				for (int i = 0; i < 2; i++)
				{
					if(i == 0)slvIf->SlvReceiveWriteData(mem_address_dst0); //get compressed video destination address
					else slvIf->SlvReceiveWriteData(mem_address_dst1);
				}
				read.notify();
			}
			else //read
			{
				slvIf->SlvSendReadData(frame_length);
			}
		}

	}
}

void encoder::read_file()
{
	wait(read);
	int count = 0;

	unsigned adr_num = 0;

	double tic = 0;
	double toc = 0;
	while (!feof(file))
	{
		tic = sc_time_stamp().to_double();
		new_frame();//fill buffer with a compressed frame
					//send data
		//wait(8, SC_MS);
		wait(3.5, SC_MS);

		frame_length = buffer_ptr;

		for (int i = 0; i < buffer_ptr; i++)
		{
			bufferInt[i] = buffer[i];
		}
		if (adr_num == 0) {
			this->MasterDataTransfer(bufferInt, false, mem_address_dst0, buffer_ptr); //write data to memory
			adr_num = 1;
		}
		else
		{
			this->MasterDataTransfer(bufferInt, false, mem_address_dst1, buffer_ptr); //write data to memory
			adr_num = 0;
		}
		encode_done.notify();
//		encoderEnergy += ENCODER_POWER * (0.008);
		encoderEnergy += ENCODER_POWER * (0.0035);
//		encoderEnergy += ENCODER_LOW_POWER * (0.025);
		encoderEnergy += ENCODER_LOW_POWER * (0.0295);
		toc = sc_time_stamp().to_double();//get end time

		cout << "Encoder time: " << toc - tic << endl;

		if (toc - tic < 33000000.f)//wait for 33ms since last frame
		{
			wait(33000000.f - (toc - tic), SC_NS);
		}
		count++;

		if(count >= 11 || DEMO)
			return;
	}
	fclose(file);
}

void encoder::new_frame()
{
	char nal_header = -1;
	unsigned zeros = 0;
	unsigned done = 0;
	buffer_ptr = 0;
	unsigned char cur_byte;
	while (!done)
	{
		do
		{
			if (feof(file))return;
			zeros++;
			fread(&cur_byte, sizeof(unsigned char), 1, file);
			if (nal_header == -1)//have not recieved nal header
			{
				nal_header = cur_byte & 31;
			}
			buffer[buffer_ptr++] = cur_byte;//save current byte in buffer
		} while (cur_byte == '\0');//exit to check if reached the end of NAL

									//check if reached next nal package
		if ((cur_byte == 1 || cur_byte == 2 || cur_byte == 3) && zeros >= 3)
		{
			buffer_ptr -= 4;//remove the NAL separation message 0x00000001
			if (nal_header < 7 && nal_header >= 1)//nal message was a frame -> done (nal_header != -1)
			{
				done = 1;
				//cout << "nal header " << (int)nal_header << endl;
			}
			else//this header was not a frame, get next one
			{
				//reset header
				nal_header = -1;
			}
		}
		zeros = 0;
	}
}

//--------------------------------------------------------------------------//