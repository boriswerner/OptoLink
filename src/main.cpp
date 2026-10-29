#include <Timer.h>

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

#include "mqtt.h"
#include "Logging.h"
#include "WebServer.h"
#include "viessmann.h"
#include "secrets.h"
#include <time.h>
#include <coredecls.h>

const int LED_PIN = LED_BUILTIN;
const int PORT = 81;
const unsigned long WIFI_TIMEOUT_MS = 5 * 60 * 1000UL;	// Neustart nach 5 Minuten ohne WLAN
unsigned long wifiLostSince = 0;

WiFiServer* server = NULL;
WiFiClient serverClient;

Timer t;

void wifiSerialLoop();
void setupArduinoOta();
void setupNtp();
void checkWifi();
void getValues();

void setup() {
	Serial.begin(4800);
	pinMode(LED_PIN, OUTPUT);

	// -------- WLAN Connection
	Log("Connecting to WLAN");
	WiFi.mode(WIFI_STA);
	WiFi.setSleepMode(WIFI_NONE_SLEEP);
	WiFi.setAutoReconnect(true);
	WiFi.hostname("OptoLink");
	WiFi.begin(WIFI_SSID, WIFI_PW);

	unsigned long wifiStart = millis();
	while (WiFi.status() != WL_CONNECTED) {
		if (millis() - wifiStart > WIFI_TIMEOUT_MS) {
			ESP.restart();	// Zugangsdaten bleiben erhalten, nur neu versuchen
		}
		digitalWrite(LED_PIN, HIGH);
		delay(200);
		digitalWrite(LED_PIN, LOW);
		delay(200);
	}

	setupArduinoOta();
	setupNtp();
	t.every(10000, checkWifi);

  setupMqtt();
  t.every(10000,checkMqtt);
  checkMqtt();

	IPAddress HTTPS_ServerIP = WiFi.localIP(); // Obtain the IP of the Server
	Log("Server IP is: " + HTTPS_ServerIP.toString()); // Print the IP to the monitor window

	StartWebServer();
 // swSer.begin(4800);
  server = new WiFiServer(PORT);
  server->begin();
  setupVito();

  t.every(120000,getValues);
  getValues();
}

void loop() {
	WebServer.handleClient();
	ArduinoOTA.handle();
  mqttClient.loop();
  VitoWiFi.loop();
  t.update();
  //delay(200);
  //wifiSerialLoop();
}

void getValues() {
  Log("getvalues()");
  getVitoData();
}

// Kurze Aussetzer überbrückt setAutoReconnect(); nach 5 Minuten ohne WLAN Neustart
void checkWifi() {
	if (WiFi.status() == WL_CONNECTED) {
		if (wifiLostSince != 0) {
			Log("WLAN wieder verbunden");
			wifiLostSince = 0;
		}
		return;
	}
	if (wifiLostSince == 0) {
		wifiLostSince = millis();
		Log("WLAN getrennt");
	}
	else if (millis() - wifiLostSince > WIFI_TIMEOUT_MS) {
		ESP.restart();
	}
}

void setupNtp() {
	// Zeitserver ist der Router; Zeitzone Deutschland inkl. Sommerzeit.
	// configTime() merkt sich nur den Zeiger, daher static.
	static String ntpServer = WiFi.gatewayIP().toString();
	settimeofday_cb([]() {
		static bool first = true;
		if (first) {
			first = false;
			Log("NTP: Zeit synchronisiert");
		}
	});
	configTime("CET-1CEST,M3.5.0,M10.5.0/3", ntpServer.c_str());
	Log("NTP-Server: " + ntpServer);
}

void setupArduinoOta() {
  ArduinoOTA.onStart([]() {

		});
	ArduinoOTA.onEnd([]() {
		//Serial.println("\nEnd");
		});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		//Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
		});
	ArduinoOTA.onError([](ota_error_t error) {
		//Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			//Serial.println("Auth Failed");
		}
		else if (error == OTA_BEGIN_ERROR) {
			//Serial.println("Begin Failed");
		}
		else if (error == OTA_CONNECT_ERROR) {
			//Serial.println("Connect Failed");
		}
		else if (error == OTA_RECEIVE_ERROR) {
			//Serial.println("Receive Failed");
		}
		else if (error == OTA_END_ERROR) {
			//Serial.println("End Failed");
		}
		});
	ArduinoOTA.setHostname("optolink");
	ArduinoOTA.begin();
}

/* void wifiSerialLoop() 
{
  // check if there is a new client trying to connect
  if (server && server->hasClient())
  {
    // check, if no client is already connected
    if (!serverClient || !serverClient.connected())
    {
      if (serverClient) serverClient.stop();
      serverClient = server->available();
      Log("New client connected\n");
    }
    else
    {
      // reject additional connection requests.
      WiFiClient serverClient = server->available();
      serverClient.stop();
    }
  }
  yield();

  // check client for data
  if (serverClient && serverClient.connected())
  {
    size_t len = serverClient.available();

    if (len)
    {
      uint8_t sbuf[len];

      serverClient.read(sbuf, len);
      // write received WiFi data to serial
      swSer.write(sbuf, len);
      yield();

      // debug output received WiFi data to Serial1
      //Serial1.println();
      //Log("WiFi: ");
      for (uint8_t n = 0; n < len; n++)
      {
        //Serial.print(sbuf[n], HEX);
      }
      //Serial.println();
    }
  }
  yield();

  //   check UART for data
  if (Serial.available())
  {
    size_t len = Serial.available();
    uint8_t sbuf[len];

    Serial.readBytes(sbuf, len);
    // push UART data to connected WiFi client
    swSer.write(sbuf, len);
    yield();

    // debug output received Serial data to Serial1
    //Serial1.println();
    Log("Serial: ");
    for (uint8_t n = 0; n < len; n++)
    {
      //Serial.printf("%02x ", sbuf[n]);
    }
    //Serial.println();
  }

  //##################

  // check UART for data
  if (swSer.available())
  {
    size_t len = swSer.available();
    uint8_t sbuf[len];

    swSer.readBytes(sbuf, len);

    // push UART data to connected WiFi client
    if (serverClient && serverClient.connected())
    {
      serverClient.write(sbuf, len);
    }
    yield();

    // debug output received Serial data to Serial1
    //Serial1.println();
    Serial.print("swSerial: ");
    Serial.write(sbuf, len);
    for (uint8_t n = 0; n < len; n++)
    {
      //Serial.printf("%02x ", sbuf[n]);
    }
    //Serial.println();
  }


}*/
