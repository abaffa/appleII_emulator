#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Saturn 128k Card Emulator
// by Augusto Baffa jun 2023

int saturn_slot = 0;
bool card_saturn = true;

class SaturnCard {

	/*
	//notes:
	//
	// To Control the card:
	//
	// DEVSEL (0xC0B0-C0BF - slot 3)
	// Read
	//		saturncard.swap_bank(addr, ram);
	//
	// Write
	//		saturncard.swap_bank(addr, ram);
	//
	// Every Read (0xD000-FFFF)
	//		INH = saturncard.INH(addr, false);
	//		if (INH)  // READ ROM FROM COMPUTER ROM
	//			r = computer_rom[addr];
	//		else      // READ ROM FROM SATURN MEM
	//			r = saturncard.mem_read(addr);
	//
	// Every Write (0xD000-FFFF)
	//		INH = saturncard.INH(addr, false);
	//		if (!INH) // WRITE ROM TO SATURN MEM
	//			saturncard.mem_write(addr, val);
	//
	*/

private:
	uint8_t g_aSaturnPagesa[0x8][0x3000];
	uint8_t g_aSaturnPagesb[0x8][0x1000];

	char bank = 'A';
	int g_uSaturnActiveBank = 1;

	bool ENRD = false;
	bool ENWR = false;
	bool ENWR1 = false;

public:

	bool INH(uint16_t addr, bool RnotW) {

		bool INHOE =
			((ENRD && RnotW) || (ENWR && !RnotW))
			&& addr >= 0xd000;

		return !INHOE;
	}

	void swap_bank(uint16_t addr, uint8_t *ram) {
		if (((addr & 0b11111111) >= 0x80) && ((addr & 0b11111111) <= 0x8F))
		{

			/*
					Bin   Addr.
					Bank A
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

					N = 8 + Saturn 32K board slot # ( for slot 0, N=8).
			*/
			switch (addr & 0b1111) {
			case 0x0://		$C0N0 4K Bank A, RAM read, Write protect
				ENRD = (addr & 0b1) == ((addr >> 1) & 0b1);
				ENWR = (addr & 0b1) == 1 && ENWR1;
				ENWR1 = (addr & 0b1);
				bank = 'A';
				break;
			case 0x1://		$C0N1 4K Bank A, ROM read, Write enabled
				ENRD = (addr & 0b1) == ((addr >> 1) & 0b1);
				ENWR = (addr & 0b1) == 1 && ENWR1;
				ENWR1 = (addr & 0b1);
				bank = 'A';
				break;
			case 0x2://		$C0N2 4K Bank A, ROM read, Write protect
				ENRD = (addr & 0b1) == ((addr >> 1) & 0b1);
				ENWR = (addr & 0b1) == 1 && ENWR1;
				ENWR1 = (addr & 0b1);
				bank = 'A';
				break;
			case 0x3://		$C0N3 4K Bank A, RAM read, Write enabled
				ENRD = (addr & 0b1) == ((addr >> 1) & 0b1);
				ENWR = (addr & 0b1) == 1 && ENWR1;
				ENWR1 = (addr & 0b1);
				bank = 'A';
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

			case 0x8://		$C0N8 4K Bank B, RAM read (saturn mem enabled), Write protect
				ENRD = (addr & 0b1) == ((addr >> 1) & 0b1);
				ENWR = (addr & 0b1) == 1 && ENWR1;
				ENWR1 = (addr & 0b1);
				bank = 'B';
				break;
			case 0x9://		$C0N9 4K Bank B, ROM read (saturn mem disabled), Write enabled 
				ENRD = (addr & 0b1) == ((addr >> 1) & 0b1);
				ENWR = (addr & 0b1) == 1 && ENWR1;
				ENWR1 = (addr & 0b1);
				bank = 'B';
				break;
			case 0xA://		$C0NA 4K Bank B, ROM read (saturn mem disabled), Write protect
				ENRD = (addr & 0b1) == ((addr >> 1) & 0b1);
				ENWR = (addr & 0b1) == 1 && ENWR1;
				ENWR1 = (addr & 0b1);
				bank = 'B';
				break;
			case 0xB://		$C0NB 4K Bank B, RAM read (saturn mem enabled), Write enabled
				ENRD = (addr & 0b1) == ((addr >> 1) & 0b1);
				ENWR = (addr & 0b1) == 1 && ENWR1;
				ENWR1 = (addr & 0b1);
				bank = 'B';
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

	uint8_t mem_read(uint16_t addr) {

		if (ENRD) {
			uint16_t _addr = addr - 0xd000;

			if (bank == 'A')
				return g_aSaturnPagesa[g_uSaturnActiveBank - 1][_addr];
			else if (bank == 'B') {
				if (_addr < 0x1000)
					return g_aSaturnPagesb[g_uSaturnActiveBank - 1][_addr];
				else
					return g_aSaturnPagesa[g_uSaturnActiveBank - 1][_addr];
			}
		}
		return 0;
	}

	void mem_write(uint16_t addr, uint8_t val) {

		uint16_t _addr = addr - 0xd000; //U8D, U10D, U9A

		// U15, U8B, U8A, U10B
		if (bank == 'A')
			g_aSaturnPagesa[g_uSaturnActiveBank - 1][_addr] = val;

		else if (bank == 'B') {
			if (_addr < 0x1000)
				g_aSaturnPagesb[g_uSaturnActiveBank - 1][_addr] = val;
			else
				g_aSaturnPagesa[g_uSaturnActiveBank - 1][_addr] = val;
		}
		else {
			bank = bank;
		}
	}

};