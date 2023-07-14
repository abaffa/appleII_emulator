#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

bool disk = true;

static uint8_t drive_rom[0x100000];

uint8_t card_prodos_LowAddrLatch1 = 0;
uint8_t card_prodos_HiAddrLatch1 = 0;

int drive_slot = 7;


uint8_t board_prodos_read(uint16_t addr, bool IO_SEL_SLOTS, bool DEV_SEL_SLOTS) {


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

void board_prodos_write(uint16_t addr, uint8_t val, bool IO_SEL_SLOTS, bool DEV_SEL_SLOTS) {

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

