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

// GET /api/raw?list=0x0800:2,0x0802:2,...  (bis zu SCAN_BATCH Einträge Adresse:Länge, Länge 1..9)
//   -> {"id":n}; Ergebnis über /api/raw/result abholen
void HandleRawRead() {
	uint16_t addrs[SCAN_BATCH];
	uint8_t lens[SCAN_BATCH];
	int count = 0;
	String list = WebServer.arg("list");
	int pos = 0;
	while (pos < (int)list.length()) {
		int comma = list.indexOf(',', pos);
		if (comma < 0) {
			comma = list.length();
		}
		String item = list.substring(pos, comma);
		int colon = item.indexOf(':');
		long addr = strtol(item.substring(0, colon).c_str(), nullptr, 0);
		long len = colon > 0 ? item.substring(colon + 1).toInt() : 0;
		if (count >= SCAN_BATCH || addr < 0 || addr > 0xFFFF || len < 1 || len > MAX_DP_LENGTH) {
			WebServer.send(400, "text/plain", "list=Adresse:Länge,... (max. 16 Einträge, Länge 1..9)");
			return;
		}
		addrs[count] = (uint16_t)addr;
		lens[count] = (uint8_t)len;
		count++;
		pos = comma + 1;
	}
	if (count == 0) {
		WebServer.send(400, "text/plain", "list=Adresse:Länge,... angeben");
		return;
	}
	if (!isScanActive()) {
		WebServer.send(409, "text/plain", "Scan nicht aktiv (POST /api/scan/start)");
		return;
	}
	int id = scanRead(addrs, lens, count);
	if (id < 0) {
		WebServer.send(409, "text/plain", "belegt, später erneut versuchen");
		return;
	}
	WebServer.send(202, "application/json", "{\"id\":" + String(id) + "}");
}

void HandleRawResult() {
	static const char* const STATES[] = {"idle", "busy", "done", "failed"};
	String json = "{\"id\":" + String(getScanId()) + ",\"busy\":" + (scanBusy() ? "true" : "false") + ",\"results\":[";
	for (int i = 0; i < getScanCount(); i++) {
		char addr[8];
		snprintf(addr, sizeof(addr), "0x%04X", getScanAddress(i));
		if (i > 0) {
			json += ',';
		}
		json += "{\"addr\":\"" + String(addr) + "\",\"len\":" + String(getScanLength(i)) +
			",\"state\":\"" + STATES[getScanState(i)] + "\",\"hex\":\"" + getScanHex(i) + "\"}";
	}
	json += "]}";
	WebServer.send(200, "application/json", json);
}

void HandleRestart() {
	Log("HandleRestart");
	WebServer.send(200, "text/plain", "Neustart");
	delay(200);		// Antwort noch ausliefern
	ESP.restart();
}
