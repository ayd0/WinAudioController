#include "SerialReader.h"
#include <iostream>

SerialReader::SerialReader(const std::string& portName) {
	std::wstring wPortName(portName.begin(), portName.end());

	hSerial = CreateFile(wPortName.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hSerial == INVALID_HANDLE_VALUE) {
		ErrorExit("Error opening serial port");
	}

	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	if (!GetCommState(hSerial, &dcbSerialParams)) {
		CloseHandle(hSerial);
		ErrorExit("Error getting serial port state");
	}

	dcbSerialParams.BaudRate = CBR_9600;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;

	if (!SetCommState(hSerial, &dcbSerialParams)) {
		CloseHandle(hSerial);
		ErrorExit("Error setting serial port state");
	}
}

SerialReader::~SerialReader() {
	closePort();
}

void SerialReader::ErrorExit(const char* err) {
	std::cerr << err << std::endl;
	ExitProcess(1);
}

std::string SerialReader::readData() {
	const int bufferSize = 1024;
	char buffer[bufferSize];
	DWORD bytesRead;
	std::string collectedData;

	if (ReadFile(hSerial, buffer, bufferSize, &bytesRead, NULL)) {
		if (bytesRead > 0) {
			collectedData.assign(buffer, bytesRead);
		}
	}
	else {
		CloseHandle(hSerial);
		ErrorExit("Error reading from serial port");
	}

	return collectedData;
}

void SerialReader::closePort() {
	if (hSerial != INVALID_HANDLE_VALUE) {
		CloseHandle(hSerial);
		hSerial = INVALID_HANDLE_VALUE;
	}
}