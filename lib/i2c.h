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
#ifndef I2C_H
#define I2C_H

#include "buffer.h"
#include "types.h"

#define CONFIG_BYTE_DISCON (1<<6)
#define CONFIG_BYTE_400KHZ (1<<0)

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum {
		I2C_SUCCESS,
		I2C_BUFFER_ERROR,
		I2C_NOT_INITIALISED,
		I2C_DEST_BUFFER_NOT_EMPTY
	} I2CStatus;

	I2CStatus i2cInitialise(Buffer *buf, uint16 vid, uint16 pid, uint16 did, uint8 configByte);
	I2CStatus i2cWritePromRecords(Buffer *destination, const Buffer *sourceData, const Buffer *sourceMask);
	I2CStatus i2cReadPromRecords(Buffer *destData, Buffer *destMask, const Buffer *source);
	I2CStatus i2cFinalise(Buffer *buf);

#ifdef __cplusplus
}
#endif

#endif
