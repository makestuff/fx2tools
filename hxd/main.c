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
#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#pragma warning(disable : 4996)
#endif

int main(int argc, const char *argv[]) {
	FILE *input;
	unsigned char line[16];
	unsigned int i, bytesRead, offset = 0;

	if ( argc == 2 ) {
		input = fopen(argv[1], "rb");
		if ( !input ) {
			fprintf(stderr, "%s: file not found\n", argv[1]);
			exit(1);
		}
	} else if ( argc == 1 ) {
		input = stdin;
		#ifdef WIN32
			_setmode(fileno(stdin), O_BINARY);
		#endif
	} else {
		fprintf(stderr, "Synopsis: %s <file>\n", argv[0]);
		exit(1);
	}

	do {
		printf("%08X ", offset);
		bytesRead = fread(line, 1, 16, input);
		offset += bytesRead;
		for ( i = 0; i < bytesRead; i++ ) {
			printf("%02X ", line[i]);
		}
		if ( bytesRead != 16 ) {
			for ( i = 0; i < 16-bytesRead; i++ ) {
				printf("   ");
			}
		}
		for ( i = 0; i < bytesRead; i++ ) {
			if ( line[i] < 32 || line[i] > 126 ) {
				printf ( "." );
			} else {
				printf("%c", line[i]);
			}
		}
		printf("\n");
	} while ( !feof(input) );

	if ( input != stdin ) {
		fclose(input);
	}
	return 0;
}
