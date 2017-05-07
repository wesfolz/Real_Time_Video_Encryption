
//--------------------------------------------------------------------------//
#include "crypto.h"

EC_KEY *ecKey;
sc_event encrypt_done, decrypt_done;
char *cipherText;

//--------------------------------------------------------------------------//

crypto::crypto(sc_module_name name, unsigned id) : sc_module(name),
	Master(id)
{
	SSL_library_init();
	SSL_load_error_strings();
	ecies_group_init();
	srand(time(NULL));
	energy = 0.0;
	
	publicKey = ecies_key_public_get_hex(ecKey);
	privateKey = ecies_key_private_get_hex(ecKey);

	uint8_t tempKey[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	uint8_t tempIV[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
	key = new uint8_t[16];
	iv = new uint8_t[16];
	for (int i = 0; i < 16; i++)
	{
		key[i] = tempKey[i];
		iv[i] = tempIV[i];
		buffer[i] = 'a';
	}
	SC_THREAD(cryptography);
#if AES
	SC_THREAD(aesEncrypt);
	SC_THREAD(aesDecrypt);
#endif
#if ECC
	SC_THREAD(eccEncrypt);
	SC_THREAD(eccDecrypt);
#endif
}

void crypto::cryptography()
{
	unsigned addr, len;
	bool rdnwr;

	while (true)
	{
		slvIf->SlvListen(addr, rdnwr, len);

		if (addr >= CRYPTO_START_ADDR && addr + len <= CRYPTO_END_ADDR)
		{
			slvIf->SlvAcknowledge();

			if (!rdnwr) //write
			{
				for (int i = 0; i < 4; i++)
				{
					if (i == 0)
						slvIf->SlvReceiveWriteData(mem_address_src); //get source address
					else if (i == 1)
						slvIf->SlvReceiveWriteData(mem_address_dst); //get dst address
					else if (i == 2)
						slvIf->SlvReceiveWriteData(encrypt); //encrypt or decrypt
					else
						slvIf->SlvReceiveWriteData(length); //get length of packet
				}
			}

			if (encrypt)encrypt_event.notify();
			else	decrypt_event.notify();
		}
		//AES128_CBC_encrypt_buffer(buffer, in, 64, key, iv);
		//AES128_ECB_decrypt(in, key, buffer);
	}
}

void crypto::aesEncrypt()
{
	while (true)
	{
		wait(encrypt_event);
		sc_time tic = sc_time_stamp();
		read_data(); //get block from memory

		unsigned int length16 = length;// +(16 - length % 16);

		if(length%16 != 0)
			length16 = length + (16 - length % 16);

		char* charData = new char[length16];
		bufferInt = new unsigned[length16];

		for (int i = 0; i < length16; i++) {
			if (i < length)
				charData[i] = (signed char)data[i];
			else
				charData[i] = 0;
		}

		AES128_CBC_encrypt_buffer(buffer, (unsigned char*)charData, length16, key, iv);//encrypt

		for (int i = 0; i < length16; i++) {
			bufferInt[i] = buffer[i];
		}
		energy += (double)AES_POWER*((double)(double)length16*AES_THROUGHPUT) / 1000000000;

		wait(length16*AES_THROUGHPUT, SC_NS);
		write_data(length16);//write back to memory
		cout << "Encrypt time: " << sc_time_stamp() - tic << endl;
		encrypt_done.notify();
	}
}

void crypto::aesDecrypt()
{
	while (true)
	{
		wait(decrypt_event);
		sc_time tic = sc_time_stamp();

		read_data(); //get block from memory

		unsigned int length16 = length;// +(16 - length % 16);
		if(length%16 != 0)
			length16 = length + (16 - length % 16);


		unsigned char* charData = new unsigned char[length16];
		bufferInt = new unsigned[length16];

		for (int i = 0; i < length16; i++) {
			if (i < length)
				charData[i] = data[i];
			else
				charData[i] = 0;
		}

		AES128_CBC_decrypt_buffer(buffer, charData, length16, key, iv);

		energy += (double)AES_POWER*((double)(double)length16 * AES_THROUGHPUT) / 1000000000;

		wait(length16*AES_THROUGHPUT, SC_NS);
		for (int i = 0; i < length16; i++) {
			bufferInt[i] = buffer[i];
		}
		write_data(length16);//write back to memory
		cout << "Decrypt time: " << sc_time_stamp() - tic << endl;
		decrypt_done.notify();
	}
}

void crypto::eccEncrypt()
{
	while (true)
	{
		wait(encrypt_event);

		sc_time tic = sc_time_stamp();

		bufferInt = new unsigned[length];
		unsigned char* cd = new unsigned char[length];
		char *buff = new char[length];
		read_data();
		for (int i = 0; i < length; i++) {
			cd[i] = data[i];
		}
		if (!(cipherText = ecies_encrypt(publicKey, cd, length))) {

			printf("The encryption process failed!\n");
			return;
		}
		bufferInt = (unsigned *)cipherText;

		energy += ECC_POWER*(length * ECC_THROUGHPUT) / 1000000000;

		wait(length*ECC_THROUGHPUT, SC_NS);
		write_data(length);//write back to memory

		cout << "Encrypt time: " << sc_time_stamp() - tic << endl;

		encrypt_done.notify();
		delete[] cd;
		delete[] buff;
	}
}

void crypto::eccDecrypt()
{
	while (true)
	{
		wait(decrypt_event);
		sc_time tic = sc_time_stamp();

		size_t olen;
		bufferInt = new unsigned[length];
		char* cd = new char[length];
		unsigned char *buff = new unsigned char[length];
		read_data();

		cd = (char*)data;

		if (!(buff = ecies_decrypt(privateKey, cipherText, &olen))) {
			printf("The decryption process failed!\n");
			return;
		}
		for (int i = 0; i < length; i++) {
			bufferInt[i] = buff[i];
		}

		energy += ECC_POWER*(length * ECC_THROUGHPUT) / 1000000000;

		wait(length*ECC_THROUGHPUT, SC_NS);
		write_data(length);//write back to memory
		cout << "Decrypt time: " << sc_time_stamp() - tic << endl;
		decrypt_done.notify();
	}
}

void crypto::write_data(unsigned int len)
{
	this->MasterDataTransfer(bufferInt, false, mem_address_dst, len); //write data to memory+	
}

void crypto::read_data()
{
	data = this->MasterDataTransfer(NULL, true, mem_address_src, length); //read block from memory
}


//--------------------------------------------------------------------------//