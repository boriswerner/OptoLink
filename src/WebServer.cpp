#include "WebServer.h"
#include "WebUi.h"
#include "Logging.h"
#include "viessmann.h"
#include "mqtt.h"
#include <ESP8266WebServer.h>

ESP8266WebServer WebServer(80);

void HandleRoot();
void HandleStatus();
void HandleLogs();
void HandleRestart();

void StartWebServer() {
	Log("Start WebServer");
	WebServer.on("/", HTTP_GET, HandleRoot);
	WebServer.on("/api/status", HTTP_GET, HandleStatus);
	WebServer.on("/api/logs", HTTP_GET, HandleLogs);
	WebServer.on("/api/restart", HTTP_POST, HandleRestart);
	WebServer.onNotFound([]() {
		WebServer.send(404, "text/plain", "Nicht gefunden");
	});

	WebServer.begin();
}

void HandleRoot() {
	WebServer.send_P(200, "text/html; charset=utf-8", WEB_UI_PAGE);
}

// Text als JSON-String-Inhalt (ohne Anführungszeichen außen)
String jsonEscape(const String& s) {
	String out;
	out.reserve(s.length() + 4);
	for (unsigned int i = 0; i < s.length(); i++) {
		char c = s[i];
		if (c == '"' || c == '\\') {
			out += '\\';
			out += c;
		}
		else if ((unsigned char)c < 0x20) {
			out += ' ';
		}
		else {
			out += c;
		}
	}
	return out;
}

// JSON stückweise senden, damit kein großer String entsteht
void HandleStatus() {
	WebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
	WebServer.send(200, "application/json; charset=utf-8", "");

	String chunk;
	chunk.reserve(1100);
	chunk += "{\"time\":";
	chunk += String((unsigned long)time(nullptr));
	chunk += ",\"uptime\":";
	chunk += String(millis() / 1000);
	chunk += ",\"heap\":";
	chunk += String(ESP.getFreeHeap());
	chunk += ",\"rssi\":";
	chunk += String(WiFi.RSSI());
	chunk += ",\"mqtt\":";
	chunk += mqttClient.connected() ? "true" : "false";
	chunk += ",\"error\":";
	chunk += String(getErrorState());
	chunk += ",\"interval\":";
	chunk += String(VALUES_INTERVAL_MS / 1000);
	chunk += ",\"lastCycle\":";
	chunk += String((unsigned long)getLastCycleTime());
	chunk += ",\"nextCycleIn\":";
	chunk += String(getNextCycleInMs());
	chunk += ",\"values\":[";

	bool first = true;
	for (const LiveValue& lv : getLiveValues()) {
		if (!first) {
			chunk += ',';
		}
		first = false;
		chunk += "{\"n\":\"";
		chunk += jsonEscape(lv.dp->getName());
		chunk += "\",\"g\":\"";
		chunk += jsonEscape(lv.dp->getGroup());
		chunk += "\",\"v\":\"";
		chunk += jsonEscape(lv.value);
		chunk += "\",\"t\":";
		chunk += String((unsigned long)lv.timestamp);
		chunk += '}';
		if (chunk.length() > 1000) {
			WebServer.sendContent(chunk);
			chunk = "";
		}
	}
	chunk += "]}";
	WebServer.sendContent(chunk);
}

void HandleLogs() {
	WebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
	WebServer.send(200, "text/plain; charset=utf-8", "");
	String chunk;
	chunk.reserve(1100);
	for (int i = 0; i < GetLogCount(); i++) {
		chunk += GetLogLine(i);
		chunk += "\n";
		if (chunk.length() > 1000) {
			WebServer.sendContent(chunk);
			chunk = "";
		}
	}
	if (chunk.length() > 0) {
		WebServer.sendContent(chunk);
	}
}

void HandleRestart() {
	Log("HandleRestart");
	WebServer.send(200, "text/plain", "Neustart");
	delay(200);		// Antwort noch ausliefern
	ESP.restart();
}
