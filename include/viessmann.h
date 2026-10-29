#pragma once
#include <SoftwareSerial.h>
#include <Logging.h>
#include <VitoWiFi.h>

extern VitoWiFi_setProtocol(KW);
//extern HardwareSerial swSer;
void setupVito();

void getVitoData();
void loopVito();            // statt VitoWiFi.loop(): schließt außerdem fertige Zyklen ab
void publishErrorState();   // MQTT "error" erneut senden (z. B. nach Neuverbindung)
int getErrorState();        // 0/1, -1 vor dem ersten abgeschlossenen Zyklus
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