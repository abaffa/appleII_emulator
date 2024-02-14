#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Videx Videoterm Card Emulator
// by Augusto Baffa feb 2024
// 
// Based on Apple II Simulator
// by Peter Koch, May 6, 1993 

int videx_slot = 3;
bool card_videx = true;
bool card_videx_mem_on = false;
bool card_videx_80col = false;

class VidexCard {


	/*
	//notes:

	// To turn on/off 80 columns mode:
	// Write at 0xC058 or 0xC059 
	//		card_videx_80col = (addr == 0xc059);
	//
	// To Control the card:
	//
	// IOSEL (0xC300-C3FFF - slot 3)
	// Read 
	//		r = videxcard.getRomIoSel(addr & 0x1FF);
	//		card_videx_mem_on = true;
	// Write
	//		card_videx_mem_on = true;
	//
	// DEVSEL (0xC0B0-C0BF - slot 3)
	// Read
	//		r = videxcard.getC0SLOTX(addr);
	//		
	// Write
	//		videxcard.putC0SLOTX(addr, val);
	//
	// IOSTB (0xC800-0xCFFF)
	// Read
	//		if (((addr >> 9) & 0b11) == 0b11) card_videx_mem_on = false;
	//
	//		if (card_videx_mem_on && addr >= 0xC800 && addr <= 0xCDFF)
	//			r = videxcard.getSLOTC8XX(addr & 0x7ff);
	//
	// Write
	//		if (((addr >> 9) & 0b11) == 0b11) card_videx_mem_on = false;
	//
	//		if (card_videx_mem_on && addr >= 0xC800 && addr <= 0xCDFF) 
	//			videxcard.putSLOTC8XX(addr - 0xC800, val);   
	//
	// To Render:
	//
	//
	//for (int y = 0; y < 25; y++) {
	//	for (int x = 0; x < 80; x++) {
	//
	//		int vkeycode = videxcard.getCSLOTXX(x, y); // get ascii code from memory
	//		bool vinverse = videxcard.isCursorPosition(x, y); // check cursor position to inverse
	//
	//		for (int i = 0; i < 9; i++) {
	//
	//			int video_bits = videxcard.getVideoBits(vkeycode, i); // get bits for the current char/scanline
	//
	//			if (vinverse)
	//				display_bit_80col(renderer, x, y, ~video_bits, i); // render char/scanline
	//			else
	//				display_bit_80col(renderer, x, y, video_bits, i); // render char/scanline
	//
	//		}
	//	}
	//}
	*/

private:
	uint8_t videx_rom[0x400]; // 0x0400 at C800-CBFF 
	uint8_t videx_chars[0xfff]; // 0x0-0x7ff rom 1 | 0x800-0xfff rom2 character rom (12x8) Matrix)  
	uint8_t videx_vram[2048]; // 0x0800 at CC00-CDFF in 4 banks of 0x1FF


	// internal variables 
	int videx_bankSLOT, videx_regvalSLOT;  // active memory bank, selected register 

	unsigned char videx_regSLOT[17];// registers of the CRT-controller 
	//int videx_oldcursorSLOT;	  // old cursor position 
	//int videx_upperSLOT, videx_lowerSLOT;  // cursor size 

	//*******************************************************
	// Registers
	//    register r/w     normal value    Name              
	//    00:      w       7B              Horiz. total      
	//    01:      w       50              Horiz. displayed  
	//    02:      w       62              Horiz. sync pos   
	//    03:      w       29              Horiz. sync width 
	//    04:      w       1B              Vert. total       
	//    05:      w       08              Vert. adjust      
	//    06:      w       18              Vert. displayed   
	//    07:      w       19              Vert. sync pos    
	//    08:      w       00              Interlaced        
	//    09:      w       08              Max. scan line    
	//    10:0A    w       C0              Cursor upper      
	//    11:0B    w       08              Cursor lower      
	//    12:0C    w       00              Startpos Hi       
	//    13:0D    w       00              Startpos Lo       
	//    14:0E    r/w     00              Cursor Hi         
	//    15:0F    r/w     00              Cursor Lo         
	//    16:10    r       00              Lightpen Hi       
	//    17:11    r       00              Lightpen Lo       
	//
	// The registers are addressed as follows:
	//    To write                  To read
	//    LDA #$<register>          LDA #$<register>         
	//    STA $C0B0                 STA $C0B0                
	//    LDA #$<value>             LDA $C0B1                
	//    STA $C0B1                                          
	//*******************************************************
	
public:
	int videx_upperSLOT, videx_lowerSLOT;  // cursor size 
	int videx_blink_mode, videx_blink_cursor;

	void reset() {

		card_videx_mem_on = false;
		card_videx_80col = false;
	}

	int getC0SLOTX(int adr)// 0x0400 at C800-CBFF 
	{
		int value = 0x00;

		//define current memory bank
		videx_bankSLOT = (adr & 0x000c) >> 2;

		if (adr & 0x0001)
			// get current register value
			value = videx_regSLOT[videx_regvalSLOT];
		else 
			// define current register
			videx_regvalSLOT = value;

		//printf("VIDEX DEV [R] %04X = %02X\n", adr, value);
		return value;
	}


	void putC0SLOTX(int adr, int value)// 0x0400 at C800-CBFF 
	{

		//define current memory bank
		videx_bankSLOT = (adr & 0x000c) >> 2;

		if (adr & 0x0001)
		{
			// set current register value
			videx_regSLOT[videx_regvalSLOT] = value;

			if ((videx_regvalSLOT == 10) || (videx_regvalSLOT == 11))
				modifySLOT();
			/*
			//10:0A Cursor upper //11:0B Cursor lower
			if ((videx_regvalSLOT == 10) || (videx_regvalSLOT == 11)) 
				modifySLOT();

			//14:0E Cursor Hi //15:0F Cursor Lo
			if ((videx_regvalSLOT == 14) || (videx_regvalSLOT == 15)) 
				cursorSLOT();

			//13:0D Startpos Lo
			if (videx_regvalSLOT == 13) 
				redrawSLOT();
			
			//12:0C Startpos Hi
			*/
			
		}
		else
			// define current register
			videx_regvalSLOT = value;

		//printf("VIDEX DEV [W] %04X = %02X\n", adr, value);
	}
	
	// IOSEL READS ROM
	// when reading using iosel, adds bit at A9
	uint8_t getRomIoSel(int addr) {
		uint8_t value = 0;

		addr |= 0b1000000000;
		if (addr < 0x0400)  value = videx_rom[addr & 0x03ff];
		
		//printf("VIDEX I/O [R] %04X = %02X\n", addr, value);
		return value;
	}

	// Read Video Memory using position x, y
	int8_t getCSLOTXX(int col, int row) {
		int vstart = ((videx_regSLOT[12] << 8) | videx_regSLOT[13]);
		int vcursor = ((videx_regSLOT[14] << 8) | videx_regSLOT[15]);

		int vaddr = (((row * 80) + col) + vstart) % 0x800;
		
		return videx_vram[vaddr];
	}
	
	// Read Character Rom (Char/scanline)
	int8_t getVideoBits(int8_t keycode, int line) {

		return videx_chars[keycode * 16 + line];
	}

	// Is cursor at defined x, y?
	bool isCursorPosition(int col, int row) {
		int vstart = ((videx_regSLOT[12] << 8) | videx_regSLOT[13]);
		int vcursor = ((videx_regSLOT[14] << 8) | videx_regSLOT[15]);
		return vcursor == (((row * 80) + col) + vstart);
	}

	// IOSTB - Get Rom or VRam 
	int getSLOTC8XX(int adr)  
	{
		if (adr < 0x0400) // Rom is at 0x0-0x3ff
			return(videx_rom[adr & 0x03ff]);
		else
			// VRAM is at 0x600-0x7FF
			// VRAM has 0x0800 at CC00-CDFF divided in 4 banks of 0x1FF
			if (adr < 0x0600) 
				return(videx_vram[(adr & 0x01ff) + videx_bankSLOT * 0x0200]);
			else //600 (or CE00-CF00)
				return(0);
	}

	// IOSTB - set VRam 
	void putSLOTC8XX(int adr, int value) 
	{
		// VRAM is at 0x600-0x7FF
		// VRAM has 0x0800 at CC00-CDFF divided in 4 banks of 0x1FF
		if ((adr >= 0x0400) && (adr < 0x0600))
		{
			int vadr;

			vadr = (adr & 0x01ff) + videx_bankSLOT * 0x0200;
			videx_vram[vadr] = value;
			
			//drawSLOT(vadr, value); // ignored 
		}
	}


	void card_videx_init() {

		FILE *g;
		int i;
		int value, bits, crow, ccol;

		// Read videx rom 
		printf("Loading rom: videx_firmware.rom\n");
		g = fopen("videx_firmware.rom", "r");
		if (g == 0)
		{
			fprintf(stderr, "can't find 'videx_firmware.rom'.\n");
			exit(1);
		}
		fread(videx_rom, 1, 0x0400, g);
		fclose(g);

		// Read videx character rom 
		printf("Loading rom: videx_chars.rom\n");
		g = fopen("videx_chars.rom", "r");
		if (g == 0)
		{
			fprintf(stderr, "can't find 'videx_chars.rom'.\n");
			exit(1);
		}
		fread(videx_chars, 1, 0x1000, g);
		fclose(g);


		// initializing registers
		videx_bankSLOT = 0;
		videx_regvalSLOT = 0;
		videx_regSLOT[0] = 0x7b;
		videx_regSLOT[1] = 0x50;
		videx_regSLOT[2] = 0x62;
		videx_regSLOT[3] = 0x29;
		videx_regSLOT[4] = 0x1b;
		videx_regSLOT[5] = 0x08;
		videx_regSLOT[6] = 0x18;
		videx_regSLOT[7] = 0x19;
		videx_regSLOT[8] = 0x0;
		videx_regSLOT[9] = 0x8;
		videx_regSLOT[10] = 0xc0;
		videx_regSLOT[11] = 0x8;
		videx_regSLOT[12] = 0x0;
		videx_regSLOT[13] = 0x0;
		videx_regSLOT[14] = 0x0;
		videx_regSLOT[15] = 0x0;
		videx_regSLOT[16] = 0x0;
		//videx_upperSLOT = 0;
		//videx_lowerSLOT = 8;
		//videx_oldcursorSLOT = 0;

		for (i = 0; i < 2048; i++)
			videx_vram[i] = i & 0xff;

		modifySLOT();
	}



	void modifySLOT()
	{

		videx_blink_cursor = (videx_regSLOT[10] & 0x40) > 0;
		videx_blink_mode = (videx_regSLOT[10] & 0x20) > 0;

		//If videx_blink_cursor is false then
		//videx_blink_mode determines if there is a cursor displayed (false) or not (true).

		//If videx_blink_cursor is true then
		//videx_blink_mode determines blink rate
		// - false = 1/16th field rate blink
		// - true = 1/32th field rate blink

		//glyph start
		videx_upperSLOT = videx_regSLOT[10] & 0xf;
		if (videx_upperSLOT > 11) videx_upperSLOT = 11;

		//glyph end
		videx_lowerSLOT = videx_regSLOT[11] & 0xf;
		if (videx_lowerSLOT > 11) videx_lowerSLOT = 11;

		// if the setting is ridiculous
		if (videx_upperSLOT >= videx_lowerSLOT)
		{
			videx_upperSLOT = 0;
			videx_lowerSLOT = 11;
		}

		// draw new cursor
		//cursorSLOT();
	}

	/*
	void drawSLOT(int adr, int value)
	{
		int sadr, crow, ccol;

		// update memory pixmap
		//XCopyArea(display, charsetSLOT, memorySLOT, gc, 8 * value, 0, 8, 12, adr * 8, 0);

		// update screen
		sadr = (0x800 + adr - 256 * videx_regSLOT[12] - videx_regSLOT[13]) & 0x7ff;
		crow = sadr / 80;
		ccol = sadr % 80;

		//printf("FRAMEBUFFER [W] %d x %d = %02X {%02x}\n", ccol, crow, value, ((crow * 80) + ccol) * 8 );
		//XCopyArea(display, charsetSLOT, wSLOT, gc, 8 * value, 0, 8, 12, ccol * 8, crow * 12);
	}

	void cursorSLOT()
	{
		int sadr, newcursor, crow, ccol;

		// remove old cursor
		drawSLOT(videx_oldcursorSLOT, videx_vram[videx_oldcursorSLOT]);

		// update cursor
		newcursor = (256 * videx_regSLOT[14] + videx_regSLOT[15]) & 0x7ff;
		sadr = (0x800 + newcursor - (256 * videx_regSLOT[12] + videx_regSLOT[13])) & 0x7ff;
		crow = sadr / 80;
		ccol = sadr % 80;

		//XFillRectangle(display, wSLOT, xgcSLOT, ccol * 8, crow * 12 + upperSLOT, 7, lowerSLOT - upperSLOT);
		//XFlush(display);

		// remember old cursor
		videx_oldcursorSLOT = newcursor;
	}


	void redrawSLOT()
	{
		int sadr, crow, ccol;

		sadr = (256 * videx_regSLOT[12] + videx_regSLOT[13]) & 0x7ff;
		for (crow = 0; crow < 24; crow++)
		{
			ccol = sadr + 80;
			if (ccol > 0x800)
			{
				ccol &= 0x7ff;
				//printf("%d x %d\n", ccol, crow);
				//XCopyArea(display, memorySLOT, wSLOT, gc, 8 * sadr, 0, 8 * (80 - ccol), 12, 0, crow * 12);
				//XCopyArea(display, memorySLOT, wSLOT, gc, 0, 0, 8 * ccol, 12, 8 * (80 - ccol), crow * 12);
			}
			else {
				//XCopyArea(display, memorySLOT, wSLOT,  gc, 8 * sadr, 0, 8 * 80, 12, 0, crow * 12);

			}
			sadr = ccol;
		}
		cursorSLOT();
	}
	void modifySLOT()
	{
		int val;

		// set upper cursor bound
		videx_upperSLOT = 0;
		val = videx_regSLOT[10] & 0x7f; // remove blinking-flag
		if (val)
			while ((val & 0x40) == 0)
			{
				videx_upperSLOT++;
				val = val << 1;
			}

		// set lower cursor bound
		videx_lowerSLOT = 12;
		val = videx_regSLOT[11];
		if (val)
			while ((val & 0x01) == 0)
			{
				videx_lowerSLOT--;
				val = val >> 1;
			}

		// if the setting is ridiculous
		if (videx_upperSLOT >= videx_lowerSLOT)
		{
			videx_upperSLOT = 0;
			videx_lowerSLOT = 12;
		}

		// draw new cursor
		cursorSLOT();
	}
*/
};