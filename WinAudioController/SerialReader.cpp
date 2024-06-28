#include "SerialReader.h"

#include <iostream>
#include <unordered_map>

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

std::string SerialReader::trimData(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\n\r");
	if (first == std::string::npos) {
		return "";
	}
	size_t last = str.find_last_not_of(" \t\n\r");
	return str.substr(first, last - first + 1);
}

std::string SerialReader::readData() {
	const int bufferSize = 1024;
	char buffer[bufferSize];
	DWORD bytesRead;
	std::string collectedData;

	if (ReadFile(hSerial, buffer, bufferSize, &bytesRead, NULL)) {
		if (bytesRead > 0) {
			collectedData.assign(buffer, bytesRead);
			for (DWORD i = 0; i < bytesRead; ++i) {
				if (buffer[i] == '\n') {
					collectedData = this->buffer;
					this->buffer.clear();
					return trimData(collectedData);
				}
				else {
					this->buffer += buffer[i];
				}
			}
		}
	}
	else {
		CloseHandle(hSerial);
		ErrorExit("Error reading from serial port");
	}

	return "";
}

void SerialReader::closePort() {
	if (hSerial != INVALID_HANDLE_VALUE) {
		CloseHandle(hSerial);
		hSerial = INVALID_HANDLE_VALUE;
	}
}

RemoteButton SerialReader::getButton(const std::string& btnCode) {
	static const std::unordered_map<std::string, RemoteButton> hexToButtonMap = {
		{ "BA45FF00", RemoteButton::POWER },
		{ "B946FF00", RemoteButton::VOL_UP },
		{ "B847FF00", RemoteButton::FUNC },
		{ "BB44FF00", RemoteButton::REWIND },
		{ "BF40FF00", RemoteButton::PAUSE },
		{ "BC43FF00", RemoteButton::FAST_FORWARD },
		{ "F807FF00", RemoteButton::DOWN },
		{ "EA15FF00", RemoteButton::VOL_DOWN },
		{ "F609FF00", RemoteButton::UP },
		{ "E916FF00", RemoteButton::BTN_0 },
		{ "E619FF00", RemoteButton::EQ },
		{ "F20DFF00", RemoteButton::REPT },
		{ "F30CFF00", RemoteButton::BTN_1 },
		{ "E718FF00", RemoteButton::BTN_2 },
		{ "A15EFF00", RemoteButton::BTN_3 },
		{ "F708FF00", RemoteButton::BTN_4 },
		{ "E31CFF00", RemoteButton::BTN_5 },
		{ "A55AFF00", RemoteButton::BTN_6 },
		{ "BD42FF00", RemoteButton::BTN_7 },
		{ "AD52FF00", RemoteButton::BTN_8 },
		{ "B54AFF00", RemoteButton::BTN_9 }
	};

	auto it = hexToButtonMap.find(btnCode);
	if (it != hexToButtonMap.end()) {
		return it->second;
	}
	else {
		return RemoteButton::UNKNOWN;
	}
}

