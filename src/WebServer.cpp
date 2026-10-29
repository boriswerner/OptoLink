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
void HandleScanStart();
void HandleScanStop();
void HandleRawRead();
void HandleRawResult();

void StartWebServer() {
	Log("Start WebServer");
	WebServer.on("/", HTTP_GET, HandleRoot);
	WebServer.on("/api/status", HTTP_GET, HandleStatus);
	WebServer.on("/api/logs", HTTP_GET, HandleLogs);
	WebServer.on("/api/restart", HTTP_POST, HandleRestart);
	WebServer.on("/api/scan/start", HTTP_POST, HandleScanStart);
	WebServer.on("/api/scan/stop", HTTP_POST, HandleScanStop);
	WebServer.on("/api/raw", HTTP_GET, HandleRawRead);
	WebServer.on("/api/raw/result", HTTP_GET, HandleRawResult);
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

// ---- Scan (nur lesend): siehe scanRead() in viessmann.cpp ----

void HandleScanStart() {
	scanStart();
	WebServer.send(200, "application/json", "{\"active\":true}");
}

void HandleScanStop() {
	scanStop();
	WebServer.send(200, "application/json", "{\"active\":false}");
}

// GET /api/raw?addr=0x0800&len=2  ->  {"id":n}; Ergebnis über /api/raw/result abholen
void HandleRawRead() {
	long addr = strtol(WebServer.arg("addr").c_str(), nullptr, 0);
	long len = WebServer.arg("len").toInt();
	if (!WebServer.hasArg("addr") || addr < 0 || addr > 0xFFFF || len < 1 || len > MAX_DP_LENGTH) {
		WebServer.send(400, "text/plain", "addr (0..0xFFFF) und len (1..9) angeben");
		return;
	}
	if (!isScanActive()) {
		WebServer.send(409, "text/plain", "Scan nicht aktiv (POST /api/scan/start)");
		return;
	}
	int id = scanRead((uint16_t)addr, (uint8_t)len);
	if (id < 0) {
		WebServer.send(409, "text/plain", "belegt, später erneut versuchen");
		return;
	}
	WebServer.send(202, "application/json", "{\"id\":" + String(id) + "}");
}

void HandleRawResult() {
	static const char* const STATES[] = {"idle", "busy", "done", "failed"};
	char addr[8];
	snprintf(addr, sizeof(addr), "0x%04X", getScanAddress());
	String json = "{\"id\":" + String(getScanId()) +
		",\"state\":\"" + STATES[getScanState()] +
		"\",\"addr\":\"" + addr +
		"\",\"len\":" + String(getScanLength()) +
		",\"hex\":\"" + getScanHex() + "\"}";
	WebServer.send(200, "application/json", json);
}

void HandleRestart() {
	Log("HandleRestart");
	WebServer.send(200, "text/plain", "Neustart");
	delay(200);		// Antwort noch ausliefern
	ESP.restart();
}
