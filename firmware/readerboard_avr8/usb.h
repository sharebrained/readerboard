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

#ifndef __USB_H__
#define __USB_H__

#include <stdint.h>

typedef struct {
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint8_t wValue_L;
	uint8_t wValue_H;
	uint8_t wIndex_L;
	uint8_t wIndex_H;
	uint8_t wLength_L;
	uint8_t wLength_H;
} usb_setup_t;

typedef enum {
	USB_DESCRIPTOR_TYPE_DEVICE = 1,
	USB_DESCRIPTOR_TYPE_CONFIGURATION = 2,
	USB_DESCRIPTOR_TYPE_STRING = 3,
	USB_DESCRIPTOR_TYPE_INTERFACE = 4,
	USB_DESCRIPTOR_TYPE_ENDPOINT = 5,
} usb_descriptor_type_t;

typedef enum {
	USB_REQUEST_TYPE_STANDARD = 0,
	USB_REQUEST_TYPE_CLASS = 1,
	USB_REQUEST_TYPE_VENDOR = 2,
	USB_REQUEST_TYPE_RESERVED = 3,
} usb_request_type_t;
	
typedef enum {
	USB_GET_STATUS = 0,
	USB_CLEAR_FEATURE = 1,
	USB_SET_FEATURE = 3,
	USB_SET_ADDRESS = 5,
	USB_GET_DESCRIPTOR = 6,
	USB_SET_DESCRIPTOR = 7,
	USB_GET_CONFIGURATION = 8,
	USB_SET_CONFIGURATION = 9,
	USB_GET_INTERFACE = 10,
	USB_SET_INTERFACE = 11,
	USB_SYNCH_FRAME = 12,
} usb_standard_request_t;

void configure_usb();
void usb_attach();
//void usb_stall_endpoint();

bool usb_handle_vendor_request(const usb_setup_t& setup);

void usb_clear_out();
void usb_wait_for_status_out();

#endif//__USB_H__
