//
// main.cpp
//
////// BEGIN LICENSE NOTICE//////
//
//Apple][+ Emulator 
//
//Copyright(C) 2023 Augusto Baffa, (baffa-6502.baffasoft.com.br)
//
//This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110 - 1301, USA.
//
////// END LICENSE NOTICE//////
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include "6502.h"
#include "utils.h"
#include "keyboard.h"

#include "SDL2/SDL.h"

#ifdef _MSC_VER    
#include <windows.h>
#include <conio.h>
#endif


#ifdef __MINGW32__
#include <conio.h>
#endif

#if defined(__linux__) || defined(__MINGW32__)
#include <fcntl.h>
#include <pthread.h> 
#else
#include <mutex> 
#endif

#if defined(__linux__) || defined(__MINGW32__)
#include <time.h>
#else
#include <chrono>
using namespace std::chrono;

#endif

#include <queue>
using namespace std;

#ifdef _MSC_VER    
mutex mtx_out;
condition_variable cv_out;

#else
pthread_mutex_t mtx_out;
#endif

#include "card_saturn_mem.h"
#include "card_videx.h"
#include "card_prodos.h"

VidexCard videxcard;
SaturnCard saturncard;
ProdosCard prodoscard;

struct keyboard keyboard;

static uint8_t rom[0x10000];	/* Covers the banked card */
static uint8_t ram[0x10000];	/* Covers the banked card */
static uint8_t video_rom[0x800];

#define VIDEO_ROM  0

static uint8_t fast = 0;

static uint16_t addrinvert = 0x0000;

static uint16_t tstate_steps = 200;	/* 4MHz */

/* Who is pulling on the interrupt line */

static uint8_t live_irq;


#define IRQ_VIA		3
#define IRQ_PIA		33

//static struct via6522 *via;
//static struct m6821 *pia;


static volatile int done;

#define TRACE_RAM	1
#define TRACE_IO	2
#define TRACE_IRQ	4
#define TRACE_UNK	8
#define TRACE_ROM	16
#define TRACE_SLOT	32
#define TRACE_CPU	128
#define TRACE_VIA	4096
#define TRACE_PIA	8192

static int trace = 0;



static bool shift = false;
static bool ctrl = false;



bool DEV_SEL_SLOTS[8];
bool IO_SEL_SLOTS[8];


bool text_mode = false;
bool mix_mode = false;
bool video_page2 = false;
bool hires_mode = false;

int blink = 0;
char lastchar = 0;
//bool ROM_CS = true;
bool INH = true;
bool IOSLOTS = false;
bool F12_15 = false;

char *filename = "appleIIp.rom";

#if VIDEO_ROM == 0
//char *video_filename = "appleII_video.rom";
char *video_filename = "appleII_video_small.rom";

#elif VIDEO_ROM == 1
char *video_filename = "appleII_videoiie.rom";
#endif

char *drive_filename = "GamesWithFirmware.po";
//char *drive_filename = "BlankDriveWithFirmware.po";

char convert_ascii(char _key, bool ctrl, bool shift) {

	uint8_t key = (uint8_t)_key;
	printf("%x\n", key);

	// set key position similar to apple's
	if (key == '-') key = ':';
	else if (key == '=') key = '-';
	else if (key >= 0x61 && key <= 0x7a) key -= 0x20;
	//

	if (shift) key = (key & 0x10) ? key - 0x10 : key + 0x10;

	if (ctrl && (key >= 0x40 && key <= 0x5F))  key -= 0x40;
	//if (ctrl && (key >= 0x60 && key <= 0x7F))  key -= 0x60;


	return (key & 0x7f) | ((key & 0x80) ^ 0x80);
}

void display_bit_80col(SDL_Renderer* renderer, int x, int y, unsigned char b, int bit_y, int mode) {


	// appleii
	int bit_x = 0;

	int HEIGHT_MULTIPLIER = 1;
	if (mode == 0)
		HEIGHT_MULTIPLIER = ZX80_WINDOW_MULTIPLIER;
	else if (mode == 1)
		HEIGHT_MULTIPLIER = ZX80_WINDOW_MULTIPLIER / 2;
	else if (mode == 2) {
		HEIGHT_MULTIPLIER = ZX80_WINDOW_MULTIPLIER / 2;

	}

	for (bit_x = 0; bit_x < 8; bit_x++) {

		if (
			((b & 0xFF) << bit_x) & 0x80
			) {
			SDL_Rect r;
			//r.x = ((x * 8 * ZX80_WINDOW_MULTIPLIER) + (bit_x * ZX80_WINDOW_MULTIPLIER)) + ZX80_BORDER;
			//r.y = (y * 8 * ZX80_WINDOW_MULTIPLIER + bit_y * ZX80_WINDOW_MULTIPLIER) + ZX80_BORDER;
			r.x = ((x * 8 * ZX80_WINDOW_MULTIPLIER80) + ((bit_x)* ZX80_WINDOW_MULTIPLIER80)) + ZX80_BORDER80;
			//r.y = (y * 8 * ZX80_WINDOW_MULTIPLIER + bit_y * ZX80_WINDOW_MULTIPLIER) + ZX80_BORDER80;
			r.y = (y * 9 * HEIGHT_MULTIPLIER + bit_y * HEIGHT_MULTIPLIER) + ZX80_BORDER80;
			r.w = ZX80_WINDOW_MULTIPLIER80;
			r.h = HEIGHT_MULTIPLIER;
			SDL_RenderFillRect(renderer, &r);
		}
	}

}

void display_bit_ii(SDL_Renderer* renderer, int x, int y, unsigned char b, int bit_y) {


	// appleii
	int bit_x = 0;
	for (bit_x = 1; bit_x < 8; bit_x++) {
		if (
			((b & 0xFF) << bit_x) & 0x80
			) {
			SDL_Rect r;
			//r.x = ((x * 8 * ZX80_WINDOW_MULTIPLIER) + (bit_x * ZX80_WINDOW_MULTIPLIER)) + ZX80_BORDER;
			//r.y = (y * 8 * ZX80_WINDOW_MULTIPLIER + bit_y * ZX80_WINDOW_MULTIPLIER) + ZX80_BORDER;
			r.x = ((x * 7 * ZX80_WINDOW_MULTIPLIER) + ((bit_x - 1) * ZX80_WINDOW_MULTIPLIER)) + ZX80_BORDERX40 * ZX80_WINDOW_MULTIPLIER;
			r.y = (y * 8 * ZX80_WINDOW_MULTIPLIER + bit_y * ZX80_WINDOW_MULTIPLIER) + ZX80_BORDERY40 * ZX80_WINDOW_MULTIPLIER;
			r.w = ZX80_WINDOW_MULTIPLIER;
			r.h = ZX80_WINDOW_MULTIPLIER;
			SDL_RenderFillRect(renderer, &r);
		}
	}

}


void display_bit_iie(SDL_Renderer* renderer, int x, int y, unsigned char b, int bit_y) {


	//appleiie
	int bit_x = 0;
	for (bit_x = 7; bit_x > 0; bit_x--) {
		if (
			((b & 0xFF) << bit_x) & 0x80
			) {
			SDL_Rect r;
			//r.x = ((x * 8 * ZX80_WINDOW_MULTIPLIER) + (bit_x * ZX80_WINDOW_MULTIPLIER)) + ZX80_BORDER;
			//r.y = (y * 8 * ZX80_WINDOW_MULTIPLIER + bit_y * ZX80_WINDOW_MULTIPLIER) + ZX80_BORDER;
			r.x = ((x * 7 * ZX80_WINDOW_MULTIPLIER) + ((8 - bit_x) * ZX80_WINDOW_MULTIPLIER)) + ZX80_BORDERX40;
			r.y = (y * 8 * ZX80_WINDOW_MULTIPLIER + bit_y * ZX80_WINDOW_MULTIPLIER) + ZX80_BORDERY40;
			r.w = ZX80_WINDOW_MULTIPLIER;
			r.h = ZX80_WINDOW_MULTIPLIER;
			SDL_RenderFillRect(renderer, &r);
		}
	}
}


/* We do this in the 6502 loop instead. Provide a dummy for the device models */
void recalc_interrupts(void)
{
}

static void int_set(int src)
{
	live_irq |= (1 << src);
}

static void int_clear(int src)
{
	live_irq &= ~(1 << src);
}


uint8_t mmio_read_6502(uint16_t addr)
{
	if (trace & TRACE_IO)
		fprintf(stderr, "read %02x\n", addr);
	//if (addr >= 0x60 && addr <= 0x6F)
	//	return via_read(via, addr & 0x0F);

	if (addr >= 0xD000 && addr < 0xE000) {
		//uint8_t val = m6821_read(pia, addr & 0x0F);
		//printf("R: %04X = %c (%02X)\n", addr, val, val);
		//return val;
	}

	if (trace & TRACE_UNK)
		fprintf(stderr, "Unknown read from port %04X\n", addr);
	return 0xFF;
}

void mmio_write_6502(uint16_t addr, uint8_t val)
{
	if (trace & TRACE_IO)
		fprintf(stderr, "write %02x <- %02x\n", addr, val);
	//	else if (addr >= 0x60 && addr <= 0x6F)
	//		via_write(via, addr & 0x0F, val);

	if (addr >= 0xD000 && addr < 0xE000) {
		//printf("W: %04X = %c (%02X)\n", addr, val, val);
		//m6821_write(pia, addr & 0x0F, val);
	}

	else if (addr == 0x00) {
		//printf("trace set to %d\n", val);
		//trace = val;
		if (trace & TRACE_CPU)
			log_6502 = 1;
		else
			log_6502 = 0;
	}
	else if (trace & TRACE_UNK)
		fprintf(stderr, "Unknown write to port %04X of %02X\n", addr, val);
}



void reset_DEV_sel_slots() {
	DEV_SEL_SLOTS[0] = false;
	DEV_SEL_SLOTS[1] = false;
	DEV_SEL_SLOTS[2] = false;
	DEV_SEL_SLOTS[3] = false;
	DEV_SEL_SLOTS[4] = false;
	DEV_SEL_SLOTS[5] = false;
	DEV_SEL_SLOTS[6] = false;
	DEV_SEL_SLOTS[7] = false;
}

void set_dev_sel_slots(uint16_t addr) {

	if ((addr >> 7) & 1) {
		switch ((addr >> 4) & 0b111) {
		case 0:
			DEV_SEL_SLOTS[0] = true;
			if (trace & TRACE_SLOT) fprintf(stderr, "DEV_SEL_SLOTS [0]\n");
			break;
		case 1:
			DEV_SEL_SLOTS[1] = true;
			if (trace & TRACE_SLOT) fprintf(stderr, "DEV_SEL_SLOTS [1]\n");
			break;
		case 2:
			DEV_SEL_SLOTS[2] = true;
			if (trace & TRACE_SLOT) fprintf(stderr, "DEV_SEL_SLOTS [2]\n");
			break;
		case 3:
			DEV_SEL_SLOTS[3] = true;
			if (trace & TRACE_SLOT) fprintf(stderr, "DEV_SEL_SLOTS [3]\n");
			break;
		case 4:
			DEV_SEL_SLOTS[4] = true;
			if (trace & TRACE_SLOT) fprintf(stderr, "DEV_SEL_SLOTS [4]\n");
			break;
		case 5:
			DEV_SEL_SLOTS[5] = true;
			if (trace & TRACE_SLOT) fprintf(stderr, "DEV_SEL_SLOTS [5]\n");
			break;
		case 6:
			DEV_SEL_SLOTS[6] = true;
			//board_prodos_read(addr, IO_SEL_SLOTS[drive_slot],DEV_SEL_SLOTS[drive_slot]);
			if (trace & TRACE_SLOT) fprintf(stderr, "DEV_SEL_SLOTS [6]\n");
			break;
		case 7:
			DEV_SEL_SLOTS[7] = true;
			if (trace & TRACE_SLOT) fprintf(stderr, "DEV_SEL_SLOTS [7]\n");
			break;
		}
	}
}



void reset_io_sel_slots() {
	IO_SEL_SLOTS[0] = false;
	IO_SEL_SLOTS[1] = false;
	IO_SEL_SLOTS[2] = false;
	IO_SEL_SLOTS[3] = false;
	IO_SEL_SLOTS[4] = false;
	IO_SEL_SLOTS[5] = false;
	IO_SEL_SLOTS[6] = false;
	IO_SEL_SLOTS[7] = false;
}




void set_io_sel_slots(uint16_t addr) {

	switch ((addr >> 8) & 0b111) {
	case 0:
		IO_SEL_SLOTS[0] = true;
		set_dev_sel_slots(addr);

		//if (addr == 0xc058 || addr == 0xc059)
		//	printf("VIDEX MODE [!] %04X\n", addr);

		//if (trace & TRACE_SLOT) fprintf(stderr, "IO_SEL_SLOTS [%04X]\n", addr);
		break;
	case 1:
		IO_SEL_SLOTS[1] = true;
		if (trace & TRACE_SLOT) fprintf(stderr, "IO_SEL_SLOTS [1]\n");
		break;
	case 2:
		IO_SEL_SLOTS[2] = true;
		if (trace & TRACE_SLOT) fprintf(stderr, "IO_SEL_SLOTS [2]\n");
		break;
	case 3:
		IO_SEL_SLOTS[3] = true;
		if (trace & TRACE_SLOT) fprintf(stderr, "IO_SEL_SLOTS [3]\n");
		break;
	case 4:
		IO_SEL_SLOTS[4] = true;
		if (trace & TRACE_SLOT) fprintf(stderr, "IO_SEL_SLOTS [4]\n");
		break;
	case 5:
		IO_SEL_SLOTS[5] = true;
		if (trace & TRACE_SLOT) fprintf(stderr, "IO_SEL_SLOTS [5]\n");
		break;
	case 6:
		IO_SEL_SLOTS[6] = true;
		if (trace & TRACE_SLOT) fprintf(stderr, "IO_SEL_SLOTS [6]\n");
		break;
	case 7:
		IO_SEL_SLOTS[7] = true;
		if (trace & TRACE_SLOT) fprintf(stderr, "IO_SEL_SLOTS [7]\n");
		break;
	}


}


uint8_t read6502(uint16_t addr)
{
	uint8_t r = 0;

	IOSLOTS = false;
	F12_15 = false;

	reset_io_sel_slots();
	reset_DEV_sel_slots();
	//if (addr >> 8 == iopage)
	/*
	if (addr >= 0xD000 && addr < 0xE000) {
		return mmio_read_6502(addr);
	}

	r = do_6502_read(addr);
	*/

	//if (addr == 0xc058 || addr == 0xc059)
	//	printf("VIDEX MODE [R] %04X\n", addr);

	//INH = saturncard.ROMCE;
	//INH = saturncard.INH(addr, true);

	if (((addr >> 14) & 0b11) == 0b11) {

		switch ((addr >> 11) & 0b111) {
		case 0: //ioslots 0xC000-C7FFF
			F12_15 = true;
			set_io_sel_slots(addr);

			//IOSEL

			// IOSEL (0xC300-C3FFF - slot 3)
			if (card_videx && IO_SEL_SLOTS[videx_slot]) {
				r = videxcard.getRomIoSel(addr & 0x1FF);
				card_videx_mem_on = true;
			}

			//DEVSEL

			// DEVSEL (0xC080-C08F - slot 0)
			if (card_saturn && DEV_SEL_SLOTS[saturn_slot]) saturncard.swap_bank(addr, ram);

			// DEVSEL (0xC0B0-C0BF - slot 3)
			if (card_videx && DEV_SEL_SLOTS[videx_slot]) r = videxcard.getC0SLOTX(addr);

			// IOSEL (0xC700-C7FF - slot 7)
			// DEVSEL (0xC0F0-C0FF - slot 7)
			if (card_prodos && (IO_SEL_SLOTS[drive_slot] || DEV_SEL_SLOTS[drive_slot])) r = prodoscard.read(addr, IO_SEL_SLOTS[drive_slot], DEV_SEL_SLOTS[drive_slot]);

			// IOSEL (0xC000-C0FF - H12-15)
			if (IO_SEL_SLOTS[0] == true && ((addr >> 7) & 1) == 0) {

				if (addr == 0xc063)
					r = 255;

				if ((((addr >> 4) & 0b111) == 0x0) && (lastchar != 0)) {
					r = lastchar;
					//return r;
				}
				else if (((addr >> 4) & 0b111) == 0x1) {
					lastchar = 0;
					//return 0;
				}
				else if (((addr >> 4) & 0b111) == 0x4) {
					fprintf(stderr, "IO_SEL_SLOTS [%04X]\n", addr);
					//return 0;
				}
				else if (((addr >> 4) & 0b111) == 0x5) {

					switch ((addr >> 1) & 0b111) {
					case 0: {
						text_mode = (addr & 0b1);
						break;
					}
					case 1: {
						mix_mode = (addr & 0b1);
						break;
					}
					case 2: {
						video_page2 = (addr & 0b1);
						break;
					}
					case 3: {
						hires_mode = (addr & 0b1);
						break;
					}
					}

					//return 0;
				}
			}
			break;
		case 1:// !I/O_STB 0xC800-CFFFF
			IOSLOTS = true;

			// !I/O_STB (0xC800-CFFF)
			if (card_videx && ((addr >> 9) & 0b11) == 0b11) card_videx_mem_on = false;
			if (card_videx && card_videx_mem_on) {
				if (addr >= 0xC800 && addr <= 0xCFFF) {
					r = videxcard.getSLOTC8XX(addr & 0x7ff);   // 0x0800 at CC00-CDFF in 4 banks of 0x1FF				
					//printf("SLOTC8XX I/O STB [R] %04X = %02X\n", addr, r);
				}
			}

			break;
		case 2: // 0xD000
		case 3: // 0xD800
		case 4: // 0xE000
		case 5: // 0xE800
		case 6: // 0xF000
		case 7: // 0xF800
			break;
		}

		if (addr >= 0xD000 && addr <= 0xFFFF) {
			if (card_saturn) INH = saturncard.INH(addr, true);
			if (INH) {	// READ ROM FROM COMPUTER ROM
				r = rom[addr];
				if (trace & TRACE_ROM)
					fprintf(stderr, "ROM [R] %04X = %02X\n", addr, r);
			}
			else if (card_saturn) {		// READ ROM FROM SATURN MEM
				r = saturncard.mem_read(addr);
				//if (trace & TRACE_ROM)
				//fprintf(stderr, "SATURN RAM [R]: %04X = %02X %c\n", addr, r, r);
			}

		}
	}
	else {
		r = ram[addr];
	}
	return r;
}





uint8_t read6502_debug(uint16_t addr)
{
	uint8_t ret = read6502(addr);
	return ret;
}


void write6502(uint16_t addr, uint8_t val)
{
	reset_io_sel_slots();
	reset_DEV_sel_slots();

	// Write at 0xC058 or 0xC059 
	if (card_videx && (addr == 0xc058 || addr == 0xc059)) {
		//printf("VIDEX MODE[W] %04X = %02X\n", addr, val);
		card_videx_80col = (addr == 0xc059);
	}

	//INH = saturncard.ROMCE;
	//INH = saturncard.INH(addr, false);

	if (((addr >> 14) & 0b11) == 0b11) {// C0XX-FFFF

		switch ((addr >> 11) & 0b111) { // C00X-FFFX
		case 0: //ioslots 0xC000-C7FF
			F12_15 = true;
			set_io_sel_slots(addr);

			// IOSEL 

			// IOSEL (0xC300-C3FFF - slot 3)
			if (card_videx && IO_SEL_SLOTS[videx_slot]) {
				//printf("VIDEX I/O [W] %04X = %02X\n", addr, val);
				card_videx_mem_on = true;
			}

			// DEVSEL 

			// DEVSEL (0xC080-C08F - slot 0)
			if (card_saturn && DEV_SEL_SLOTS[saturn_slot]) saturncard.swap_bank(addr, ram);

			// DEVSEL (0xC0B0-C0BF - slot 3)
			if (card_videx && DEV_SEL_SLOTS[videx_slot])  videxcard.putC0SLOTX(addr, val);

			// IOSEL (0xC700-C7FF - slot 7)
			// DEVSEL (0xC0F0-C0FF - slot 7)
			if (card_prodos && (IO_SEL_SLOTS[drive_slot] || DEV_SEL_SLOTS[drive_slot])) prodoscard.write(addr, val, IO_SEL_SLOTS[drive_slot], DEV_SEL_SLOTS[drive_slot]);

			break;
		case 1:// !I/O_STB 0xC800-CFFFF

			// !I/O_STB (0xC800-CFFF)
			if (card_videx && ((addr >> 9) & 0b11) == 0b11) card_videx_mem_on = false;
			if (card_videx && card_videx_mem_on) {
				if (addr >= 0xC800 && addr <= 0xCDFF) {
					videxcard.putSLOTC8XX(addr - 0xC800, val);   // 0x0800 at CC00-CDFF in 4 banks of 0x1FF				
					//printf("SLOTC8XX I/O STB [W] %04X = %02X\n", addr, val);
				}
			}
			break;
		case 2: // 0xD000
		case 3: // 0xD800
		case 4: // 0xE000
		case 5: // 0xE800
		case 6: // 0xF000
		case 7: // 0xF800
			break;
		}

		if (card_saturn && addr >= 0xD000 && addr <= 0xFFFF) {
			INH = saturncard.INH(addr, false);
			if (!INH) {	// WRITE ROM TO SATURN MEM
				saturncard.mem_write(addr, val);
				//fprintf(stderr, "SATURN RAM [W]: %04X = %02X %c\n", addr, val, val);
			}
		}
	}
	else {
		if (trace & TRACE_RAM) {
			if (addr > 0x0400 && addr < 0x0c00)
				//fprintf(stderr, "RAM [W]: %04X = %02X\n", addr, val);
				fprintf(stderr, "RAM [W]: %04X = %02X %c\n", addr, val, val);
		}

		/*
		if (addr >= 0x400 && addr <= 0x7FF)
			if (addr >= 0x7d5 && addr <= 0x7d9)
				fprintf(stderr, "[V ??] %4x %2x %c %c\n", addr, val, val, val - 0x80);
			else
				fprintf(stderr, "[V] %4x %2x %c %c\n", addr, val, val, val - 0x80);
				*/
				//reset();

			//if (addr >= 0x800 && addr <= 0x9FF)
				//	fprintf(stderr, "%c", val);

		ram[addr] = val;
	}
	//}
}

static void poll_irq_event(void) {
	int_clear(IRQ_PIA);
}

static void irqnotify(void)
{
	if (live_irq)
		irq6502();
}

void draw_text(SDL_Renderer* renderer, bool normal_video, int ka, int x, int y, int bit_y) {

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

#if VIDEO_ROM == 0
	if (normal_video)
		display_bit_ii(renderer, x, y, video_rom[ka], bit_y);
	else // inverse
		display_bit_ii(renderer, x, y, ~video_rom[ka], bit_y);
#elif VIDEO_ROM == 1
	if (normal_video)
		display_bit_iie(renderer, x, y, ~video_rom[ka], bit_y);
	else // inverse
		display_bit_iie(renderer, x, y, video_rom[ka], bit_y);
#endif
}


void draw_gr(SDL_Renderer* renderer, int keymap_code, int x, int y, int bit_y) {
	if (keymap_code == 0)
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	else if (keymap_code == 0x1 || (keymap_code >> 4) == 0x1)
		SDL_SetRenderDrawColor(renderer, 227, 30, 96, 255);
	else if (keymap_code == 0x2 || (keymap_code >> 4) == 0x2)
		SDL_SetRenderDrawColor(renderer, 96, 78, 189, 255);
	else if (keymap_code == 0x3 || (keymap_code >> 4) == 0x3)
		SDL_SetRenderDrawColor(renderer, 255, 68, 253, 255);
	else if (keymap_code == 0x4 || (keymap_code >> 4) == 0x4)
		SDL_SetRenderDrawColor(renderer, 0, 163, 96, 255);
	else if (keymap_code == 0x5 || (keymap_code >> 4) == 0x5)
		SDL_SetRenderDrawColor(renderer, 156, 156, 156, 255);
	else if (keymap_code == 0x6 || (keymap_code >> 4) == 0x6)
		SDL_SetRenderDrawColor(renderer, 20, 207, 253, 255);
	else if (keymap_code == 0x7 || (keymap_code >> 4) == 0x7)
		SDL_SetRenderDrawColor(renderer, 208, 195, 255, 255);
	else if (keymap_code == 0x8 || (keymap_code >> 4) == 0x8)
		SDL_SetRenderDrawColor(renderer, 96, 114, 3, 255);
	else if (keymap_code == 0x9 || (keymap_code >> 4) == 0x9)
		SDL_SetRenderDrawColor(renderer, 255, 106, 60, 255);
	else if (keymap_code == 0xA || (keymap_code >> 4) == 0xA)
		SDL_SetRenderDrawColor(renderer, 156, 156, 156, 255);
	else if (keymap_code == 0xB || (keymap_code >> 4) == 0xB)
		SDL_SetRenderDrawColor(renderer, 255, 160, 208, 255);
	else if (keymap_code == 0xC || (keymap_code >> 4) == 0xC)
		SDL_SetRenderDrawColor(renderer, 20, 245, 60, 255);
	else if (keymap_code == 0xD || (keymap_code >> 4) == 0xD)
		SDL_SetRenderDrawColor(renderer, 208, 221, 141, 255);
	else if (keymap_code == 0xE || (keymap_code >> 4) == 0xE)
		SDL_SetRenderDrawColor(renderer, 114, 255, 208, 255);
	else if (keymap_code == 0xF || (keymap_code >> 4) == 0xF)
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

#if VIDEO_ROM == 0
	if (
		(bit_y < 4 && (keymap_code & 0b1111)) ||
		(bit_y >= 4 && (keymap_code & 0b11110000))
		)
		display_bit_ii(renderer, x, y, 0xff, bit_y);
#elif VIDEO_ROM == 1
	if (
		(bit_y < 4 && (keymap_code & 0b1111)) ||
		(bit_y >= 4 && (keymap_code & 0b11110000))
		)
		display_bit_ii(renderer, x, y, 0x00, bit_y);
#endif

}


void set_hgr_color_ct1(SDL_Renderer* renderer, uint8_t color, uint8_t G) {
	switch (color) {
	case 0: //00 = Black
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		break;
	case 1: //01 = Green or Orange
		if (G == 0)
			SDL_SetRenderDrawColor(renderer, 0, 163, 96, 255);
		else
			SDL_SetRenderDrawColor(renderer, 255, 106, 60, 255);
		break;
	case 2: //10 = Magenta or Blue
		if (G == 0)
			SDL_SetRenderDrawColor(renderer, 255, 68, 253, 255);
		else
			SDL_SetRenderDrawColor(renderer, 20, 207, 253, 255);
		break;
	case 3: //11 = White
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		break;
	}
}


void set_hgr_color_ct2(SDL_Renderer* renderer, uint8_t color, uint8_t G) {
	switch (color) {
	case 0: //00 = Black
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		break;
	case 1: //01 = Magenta or Blue
		if (G == 0)
			SDL_SetRenderDrawColor(renderer, 255, 68, 253, 255);
		else
			SDL_SetRenderDrawColor(renderer, 20, 207, 253, 255);

		break;
	case 2: //10 = Green or Orange
		if (G == 0)
			SDL_SetRenderDrawColor(renderer, 0, 163, 96, 255);
		else
			SDL_SetRenderDrawColor(renderer, 255, 106, 60, 255);
		break;
	case 3: //11 = White
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		break;
	}
}

int init_rom(char* filename) {
	//printf("The filename to load is: %s\n", filename);
	printf("Loading rom: %s\n", filename);
	FILE* f = fopen(filename, "rb");
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
	memcpy(&rom[0x0000], buf, size);
	return 0;
}

int init_videorom(char* video_filename) {

	printf("Loading rom: %s\n", video_filename);
	FILE* f = fopen(video_filename, "rb");
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

	memcpy(&video_rom[0x0000], buf, size);

	return 0;
}


void reset() {

	if (card_videx) videxcard.reset();
	reset6502();
}
int main(int argc, char *argv[])
{
	///////////////////////////////////////////////////////////////////////////
	memset(&ram, 0, 0x10000 * sizeof(uint8_t));
	init_rom(filename);
	///////////////////////////////////////////////////////////////////////////
	init_videorom(video_filename);
	///////////////////////////////////////////////////////////////////////////
	if (card_prodos) prodoscard.init(drive_filename);
	///////////////////////////////////////////////////////////////////////////
	if (card_videx) videxcard.card_videx_init();
	///////////////////////////////////////////////////////////////////////////

	trace = 0;

	if (trace & TRACE_CPU)
		log_6502 = 1;


	init6502();
	reset();
	hookexternal(irqnotify);





	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window = SDL_CreateWindow(
		EMULATOR_WINDOW_TITLE,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		(ZX80_WIDTH * ZX80_WINDOW_MULTIPLIER) + (ZX80_BORDER * 2),
		(ZX80_HEIGHT * ZX80_WINDOW_MULTIPLIER) + (ZX80_BORDER * 2),
		SDL_WINDOW_SHOWN
	);



	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;


	/* This is the wrong way to do it but it's easier for the moment. We
	   should track how much real time has occurred and try to keep cycle
	   matched with that. The scheme here works fine except when the host
	   is loaded though */

	   /* We run 4000000 t-states per second */
	   /* We run 200 cycles per I/O check, do that 100 times then poll the
		  slow stuff and nap for 5ms. */
	while (!done) {



		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			switch (event.type) {


			case SDL_QUIT:
				goto out;
				break;

			case SDL_KEYDOWN: {
				int key = event.key.keysym.sym;

				if ((key & 0xFF) == 0xe1) {
					shift = true;
					break;
				}
				else if ((key & 0xFF) == 0xe0) {
					ctrl = true;
					break;
				}
				else if (key == 0x40000052) {
					ctrl = true;
					key = 'K';
				}
				else if (key == 0x40000051) {
					ctrl = true;
					key = 'J';
				}
				else if (key == 0x4000004f) {
					ctrl = true;
					key = 'U';
				}
				else if (key == 0x40000050) {
					ctrl = true;
					key = 'H';
				}

				int vkey = convert_ascii(key, ctrl, shift);
				if (vkey != -1) {
					//keyboard_down(&keyboard, vkey);
					lastchar = vkey;

				}
			}
							  break;

			case SDL_KEYUP: {
				int key = event.key.keysym.sym;

				if ((key & 0xFF) == 0xe1) {
					shift = false;
					break;
				}
				else if ((key & 0xFF) == 0xe0) {
					ctrl = false;
					break;
				}
				else if (key == 0x40000052) {
					ctrl = false;
					key = 'K';
				}
				else if (key == 0x40000051) {
					ctrl = false;
					key = 'J';
				}
				else if (key == 0x4000004f) {
					ctrl = false;
					key = 'U';
				}
				else if (key == 0x40000050) {
					ctrl = false;
					key = 'H';
				}

				if (ctrl && key == (char)0x7f){

					reset();
				}

				int vkey = convert_ascii(key, ctrl, shift);
				if (vkey != -1) {
					lastchar = 0;
					//keyboard_up(&keyboard, vkey);


				}
			}
							break;
			}
		}





		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);










		int i;
		/* 36400 T states for base RC2014 - varies for others */
		for (i = 0; i < 100; i++) {
			/* FIXME: should check return and keep adjusting */
			exec6502(tstate_steps);

			//via_tick(via, tstate_steps);

		}

		poll_irq_event();

		int y = 0;



		if (card_videx_80col) {


			int mode = 0;
			int bitfactor = 1;
			if (mode == 2)
				bitfactor = 2;

			for (int crow = 0; crow < 25; crow++) {


				for (int ccol = 0; ccol < 80; ccol++) {

					int vkeycode = videxcard.getCSLOTXX(ccol, crow);
					bool has_cursor = videxcard.isCursorPosition(ccol, crow);
					if (vkeycode != 0xa0 && vkeycode != 0x20 && vkeycode != 0x0)
						int a = 1;

					int bit_y = 0;
					int x = ccol;
					y = crow;

					bool blink_clk_high = videxcard.videx_blink_mode && blink > 50 || blink > 100;


					for (int i = 0; i < 9; i += bitfactor) {

						int video_bits = videxcard.getVideoBits(vkeycode, i);

						SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

						if (has_cursor && videxcard.videx_upperSLOT <= i && videxcard.videx_lowerSLOT >= i) {
							if ((videxcard.videx_blink_cursor && blink_clk_high) || !videxcard.videx_blink_mode)
								display_bit_80col(renderer, x, y, ~video_bits, bit_y, mode);
						}
						else
							display_bit_80col(renderer, x, y, video_bits, bit_y, mode);
						bit_y++;
					}
				}
			}
			blink++;
			if (videxcard.videx_blink_mode && blink > 100 || blink > 200)
				blink = 0;

		}
		else {
			if (text_mode) {
				for (int bank = 0; bank < 3; bank++) {

					for (int base = 0; base < 8; base++) {
						int addr_base = (video_page2 ? 0x800 : 0x400) + bank * 0x28 + base * 0x80;

						for (int x = 0; x < 40; x++) {

							unsigned char keymap_code = ram[addr_base + x];

							int keymap_address = keymap_code * 8;

							int bit_y = 0;
							int ka;

							bool blink_clk_high = blink > 150;

							for (ka = keymap_address; ka < keymap_address + 8; ka++) {
								//display_bit(renderer, x, y, video_rom[ka], bit_y);

								char video_bits = video_rom[ka];

								if (text_mode) {
#if VIDEO_ROM ==  0
									bool normal_video = ((blink_clk_high && (video_bits & 0b10000000)) || (keymap_code & 0b10000000));
#elif VIDEO_ROM == 1
									bool normal_video = ((blink_clk_high && (video_bits & 0b01111111)) || (keymap_code & 0b10000000));
#endif

									draw_text(renderer, normal_video, ka, x, y, bit_y);
								}
								bit_y++;
							}
						}
						y++;
					}
				}
			}
			else if (!hires_mode) {
				for (int bank = 0; bank < 3; bank++) {

					for (int base = 0; base < 8; base++) {
						int addr_base = (video_page2 ? 0x800 : 0x400) + bank * 0x28 + base * 0x80;

						for (int x = 0; x < 40; x++) {

							unsigned char keymap_code = ram[addr_base + x];

							int keymap_address = keymap_code * 8;

							int bit_y = 0;
							int ka;

							bool blink_clk_high = blink > 150;

							for (ka = keymap_address; ka < keymap_address + 8; ka++) {
								//display_bit(renderer, x, y, video_rom[ka], bit_y);

								char video_bits = video_rom[ka];


								if (mix_mode && y > 19) {
									bool normal_video = ((blink_clk_high && (video_bits & 0b10000000)) || (keymap_code & 0b10000000));
									draw_text(renderer, normal_video, ka, x, y, bit_y);
								}
								else {
									draw_gr(renderer, keymap_code, x, y, bit_y);
								}

								bit_y++;
							}
						}
						y++;
					}
				}
			}
			else {


				for (int bank = 0; bank < 8; bank++) {
					for (int base = 0; base < 8; base++) {
						int addr_base = (video_page2 ? 0x4000 : 0x2000) + bank * 0x80 + base * 0x400;
						int ka = 0;
						for (int ii = 0; ii < 3; ii++) {
							for (int x = 0; x < 0x27; x = x + 2) {
								ka = addr_base + x + (0x28 * ii);
								uint8_t vb1 = ram[ka];
								uint8_t vb2 = ram[ka + 1];

								uint8_t G1 = (vb1 & 0b10000000) > 0;
								uint8_t G2 = (vb2 & 0b10000000) > 0;

								uint8_t ct1 = (((vb1 >> 6) & 0b1) << 1) | ((vb2 >> 6) & 0b1);

								SDL_Rect r;
								if (vb1)
									vb1 = vb1;
								for (int b = 0; b < 6; b += 2)
								{
									uint8_t color = (vb1 >> b) & 0b11;
									set_hgr_color_ct2(renderer, color, G1);
									if (color != 0)
										color = color;
									r.x = (((x) * 7) + (b)) * ZX80_WINDOW_MULTIPLIER;
									r.y = (y + 64 * ii)* ZX80_WINDOW_MULTIPLIER;
									r.w = 2 * ZX80_WINDOW_MULTIPLIER;
									r.h = ZX80_WINDOW_MULTIPLIER;
									SDL_RenderFillRect(renderer, &r);
								}

								uint8_t color = ((vb1 >> 5) & 0b10) | (vb2 & 0b1);
								set_hgr_color_ct1(renderer, color, G1);
								r.x = (((x) * 7) + (6)) * ZX80_WINDOW_MULTIPLIER;
								r.y = (y + 64 * ii)* ZX80_WINDOW_MULTIPLIER;
								r.w = 1 * ZX80_WINDOW_MULTIPLIER;
								r.h = ZX80_WINDOW_MULTIPLIER;
								SDL_RenderFillRect(renderer, &r);

								set_hgr_color_ct1(renderer, color, G2);
								r.x = (((x) * 7) + (7)) * ZX80_WINDOW_MULTIPLIER;
								r.y = (y + 64 * ii)* ZX80_WINDOW_MULTIPLIER;
								r.w = 1 * ZX80_WINDOW_MULTIPLIER;
								r.h = ZX80_WINDOW_MULTIPLIER;
								SDL_RenderFillRect(renderer, &r);

								for (int b = 1; b < 7; b += 2)
								{
									uint8_t color = (vb2 >> b) & 0b11;
									set_hgr_color_ct2(renderer, color, G2);

									r.x = (((x) * 7) + 7 + (b)) * ZX80_WINDOW_MULTIPLIER;
									r.y = (y + 64 * ii)* ZX80_WINDOW_MULTIPLIER;
									r.w = 2 * ZX80_WINDOW_MULTIPLIER;
									r.h = ZX80_WINDOW_MULTIPLIER;
									SDL_RenderFillRect(renderer, &r);
								}
							}

						}
						y += 1;
					}

					//}
				}


				if (mix_mode) {
					y = 0;
					for (int bank = 0; bank < 3; bank++) {

						for (int base = 0; base < 8; base++) {
							int addr_base = (video_page2 ? 0x800 : 0x400) + bank * 0x28 + base * 0x80;

							for (int x = 0; x < 40; x++) {

								unsigned char keymap_code = ram[addr_base + x];

								int keymap_address = keymap_code * 8;

								int bit_y = 0;
								int ka;

								bool blink_clk_high = blink > 150;

								for (ka = keymap_address; ka < keymap_address + 8; ka++) {
									//display_bit(renderer, x, y, video_rom[ka], bit_y);

									char video_bits = video_rom[ka];

									if (mix_mode && y > 19) {
										bool normal_video = ((blink_clk_high && (video_bits & 0b10000000)) || (keymap_code & 0b10000000));
										draw_text(renderer, normal_video, ka, x, y, bit_y);
									}


									bit_y++;
								}
							}
							y++;
						}
					}
				}
			}
			blink++;
			if (blink > 300)
				blink = 0;

		}
		SDL_RenderPresent(renderer);
	}
out:
	SDL_DestroyWindow(window);

	exit(0);
}