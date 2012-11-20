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

#include <avr/io.h>
#include <avr/interrupt.h>

void configure_usb() {
	/* Wait for PLL lock */
	while( !(PLLCSR & _BV(PLOCK)) );
	
	/* Enable USB, unfreeze clock */
	/* It seems the peripheral must be enabled before clock is unfrozen. */
	USBCON = _BV(USBE) | _BV(FRZCLK);
	USBCON = _BV(USBE);
	
	UERST = _BV(EPRST4) | _BV(EPRST3) | _BV(EPRST2) | _BV(EPRST1) | _BV(EPRST0);
	
	/* USB interrupts enabled: End of Reset
	 */
	UDIEN = _BV(EORSTE);
}

void usb_attach() {
	/* Attach USB */
	UDCON = 0;
}

static volatile uint8_t usb_configuration = 0;

static bool usb_setup_received() {
	return UEINTX & _BV(RXSTPI);
}

static void usb_clear_setup() {
	UEINTX &= ~(_BV(RXSTPI));
}

static void usb_clear_in() {
	UEINTX &= ~(_BV(TXINI) | _BV(FIFOCON));
}

void usb_clear_out() {
	UEINTX &= ~(_BV(RXOUTI) | _BV(FIFOCON));
}

void usb_wait_for_status_out() {
	while( !(UEINTX & _BV(RXOUTI)) );
}

static void usb_wait_for_in_ready() {
	while( !(UEINTX & _BV(TXINI)) );
}

static void usb_stall_endpoint() {
	UECONX |= _BV(STALLRQ);
}

static void usb_write_descriptor(const uint8_t* descriptor, uint8_t descriptor_length, uint8_t requested_length) {
	for(uint_fast8_t i=0; (i<descriptor_length) && (i<requested_length); i++) {
		UEDATX = pgm_read_byte(descriptor++);
	}
}

static void usb_handle_standard_request(const usb_setup_t& setup) {
	switch(setup.bRequest) {
	case USB_SET_ADDRESS:
		{
			const uint8_t new_address = setup.wValue_L;
			UDADDR = new_address;
			usb_clear_in();
			usb_wait_for_in_ready();
			UDADDR |= _BV(ADDEN);
		}
		break;
	
	case USB_GET_DESCRIPTOR:
		{
			const uint8_t descriptor_index = setup.wValue_L;
			const uint8_t descriptor_type = setup.wValue_H;
			uint16_t length = (setup.wLength_H << 8) | setup.wLength_L;
			
			const uint8_t* descriptor = 0;
			uint8_t descriptor_length = 0;
			
			switch( descriptor_type ) {
			case USB_DESCRIPTOR_TYPE_DEVICE:
				if( descriptor_index == 0 ) {
					descriptor = device_descriptor;
					descriptor_length = sizeof(device_descriptor);
				}
				break;
				
			case USB_DESCRIPTOR_TYPE_CONFIGURATION:
				if( descriptor_index == 0 ) {
					descriptor = configuration_descriptor;
					descriptor_length = sizeof(configuration_descriptor);
				}
				break;
				
			case USB_DESCRIPTOR_TYPE_STRING:
				if( descriptor_index < string_descriptors_count ) {
					descriptor = string_descriptors[descriptor_index];
					descriptor_length = descriptor[0];
				}
				break;
			
			default:
				break;
			}

			if( descriptor ) {
				usb_write_descriptor(descriptor, descriptor_length, length);
				usb_clear_in();
				usb_wait_for_status_out();
				usb_clear_out();
			} else {
				usb_stall_endpoint();
			}
		}
		break;
		
	case USB_GET_CONFIGURATION:
		{
			UEDATX = usb_configuration;
			usb_clear_in();
			usb_wait_for_status_out();
			usb_clear_out();
		}
		break;
		
	case USB_SET_CONFIGURATION:
		{
			const uint8_t new_configuration = setup.wValue_L;
			if( new_configuration > 1 ) {
				usb_stall_endpoint();
			} else {
				if( new_configuration == 0 ) {
					// No configuration, go to ADDRESS state.
					UDADDR = 0;
				}
				usb_configuration = new_configuration;
				usb_clear_in();
				usb_wait_for_in_ready();
			}
		}
		break;
		
	//case USB_SET_DESCRIPTOR:
	//case USB_GET_STATUS:
	//case USB_CLEAR_FEATURE:
	//case USB_SET_FEATURE:
	//case USB_GET_INTERFACE:
	//case USB_SET_INTERFACE:
	//case USB_SYNCH_FRAME:
	
	default:
		usb_stall_endpoint();
		break;
	}
}

ISR(USB_COM_vect) {
	UENUM = 0;
	
	if( usb_setup_received() ) {
		usb_setup_t setup;
		uint8_t* p = (uint8_t*)&setup;
		
		for(uint_fast8_t i=0; i<8; i++) {
			*(p++) = UEDATX;
		}
		
		usb_clear_setup();
		
		const usb_request_type_t request_type = (usb_request_type_t)((setup.bmRequestType >> 5) & 0x3);
		switch( request_type ) {
		case USB_REQUEST_TYPE_STANDARD:
			usb_handle_standard_request(setup);
			break;
		
		case USB_REQUEST_TYPE_VENDOR:
			if( usb_handle_vendor_request(setup) ) {
				usb_clear_in();
			} else {
				usb_stall_endpoint();
			}
			break;
			
		default:
			usb_stall_endpoint();
			break;
		}
	} else {
		// TODO: Report error or unexpected event.
	}
}

ISR(USB_GEN_vect) {
	const uint8_t flags = UDINT;
	UDINT = 0;
	
	if( flags & _BV(EORSTI) ) {
		usb_configuration = 0;
		
		UENUM = 0;
		UECONX = _BV(EPEN);
		UECFG0X = 0;
		UECFG1X = (3 << EPSIZE0) | _BV(ALLOC);
		UEIENX = _BV(RXSTPE);
	}
}
