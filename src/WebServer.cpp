#include "WebServer.h"
#include "Logging.h"
#include "viessmann.h"
#include <ESP8266WebServer.h>

ESP8266WebServer WebServer(80);

String onmessage = "&nbsp;";
String offmessage = "&nbsp;";


void StartWebServer() {
	Log("Start WebServer");
	WebServer.on("/", HandleRoot);
	WebServer.on("/Logs", HandleLogs);
	WebServer.on("/getLogs", HandleGetLogs);
	WebServer.on("/restart", HandleRestart);

	WebServer.begin();

}



// Seite stückweise senden statt als großen String zusammenzubauen (Heap-Fragmentierung)
void ShowPage(String content, String script, String style) {
	// replacements
	content.replace("{menu}", FPSTR(HTTP_MENU));
	content.replace("{onmessage}", onmessage);
	content.replace("{offmessage}", offmessage);
	content.replace("{color}", "#FF0000");
	onmessage = "&nbsp;";
	offmessage = "&nbsp;";

	WebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
	WebServer.send(200, "text/html", "");
	WebServer.sendContent_P(HTTP_HEADER);
	WebServer.sendContent(script);
	WebServer.sendContent_P(HTTP_SCRIPT_END);
	WebServer.sendContent(style);
	WebServer.sendContent_P(HTTP_CONTAINER);
	WebServer.sendContent(content);
	WebServer.sendContent_P(HTTP_END);
}

String MainMenu() {
	return "";
}

void HandleRoot() {
	Serial1.println("HandleRoot");
	String content = FPSTR(HTTP_MAIN);
	content += "{menu}";

	ShowPage(content, FPSTR(HTTP_MAIN_SCRIPT), FPSTR(HTTP_STYLE));
}


void HandleLogs() {
	Serial1.println("HandleGetLogs");
	String content = "<textarea readonly id=\"logs\" cols=\"45\" wrap=\"off\"></textarea>";
	content += FPSTR(HTTP_MENU_BACK);

	ShowPage(content, FPSTR(HTTP_LOGS_SCRIPT), FPSTR(HTTP_STYLE));
}

void HandleRestart() {
	Log("HandleRestart");

	onmessage = "Restart wird ausgeführt!";
	WebServer.sendHeader("Location", String("/"), true);
	WebServer.send(302, "text/plain", "");
	ESP.restart();
}

void HandleGetLogs() {
	WebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
	WebServer.send(200, "text/html", "");
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

WebServer::WebServer()
{

}


WebServer::~WebServer()
{
}
