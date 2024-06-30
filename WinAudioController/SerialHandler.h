#ifndef SERIAL_HANDLER
#define SERIAL_HANDLER

#include <string>
#include <windows.h>

enum class RemoteButton {
	POWER,
	VOL_UP,
	FUNC,
	REWIND,
	PAUSE,
	FAST_FORWARD,
	DOWN,
	VOL_DOWN,
	UP,
	BTN_0,
	EQ,
	REPT,
	BTN_1,
	BTN_2,
	BTN_3,
	BTN_4,
	BTN_5,
	BTN_6,
	BTN_7,
	BTN_8,
	BTN_9,
	UNKNOWN
};

class SerialHandler {
public:
	SerialHandler(const std::string& portName);
	~SerialHandler();

	void readData();
	void writeData(const std::string& writeStr);
	void closePort();
	RemoteButton getButton();

	std::string data;

private:
	void ErrorExit(const char* err);
	std::string trimData(const std::string& str);
	HANDLE hSerial;
	std::string portName;
	std::string buffer;
};

#endif // SERIAL_HANDLER