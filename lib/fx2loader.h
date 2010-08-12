/* 
 * Copyright (C) 2009-2010 Chris McClelland
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
#ifndef FX2LOADER_H
#define FX2LOADER_H

#include <stddef.h>
#include "types.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif
	typedef enum {
		FX2_SUCCESS = 0,
		FX2_USBERR,
		FX2_BUFERR
	} FX2Status;
	
	// Defined in error.c:
	const char *fx2StrError(void);

	// Defined in ram.c:
	FX2Status fx2WriteRAM(uint16 vid, uint16 pid, const Buffer *sourceData);

	// Defined in eeprom.c:
	FX2Status fx2WriteEEPROM(uint16 vid, uint16 pid, const Buffer *i2cBuffer);
	FX2Status fx2ReadEEPROM(uint16 vid, uint16 pid, uint32 numBytes, Buffer *i2cBuffer);

	#ifdef FX2LOADER_PRIVATE
		#define FX2_ERR_MAXLENGTH 256
	#endif

#ifdef __cplusplus
}
#endif

#endif
