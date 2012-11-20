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

#ifndef __USB_DESCRIPTOR_H__
#define __USB_DESCRIPTOR_H__

#include <stdint.h>

#include <avr/pgmspace.h>

#include "usb.h"

#define USB_WORD(x)	(x & 0xFF), ((x >> 8) & 0xFF)

PROGMEM const uint8_t device_descriptor[] = {
	18,		// descriptor length
	USB_DESCRIPTOR_TYPE_DEVICE,
	USB_WORD(0x0200),	// bcdUSB
	0xFF,	// bDeviceClass
	0x00,	// bDeviceSubClass
	0xFF,	// bDeviceProtocol
	64,		// bMaxPacketSize0
	USB_WORD(0x8080),	// idVendor
	USB_WORD(0x6464),	// idProduct
	USB_WORD(0x0100),	// bcdDevice
	0,		// iManufacturer
	0,		// iProduct
	0,		// iSerialNumber
	1		// bNumConfigurations
};

PROGMEM const uint8_t configuration_descriptor[] = {
	9,
	USB_DESCRIPTOR_TYPE_CONFIGURATION,
	18,		// wTotalLength
	0,	
	1,		// bNumInterfaces
	1,		// bConfigurationValue
	0,		// iConfiguration
	0x80,	// bmAttributes
	250,	// bMaxPower

	// INTERFACE 0 descriptor
	9,
	USB_DESCRIPTOR_TYPE_INTERFACE,
	0,		// bInterfaceNumber
	0,		// bAlternateSetting
	0,		// bNumEndpoints
	0xFF,	// bInterfaceClass
	0,		// bInterfaceSubClass
	0xFF,	// bInterfaceProtocol
	0,		// iInterface
	
	/*7,		// bLength
	USB_DESCRIPTOR_TYPE_ENDPOINT,
	0x01,	// bEndpointAddress: OUT, ep #1
	0x02,	// bmAttributes: BULK
	0x10,	// wMaxPacketSize: 16
	0x00,	
	0,		// bInterval: never NAK*/
};

PROGMEM const uint8_t languages_string_descriptor[] = {
	4,
	USB_DESCRIPTOR_TYPE_STRING,
	0x09,
	0x04
};

PROGMEM const uint8_t manufacturer_string_descriptor[] = {
	60,
	USB_DESCRIPTOR_TYPE_STRING,
	'S', 0,
	'h', 0,
	'a', 0,
	'r', 0,
	'e', 0,
	'B', 0,
	'r', 0,
	'a', 0,
	'i', 0,
	'n', 0,
	'e', 0,
	'd', 0,
	' ', 0,
	'T', 0,
	'e', 0,
	'c', 0,
	'h', 0,
	'n', 0,
	'o', 0,
	'l', 0,
	'o', 0,
	'g', 0,
	'y', 0,
	',', 0,
	' ', 0,
	'I', 0,
	'n', 0,
	'c', 0,
	'.', 0
};

PROGMEM const uint8_t product_string_descriptor[] = {
	28,
	USB_DESCRIPTOR_TYPE_STRING,
	'M', 0,
	'o', 0,
	'n', 0,
	'u', 0,
	'l', 0,
	'a', 0,
	't', 0,
	'o', 0,
	'r', 0,
	' ', 0,
	'U', 0,
	'S', 0,
	'B', 0
};

const uint8_t* const string_descriptors[] = {
	languages_string_descriptor,
	manufacturer_string_descriptor,
	product_string_descriptor
};
const uint8_t string_descriptors_count = sizeof(string_descriptors) / sizeof(string_descriptors[0]);

#endif//__USB_DESCRIPTOR_H__
