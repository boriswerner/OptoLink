#pragma once
#include "Arduino.h"

#include <ESP8266WebServer.h>

extern ESP8266WebServer WebServer;

// Startet den Webserver auf Port 80:
//   GET  /             Startseite (include/WebUi.h)
//   GET  /api/status   Live-Werte, Zeitstempel, error-Status als JSON
//   GET  /api/logs     Log als Text
//   POST /api/restart  Neustart
void StartWebServer();
