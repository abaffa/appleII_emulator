#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

bool card_prodos = true;

int drive_slot = 7;

// Prodos Card Emulator
// by Augusto Baffa jun 2023

class ProdosCard {

private:
	uint8_t drive_rom[0x100000];

	uint8_t card_prodos_LowAddrLatch1 = 0;
	uint8_t card_prodos_HiAddrLatch1 = 0;

public:


	int init(char *drive_filename) {
		printf("Loading rom: %s\n", drive_filename);
		FILE* f = fopen(drive_filename, "rb");
		if (!f)
		{
			printf("Failed to open the file\n");
			printf("Press any key to continue");
			return -1;
		}

		fseek(f, 0, SEEK_END);
		long size = ftell(f);
		fseek(f, 0, SEEK_SET);

		char* buf = (char*)malloc(size * sizeof(char));

		int res = fread(buf, size, 1, f);
		if (res != 1)
		{
			printf("Failed to read from file");
			return -1;
		}

		memcpy(&drive_rom[0x0000], buf, size);
		return 0;
	}

	uint8_t read(uint16_t addr, bool IO_SEL_SLOTS, bool DEV_SEL_SLOTS) {

		uint8_t c4 = 0;
		uint8_t c12 = 0;

		if (IO_SEL_SLOTS) {
			c4 = ((addr >> 4) & 0b1111) | 0b00110000;
			c12 = 0;
		}

		if (DEV_SEL_SLOTS) {
			c4 = card_prodos_LowAddrLatch1;
			c12 = card_prodos_HiAddrLatch1;
		}

		uint32_t _addr = ((addr & 0b1111) | (c4 << 4) | (c12 << 12)) & 0xFFFFF;
		/*
		if (_addr >= 0x7c54 && _addr <= 0x7df7)
			fprintf(stderr, "%c", drive_rom[_addr]);

		if (_addr >= 0x864d && _addr <= 0x867e)
			fprintf(stderr, "%c", drive_rom[_addr]);
			*/

			//if((_addr >= 0xff613 && _addr < 0xff61f) || _addr == 0x7c13)
			//	fprintf(stderr, "%c", drive_rom[_addr] + 0x60);

			//fprintf(stderr, "%4x %c\n", _addr,drive_rom[_addr]);

		if (IO_SEL_SLOTS & (_addr >= 0x300 && _addr <= 0x3ff)) {
			//if (_addr >= 0x3cf && _addr <= 0x3df)
				//fprintf(stderr, "### %4x %4x %2x %c %c\n", addr, _addr, drive_rom[_addr], drive_rom[_addr], drive_rom[_addr] -0x80);
			return drive_rom[_addr];
		}
		else {
			return drive_rom[_addr];
		}

	}

	void write(uint16_t addr, uint8_t val, bool IO_SEL_SLOTS, bool DEV_SEL_SLOTS) {

		if (DEV_SEL_SLOTS & !(addr & 0b1)) card_prodos_LowAddrLatch1 = val;

		if (DEV_SEL_SLOTS & (addr & 0b1))  card_prodos_HiAddrLatch1 = val;

		uint8_t c4 = 0;
		uint8_t c12 = 0;

		if (IO_SEL_SLOTS) {

			c4 = ((addr >> 4) & 0b1111) | 0b00110000;
			c12 = 0;
		}
		/*
		if (DEV_SEL_SLOTS) {
			c4 = card_prodos_LowAddrLatch1;
			c12 = card_prodos_HiAddrLatch1;
		}
		*/
		uint32_t _addr = ((addr & 0b1111) | (c4 << 4) | (c12 << 12)) & 0xFFFFF;

		if (IO_SEL_SLOTS) {
			//if (_addr >= 0x3cf && _addr <= 0x3df)
				//fprintf(stderr, "### %4x %4x %2x %c %c\n", addr, _addr, drive_rom[_addr], drive_rom[_addr], drive_rom[_addr] -0x80);

			drive_rom[_addr] = val;
		}
	}
};
