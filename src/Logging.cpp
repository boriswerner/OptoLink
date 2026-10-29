#include "Logging.h"
#include "Arduino.h"
#include <WiFiClient.h>
const int LOG_DATA_SIZE = 80;
String LOG_DATA[LOG_DATA_SIZE];
int LOG_DATA_INDEX = 0;

void Log(String message) {
	Log(message, false);
}

void Log(String message, bool dummyTime) {
	// Keine Ausgabe auf Serial: die UART ist die Optolink-Leitung zur Heizung.
	if (LOG_DATA_INDEX > LOG_DATA_SIZE - 1) {
		for (int i = 1; i < LOG_DATA_SIZE; i++) {
			LOG_DATA[i - 1] = LOG_DATA[i];
		}
		LOG_DATA_INDEX = LOG_DATA_SIZE - 1;
	}
	char logString[170];
	if (dummyTime) {
		sprintf(logString, "00.00. 00:00:00 - %s", message.c_str());
		LOG_DATA[LOG_DATA_INDEX] = logString;
	}
	else {
		sprintf(logString, "%02d.%02d. %02d:%02d:%02d - %s", day(), month(), hour(), minute(), second(), message.c_str());
		LOG_DATA[LOG_DATA_INDEX] = logString;
	}
	LOG_DATA_INDEX++;
}

String GetLogs() {
	String response;
	for (int i = 0; i < LOG_DATA_INDEX; i++) {
		response += LOG_DATA[i] + "\n";
	}
	return response;
}

// Sammelt Ausgaben zeilenweise und übernimmt nur Fehlerzeilen ins Log
size_t ErrorLogPrint::write(uint8_t c) {
	if (c == '\n') {
		if (_line.indexOf("error") >= 0 || _line.indexOf("timeout") >= 0) {
			Log("VitoWiFi: " + _line);
		}
		_line = "";
	}
	else if (c != '\r' && _line.length() < 120) {
		_line += (char)c;
	}
	return 1;
}

Logging::Logging()
{
}


Logging::~Logging()
{
}
