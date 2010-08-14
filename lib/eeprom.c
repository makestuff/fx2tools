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

#define A2_ERROR "This firmware does not seem to support EEPROM operations - try loading an appropriate firmware into RAM first\nDiagnostic information: failed writing %lu bytes to 0x%04X return code %d: %s\n"
#define BLOCK_SIZE 4096L

// Write the supplied reader buffer to EEPROM, using the supplied VID/PID.
//
FX2Status fx2WriteEEPROM(uint16 vid, uint16 pid, const Buffer *i2cBuffer) {
	FX2Status status;
	UsbDeviceHandle *deviceHandle;
	uint16 address = 0x0000;
	const uint8 *bufPtr;
	uint32 bytesRemaining;
	int returnCode;
	bufPtr = i2cBuffer->data;
	bytesRemaining = i2cBuffer->length;
	usbInitialise();
	if ( usbOpenDevice(vid, pid, 1, 0, 0, &deviceHandle) ) {
		snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, "%s\n", usbStrError());
		status = FX2_USBERR;
		goto cleanup;
	}
	usb_clear_halt(deviceHandle, 2);
	while ( bytesRemaining > BLOCK_SIZE ) {
		returnCode = usb_control_msg(
			deviceHandle,
			(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
			0xA2, address, 0x0000, (char*)bufPtr, BLOCK_SIZE, 5000
		);
		if ( returnCode != BLOCK_SIZE ) {
			snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, A2_ERROR, BLOCK_SIZE, address, returnCode, usb_strerror());
			status = FX2_USBERR;
			goto cleanup;
		}
		bytesRemaining -= BLOCK_SIZE;
		bufPtr += BLOCK_SIZE;
		address += BLOCK_SIZE;
	}
	returnCode = usb_control_msg(
		deviceHandle,
		(USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		0xA2, address, 0x0000, (char*)bufPtr, bytesRemaining, 5000
	);
	if ( returnCode != (int)bytesRemaining ) {
		snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, A2_ERROR, bytesRemaining, address, returnCode, usb_strerror());
		status = FX2_USBERR;
		goto cleanup;
	}

	status = FX2_SUCCESS;

cleanup:
	usb_release_interface(deviceHandle, 0);
	usb_close(deviceHandle);
	return status;
}

// Read from the EEPROM into the supplied buffer, using the supplied VID/PID.
//
FX2Status fx2ReadEEPROM(uint16 vid, uint16 pid, uint32 numBytes, Buffer *i2cBuffer) {
	FX2Status status;
	UsbDeviceHandle *deviceHandle;
	uint16 address = 0x0000;
	uint8 *bufPtr;
	int returnCode;
	if ( bufAppendZeros(i2cBuffer, numBytes, NULL) ) {
		snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, "%s\n", bufStrError());
		status = FX2_BUFERR;
		goto exit;
	}
	bufPtr = i2cBuffer->data;
	usbInitialise();
	if ( usbOpenDevice(vid, pid, 1, 0, 0, &deviceHandle) ) {
		snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, "%s\n", usbStrError());
		status = FX2_USBERR;
		goto exit;
	}
	usb_clear_halt(deviceHandle, 2);
	while ( numBytes > BLOCK_SIZE ) {
		returnCode = usb_control_msg(
			deviceHandle,
			(USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
			0xA2, address, 0x0000, (char*)bufPtr, BLOCK_SIZE, 5000
		);
		if ( returnCode != BLOCK_SIZE ) {
			snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, A2_ERROR, BLOCK_SIZE, address, returnCode, usb_strerror());
			status = FX2_USBERR;
			goto cleanup;
		}
		numBytes -= BLOCK_SIZE;
		bufPtr += BLOCK_SIZE;
		address += BLOCK_SIZE;
	}
	returnCode = usb_control_msg(
		deviceHandle,
		(USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		0xA2, address, 0x0000, (char*)bufPtr, numBytes, 5000
	);
	if ( returnCode != (int)numBytes ) {
		snprintf(fx2ErrorMessage, FX2_ERR_MAXLENGTH, A2_ERROR, numBytes, address, returnCode, usb_strerror());
		status = FX2_USBERR;
		goto cleanup;
	}

	status = FX2_SUCCESS;

cleanup:
	usb_release_interface(deviceHandle, 0);
	usb_close(deviceHandle);
exit:
	return status;
}
