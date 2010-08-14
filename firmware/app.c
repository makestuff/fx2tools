/* 
 * Copyright (C) 2009 Chris McClelland
 *
 * Copyright (C) 2009 Ubixum, Inc.
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
#include <fx2regs.h>
#include <fx2macros.h>
#include <delay.h>
#include <i2c.h>
#include <setupdat.h>

#define SYNCDELAY() SYNCDELAY4;
#define EP0BUF_SIZE 0x40

#define bmSYNCFIFOS (bmIFCFG1 | bmIFCFG0)
#define bmBULK bmBIT5
#define bmDOUBLEBUFFERED bmBIT1
#define bmSKIP bmBIT7

#define bmDYN_OUT (1<<1)
#define bmENH_PKT (1<<0)

BYTE currentConfiguration;  // Current configuration
BYTE alternateSetting = 0;  // Alternate settings

// Called once at startup
//
void main_init(void) {
	CPUCS = bmCLKSPD1;  // 48MHz
	SYNCDELAY();
	IFCONFIG = (bmIFCLKSRC | bm3048MHZ | bmIFCLKOE | bmSYNCFIFOS);  // drive IFCLK with internal 30MHz clock, select synchronous FIFOs
	SYNCDELAY();
	REVCTL = (bmDYN_OUT | bmENH_PKT);
	SYNCDELAY();
	EP6CFG = (bmVALID | bmBULK | bmDOUBLEBUFFERED);
	SYNCDELAY();
	FIFORESET = bmNAKALL;
	SYNCDELAY();
	FIFORESET = bmNAKALL | 6;  // reset EP6
	SYNCDELAY();
	FIFORESET = 0x00;
	SYNCDELAY();
	OUTPKTEND = bmSKIP | 6;
	SYNCDELAY();
	OUTPKTEND = bmSKIP | 6;
	SYNCDELAY();
	EP6FIFOCFG = bmAUTOOUT;
	SYNCDELAY();
}

// Called repeatedly while the device is idle
//
void main_loop(void) { }

// Called when a Set Configuration command is received
//
BOOL handle_set_configuration(BYTE cfg) {
	currentConfiguration = cfg;
	return(TRUE);  // Handled by user code
}

// Called when a Get Configuration command is received
//
BYTE handle_get_configuration()
{
	return currentConfiguration;
}

BOOL promRead(WORD addr, BYTE length, BYTE xdata *buf);
BOOL promWrite(WORD addr, BYTE length, const BYTE xdata *buf);

// Called when a vendor command is received
//
BOOL handle_vendorcommand(BYTE cmd) {
	WORD address, length;
	BYTE i, chunkSize;
	switch(cmd) {
	case 0x80:
		// Simple example command which does the four arithmetic operations on the data from
		// the host, and sends the results back to the host
		//
		if ( SETUP_TYPE == 0xc0 ) {
			const unsigned short *inArray = (unsigned short *)(SETUPDAT+2);
			unsigned short *outArray = (unsigned short *)EP0BUF;

			// It's an IN operation
			//
			while ( EP0CS & bmEPBUSY );
			outArray[0] = inArray[0] + inArray[1];
			outArray[1] = inArray[0] - inArray[1];
			outArray[2] = inArray[0] * inArray[1];
			outArray[3] = inArray[0] / inArray[1];
			EP0BCH = 0;
			SYNCDELAY();
			EP0BCL = 8;
		} else {
			// This command does not support OUT operations
			//
			return FALSE;
		}
		break;
	case 0xa2:
		// Command to talk to the EEPROM
		//
		I2CTL |= bm400KHZ;
		address = SETUPDAT[2];
		address |= SETUPDAT[3] << 8;
		length = SETUPDAT[6];
		length |= SETUPDAT[7] << 8;
		if ( SETUP_TYPE == 0xc0 ) {
			// It's an IN operation - read from prom and send to host
			//
			while ( length ) {
				while ( EP0CS & bmEPBUSY );
				chunkSize = length < EP0BUF_SIZE ? length : EP0BUF_SIZE;
				for ( i = 0; i < chunkSize; i++ ) {
					EP0BUF[i] = 0x23;
				}
				promRead(address, chunkSize, EP0BUF);
				EP0BCH = 0;
				SYNCDELAY();
				EP0BCL = chunkSize;
				address += chunkSize;
				length -= chunkSize;
			}
		} else if ( SETUP_TYPE == 0x40 ) {
			// It's an OUT operation - read from host and send to prom
			//
			while ( length ) {
				EP0BCL = 0x00; // allow pc transfer in
				while ( EP0CS & bmEPBUSY ); // wait for data
				chunkSize = EP0BCL;
				promWrite(address, chunkSize, EP0BUF);
				address += chunkSize;
				length -= chunkSize;
			}
		}
		else {
			return FALSE;
		}
		break;
	default:
		return FALSE;  // unrecognised command
	}
	return TRUE;
}

// Called when a Get Interface command is received
//
BOOL handle_get_interface(BYTE ifc, BYTE *alt) {
	*alt = alternateSetting;
	return TRUE;
}

// Called when a Set Interface command is received
//
BOOL handle_set_interface(BYTE ifc, BYTE alt) {
	alternateSetting = alt;
	return TRUE;
}

/*void sof_isr() interrupt SOF_ISR {
	CLEAR_SOF();
}
void sutok_isr() interrupt SUTOK_ISR {}
void ep0ack_isr() interrupt EP0ACK_ISR {}
void ep0in_isr() interrupt EP0IN_ISR {}
void ep0out_isr() interrupt EP0OUT_ISR {}
void ep1in_isr() interrupt EP1IN_ISR {}
void ep1out_isr() interrupt EP1OUT_ISR {}
void ep2_isr() interrupt EP2_ISR {}
void ep4_isr() interrupt EP4_ISR {}
void ep6_isr() interrupt EP6_ISR {}
void ep8_isr() interrupt EP8_ISR {}
void ibn_isr() interrupt IBN_ISR {}
void ep0ping_isr() interrupt EP0PING_ISR {}
void ep1ping_isr() interrupt EP1PING_ISR {}
void ep2ping_isr() interrupt EP2PING_ISR {}
void ep4ping_isr() interrupt EP4PING_ISR {}
void ep6ping_isr() interrupt EP6PING_ISR {}
void ep8ping_isr() interrupt EP8PING_ISR {}
void errlimit_isr() interrupt ERRLIMIT_ISR {}
void ep2isoerr_isr() interrupt EP2ISOERR_ISR {}
void ep4isoerr_isr() interrupt EP4ISOERR_ISR {}
void ep6isoerr_isr() interrupt EP6ISOERR_ISR {}
void ep8isoerr_isr() interrupt EP8ISOERR_ISR {}
void spare_isr() interrupt RESERVED_ISR {}
void ep2pf_isr() interrupt EP2PF_ISR{}
void ep4pf_isr() interrupt EP4PF_ISR{}
void ep6pf_isr() interrupt EP6PF_ISR{}
void ep8pf_isr() interrupt EP8PF_ISR{}
void ep2ef_isr() interrupt EP2EF_ISR{}
void ep4ef_isr() interrupt EP4EF_ISR{}
void ep6ef_isr() interrupt EP6EF_ISR{}
void ep8ef_isr() interrupt EP8EF_ISR{}
void ep2ff_isr() interrupt EP2FF_ISR{}
void ep4ff_isr() interrupt EP4FF_ISR{}
void ep6ff_isr() interrupt EP6FF_ISR{}
void ep8ff_isr() interrupt EP8FF_ISR{}
void gpifdone_isr() interrupt GPIFDONE_ISR{}
void gpifwf_isr() interrupt GPIFWF_ISR{}
*/

BOOL promWaitForDone() {
	BYTE i;
	while ( !((i = I2CS) & 1) );  // Poll the done bit
	if ( i & bmBERR ) {
		return 1;
	} else {
		return 0;
	}
}	

BOOL promWaitForAck()
{
	BYTE i;
	while ( !((i = I2CS) & 1) );  // Poll the done bit
	if ( i & bmBERR ) {
		return 1;
	} else if ( !(i & bmACK) ) {
		return 1;
	} else {
		return 0;
	}
}

BOOL promRead(WORD addr, BYTE length, BYTE xdata *buf) {
	BYTE i;
	
	// Wait for I2C idle
	//
	while ( I2CS & bmSTOP );

	// Send the WRITE command
	//
	I2CS = bmSTART;
	I2DAT = 0xA2;  // Write I2C address byte (WRITE)
	if ( promWaitForAck() ) {
		return 1;
	}
	
	// Send the address, MSB first
	//
	I2DAT = MSB(addr);  // Write MSB of address
	if ( promWaitForAck() ) {
		return 1;
	}
	I2DAT = LSB(addr);  // Write LSB of address
	if ( promWaitForAck() ) {
		return 1;
	}

	// Send the READ command
	//
	I2CS = bmSTART;
	I2DAT = 0xA3;  // Write I2C address byte (READ)
	if ( promWaitForDone() ) {
		return 1;
	}

	// Read dummy byte
	//
	i = I2DAT;
	if ( promWaitForDone() ) {
		return 1;
	}

	// Now get the actual data
	//
	for ( i = 0; i < (length-1); i++ ) {
		*(buf+i) = I2DAT;
		if ( promWaitForDone() ) {
			return 1;
		}
	}

	// Terminate the read operation and get last byte
	//
	I2CS = bmLASTRD;
	if ( promWaitForDone() ) {
		return 1;
	}
	*(buf+i) = I2DAT;
	if ( promWaitForDone() ) {
		return 1;
	}
	I2CS = bmSTOP;
	i = I2DAT;

	return 0;
}

BOOL promWrite(WORD addr, BYTE length, const BYTE xdata *buf) {
	BYTE i;

	// Wait for I2C idle
	//
	while ( I2CS & bmSTOP );

	// Send the WRITE command
	//
	I2CS = bmSTART;
	I2DAT = 0xA2;  // Write I2C address byte (WRITE)
	if ( promWaitForAck() ) {
		return 1;
	}

	// Send the address, MSB first
	//
	I2DAT = MSB(addr);  // Write MSB of address
	if ( promWaitForAck() ) {
		return 1;
	}
	I2DAT = LSB(addr);  // Write LSB of address
	if ( promWaitForAck() ) {
		return 1;
	}

	// Write the data
	//
	for ( i = 0; i < length; i++ ) {
        I2DAT = *buf++;
		if ( promWaitForDone() ) {
			return 1;
		}
    }
	I2CS |= bmSTOP;

	// Wait for I2C idle
	//
	while ( I2CS & bmSTOP );

	do {
		I2CS = bmSTART;
		I2DAT = 0xA2;  // Write I2C address byte (WRITE)                                                                                                                                                                
		if ( promWaitForDone() ) {
			return 1;
		}
		I2CS |= bmSTOP;
		while ( I2CS & bmSTOP );
	} while ( !(I2CS & bmACK) );
	
	return 0;
}
