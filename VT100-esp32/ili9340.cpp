/**
	This file is part of FORTMAX.

	FORTMAX is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	FORTMAX is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FORTMAX.  If not, see <http://www.gnu.org/licenses/>.

	Copyright: Martin K. Schröder (info@fortmax.se) 2014
*/

#include "ili9340.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <limits.h>

// on 1284 reset to chip reset, sdk to 7,  miso not connected
// mosi to 5, dc to 2, CS to 1

#define SPI_DDR DDRB
#define SPI_PORT PORTB
#define SPI_MISO PB6
#define SPI_MOSI PB5
#define SPI_SCK PB7
#define SPI_SS PB4

#define SET_BIT(port, bitMask) *(port) |= (bitMask)
#define CLEAR_BIT(port, bitMask) *(port) &= ~(bitMask)

#define ILI_PORT PORTB
#define ILI_DDR DDRB
#define CS_PIN PB1
#define RST_PIN 0
#define DC_PIN PB2

#define _SB(port, pin) {port |= _BV(pin);}
#define _RB(port, pin) {port &= ~_BV(pin);}
#define CS_HI _SB(ILI_PORT, CS_PIN)
#define CS_LO _RB(ILI_PORT, CS_PIN)
#define RST_HI _SB(ILI_PORT, RST_PIN)
#define RST_LO _RB(ILI_PORT, RST_PIN)
#define DC_HI _SB(ILI_PORT, DC_PIN)
#define DC_LO _RB(ILI_PORT, DC_PIN)

static const unsigned char font[] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x00,
0x3E, 0x5B, 0x4F, 0x5B, 0x3E,
0x3E, 0x6B, 0x4F, 0x6B, 0x3E,
0x1C, 0x3E, 0x7C, 0x3E, 0x1C,
0x18, 0x3C, 0x7E, 0x3C, 0x18,
0x1C, 0x57, 0x7D, 0x57, 0x1C,
0x1C, 0x5E, 0x7F, 0x5E, 0x1C,
0x00, 0x18, 0x3C, 0x18, 0x00,
0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
0x00, 0x18, 0x24, 0x18, 0x00,
0xFF, 0xE7, 0xDB, 0xE7, 0xFF,
0x30, 0x48, 0x3A, 0x06, 0x0E,
0x26, 0x29, 0x79, 0x29, 0x26,
0x40, 0x7F, 0x05, 0x05, 0x07,
0x40, 0x7F, 0x05, 0x25, 0x3F,
0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
0x7F, 0x3E, 0x1C, 0x1C, 0x08,
0x08, 0x1C, 0x1C, 0x3E, 0x7F,
0x14, 0x22, 0x7F, 0x22, 0x14,
0x5F, 0x5F, 0x00, 0x5F, 0x5F,
0x06, 0x09, 0x7F, 0x01, 0x7F,
0x00, 0x66, 0x89, 0x95, 0x6A,
0x60, 0x60, 0x60, 0x60, 0x60,
0x94, 0xA2, 0xFF, 0xA2, 0x94,
0x08, 0x04, 0x7E, 0x04, 0x08,
0x10, 0x20, 0x7E, 0x20, 0x10,
0x08, 0x08, 0x2A, 0x1C, 0x08,
0x08, 0x1C, 0x2A, 0x08, 0x08,
0x1E, 0x10, 0x10, 0x10, 0x10,
0x0C, 0x1E, 0x0C, 0x1E, 0x0C,
0x30, 0x38, 0x3E, 0x38, 0x30,
0x06, 0x0E, 0x3E, 0x0E, 0x06,
0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x5F, 0x00, 0x00,
0x00, 0x07, 0x00, 0x07, 0x00,
0x14, 0x7F, 0x14, 0x7F, 0x14,
0x24, 0x2A, 0x7F, 0x2A, 0x12,
0x23, 0x13, 0x08, 0x64, 0x62,
0x36, 0x49, 0x56, 0x20, 0x50,
0x00, 0x08, 0x07, 0x03, 0x00,
0x00, 0x1C, 0x22, 0x41, 0x00,
0x00, 0x41, 0x22, 0x1C, 0x00,
0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
0x08, 0x08, 0x3E, 0x08, 0x08,
0x00, 0x80, 0x70, 0x30, 0x00,
0x08, 0x08, 0x08, 0x08, 0x08,
0x00, 0x00, 0x60, 0x60, 0x00,
0x20, 0x10, 0x08, 0x04, 0x02,
0x3E, 0x51, 0x49, 0x45, 0x3E,
0x00, 0x42, 0x7F, 0x40, 0x00,
0x72, 0x49, 0x49, 0x49, 0x46,
0x21, 0x41, 0x49, 0x4D, 0x33,
0x18, 0x14, 0x12, 0x7F, 0x10,
0x27, 0x45, 0x45, 0x45, 0x39,
0x3C, 0x4A, 0x49, 0x49, 0x31,
0x41, 0x21, 0x11, 0x09, 0x07,
0x36, 0x49, 0x49, 0x49, 0x36,
0x46, 0x49, 0x49, 0x29, 0x1E,
0x00, 0x00, 0x14, 0x00, 0x00,
0x00, 0x40, 0x34, 0x00, 0x00,
0x00, 0x08, 0x14, 0x22, 0x41,
0x14, 0x14, 0x14, 0x14, 0x14,
0x00, 0x41, 0x22, 0x14, 0x08,
0x02, 0x01, 0x59, 0x09, 0x06,
0x3E, 0x41, 0x5D, 0x59, 0x4E,
0x7C, 0x12, 0x11, 0x12, 0x7C,
0x7F, 0x49, 0x49, 0x49, 0x36,
0x3E, 0x41, 0x41, 0x41, 0x22,
0x7F, 0x41, 0x41, 0x41, 0x3E,
0x7F, 0x49, 0x49, 0x49, 0x41,
0x7F, 0x09, 0x09, 0x09, 0x01,
0x3E, 0x41, 0x41, 0x51, 0x73,
0x7F, 0x08, 0x08, 0x08, 0x7F,
0x00, 0x41, 0x7F, 0x41, 0x00,
0x20, 0x40, 0x41, 0x3F, 0x01,
0x7F, 0x08, 0x14, 0x22, 0x41,
0x7F, 0x40, 0x40, 0x40, 0x40,
0x7F, 0x02, 0x1C, 0x02, 0x7F,
0x7F, 0x04, 0x08, 0x10, 0x7F,
0x3E, 0x41, 0x41, 0x41, 0x3E,
0x7F, 0x09, 0x09, 0x09, 0x06,
0x3E, 0x41, 0x51, 0x21, 0x5E,
0x7F, 0x09, 0x19, 0x29, 0x46,
0x26, 0x49, 0x49, 0x49, 0x32,
0x03, 0x01, 0x7F, 0x01, 0x03,
0x3F, 0x40, 0x40, 0x40, 0x3F,
0x1F, 0x20, 0x40, 0x20, 0x1F,
0x3F, 0x40, 0x38, 0x40, 0x3F,
0x63, 0x14, 0x08, 0x14, 0x63,
0x03, 0x04, 0x78, 0x04, 0x03,
0x61, 0x59, 0x49, 0x4D, 0x43,
0x00, 0x7F, 0x41, 0x41, 0x41,
0x02, 0x04, 0x08, 0x10, 0x20,
0x00, 0x41, 0x41, 0x41, 0x7F,
0x04, 0x02, 0x01, 0x02, 0x04,
0x40, 0x40, 0x40, 0x40, 0x40,
0x00, 0x03, 0x07, 0x08, 0x00,
0x20, 0x54, 0x54, 0x78, 0x40,
0x7F, 0x28, 0x44, 0x44, 0x38,
0x38, 0x44, 0x44, 0x44, 0x28,
0x38, 0x44, 0x44, 0x28, 0x7F,
0x38, 0x54, 0x54, 0x54, 0x18,
0x00, 0x08, 0x7E, 0x09, 0x02,
0x18, 0xA4, 0xA4, 0x9C, 0x78,
0x7F, 0x08, 0x04, 0x04, 0x78,
0x00, 0x44, 0x7D, 0x40, 0x00,
0x20, 0x40, 0x40, 0x3D, 0x00,
0x7F, 0x10, 0x28, 0x44, 0x00,
0x00, 0x41, 0x7F, 0x40, 0x00,
0x7C, 0x04, 0x78, 0x04, 0x78,
0x7C, 0x08, 0x04, 0x04, 0x78,
0x38, 0x44, 0x44, 0x44, 0x38,
0xFC, 0x18, 0x24, 0x24, 0x18,
0x18, 0x24, 0x24, 0x18, 0xFC,
0x7C, 0x08, 0x04, 0x04, 0x08,
0x48, 0x54, 0x54, 0x54, 0x24,
0x04, 0x04, 0x3F, 0x44, 0x24,
0x3C, 0x40, 0x40, 0x20, 0x7C,
0x1C, 0x20, 0x40, 0x20, 0x1C,
0x3C, 0x40, 0x30, 0x40, 0x3C,
0x44, 0x28, 0x10, 0x28, 0x44,
0x4C, 0x90, 0x90, 0x90, 0x7C,
0x44, 0x64, 0x54, 0x4C, 0x44,
0x00, 0x08, 0x36, 0x41, 0x00,
0x00, 0x00, 0x77, 0x00, 0x00,
0x00, 0x41, 0x36, 0x08, 0x00,
0x02, 0x01, 0x02, 0x04, 0x02,
0x3C, 0x26, 0x23, 0x26, 0x3C,
0x1E, 0xA1, 0xA1, 0x61, 0x12,
0x3A, 0x40, 0x40, 0x20, 0x7A,
0x38, 0x54, 0x54, 0x55, 0x59,
0x21, 0x55, 0x55, 0x79, 0x41,
0x22, 0x54, 0x54, 0x78, 0x42, // a-umlaut
0x21, 0x55, 0x54, 0x78, 0x40,
0x20, 0x54, 0x55, 0x79, 0x40,
0x0C, 0x1E, 0x52, 0x72, 0x12,
0x39, 0x55, 0x55, 0x55, 0x59,
0x39, 0x54, 0x54, 0x54, 0x59,
0x39, 0x55, 0x54, 0x54, 0x58,
0x00, 0x00, 0x45, 0x7C, 0x41,
0x00, 0x02, 0x45, 0x7D, 0x42,
0x00, 0x01, 0x45, 0x7C, 0x40,
0x7D, 0x12, 0x11, 0x12, 0x7D, // A-umlaut
0xF0, 0x28, 0x25, 0x28, 0xF0,
0x7C, 0x54, 0x55, 0x45, 0x00,
0x20, 0x54, 0x54, 0x7C, 0x54,
0x7C, 0x0A, 0x09, 0x7F, 0x49,
0x32, 0x49, 0x49, 0x49, 0x32,
0x3A, 0x44, 0x44, 0x44, 0x3A, // o-umlaut
0x32, 0x4A, 0x48, 0x48, 0x30,
0x3A, 0x41, 0x41, 0x21, 0x7A,
0x3A, 0x42, 0x40, 0x20, 0x78,
0x00, 0x9D, 0xA0, 0xA0, 0x7D,
0x3D, 0x42, 0x42, 0x42, 0x3D, // O-umlaut
0x3D, 0x40, 0x40, 0x40, 0x3D,
0x3C, 0x24, 0xFF, 0x24, 0x24,
0x48, 0x7E, 0x49, 0x43, 0x66,
0x2B, 0x2F, 0xFC, 0x2F, 0x2B,
0xFF, 0x09, 0x29, 0xF6, 0x20,
0xC0, 0x88, 0x7E, 0x09, 0x03,
0x20, 0x54, 0x54, 0x79, 0x41,
0x00, 0x00, 0x44, 0x7D, 0x41,
0x30, 0x48, 0x48, 0x4A, 0x32,
0x38, 0x40, 0x40, 0x22, 0x7A,
0x00, 0x7A, 0x0A, 0x0A, 0x72,
0x7D, 0x0D, 0x19, 0x31, 0x7D,
0x26, 0x29, 0x29, 0x2F, 0x28,
0x26, 0x29, 0x29, 0x29, 0x26,
0x30, 0x48, 0x4D, 0x40, 0x20,
0x38, 0x08, 0x08, 0x08, 0x08,
0x08, 0x08, 0x08, 0x08, 0x38,
0x2F, 0x10, 0xC8, 0xAC, 0xBA,
0x2F, 0x10, 0x28, 0x34, 0xFA,
0x00, 0x00, 0x7B, 0x00, 0x00,
0x08, 0x14, 0x2A, 0x14, 0x22,
0x22, 0x14, 0x2A, 0x14, 0x08,
0xAA, 0x00, 0x55, 0x00, 0xAA,
0xAA, 0x55, 0xAA, 0x55, 0xAA,
0x00, 0x00, 0x00, 0xFF, 0x00,
0x10, 0x10, 0x10, 0xFF, 0x00,
0x14, 0x14, 0x14, 0xFF, 0x00,
0x10, 0x10, 0xFF, 0x00, 0xFF,
0x10, 0x10, 0xF0, 0x10, 0xF0,
0x14, 0x14, 0x14, 0xFC, 0x00,
0x14, 0x14, 0xF7, 0x00, 0xFF,
0x00, 0x00, 0xFF, 0x00, 0xFF,
0x14, 0x14, 0xF4, 0x04, 0xFC,
0x14, 0x14, 0x17, 0x10, 0x1F,
0x10, 0x10, 0x1F, 0x10, 0x1F,
0x14, 0x14, 0x14, 0x1F, 0x00,
0x10, 0x10, 0x10, 0xF0, 0x00,
0x00, 0x00, 0x00, 0x1F, 0x10,
0x10, 0x10, 0x10, 0x1F, 0x10,
0x10, 0x10, 0x10, 0xF0, 0x10,
0x00, 0x00, 0x00, 0xFF, 0x10,
0x10, 0x10, 0x10, 0x10, 0x10,
0x10, 0x10, 0x10, 0xFF, 0x10,
0x00, 0x00, 0x00, 0xFF, 0x14,
0x00, 0x00, 0xFF, 0x00, 0xFF,
0x00, 0x00, 0x1F, 0x10, 0x17,
0x00, 0x00, 0xFC, 0x04, 0xF4,
0x14, 0x14, 0x17, 0x10, 0x17,
0x14, 0x14, 0xF4, 0x04, 0xF4,
0x00, 0x00, 0xFF, 0x00, 0xF7,
0x14, 0x14, 0x14, 0x14, 0x14,
0x14, 0x14, 0xF7, 0x00, 0xF7,
0x14, 0x14, 0x14, 0x17, 0x14,
0x10, 0x10, 0x1F, 0x10, 0x1F,
0x14, 0x14, 0x14, 0xF4, 0x14,
0x10, 0x10, 0xF0, 0x10, 0xF0,
0x00, 0x00, 0x1F, 0x10, 0x1F,
0x00, 0x00, 0x00, 0x1F, 0x14,
0x00, 0x00, 0x00, 0xFC, 0x14,
0x00, 0x00, 0xF0, 0x10, 0xF0,
0x10, 0x10, 0xFF, 0x10, 0xFF,
0x14, 0x14, 0x14, 0xFF, 0x14,
0x10, 0x10, 0x10, 0x1F, 0x00,
0x00, 0x00, 0x00, 0xF0, 0x10,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
0xFF, 0xFF, 0xFF, 0x00, 0x00,
0x00, 0x00, 0x00, 0xFF, 0xFF,
0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
0x38, 0x44, 0x44, 0x38, 0x44,
0xFC, 0x4A, 0x4A, 0x4A, 0x34, // sharp-s or beta
0x7E, 0x02, 0x02, 0x06, 0x06,
0x02, 0x7E, 0x02, 0x7E, 0x02,
0x63, 0x55, 0x49, 0x41, 0x63,
0x38, 0x44, 0x44, 0x3C, 0x04,
0x40, 0x7E, 0x20, 0x1E, 0x20,
0x06, 0x02, 0x7E, 0x02, 0x02,
0x99, 0xA5, 0xE7, 0xA5, 0x99,
0x1C, 0x2A, 0x49, 0x2A, 0x1C,
0x4C, 0x72, 0x01, 0x72, 0x4C,
0x30, 0x4A, 0x4D, 0x4D, 0x30,
0x30, 0x48, 0x78, 0x48, 0x30,
0xBC, 0x62, 0x5A, 0x46, 0x3D,
0x3E, 0x49, 0x49, 0x49, 0x00,
0x7E, 0x01, 0x01, 0x01, 0x7E,
0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
0x44, 0x44, 0x5F, 0x44, 0x44,
0x40, 0x51, 0x4A, 0x44, 0x40,
0x40, 0x44, 0x4A, 0x51, 0x40,
0x00, 0x00, 0xFF, 0x01, 0x03,
0xE0, 0x80, 0xFF, 0x00, 0x00,
0x08, 0x08, 0x6B, 0x6B, 0x08,
0x36, 0x12, 0x36, 0x24, 0x36,
0x06, 0x0F, 0x09, 0x0F, 0x06,
0x00, 0x00, 0x18, 0x18, 0x00,
0x00, 0x00, 0x10, 0x10, 0x00,
0x30, 0x40, 0xFF, 0x01, 0x01,
0x00, 0x1F, 0x01, 0x01, 0x1E,
0x00, 0x19, 0x1D, 0x17, 0x12,
0x00, 0x3C, 0x3C, 0x3C, 0x3C,
0x00, 0x00, 0x00, 0x00, 0x00
};

#define swap(a,b) {a^=b; b^=a; a^=b;}

//static uint16_t _width = ILI9340_TFTWIDTH, _height  = ILI9340_TFTHEIGHT;

static struct ili9340 {
	uint16_t screen_width, screen_height; 
	int16_t cursor_x, cursor_y;
	int8_t char_width, char_height;
	uint16_t back_color, front_color;
	uint16_t scroll_start; 
} term;


void _spi_init(void) {
    SPI_DDR &= ~((1<<SPI_MISO)); //input
    SPI_DDR |= ((1<<SPI_MOSI) | (1<<SPI_SS) | (1<<SPI_SCK)); //output

		// pullup! 
		SPI_PORT |= (1<<SPI_MISO);
		
    SPCR = ((1<<SPE)|               // SPI Enable
            (0<<SPIE)|              // SPI Interupt Enable
            (0<<DORD)|              // Data Order (0:MSB first / 1:LSB first)
            (1<<MSTR)|              // Master/Slave select
            (0<<SPR1)|(0<<SPR0)|    // SPI Clock Rate
            (0<<CPOL)|              // Clock Polarity (0:SCK low / 1:SCK hi when idle)
            (0<<CPHA));             // Clock Phase (0:leading / 1:trailing edge sampling)

    SPSR = (1<<SPI2X); // Double SPI Speed Bit
}

void _spi_write(uint8_t c) {
	SPDR = c;
	while(!(SPSR & _BV(SPIF)));
}


void _wr_command(uint8_t c) {
	DC_LO;
	CS_LO; 
  //CLEAR_BIT(dcport, dcpinmask);
  //digitalWrite(_dc, LOW);
  //CLEAR_BIT(clkport, clkpinmask);
  //digitalWrite(_sclk, LOW);
  //CLEAR_BIT(csport, cspinmask);
  //digitalWrite(_cs, LOW);

  _spi_write(c);

	CS_HI; 
  //SET_BIT(csport, cspinmask);
  //digitalWrite(_cs, HIGH);
}


void _wr_data(uint8_t c) {
	DC_HI; 
  CS_LO; 
  _spi_write(c);
	CS_HI; 
} 

void _wr_data16(uint16_t c){
	DC_HI; 
  CS_LO; 
  _spi_write(c >> 8);
  _spi_write(c & 0xff); 
	CS_HI;
}
// Rather than a bazillion _wr_command() and _wr_data() calls, screen
// initialization commands and arguments are organized in these tables
// stored in PROGMEM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.
#define DELAY 0x80

void ili9340_init(void) {
	ILI_DDR |= _BV(RST_PIN);
	ILI_DDR |= _BV(DC_PIN);
	ILI_DDR |= _BV(CS_PIN);
	
	RST_LO; 

	_spi_init();
	
  RST_HI; 
  _delay_ms(5); 
  RST_LO; 
  _delay_ms(20);
  RST_HI; 
  _delay_ms(150);

  _wr_command(0xEF);
  _wr_data(0x03);
  _wr_data(0x80);
  _wr_data(0x02);

  _wr_command(0xCF);  
  _wr_data(0x00); 
  _wr_data(0XC1); 
  _wr_data(0X30); 

  _wr_command(0xED);  
  _wr_data(0x64); 
  _wr_data(0x03); 
  _wr_data(0X12); 
  _wr_data(0X81); 
 
  _wr_command(0xE8);  
  _wr_data(0x85); 
  _wr_data(0x00); 
  _wr_data(0x78); 

  _wr_command(0xCB);  
  _wr_data(0x39); 
  _wr_data(0x2C); 
  _wr_data(0x00); 
  _wr_data(0x34); 
  _wr_data(0x02); 
 
  _wr_command(0xF7);  
  _wr_data(0x20); 

  _wr_command(0xEA);  
  _wr_data(0x00); 
  _wr_data(0x00); 
 
  _wr_command(ILI9340_PWCTR1);    //Power control 
  _wr_data(0x23);   //VRH[5:0] 
 
  _wr_command(ILI9340_PWCTR2);    //Power control 
  _wr_data(0x10);   //SAP[2:0];BT[3:0] 
 
  _wr_command(ILI9340_VMCTR1);    //VCM control 
  _wr_data(0x3e); //�Աȶȵ���
  _wr_data(0x28); 
  
  _wr_command(ILI9340_VMCTR2);    //VCM control2 
  _wr_data(0x86);  //--
 
  _wr_command(ILI9340_MADCTL);    // Memory Access Control 
  _wr_data(ILI9340_MADCTL_MX | ILI9340_MADCTL_BGR);

  _wr_command(ILI9340_PIXFMT);    
  _wr_data(0x55); 
  
  _wr_command(ILI9340_FRMCTR1);    
  _wr_data(0x00);  
  _wr_data(0x18); 
 
  _wr_command(ILI9340_DFUNCTR);    // Display Function Control 
  _wr_data(0x08); 
  _wr_data(0x82);
  _wr_data(0x27);  
 
  _wr_command(0xF2);    // 3Gamma Function Disable 
  _wr_data(0x00); 
 
  _wr_command(ILI9340_GAMMASET);    //Gamma curve selected 
  _wr_data(0x01); 
 
  _wr_command(ILI9340_GMCTRP1);    //Set Gamma 
  _wr_data(0x0F); 
  _wr_data(0x31); 
  _wr_data(0x2B); 
  _wr_data(0x0C); 
  _wr_data(0x0E); 
  _wr_data(0x08); 
  _wr_data(0x4E); 
  _wr_data(0xF1); 
  _wr_data(0x37); 
  _wr_data(0x07); 
  _wr_data(0x10); 
  _wr_data(0x03); 
  _wr_data(0x0E); 
  _wr_data(0x09); 
  _wr_data(0x00); 
  
  _wr_command(ILI9340_GMCTRN1);    //Set Gamma 
  _wr_data(0x00); 
  _wr_data(0x0E); 
  _wr_data(0x14); 
  _wr_data(0x03); 
  _wr_data(0x11); 
  _wr_data(0x07); 
  _wr_data(0x31); 
  _wr_data(0xC1); 
  _wr_data(0x48); 
  _wr_data(0x08); 
  _wr_data(0x0F); 
  _wr_data(0x0C); 
  _wr_data(0x31); 
  _wr_data(0x36); 
  _wr_data(0x0F); 

  _wr_command(ILI9340_SLPOUT);    //Exit Sleep 
  _delay_ms(120); 		
  _wr_command(ILI9340_DISPON);    //Display on

  term.screen_width = ILI9340_TFTWIDTH;
  term.screen_height = ILI9340_TFTHEIGHT;
  term.char_height = 8;
  term.char_width = 6;
  term.back_color = 0x0000;
  term.front_color = 0xffff;
  term.cursor_x = term.cursor_y = 0;
  term.scroll_start = 0; 
}

void ili9340_setScrollStart(uint16_t start){
  _wr_command(0x37); // Vertical Scroll definition.
  _wr_data16(start);
  term.scroll_start = start; 
}


void ili9340_setScrollMargins(uint16_t top, uint16_t bottom) {
  // Did not pass in VSA as TFA+VSA=BFA must equal 320
	_wr_command(0x33); // Vertical Scroll definition.
  _wr_data16(top);
  _wr_data16(ili9340_height()-(top+bottom));
  _wr_data16(bottom); 
}

void ili9340_setAddrWindow(int16_t x0, int16_t y0, int16_t x1,
 int16_t y1) {
	/*y0 = (y0 - term.scroll_start);
	y1 = (y1 - term.scroll_start);
	if(y0 < 0) y0 = term.screen_height - y0;
	if(y1 < 0) y1 = term.screen_height - y0; */
	//y0 = (y0 + term.scroll_start) % term.screen_height;
	//y1 = (y1 + term.scroll_start) % term.screen_height;
	
  _wr_command(ILI9340_CASET); // Column addr set
  _wr_data(x0 >> 8);
  _wr_data(x0 & 0xFF);     // XSTART 
  _wr_data(x1 >> 8);
  _wr_data(x1 & 0xFF);     // XEND

  _wr_command(ILI9340_PASET); // Row addr set
  _wr_data(y0>>8);
  _wr_data(y0);     // YSTART
  _wr_data(y1>>8);
  _wr_data(y1);     // YEND

  _wr_command(ILI9340_RAMWR); // write to RAM
}


void ili9340_pushColor(uint16_t color) {
  DC_HI;
  CS_LO; 

  _spi_write(color >> 8);
  _spi_write(color);

	CS_HI; 
}
uint16_t ili9340_width(void){
	return term.screen_width;
}

uint16_t ili9340_height(void){
	return term.screen_height;
}

// PS extracted this from Adafruit and added it in.
void ili9340_drawPixel(int16_t x, int16_t y, uint16_t color) {
  struct ili9340 *t = &term;
  if((x < 0) ||(x >= t->screen_width) || (y < 0) || (y >= t->screen_height)) return;

  ili9340_setAddrWindow(x,y,x+1,y+1);

  //digitalWrite(_dc, HIGH);
 // SET_BIT(dcport, dcpinmask);
  //digitalWrite(_cs, LOW);
 // CLEAR_BIT(csport, cspinmask);

   	DC_HI;
	CS_LO; 
  
  _spi_write(color >> 8);
  _spi_write(color);

  //SET_BIT(csport, cspinmask);
  //digitalWrite(_cs, HIGH);

	CS_HI;
}


// PS extracted this from Adafruit and added it in.
// Bresenham's algorithm - thx wikpedia
void ili9340_drawLine(int16_t x0, int16_t y0,int16_t x1, int16_t y1,uint16_t color) {

	if (y0 == y1) {
		if (x1 > x0) {
			ili9340_drawFastHLine(x0, y0, x1 - x0 + 1, color);
		} else if (x1 < x0) {
			ili9340_drawFastHLine(x1, y0, x0 - x1 + 1, color);
		} else {
			ili9340_drawPixel(x0, y0, color);
		}
		return;
	} else if (x0 == x1) {
		if (y1 > y0) {
			ili9340_drawFastVLine(x0, y0, y1 - y0 + 1, color);
		} else {
			ili9340_drawFastVLine(x0, y1, y0 - y1 + 1, color);
		}
		return;
	}

	uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}
	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	} else {
		ystep = -1;
	}

	int16_t xbegin = x0;
	if (steep) {
		for (; x0<=x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					ili9340_drawFastVLine(y0, xbegin, len + 1, color);
				} else {
					ili9340_drawPixel(y0, x0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) ili9340_drawFastVLine(y0, xbegin, x0 - xbegin, color);
	} else {
		for (; x0<=x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					ili9340_drawFastHLine(xbegin, y0, len + 1, color);
				} else {
					ili9340_drawPixel(x0, y0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) ili9340_drawFastHLine(xbegin, y0, x0 - xbegin, color);
	}
}


// draw a rectangle - added in by PS.
void ili9340_drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color,uint16_t backColor) {
ili9340_fillRect(x+1,y+1,w-2,h-2,backColor);
ili9340_drawFastVLine(x,y,h,color);
ili9340_drawFastVLine(x+(w-1),y,h,color);
ili9340_drawFastHLine(x+1,y,w-2,color);
ili9340_drawFastHLine(x+1,y+(h-1),w-2,color);  
}

// fill a rectangle
void ili9340_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
	struct ili9340 *t = &term;

	//y = (y + term.scroll_start) % term.screen_height;
	
  // rudimentary clipping (drawChar w/big text requires this)
  //if((x >= t->screen_width) || (y >= t->screen_height)) return;
  if((x + w - 1) >= t->screen_width)  w = t->screen_width  - x;
  if((y + h - 1) >= t->screen_height) h = t->screen_height - y;

  ili9340_setAddrWindow(x, y, x+w-1, y+h-1);

  uint8_t hi = color >> 8, lo = color;

  DC_HI; 
  CS_LO; 
  
  for(y=h; y>0; y--) {
    for(x=w; x>0; x--) {
      _spi_write(hi);
      _spi_write(lo);
    }
  }
  CS_HI; 
}

void ili9340_setBackColor(uint16_t col){
	//uint8_t r, uint8_t g, uint8_t b
	struct ili9340 *t = &term;
	t->back_color = col; 
	//t->back_color = (uint16_t)r << 8 | (uint16_t)g << 4 | b; 
}

void ili9340_setFrontColor(uint16_t col){
	struct ili9340 *t = &term;
	t->front_color = col; 
	//t->front_color = (uint16_t)r << 8 | (uint16_t)g << 4 | b; 
}

void ili9340_drawChar(uint16_t x, uint16_t y, uint8_t ch){
	struct ili9340 *t = &term;
	
	ili9340_setAddrWindow(x, y, x+t->char_width-1, y + t->char_height);

	DC_HI;
	CS_LO;

	// character glyph buffer
	char _buf[5]; 
	for(int j = 0; j < 5; j++){
		_buf[j] = pgm_read_byte(&font[ch * 5 + j]);
	}
	for(int b = 0; b < 8; b++){
		// draw 5 pixels for each column of the glyph
		for(int j = 0; j < 5; j++){
			uint16_t pix = t->back_color;
			if(_buf[j] & _BV(b))
				pix = t->front_color;
			_spi_write(pix >> 8);
			_spi_write(pix);
		}
		
		// draw one more separator pixel
		_spi_write(t->back_color >> 8);
		_spi_write(t->back_color);
	}
	CS_HI;
}

void ili9340_drawString(uint16_t x, uint16_t y, const char *text){
	static char _buffer[128]; // buffer for 1 char
	int len = strlen(text);
	struct ili9340 *t = &term;
	
	for(const char *_ch = text; *_ch; _ch++){
		DDRD |= _BV(5);
		PORTD |= _BV(5); 
		if(!*_ch) break;
		
		ili9340_drawChar(x, y, *_ch);
		x += t->char_width; 
		PORTD &= ~_BV(5); 
	}

}

void ili9340_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
	struct ili9340 *t = &term; 
  // Rudimentary clipping
  if((x >= t->screen_width) || (y >= t->screen_height)) return;

  if((y+h-1) >= t->screen_height) 
    h = t->screen_height-y;

  ili9340_setAddrWindow(x, y, x, y+h-1);

  uint8_t hi = color >> 8, lo = color;

  DC_HI;
  CS_LO; 

  while (h--) {
    _spi_write(hi);
    _spi_write(lo);
  }
  CS_HI; 
}





void ili9340_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
	struct ili9340 *t = &term; 
  // Rudimentary clipping
  
	//y = (y + term.scroll_start) % term.screen_height;
	
  if((x >= t->screen_width) || (y >= t->screen_height)) return;
  if((x+w-1) >= t->screen_width)  w = t->screen_width-x;
  
  ili9340_setAddrWindow(x, y, x+w-1, y);

  uint8_t hi = color >> 8, lo = color;
  DC_HI;
  CS_LO; 
  while (w--) {
    _spi_write(hi);
    _spi_write(lo);
  }
  CS_HI; 
}

void ili9340_setRotation(uint8_t m) {
	struct ili9340 *t = &term; 
  _wr_command(ILI9340_MADCTL);
  int rotation = m % 4; // can't be higher than 3
  switch (rotation) {
   case 0:
     _wr_data(ILI9340_MADCTL_MX | ILI9340_MADCTL_BGR);
     t->screen_width  = ILI9340_TFTWIDTH;
     t->screen_height = ILI9340_TFTHEIGHT;
     break;
   case 1:
     _wr_data(ILI9340_MADCTL_MV | ILI9340_MADCTL_BGR);
     t->screen_width  = ILI9340_TFTHEIGHT;
     t->screen_height = ILI9340_TFTWIDTH;
     break;
  case 2:
    _wr_data(ILI9340_MADCTL_MY | ILI9340_MADCTL_BGR);
     t->screen_width  = ILI9340_TFTWIDTH;
     t->screen_height = ILI9340_TFTHEIGHT;
    break;
   case 3:
     _wr_data(ILI9340_MADCTL_MV | ILI9340_MADCTL_MY | ILI9340_MADCTL_MX | ILI9340_MADCTL_BGR);
     t->screen_width  = ILI9340_TFTHEIGHT;
     t->screen_height = ILI9340_TFTWIDTH;
     break;
  }
}

