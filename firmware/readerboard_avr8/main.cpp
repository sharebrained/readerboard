/*
 *
 * Copyright 2012 ShareBrained Technology, Inc.
 *
 * This file is part of readerboard.
 *
 * readerboard is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * readerboard is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General
 * Public License along with readerboard. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include "usb.h"
#include "usb_descriptor.h"

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>

void Recv(volatile uint8_t* data, uint8_t count) {
	while (count--) {
		*data++ = UEDATX;
	}
}

int USB_RecvControl(void* d, int len)
{
	usb_wait_for_status_out();
	Recv((uint8_t*)d,len);
	usb_clear_out();
	return len;
}

typedef enum {
	DEVICE_STATE_UNINITIALIZED = 0,
	DEVICE_STATE_INITIALIZE_HARDWARE,
	DEVICE_STATE_FETCH_DATA,
	
	DEVICE_STATE_ERROR = 100,
} device_state_t;

#define DEVICE_STATE_FIRST_ERROR (DEVICE_STATE_ERROR)

static volatile device_state_t device_state = DEVICE_STATE_UNINITIALIZED;

static void configure_clocks() {
	/* PLL input prescaler: divide by 2 (16MHz / 2 = 8MHz)
	 */
	PLLCSR = _BV(PLLP0) | _BV(PLLE);
}

static void configure_power() {
	/* On: Timer/Counter0, Timer/Counter1
	 * Off: SPI
	 */
	PRR0 = _BV(PRSPI);
	
	/* On: USB, USART1
	 * Off: (none)
	 */
	//PRR1 = 0;
}

static void configure_pins() {
	MCUCR = 0;
	
	PORTB = 0;
	DDRB = _BV(0);
	
	PORTC = 0;
	DDRC = _BV(6) | _BV(5) | _BV(4) | _BV(2);
	
	PORTD = 0;
	DDRD = _BV(7) | _BV(6) | _BV(5) | _BV(4) | _BV(3) | _BV(1) | _BV(0);
}

static bool configure_hardware() {
	configure_clocks();
	configure_power();
	configure_pins();
	configure_usb();

	// Configure Timer 3 for ~ 60Hz * 7 (420Hz) interrupt rate.
	TCCR1A = 0;
	TCCR1C = 0;
	TCNT1 = 0;
	OCR1A = 38095;
	TIMSK1 = _BV(OCIE1A);
	TCCR1B = _BV(WGM12) | _BV(CS10);

	sei();

	usb_attach();
	
	return true;
}

void strobe_on(const uint8_t i) {
	switch(i) {
	case 0:
		PORTD |= _BV(0);
		break;
		
	case 1:
		PORTD |= _BV(1);
		break;
		
	case 2:
		PORTD |= _BV(4);
		break;
		
	case 3:
		PORTD |= _BV(5);
		break;
		
	case 4:
		PORTD |= _BV(6);
		break;
		
	case 5:
		PORTD |= _BV(7);
		break;
		
	case 6:
		PORTB |= _BV(0);
		break;
		
	default:
		break;
	}
}

void strobe_off(const uint8_t i) {
	switch(i) {
	case 0:
		PORTD &= ~(_BV(0));
		break;
		
	case 1:
		PORTD &= ~(_BV(1));
		break;
		
	case 2:
		PORTD &= ~(_BV(4));
		break;
		
	case 3:
		PORTD &= ~(_BV(5));
		break;
		
	case 4:
		PORTD &= ~(_BV(6));
		break;
		
	case 5:
		PORTD &= ~(_BV(7));
		break;
		
	case 6:
		PORTB &= ~(_BV(0));
		break;
		
	default:
		break;
	}
}

static const uint8_t sign_width = 120;
static const uint8_t sign_height = 7;

static const uint8_t sign_width_bytes = (sign_width + 7) / 8;

uint8_t data_r[2][sign_height][sign_width_bytes]; /* = {
	{
		{ 0xF8, 0x78, 0xF8, 0x78, 0xFC, 0xF8, 0x78, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0xCC, 0xCC, 0xCC, 0x30, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0xCC, 0xCC, 0xCC, 0x30, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xF8, 0xCC, 0xF8, 0xCC, 0x30, 0xF8, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0xCC, 0xCC, 0xCC, 0x30, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  
		{ 0xCC, 0xCC, 0xCC, 0xCC, 0x30, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  
		{ 0xCC, 0x78, 0xF8, 0x78, 0x30, 0xCC, 0x78, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	},
	{
		{ 0xF8, 0x78, 0xF8, 0x78, 0xFC, 0xF8, 0x78, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0xCC, 0xCC, 0xCC, 0x30, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0xCC, 0xCC, 0xCC, 0x30, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xF8, 0xCC, 0xF8, 0xCC, 0x30, 0xF8, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0xCC, 0xCC, 0xCC, 0x30, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  
		{ 0xCC, 0xCC, 0xCC, 0xCC, 0x30, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  
		{ 0xCC, 0x78, 0xF8, 0x78, 0x30, 0xCC, 0x78, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	},
};
*/
/*
uint8_t data_g[2][sign_height][sign_width_bytes]; = {
	{
		{ 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	},
	{
		{ 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	},
};
*/
PROGMEM const uint8_t character[64][7] = {
	{ 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },  // 32 ' '
	{ 0b11000000, 0b11000000, 0b11000000, 0b11000000, 0b00000000, 0b11000000 },  // 33 '!'
	{ 0b10100000, 0b10100000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },  // 34 '"'
	{ 0b00000000, 0b01010000, 0b11111000, 0b01010000, 0b11111000, 0b01010000 },  // 35 '#'
	{ 0b00100000, 0b11111000, 0b10100000, 0b11111000, 0b00101000, 0b11111000, 0b00100000 },  // 36 '$'
	{ 0b00000000, 0b11001000, 0b11010000, 0b00100000, 0b01011000, 0b10011000 },  // 37 '%'
	{ 0b00000000, 0b00000000, 0b01000000, 0b11100000, 0b01000000, 0b00000000 },  // 38 '+'
	{ 0b10000000, 0b10000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },  // 39 '''
	{ 0b00100000, 0b01000000, 0b10000000, 0b10000000, 0b01000000, 0b00100000 },  // 40 '('
	{ 0b10000000, 0b01000000, 0b00100000, 0b00100000, 0b01000000, 0b10000000 },  // 41 ')'
	{ 0b00000000, 0b10001000, 0b01010000, 0b00100000, 0b01010000, 0b10001000 },  // 42 '*'
	{ 0b00000000, 0b00100000, 0b00100000, 0b11111000, 0b00100000, 0b00100000 },  // 43 '+'
	{ 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b11000000, 0b11000000, 0b01000000 },  // 44 ','
	{ 0b00000000, 0b00000000, 0b11111000, 0b00000000, 0b00000000, 0b00000000 },  // 45 '-'
	{ 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b11000000, 0b11000000 },  // 46 '.'
	{ 0b00001000, 0b00010000, 0b00100000, 0b01000000, 0b10000000, 0b00000000 },  // 47 '/'
	{ 0b11111000, 0b10001000, 0b10001000, 0b10001000, 0b10001000, 0b11111000 },  // 48 '0'
	{ 0b01100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b11110000 },  // 49 '1'
	{ 0b01111000, 0b00001000, 0b11111000, 0b10000000, 0b10000000, 0b11110000 },  // 50 '2'
	{ 0b01111000, 0b00001000, 0b01111000, 0b00001000, 0b00001000, 0b11111000 },  // 51 '3'
	{ 0b10000000, 0b10010000, 0b10010000, 0b11111000, 0b00010000, 0b00010000 },  // 52 '4'
	{ 0b11110000, 0b10000000, 0b11111000, 0b00001000, 0b00001000, 0b11111000 },  // 53 '5'
	{ 0b11110000, 0b10000000, 0b11111000, 0b10001000, 0b10001000, 0b11111000 },  // 54 '6'
	{ 0b11111000, 0b00001000, 0b00010000, 0b00100000, 0b01000000, 0b10000000 },  // 55 '7'
	{ 0b11111000, 0b10001000, 0b11111000, 0b10001000, 0b10001000, 0b11111000 },  // 56 '8'
	{ 0b11111000, 0b10001000, 0b11111000, 0b00001000, 0b00001000, 0b01111000 },  // 57 '9'
	{ 0b00000000, 0b11000000, 0b11000000, 0b00000000, 0b11000000, 0b11000000 },  // 58 ':'
	{ 0b00000000, 0b11000000, 0b11000000, 0b00000000, 0b11000000, 0b11000000, 0b01000000 },  // 59 ';'
	{ 0b00100000, 0b01000000, 0b10000000, 0b01000000, 0b00100000, 0b00000000 },  // 60 '<'
	{ 0b00000000, 0b11111000, 0b00000000, 0b11111000, 0b00000000, 0b00000000 },  // 61 '='
	{ 0b00000000, 0b10000000, 0b01000000, 0b00100000, 0b01000000, 0b10000000 },  // 62 '>'
	{ 0b01100000, 0b10010000, 0b00110000, 0b01100000, 0b00000000, 0b01100000 },  // 63 '?'
	{ 0b01000000, 0b10100000, 0b11100000, 0b00000000, 0b11100000, 0b01000000 },  // 64 '@'
	{ 0b11111000, 0b10011000, 0b11111000, 0b10011000, 0b10011000, 0b10011000 },  // 65 'A'
	{ 0b11110000, 0b10010000, 0b11111000, 0b11001000, 0b11001000, 0b11111000 },  // 66 'B'
	{ 0b11111000, 0b11000000, 0b11000000, 0b11000000, 0b11000000, 0b11111000 },  // 67 'C'
	{ 0b11110000, 0b11001000, 0b11001000, 0b11001000, 0b11001000, 0b11110000 },  // 68 'D'
	{ 0b11111000, 0b10000000, 0b11111000, 0b11000000, 0b11000000, 0b11111000 },  // 69 'E'
	{ 0b11111000, 0b10000000, 0b11111000, 0b11000000, 0b11000000, 0b11000000 },  // 70 'F'
	{ 0b11111000, 0b11011000, 0b11000000, 0b11011000, 0b11001000, 0b11111000 },  // 71 'G'
	{ 0b11001000, 0b11001000, 0b11111000, 0b11001000, 0b11001000, 0b11001000 },  // 72 'H'
	{ 0b11000000, 0b11000000, 0b11000000, 0b11000000, 0b11000000, 0b11000000 },  // 73 'I'
	{ 0b00001000, 0b00001000, 0b00001000, 0b00001000, 0b11001000, 0b11111000 },  // 74 'J'
	{ 0b11001000, 0b11010000, 0b11100000, 0b11100000, 0b11010000, 0b11001000 },  // 75 'K'
	{ 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b11111000, 0b11111000 },  // 76 'L'
	{ 0b11111000, 0b10101000, 0b10101000, 0b10001000, 0b10001000, 0b10001000 },  // 77 'M'
	{ 0b11111000, 0b10001000, 0b11001000, 0b11001000, 0b11001000, 0b11001000 },  // 78 'N'
	{ 0b11111000, 0b10011000, 0b10001000, 0b10001000, 0b10001000, 0b11111000 },  // 79 'O'
	{ 0b11111000, 0b10001000, 0b11111000, 0b11000000, 0b11000000, 0b11000000 },  // 80 'P'
	{ 0b11111000, 0b11001000, 0b11001000, 0b11001000, 0b11010000, 0b11101000 },  // 81 'Q'
	{ 0b11110000, 0b10010000, 0b11111000, 0b11001000, 0b11001000, 0b11001000 },  // 82 'R'
	{ 0b11111000, 0b10000000, 0b11111000, 0b00001000, 0b11001000, 0b11111000 },  // 83 'S'
	{ 0b11111000, 0b00100000, 0b00110000, 0b00110000, 0b00110000, 0b00110000 },  // 84 'T'
	{ 0b10011000, 0b10011000, 0b10011000, 0b10011000, 0b10011000, 0b11111000 },  // 85 'U'
	{ 0b11001000, 0b11001000, 0b11001000, 0b11001000, 0b01010000, 0b00100000 },  // 86 'V'
	{ 0b10001000, 0b10001000, 0b10001000, 0b10101000, 0b10101000, 0b11111000 },  // 87 'W'
	{ 0b10001000, 0b01010000, 0b00100000, 0b00100000, 0b01010000, 0b10001000 },  // 88 'X'
	{ 0b11001000, 0b11001000, 0b11111000, 0b00100000, 0b00100000, 0b00100000 },  // 89 'Y'
	{ 0b11111000, 0b00010000, 0b00100000, 0b01000000, 0b11111000, 0b11111000 },  // 90 'Z'
	{ 0b11100000, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b11100000 },  // 91 '['
	{ 0b00000000, 0b10000000, 0b01000000, 0b00100000, 0b00010000, 0b00001000 },  // 92 '/'
	{ 0b11100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b11000000 },  // 93 '['
	{ 0b01000000, 0b10100000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 },  // 94 '^'
	{ 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b11111000 },  // 95 '_'
};

PROGMEM const uint8_t character_attr[64][3] = {
	{ 0, 0, 4 },  // 32 ' '
	{ 2, 7, 4 },  // 33 '!'
	{ 3, 7, 5 },  // 34 '"'
	{ 5, 7, 7 },  // 35 '#'
	{ 5, 7, 7 },  // 36 '$'
	{ 5, 7, 7 },  // 37 '%'
	{ 3, 7, 5 },  // 38 '+'
	{ 1, 7, 3 },  // 39 '''
	{ 3, 7, 5 },  // 40 '('
	{ 3, 7, 5 },  // 41 ')'
	{ 5, 7, 7 },  // 42 '*'
	{ 5, 7, 7 },  // 43 '+'
	{ 2, 7, 4 },  // 44 ','
	{ 5, 7, 7 },  // 45 '-'
	{ 2, 7, 4 },  // 46 '.'
	{ 5, 7, 7 },  // 47 '/'
	{ 5, 7, 6 },  // 48 '0'
	{ 4, 7, 6 },  // 49 '1'
	{ 5, 7, 6 },  // 50 '2'
	{ 5, 7, 6 },  // 51 '3'
	{ 5, 7, 6 },  // 52 '4'
	{ 5, 7, 6 },  // 53 '5'
	{ 5, 7, 6 },  // 54 '6'
	{ 5, 7, 6 },  // 55 '7'
	{ 5, 7, 6 },  // 56 '8'
	{ 5, 7, 6 },  // 57 '9'
	{ 2, 7, 4 },  // 58 ':'
	{ 2, 7, 4 },  // 59 ';'
	{ 3, 7, 5 },  // 60 '<'
	{ 5, 7, 7 },  // 61 '='
	{ 3, 7, 5 },  // 62 '>'
	{ 4, 7, 6 },  // 63 '?'
	{ 3, 7, 5 },  // 64 '@'
	{ 5, 7, 6 },  // 65 'A'
	{ 5, 7, 6 },  // 66 'B'
	{ 5, 7, 6 },  // 67 'C'
	{ 5, 7, 6 },  // 68 'D'
	{ 5, 7, 6 },  // 69 'E'
	{ 5, 7, 6 },  // 70 'F'
	{ 5, 7, 6 },  // 71 'G'
	{ 5, 7, 6 },  // 72 'H'
	{ 2, 7, 3 },  // 73 'I'
	{ 5, 7, 6 },  // 74 'J'
	{ 5, 7, 6 },  // 75 'K'
	{ 5, 7, 6 },  // 76 'L'
	{ 5, 7, 6 },  // 77 'M'
	{ 5, 7, 6 },  // 78 'N'
	{ 5, 7, 6 },  // 79 'O'
	{ 5, 7, 6 },  // 80 'P'
	{ 5, 7, 6 },  // 81 'Q'
	{ 5, 7, 6 },  // 82 'R'
	{ 5, 7, 6 },  // 83 'S'
	{ 5, 7, 6 },  // 84 'T'
	{ 5, 7, 6 },  // 85 'U'
	{ 5, 7, 6 },  // 86 'V'
	{ 5, 7, 6 },  // 87 'W'
	{ 5, 7, 6 },  // 88 'X'
	{ 5, 7, 6 },  // 89 'Y'
	{ 5, 7, 6 },  // 90 'Z'
	{ 3, 7, 5 },  // 91 '['
	{ 5, 7, 7 },  // 92 '\'
	{ 3, 7, 5 },  // 93 ']'
	{ 3, 7, 5 },  // 94 '^'
	{ 5, 7, 7 },  // 95 '_'
};

void blit(const uint8_t* const source,
	const uint8_t source_width_bytes,
	const uint8_t s_x1, const uint8_t s_y1,
	const uint8_t s_x2, const uint8_t s_y2,
	uint8_t* const target,
	const uint8_t target_width_bytes,
	const uint8_t t_x1, const uint8_t t_y1) {

	uint8_t s_y = s_y1;
	uint8_t t_y = s_y1;
	for(; (s_y<s_y2) && (t_y<sign_height); s_y++, t_y++) {
		uint8_t s_x = s_x1;
		uint8_t t_x = t_x1;
		for(; (s_x<s_x2) && (t_x<sign_width); s_x++, t_x++) {
			const uint8_t* const source_address = &source[(s_x >> 3) + s_y * source_width_bytes];
			const uint8_t pixel_value = (pgm_read_byte(source_address) >> ((s_x & 7) ^ 7)) & 1;

			uint8_t* const target_address = &target[(t_x >> 3) + t_y * target_width_bytes];
			const uint8_t target_mask = 1 << ((t_x & 7) ^ 7);

			if( pixel_value ) {
				*target_address |= target_mask;
			} else {
				*target_address &= ~target_mask;
			}
		}
	}
}

void draw_text(const uint8_t buffer_n, uint8_t x, uint8_t y, const char* message) {
	while( *message != 0 ) {
		char c = *(message++);
		if( (c < 32) || (c > 95) ) {
			c = ' ';
		}
		const uint8_t char_index = c - 32;
		const uint8_t character_width = pgm_read_byte(&character_attr[char_index][0]);
		const uint8_t character_height = pgm_read_byte(&character_attr[char_index][1]);
		blit(
			character[char_index], (character_width + 7) >> 3,
			0, 0, character_width, character_height,
			data_r[buffer_n][0], sign_width_bytes,
			x, y
		);
		const uint8_t character_spacing = pgm_read_byte(&character_attr[char_index][2]);
		x += character_spacing;
	}
}

#define CLOCK_BIT (1 << 2)
#define R_BIT (1 << 5)
#define G_BIT (1 << 6)

volatile uint8_t current_buffer = 0;
/*
#define SEND_BIT(bit_number) \
	__asm__ __volatile__ ( \
		"bst  %[r], %[n]\n\t" \
		"bld  %[value], 6\n\t" \
		"bst  %[g], %[n]\n\t" \
		"bld  %[value], 7\n\t" \
		"out  %[port], %[value]\n\t" \
		"sbi  %[port], 5\n\t" \
		: \
		: [port] "I" (_SFR_IO_ADDR(PORTB)), \
		[value] "r" (port_b), \
		[r] "r" (r), \
		[g] "r" (g), \
		[n] "I" (bit_number) \
		: "r0" \
	)
*/
#define SEND_BIT(bit_number) \
	__asm__ __volatile__ ( \
		"bst  %[r], %[n]\n\t" \
		"bld  %[value], 5\n\t" \
		"out  %[port], %[value]\n\t" \
		"sbi  %[port], 2\n\t" \
		: \
		: [port] "I" (_SFR_IO_ADDR(PORTC)), \
		[value] "r" (port_c), \
		[r] "r" (r), \
		[n] "I" (bit_number) \
		: "r0" \
	)

volatile bool frame_sync;

ISR(TIMER1_COMPA_vect) {
	static uint8_t current_row = 0;

	strobe_off(current_row);
	current_row = current_row + 1;
	if( current_row >= sign_height ) {
		current_row = 0;
	}
	
	const uint8_t* rp = (const uint8_t*)&data_r[current_buffer][current_row];
	
	for(uint8_t col=0; col<sign_width_bytes; col++) {
		const uint8_t r = *(rp++);
		const uint8_t port_c = PORTC & (~CLOCK_BIT);

		SEND_BIT(7);
		SEND_BIT(6);
		SEND_BIT(5);
		SEND_BIT(4);
		SEND_BIT(3);
		SEND_BIT(2);
		SEND_BIT(1);
		SEND_BIT(0);
	}
	
	strobe_on(current_row);
	
	if( current_row == (sign_height - 1) ) {
		frame_sync = true;
	}
}

typedef struct {
	void* state;
	bool (*update_fn)(void* const state);
} animation_t;

animation_t animation = {
	0, 0
};

typedef struct {
	uint8_t frame_count;
	uint8_t frames_per_pixel;
	uint8_t pixels_remaining;
} scroll_h_t;

void scroll_h_init(void* const sv, const uint8_t frames_per_pixel, const uint8_t pixels_remaining) {
	scroll_h_t* const state = (scroll_h_t* const)sv;
	state->frame_count = 0;
	state->frames_per_pixel = frames_per_pixel;
	state->pixels_remaining = pixels_remaining;
}

bool scroll_left_update(void* const sv) {
	scroll_h_t* const state = (scroll_h_t* const)sv;
	const uint_fast8_t buffer = current_buffer;
	if( state->pixels_remaining > 0 ) {
		if( state->frame_count >= state->frames_per_pixel ) {
			state->frame_count = 0;
			state->pixels_remaining -= 1;
			for(uint_fast8_t y=0; y<sign_height; y++) {
				uint8_t* p = &data_r[buffer][y][sign_width_bytes];
				uint8_t r = 0;
				__asm__ __volatile__ ( \
					"clc\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					"ld   %[r], -%a1\n\t" \
					"rol  %[r]\n\t" \
					"st   %a1, %[r]\n\t" \
					: \
					: [r] "r" (r), \
					[p] "e" (p) \
					: "r0" \
				);
			}
		} else {
			state->frame_count += 1;
		}
		return true;
	}

	return false;
}

bool scroll_right_update(void* const sv) {
	scroll_h_t* const state = (scroll_h_t* const)sv;
	const uint_fast8_t buffer = current_buffer;
	if( state->pixels_remaining > 0 ) {
		if( state->frame_count >= state->frames_per_pixel ) {
			state->frame_count = 0;
			state->pixels_remaining -= 1;
			for(uint_fast8_t y=0; y<sign_height; y++) {
				uint8_t* p = &data_r[buffer][y][0];
				uint8_t r = 0;
				__asm__ __volatile__ ( \
					"clc\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					"ld   %[r], %a1\n\t" \
					"ror  %[r]\n\t" \
					"st   %a1+, %[r]\n\t" \
					: \
					: [r] "r" (r), \
					[p] "e" (p) \
					: "r0" \
				);
			}
		} else {
			state->frame_count += 1;
		}
		return true;
	}

	return false;
}

scroll_h_t scroll_h;

/////////////////////////////////////////////////////////////////////////

bool usb_set_line(const usb_setup_t& setup) {
	const uint8_t buffer = setup.bRequest;
	uint8_t length = setup.wLength_L;
	if( buffer < 2 ) {
		usb_wait_for_status_out();
		uint8_t plane = UEDATX;
		if( plane == 0 ) {
			uint8_t row = UEDATX;
			if( row < sign_height ) {
				length -= 2;
				if( length <= sign_width_bytes ) {
					Recv((uint8_t*)&data_r[buffer][row], length);
					usb_clear_out();
					return true;
				}
			}
		}
	}

	return false;
}

bool usb_show_buffer(const uint8_t buffer) {
	if( (buffer == 0) || (buffer == 1) ) {
		current_buffer = buffer;
		return true;
	}

	return false;
}

bool usb_clear_buffer(const uint8_t buffer) {
	if( buffer < 2 ) {
		uint8_t* rp = (uint8_t*)&data_r[buffer];
		//uint8_t* gp = (uint8_t*)&data_g[buffer];
		for(uint8_t i=0; i<sizeof(data_r[buffer]); i++) {
			*(rp++) = 0;
			//*(gp++) = 0;
		}
		return true;
	}

	return false;
}

typedef struct {
	uint8_t x, y;
	char message[32];
} usb_draw_text_t;

bool usb_draw_text(const usb_setup_t& setup) {
	const uint8_t buffer = setup.wValue_L;
	uint8_t length = setup.wLength_L;

	usb_draw_text_t data;
	USB_RecvControl(&data, length);

	if( buffer < 2 ) {
		data.message[length - 2] = 0;
		draw_text(buffer, data.x, data.y, &data.message[0]);
		return true;
	}

	return false;
}

typedef struct {
	uint8_t frames_per_pixel;
	uint8_t pixel_count;
} usb_animate_scroll_h_t;

bool usb_animate_scroll_left(const usb_setup_t& setup) {
	const uint8_t buffer = setup.wValue_L;
	uint8_t length = setup.wLength_L;

	usb_animate_scroll_h_t data;
	USB_RecvControl(&data, length);

	if( buffer < 2 ) {
		if( animation.update_fn == 0 ) {
			scroll_h_init(&scroll_h, data.frames_per_pixel, data.pixel_count);
			animation.state = &scroll_h;
			animation.update_fn = scroll_left_update;
		}
		// TODO: Error if animation in progress?
		return true;
	}

	return false;
}

bool usb_animate_scroll_right(const usb_setup_t& setup) {
	const uint8_t buffer = setup.wValue_L;
	uint8_t length = setup.wLength_L;

	usb_animate_scroll_h_t data;
	USB_RecvControl(&data, length);

	if( buffer < 2 ) {
		if( animation.update_fn == 0 ) {
			scroll_h_init(&scroll_h, data.frames_per_pixel, data.pixel_count);
			animation.state = &scroll_h;
			animation.update_fn = scroll_right_update;
		}
		// TODO: Error if animation in progress?
		return true;
	}

	return false;
}

bool usb_handle_vendor_request(const usb_setup_t& setup) {
	switch( setup.bRequest ) {
	case 0:
	case 1:
		return usb_set_line(setup);

	case 2:
		return usb_show_buffer(setup.wValue_L);

	case 3:
		return usb_clear_buffer(setup.wValue_L);

	case 4:
		return usb_draw_text(setup);

	case 5:
		return usb_animate_scroll_left(setup);

	case 6:
		return usb_animate_scroll_right(setup);

	default:
		return false;
	}
}

void animate() {
	while( frame_sync != true );
	frame_sync = false;

	if( animation.update_fn != 0 ) {
		if( animation.update_fn(animation.state) == false ) {
			animation.update_fn = 0;
			animation.state = 0;
		}
	}
}

int main() {
	while(1) {
		switch(device_state) {
		case DEVICE_STATE_UNINITIALIZED:
			device_state = DEVICE_STATE_INITIALIZE_HARDWARE;
			break;
			
		case DEVICE_STATE_INITIALIZE_HARDWARE:
			if( configure_hardware() ) {
				device_state = DEVICE_STATE_FETCH_DATA;
			} else {
				device_state = DEVICE_STATE_ERROR;
			}
			break;

		case DEVICE_STATE_FETCH_DATA:
			animate();
			break;

		case DEVICE_STATE_ERROR:
		default:
		while(true);
		}
	}
	
	return 0;
}