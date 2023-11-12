// ThreadBoosterClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// Boost.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include "..\ThreadBooster\ThreadBoosterCommon.h"

int ReturnError(const char* message) {
	printf("%s (error=%u)\n", message, GetLastError());
	return 1;
}

int main(int argc, const char* argv[]) {
	if (argc < 3) {
		printf("Usage: boost <tid> <thread-priority>\n");
		return 0;
	}
	// After basic validation of command-line args, extract params
	int tid = atoi(argv[1]);
	int priority = atoi(argv[2]);

	// Obtain a handle to the device 
	// Since the symbolicLink string was Booster, we need to obtain a handle by prepending \\.\ 
	HANDLE devHandle = CreateFile(L"\\\\.\\Booster", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (devHandle == INVALID_HANDLE_VALUE)
		return ReturnError("Failed to open device");

	ThreadData data;
	data.Priority = priority;
	data.ThreadId = tid;

	DWORD returnedBytes;
	BOOL success = WriteFile(devHandle,
		&data, sizeof(data),          // buffer and length
		&returnedBytes, nullptr);
	if (!success)
		return ReturnError("Priority change failed!");

	printf("Priority change succeeded!\n");

	CloseHandle(devHandle);

	return 0;

}