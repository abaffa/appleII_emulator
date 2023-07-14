#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

bool card_saturn = true;
int mem_slot = 0;

static uint8_t g_aSaturnPagesa[0x8][0x3000];
static uint8_t g_aSaturnPagesb[0x8][0x1000];

int bank = 0;
int g_uSaturnActiveBank = 1;

bool card_saturn_ROMCE = true;
bool card_saturn_RAMCE = false;
bool card_saturn_RAMRO = false;

void card_saturn_swap_bank(uint16_t addr, uint8_t *ram) {
	if (((addr & 0b11111111) >= 0x80) && ((addr & 0b11111111) <= 0x8F))
	{

		/*
				Bin   Addr.
					  $C0N0 4K Bank A, RAM read, Write protect
					  $C0N1 4K Bank A, ROM read, Write enabled
					  $C0N2 4K Bank A, ROM read, Write protect
					  $C0N3 4K Bank A, RAM read, Write enabled
				0100  $C0N4 select 16K Bank 1
				0101  $C0N5 select 16K Bank 2
				0110  $C0N6 select 16K Bank 3
				0111  $C0N7 select 16K Bank 4
					  $C0N8 4K Bank B, RAM read, Write protect
					  $C0N9 4K Bank B, ROM read, Write enabled
					  $C0NA 4K Bank B, ROM read, Write protect
					  $C0NB 4K Bank B, RAM read, Write enabled
				1100  $C0NC select 16K Bank 5
				1101  $C0ND select 16K Bank 6
				1110  $C0NE select 16K Bank 7
				1111  $C0NF select 16K Bank 8
		*/
		switch (addr & 0b1111) {
		case 0x0://		$C0N0 4K Bank A, RAM read, Write protect
			card_saturn_RAMCE = true;
			card_saturn_ROMCE = false;
			card_saturn_RAMRO = true;
			bank = 0;
			break;
		case 0x1://		$C0N1 4K Bank A, ROM read, Write enabled
			card_saturn_RAMCE = false;
			card_saturn_ROMCE = true;
			card_saturn_RAMRO = false;
			bank = 0;
			break;
		case 0x2://		$C0N2 4K Bank A, ROM read, Write protect
			card_saturn_RAMCE = false;
			card_saturn_ROMCE = true;
			card_saturn_RAMRO = true;
			bank = 0;
			break;
		case 0x3://		$C0N3 4K Bank A, RAM read, Write enabled
			card_saturn_RAMCE = true;
			card_saturn_ROMCE = false;
			card_saturn_RAMRO = false;
			bank = 0;
			break;
		case 0x4://0100  $C0N4 select 16K Bank 1
			g_uSaturnActiveBank = 1;
			break;
		case 0x5://0101  $C0N5 select 16K Bank 2
			g_uSaturnActiveBank = 2;
			break;
		case 0x6://0110  $C0N6 select 16K Bank 3
			g_uSaturnActiveBank = 3;
			break;
		case 0x7://0111  $C0N7 select 16K Bank 4
			g_uSaturnActiveBank = 4;
			break;
		case 0x8://		$C0N8 4K Bank B, RAM read, Write protect
			card_saturn_RAMCE = true;
			card_saturn_ROMCE = false;
			card_saturn_RAMRO = true;
			bank = 1;
			break;
		case 0x9://		$C0N9 4K Bank B, ROM read, Write enabled
			card_saturn_RAMCE = false;
			card_saturn_ROMCE = true;
			card_saturn_RAMRO = false;
			bank = 1;
			break;
		case 0xA://		$C0NA 4K Bank B, ROM read, Write protect
			card_saturn_RAMCE = false;
			card_saturn_ROMCE = true;
			card_saturn_RAMRO = true;
			bank = 1;
			break;
		case 0xB://		$C0NB 4K Bank B, RAM read, Write enabled
			card_saturn_RAMCE = true;
			card_saturn_ROMCE = false;
			card_saturn_RAMRO = false;
			bank = 1;
			break;
		case 0xC://1100  $C0NC select 16K Bank 5
			g_uSaturnActiveBank = 5;
			break;
		case 0xD://1101  $C0ND select 16K Bank 6
			g_uSaturnActiveBank = 6;
			break;
		case 0xE://1110  $C0NE select 16K Bank 7
			g_uSaturnActiveBank = 7;
			break;
		case 0xF://1111  $C0NF select 16K Bank 8
			g_uSaturnActiveBank = 8;
			break;
		}
	}
}

uint8_t card_saturn_mem_read(uint16_t addr) {
	uint16_t _addr = addr - 0xd000;

	if (bank == 0)
		return g_aSaturnPagesa[g_uSaturnActiveBank - 1][_addr];
	else if (bank == 1) {
		if (_addr < 0x1000)
			return g_aSaturnPagesb[g_uSaturnActiveBank - 1][_addr];
		else
			return g_aSaturnPagesa[g_uSaturnActiveBank - 1][_addr];
	}
}
void card_saturn_mem_write(uint16_t addr, uint8_t val) {
	if (!card_saturn_RAMRO) {
		uint16_t _addr = addr - 0xd000;
		if (bank == 0)
			g_aSaturnPagesa[g_uSaturnActiveBank - 1][_addr] = val;
		else if (bank == 1) {
			if (_addr < 0x1000)
				g_aSaturnPagesb[g_uSaturnActiveBank - 1][_addr] = val;
			else
				g_aSaturnPagesa[g_uSaturnActiveBank - 1][_addr] = val;
		}
	}
	else {
		bank = bank;
	}
}

