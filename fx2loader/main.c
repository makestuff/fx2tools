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
#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include "argtable2.h"
#include "arg_uint.h"
#include "i2c.h"
#include "usbwrap.h"
#include "fx2loader.h"
#include "dump.h"

#define VID 0x04b4
#define PID 0x8613

typedef enum {
	SRC_BAD,
	SRC_EEPROM,
	SRC_HEXFILE,
	SRC_BIXFILE,
	SRC_IICFILE
} Source;

typedef enum {
	DST_RAM,
	DST_EEPROM,
	DST_HEXFILE,
	DST_IICFILE,
	DST_BIXFILE
} Destination;

int main(int argc, char *argv[]) {

	struct arg_uint *vidOpt = arg_uint0("v", "vid", "<vendorID>", "  vendor ID");
	struct arg_uint *pidOpt = arg_uint0("p", "pid", "<productID>", " product ID");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "            print this help and exit");
	struct arg_str *srcOpt = arg_str1(NULL, NULL, "<source>", "            where to read from (<eeprom:<kbitSize> | fileName.hex | fileName.bix | fileName.iic>)");
	struct arg_str *dstOpt = arg_str0(NULL, NULL, "<destination>", "         where to write to (<ram | eeprom | fileName.hex | fileName.bix | fileName.iic> - defaults to \"ram\")");
	struct arg_end *endOpt   = arg_end(20);
	void* argTable[] = {vidOpt, pidOpt, helpOpt, srcOpt, dstOpt, endOpt};
	const char *progName = "fx2loader";
	uint32 exitCode = 0;
	int numErrors;

	Source src = SRC_BAD;
	Destination dst;
	Buffer sourceData = {0};
	Buffer sourceMask = {0};
	Buffer i2cBuffer = {0};
	uint16 vid, pid;
	const char *srcExt, *dstExt;
	int eepromSize = 0;

	// Parse arguments...
	//
	if ( arg_nullcheck(argTable) != 0 ) {
		printf("%s: insufficient memory\n", progName);
		exitCode = 1;
		goto cleanup;
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("FX2Loader Copyright (C) 2009-2010 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nUpload code to the Cypress FX2LP.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		exitCode = 0;
		goto cleanup;
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		printf("Try '%s --help' for more information.\n", progName);
		exitCode = 2;
		goto cleanup;
	}

	srcExt = srcOpt->sval[0] + strlen(srcOpt->sval[0]) - 4;
	if ( !strcmp(".hex", srcExt) || !strcmp(".ihx", srcExt) ) {
		src = SRC_HEXFILE;
	} else if ( !strcmp(".bix", srcExt) ) {
		src = SRC_BIXFILE;
	} else if ( !strcmp(".iic", srcExt) ) {
		src = SRC_IICFILE;
	} else if ( !strncmp("eeprom:", srcOpt->sval[0], 7) ) {
		const char *const eepromSizeString = srcOpt->sval[0] + 7;
		eepromSize = atoi(eepromSizeString) * 128;  // size in bytes
		if ( eepromSize == 0 ) {
			fprintf(stderr, "You need to supply an EEPROM size in kilobytes (e.g -s eeprom:128)\n");
			exitCode = 5;
			goto cleanup;
		}
		src = SRC_EEPROM;
	}

	if ( src == SRC_BAD ) {
		fprintf(stderr, "Unrecognised source: %s\n", srcOpt->sval[0]);
		exitCode = 6;
		goto cleanup;
	}

	if ( dstOpt->count ) {
		dstExt = dstOpt->sval[0] + strlen(dstOpt->sval[0]) - 4;
		if ( !strcmp(".hex", dstExt) || !strcmp(".ihx", dstExt) ) {
			dst = DST_HEXFILE;
		} else if ( !strcmp(".bix", dstExt) ) {
			dst = DST_BIXFILE;
		} else if ( !strcmp(".iic", dstExt) ) {
			dst = DST_IICFILE;
		} else if ( !strcmp("ram", dstOpt->sval[0]) ) {
			dst = DST_RAM;
		} else if ( !strcmp("eeprom", dstOpt->sval[0]) ) {
			dst = DST_EEPROM;
		} else {
			fprintf(stderr, "Unrecognised destination: %s\n", srcOpt->sval[0]);
			exitCode = 9;
			goto cleanup;
		}
	} else {
		dst = DST_RAM;
	}

	vid = vidOpt->count ? (uint16)vidOpt->ival[0] : VID;
	pid = pidOpt->count ? (uint16)pidOpt->ival[0] : PID;

	// Initialise buffers...
	//
	if ( bufInitialise(&sourceData, 1024, 0x00) ) {
		fprintf(stderr, "%s\n", bufStrError());
		exitCode = 10;
		goto cleanup;
	}
	if ( bufInitialise(&sourceMask, 1024, 0x00) ) {
		fprintf(stderr, "%s\n", bufStrError());
		exitCode = 11;
		goto cleanup;
	}
	if ( bufInitialise(&i2cBuffer, 1024, 0x00) ) {
		fprintf(stderr, "%s\n", bufStrError());
		exitCode = 11;
		goto cleanup;
	}

	// Read from source...
	//
	if ( src == SRC_HEXFILE ) {
		if ( bufReadFromIntelHexFile(&sourceData, &sourceMask, srcOpt->sval[0]) ) {
			fprintf(stderr, "%s\n", bufStrError());
			exitCode = 13;
			goto cleanup;
		}
	} else if ( src == SRC_BIXFILE ) {
		if ( bufAppendFromBinaryFile(&sourceData, srcOpt->sval[0]) ) {
			fprintf(stderr, "%s\n", bufStrError());
			exitCode = 22;
			goto cleanup;
		}
		if ( bufAppendConst(&sourceMask, sourceData.length, 0x01, NULL) ) {
			fprintf(stderr, "%s\n", bufStrError());
			exitCode = 22;
			goto cleanup;
		}
	} else if ( src == SRC_IICFILE ) {
		if ( bufAppendFromBinaryFile(&i2cBuffer, srcOpt->sval[0]) ) {
			fprintf(stderr, "%s\n", bufStrError());
			exitCode = 22;
			goto cleanup;
		}
	} else if ( src == SRC_EEPROM ) {
		if ( fx2ReadEEPROM(vid, pid, eepromSize, &i2cBuffer) ) {
			fprintf(stderr, "%s\n", fx2StrError());
			exitCode = 22;
			goto cleanup;
		}
	} else {
		fprintf(stderr, "Internal error UNHANDLED_SRC\n");
		exitCode = 14;
		goto cleanup;
	}

	// Write to destination...
	//
	if ( dst == DST_RAM ) {
		// If the source data was I2C, write it to data/mask buffers
		//
		if ( i2cBuffer.length > 0 ) {
			if ( i2cReadPromRecords(&sourceData, &sourceMask, &i2cBuffer) ) {
				fprintf(stderr, "%s\n", fx2StrError());
				exitCode = 15;
				goto cleanup;
			}
		}

		// Write the data to RAM
		//
		if ( fx2WriteRAM(vid, pid, &sourceData) ) {
			fprintf(stderr, "%s\n", fx2StrError());
			exitCode = 15;
			goto cleanup;
		}
	} else if ( dst == DST_EEPROM ) {
		// If the source data was *not* I2C, construct I2C data from the raw data/mask buffers
		//
		if ( i2cBuffer.length == 0 ) {
			if ( i2cInitialise(&i2cBuffer, 0x0000, 0x0000, 0x0000, CONFIG_BYTE_400KHZ) ) {
				fprintf(stderr, "%s\n", bufStrError());
				exitCode = 12;
				goto cleanup;
			}
			if ( i2cWritePromRecords(&i2cBuffer, &sourceData, &sourceMask) ) {
				fprintf(stderr, "%s\n", fx2StrError());
				exitCode = 19;
				goto cleanup;
			}
			if ( i2cFinalise(&i2cBuffer) ) {
				fprintf(stderr, "%s\n", fx2StrError());
				exitCode = 20;
				goto cleanup;
			}
		}

		// Write the I2C data to the EEPROM
		//
		if ( fx2WriteEEPROM(vid, pid, &i2cBuffer) ) {
			fprintf(stderr, "%s\n", fx2StrError());
			exitCode = 16;
			goto cleanup;
		}
	} else if ( dst == DST_HEXFILE ) {
		// If the source data was I2C, write it to data/mask buffers
		//
		if ( i2cBuffer.length > 0 ) {
			if ( i2cReadPromRecords(&sourceData, &sourceMask, &i2cBuffer) ) {
				fprintf(stderr, "%s\n", fx2StrError());
				exitCode = 15;
				goto cleanup;
			}
		}

		// Write the data/mask buffers out as an I8HEX file
		//
		//dump(0x00000000, sourceMask.data, sourceMask.length);
		if ( bufWriteToIntelHexFile(&sourceData, &sourceMask, dstOpt->sval[0], 16, false) ) {
			fprintf(stderr, "%s\n", bufStrError());
			exitCode = 17;
			goto cleanup;
		}
	} else if ( dst == DST_BIXFILE ) {
		// If the source data was I2C, write it to data/mask buffers
		//
		if ( i2cBuffer.length > 0 ) {
			if ( i2cReadPromRecords(&sourceData, &sourceMask, &i2cBuffer) ) {
				fprintf(stderr, "%s\n", fx2StrError());
				exitCode = 15;
				goto cleanup;
			}
		}

		// Write the data buffer out as a binary file
		//
		if ( bufWriteBinaryFile(&sourceData, dstOpt->sval[0], 0x00000000, sourceData.length) ) {
			fprintf(stderr, "%s\n", bufStrError());
			exitCode = 18;
			goto cleanup;
		}
	} else if ( dst == DST_IICFILE ) {
		// If the source data was *not* I2C, construct I2C data from the raw data/mask buffers
		//
		if ( i2cBuffer.length == 0 ) {
			if ( i2cInitialise(&i2cBuffer, 0x0000, 0x0000, 0x0000, CONFIG_BYTE_400KHZ) ) {
				fprintf(stderr, "%s\n", bufStrError());
				exitCode = 12;
				goto cleanup;
			}
			if ( i2cWritePromRecords(&i2cBuffer, &sourceData, &sourceMask) ) {
				fprintf(stderr, "%s\n", fx2StrError());
				exitCode = 19;
				goto cleanup;
			}
			if ( i2cFinalise(&i2cBuffer) ) {
				fprintf(stderr, "%s\n", fx2StrError());
				exitCode = 20;
				goto cleanup;
			}
		}

		// Write the I2C data out as a binary file
		//
		if ( bufWriteBinaryFile(&i2cBuffer, dstOpt->sval[0], 0x00000000, i2cBuffer.length) ) {
			fprintf(stderr, "%s\n", bufStrError());
			exitCode = 21;
			goto cleanup;
		}
	} else {
		fprintf(stderr, "Internal error UNHANDLED_DST\n");
		exitCode = 20;
		goto cleanup;
	}

cleanup:
	if ( i2cBuffer.data ) {
		bufDestroy(&i2cBuffer);
	}
	if ( sourceMask.data ) {
		bufDestroy(&sourceMask);
	}
	if ( sourceData.data ) {
		bufDestroy(&sourceData);
	}
	arg_freetable(argTable, sizeof(argTable)/sizeof(argTable[0]));
	return exitCode;
}
