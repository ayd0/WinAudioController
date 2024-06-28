#ifndef SERIAL_READER
#define SERIAL_READER

#include <string>
#include <windows.h>

class SerialReader {
public:
	SerialReader(const std::string& portName);
	~SerialReader();

	std::string readData();
	void closePort();

private:
	void ErrorExit(const char* err);
	HANDLE hSerial;
	std::string portName;
};

#endif // SERIAL_READER