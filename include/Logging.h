#pragma once
#include <TimeLib.h>
#include "Arduino.h"
//const int LOG_DATA_SIZE = 120;
//String LOG_DATA[LOG_DATA_SIZE];
//int LOG_DATA_INDEX = 0;

void Log(String message);
void Log(String message, bool dummyTime);

int GetLogCount();
const String& GetLogLine(int index);

// Print-Ziel für den VitoWiFi-Logger: schreibt nur Fehler/Timeouts ins Log
class ErrorLogPrint : public Print {
public:
	size_t write(uint8_t c) override;
private:
	String _line;
};

class Logging
{
public:
	Logging();
	~Logging();
};
