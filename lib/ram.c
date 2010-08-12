/* 
 * Copyright (C) 2009 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include "fx2loader.h"
#include "usbwrap.h"
#include "i2c.h"

#ifdef WIN32
#pragma warning(disable : 4996)
#define snprintf sprintf_s 
#endif

extern char fx2ErrorMessage[];

// Write the supplied reader buffer to RAM, using the supplied VID/PID.
//
FX2Status fx2WriteRAM(uint16 vid, uint16 pid, const Buffer *sourceData) {
	FX2Status status;
	UsbDeviceHandle *deviceHandle;
	uint16 address = 0x0000;
	const uint8 *bufPtr = sourceData->data;
	int bytesRemaining = sourceData->length;
	int returnCode;
	char byte = 0x01;
	usbInitialise();
	if ( usbOpenDevice(vid, pid, 1, 0, 0, &deviceHandle) ) {
		snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, "Opening FX2 device failed: %s\n", usbStrError());
		status = FX2_USBERR;
		goto exit;
	}
	usb_clear_halt(deviceHandle, 2);
	returnCode = usb_control_msg(
		deviceHandle,
		(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		0xA0, 0xE600, 0x0000, &byte, 1, 5000
	);
	if ( returnCode != 1 ) {
		snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, "Failed to put the CPU in reset - usb_control_msg() failed returnCode %d: %s\n", returnCode, usb_strerror());
		status = FX2_USBERR;
		goto cleanupUsb;
	}
	while ( bytesRemaining > 4096 ) {
		returnCode = usb_control_msg(
			deviceHandle,
			(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
			0xA0, address, 0x0000, (char*)bufPtr, 4096, 5000
		);
		if ( returnCode != 4096 ) {
			snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, "Failed to write block of 4096 bytes at 0x%04X\n", address);
			status = FX2_USBERR;
			goto cleanupUsb;
		}
		bytesRemaining -= 4096;
		bufPtr += 4096;
		address += 4096;
	}
	returnCode = usb_control_msg(
		deviceHandle,
		(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		0xA0, address, 0x0000, (char*)bufPtr, bytesRemaining, 5000
	);
	if ( returnCode != bytesRemaining ) {
		snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, "Failed to write block of %d bytes at 0x%04X\n", bytesRemaining, address);
		status = FX2_USBERR;
		goto cleanupUsb;
	}
	byte = 0x00;
	returnCode = usb_control_msg(
		deviceHandle,
		(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		0xA0, 0xE600, 0x0000, &byte, 1, 5000
	);
	if ( returnCode != 1 ) {
		snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, "Failed to bring the CPU out of reset - usb_control_msg() failed returnCode %d: %s\n", returnCode, usb_strerror());
		status = FX2_USBERR;
		goto cleanupUsb;
	}

	printf("Wrote %lu bytes to FX2's RAM\n", sourceData->length);

	status = FX2_SUCCESS;

cleanupUsb:
	usb_release_interface(deviceHandle, 0);
	usb_close(deviceHandle);
exit:
	return status;
}
