#pragma once
#include <TimeLib.h>
#include "Arduino.h"
//const int LOG_DATA_SIZE = 120;
//String LOG_DATA[LOG_DATA_SIZE];
//int LOG_DATA_INDEX = 0;

void Log(String message);
void Log(String message, bool dummyTime);

String GetLogs();

class Logging
{
public:
	Logging();
	~Logging();
};
