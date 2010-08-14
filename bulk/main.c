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
#include "usbwrap.h"
#include "argtable2.h"
#include "arg_uint.h"
#include "dump.h"
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

#define VID 0x1443
#define PID 0x0005

int main(int argc, char *argv[]) {

	struct arg_uint *vidOpt  = arg_uint0("v", "vid", "<vendorID>", "  vendor ID");
	struct arg_uint *pidOpt  = arg_uint0("p", "pid", "<productID>", " product ID");
	struct arg_int  *epOpt   = arg_int0("e", "endpoint", "<endpoint>", " endpoint to write to");
	struct arg_lit  *benOpt  = arg_lit0("b", "benchmark", "            benchmark the operation");
	struct arg_lit  *chkOpt  = arg_lit0("c", "checksum", "            print 16-bit checksum");
	struct arg_lit  *helpOpt = arg_lit0("h", "help", "            print this help and exit\n");
	struct arg_file *fileOpt = arg_file1(NULL, NULL, "<data.dat>", "            the data to send");
	struct arg_end  *endOpt  = arg_end(20);
	void* argTable[] = {vidOpt, pidOpt, epOpt, benOpt, chkOpt, helpOpt, fileOpt, endOpt};
	const char *progName = "bulk";
	uint32 exitCode = 0;
	int numErrors;
	int epNum = 0x06;
	FILE *inFile = NULL;
	uint8 *buffer = NULL;
	uint32 fileLen;
	UsbDeviceHandle *deviceHandle = NULL;
	int returnCode;
	double totalTime, speed;
	uint16 vid, pid;
	unsigned short checksum = 0x0000;
	uint32 i;
	#ifdef WIN32
		LARGE_INTEGER tvStart, tvEnd, freq;
		DWORD_PTR mask = 1;
		SetThreadAffinityMask(GetCurrentThread(), mask);
		QueryPerformanceFrequency(&freq);
	#else
		struct timeval tvStart, tvEnd;
		long long startTime, endTime;
	#endif

	if ( arg_nullcheck(argTable) != 0 ) {
		printf("%s: insufficient memory\n", progName);
		exitCode = 1;
		goto cleanup;
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("Bulk Write Tool Copyright (C) 2009-2010 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nWrite data to a bulk endpoint.\n\n");
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

	vid = vidOpt->count ? (uint16)vidOpt->ival[0] : VID;
	pid = pidOpt->count ? (uint16)pidOpt->ival[0] : PID;

	inFile = fopen(fileOpt->filename[0], "rb");
	if ( !inFile ) {
		fprintf(stderr, "Unable to open file %s", fileOpt->filename[0]);
		exitCode = 3;
		goto cleanup;
	}
	fseek(inFile, 0, SEEK_END);
	fileLen = ftell(inFile);
	fseek(inFile, 0, SEEK_SET);

	buffer = (uint8 *)malloc(fileLen);
	if ( !buffer ) {
		fprintf(stderr, "Unable to allocate memory for file %s\n", fileOpt->filename[0]);
		exitCode = 4;
		goto cleanup;
	}

	if ( fread(buffer, 1, fileLen, inFile) != fileLen ) {
		fprintf(stderr, "Unable to read file %s\n", fileOpt->filename[0]);
		exitCode = 5;
		goto cleanup;
	}

	if ( chkOpt->count ) {
		for ( i = 0; i < fileLen; i++  ) {
			checksum += buffer[i];
		}
		printf("Checksum: 0x%04X\n", checksum);
	}

	if ( epOpt->count ) {
		epNum = epOpt->ival[0];
	}

	usbInitialise();
	returnCode = usbOpenDevice(vid, pid, 1, 0, 0, &deviceHandle);
	if ( returnCode ) {
		fprintf(stderr, "usbOpenDevice() failed returnCode %d: %s\n", returnCode, usbStrError());
		exitCode = 6;
		goto cleanup;
	}
	usb_clear_halt(deviceHandle, epNum);
	#ifdef WIN32
		QueryPerformanceCounter(&tvStart);
		returnCode = usb_bulk_write(deviceHandle, USB_ENDPOINT_OUT | epNum, (char*)buffer, fileLen, 5000);
		QueryPerformanceCounter(&tvEnd);
	#else
		gettimeofday(&tvStart, NULL);
		returnCode = usb_bulk_write(deviceHandle, USB_ENDPOINT_OUT | epNum, (char*)buffer, fileLen, 5000);
		gettimeofday(&tvEnd, NULL);
	#endif
	if ( returnCode != (int)fileLen ) {
		printf("Expected to write %lu bytes but actually wrote %d: %s\n", fileLen, returnCode, usb_strerror());
		exitCode = 7;
		goto cleanup;
	}
	#ifdef WIN32
		totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
		totalTime /= freq.QuadPart;
		printf("Time: %fms\n", totalTime/1000.0);
		speed = (double)fileLen / (1024*1024*totalTime);
	#else
		startTime = tvStart.tv_sec;
		startTime *= 1000000;
		startTime += tvStart.tv_usec;
		endTime = tvEnd.tv_sec;
		endTime *= 1000000;
		endTime += tvEnd.tv_usec;
		totalTime = endTime - startTime;
		totalTime /= 1000000;  // convert from uS to S.
		speed = (double)fileLen / (1024*1024*totalTime);
	#endif
	if ( benOpt->count ) {
		printf("Speed: %f MB/s\n", speed);
	}

cleanup:
	if ( buffer ) {
		free(buffer);
	}
	if ( inFile ) {
		fclose(inFile);
	}
	if ( deviceHandle ) {
		usb_release_interface(deviceHandle, 0);
		usb_close(deviceHandle);
	}
	arg_freetable(argTable, sizeof(argTable)/sizeof(argTable[0]));

	return exitCode;
}
