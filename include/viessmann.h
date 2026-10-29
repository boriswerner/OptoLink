#pragma once
#include <SoftwareSerial.h>
#include <Logging.h>
#include <VitoWiFi.h>

extern VitoWiFi_setProtocol(KW);
//extern HardwareSerial swSer;
const unsigned long VALUES_INTERVAL_MS = 60000;	// Abfrageintervall

// Letzter Wert eines Datenpunkts für die Weboberfläche
struct LiveValue {
  const IDatapoint* dp;
  String value;       // Anzeigetext ("" = noch nie gelesen)
  time_t timestamp;   // letzte erfolgreiche Abfrage
};

void setupVito();
const std::vector<LiveValue>& getLiveValues();
time_t getLastCycleTime();
long getNextCycleInMs();

void getVitoData();
void loopVito();            // statt VitoWiFi.loop(): schließt außerdem fertige Zyklen ab
void publishErrorState();   // MQTT "error" erneut senden (z. B. nach Neuverbindung)
int getErrorState();        // 0/1, -1 vor dem ersten abgeschlossenen Zyklus

// Scan: Adressen roh lesen (nur lesend), reguläre Abfrage pausiert, max. 10 Minuten
enum ScanState { SCAN_IDLE, SCAN_BUSY, SCAN_DONE, SCAN_FAILED };
void scanStart();
void scanStop();
bool isScanActive();
int scanRead(uint16_t address, uint8_t length);   // Abfrage-ID oder -1 (Scan inaktiv/belegt)
ScanState getScanState();
int getScanId();
const String& getScanHex();
uint16_t getScanAddress();
uint8_t getScanLength();
String getErrorMessage(uint8_t code);

class conv1_2_F : public DPType {
 public:
  void encode(uint8_t* out, DPValue in) {  // encode can be left empty when the DP is only used for reading.

  }
  DPValue decode(const uint8_t* in) {
    int32_t tmp = in[3] << 24 | in[2] << 16 | in[1] << 8 | in[0];  // keep endianess in mind! input is LSB first
    DPValue out(tmp / 2.0f);
    return out;
  }
  size_t getLength() const {
    return 1;
  }
};


typedef Datapoint<conv1_2_F> DPPercent;