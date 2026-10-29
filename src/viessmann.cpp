#include "viessmann.h"
#include <VitoWiFi.h>
#include "mqtt.h"
#include "Arduino.h"
#include "CycleMonitor.h"

VitoWiFi_setProtocol(KW);
//HardwareSerial swSer;
//DPCountS geraeteKennung("GeraeteKennung", "boiler", 0x00F8);
    DPTemp x2900("x2900", "temperature", 0x2900); //Vorlauftemperatur-Istwert?
DPTemp aussenTemp("AußenTemp", "temperature", 0x0800);
    DPTemp x0818("x0818", "temperature", 0x0818); //Rücklauftemperatur (17A) - Tiefpass?
DPTemp outsideTemp("aussenTemperaturTiefpass", "temperature", 0x5525); // 2-Byte, -60..60
    DPTempS partyTemperaturSoll("partyTemperaturSoll", "temperature", 0x2308);
DPTemp outsideTempSmooth("aussenTemperaturGedaempft", "temperature", 0x5527); // 2-Byte, -60..60
    DPMode ferienbetrieb2("ferienbetrieb2", "state", 0x2303);
DPTemp vorlaufSollTemp2("VorlaufSollTemp2", "temperature", 0x2544);//0 
    DPMode solarpumpe("Solarpumpe", "state", 0x6552);//0 
DPMode heizkennlinieNiveau("heizkennlinieNiveau", "state", 0x27D4);
    DPTempS redRaumtemperaturSoll("redRaumtemperaturSoll", "temperature", 0x2307);
DPCoP heizkennlinieNeigung("heizkennlinieNeigung", "state", 0x27D3);


    DPTempS raumtemperaturSoll("raumtemperaturSoll", "temperature", 0x2306);
// Currently calculated low pass outside temperature, Time constant 30 minutes.
// 0x0800 2-byte ATS Aussentemperatur (Sensor 1)
// Kesseltemperatur Tiefpass [R]
DPTemp boilerTemp("boilerTemp", "temperature", 0x0810); // 2-Byte, 0..127
// WW-Temperatur (Speicher) [R]
    DPMode nachladeunterdrueckung("Nachladeunterdrückung", "state", 0x6551);//0 
DPTemp ladespeicherObenTemp("LadespeicherObenTemp", "temperature", 0x0812);
    DPCount waermemenge("Wärmemenge", "counter", 0x6560);//0 
DPTemp ladespeicherUntenTemp("LadespeicherUntenTemp", "temperature", 0x0814);
// Raumtemperatur A1M1 Tiefpass RTS [R]
    DPTemp roomTemp("heizkreisRaumtemperatur", "temperature", 0x0896); // 2-Byte, 0..127
// Abgastemperatur Tiefpass AGTS [R]
DPTemp agtsTemp("abgastemperaturTiefpass", "temperature", 0x0816); // 2-Byte
    DPTemp vltsTemp("vorlauftemperatur", "temperature", 0x080C); // 2-Byte
DPTemp abgasTemp("AbgasTempOderRuecklauf", "temperature", 0x0808);
// Rücklauftemperatur (17A) [R]
    DPTemp rltsTemp("rücklauftemperatur17A", "temperature", 0x080A); // 2-Byte
// Vorlauftemperatur (17B) [R]
// 0x0802 2-byte KTS Kesseltemperatur (Sensor 3)
DPTemp kesselIstTemp("KesselIstTemp", "temperature", 0x0802);
    DPMode currentOperatingMode("currentOperatingMode", "state", 0x2500); // 1-Byte, 0..3
DPTemp kesselSollTemp("KesselSollTemp", "temperature", 0x555A);
//-// DPTempS kesselSollTempS("KesselSollTempS", "boiler", 0x555A);
//-// DPCountS kesselSollCountS("kesselSollCountS", "boiler", 0x555A);
// Betriebsstunden Stufe 1
//-// DPHours boilerRunHours("runHours", "burner", 0x08A7); // 4-Byte, 0..1150000
// Brennerstarts [R]
//DPCount boilerRuns("starts", "burner", 0x088A); // 4-Byte, 0..1150000
//-// DPCount brennerstarts("Brennerstarts", "boiler", 0x088A);
// aktuelle Betriebsart A1M1 [R]; 0=Standby mode, 1=Reduced mode, 2=Standard mode, 3=Standard mode
// Heizkreispumpe A1M1 [R]

// Speicherladepumpe [R]
// DPStat storePump("storePump", "pump", 0x0845);
// Zirkulationspumpe [R]
// DPStat circuitPump("circuitPump", "pump", 0x0846);
    DPHours betriebsstunden("Betriebsstunden", "hours", 0x6568);

DPTempS warmwasserSollTemp("WarmwasserSollTemp", "temperature", 0x6300);

    DPTemp kollektorTemp("KollektorTemp", "temperature", 0x6564);//3276,7 
DPTemp vorlaufSollTemp("VorlaufSollTemp", "temperature", 0x3544);//0 
    DPStat aim1Pump("a1m1Pump", "state", 0x2906);
DPTemp warmwasserIstTemp("WarmwasserIstTemp", "temperature", 0x0804);
    DPTemp vorlaufIstTemp("VorlaufIstTemp", "temperature", 0x3900);//0 
// DPTemp solarspeicherTemp("SolarspeicherTemp", "boiler", 0x6566);//3276,7 

// DPCount solartagesertrag("Solartagesertrag", "boiler", 0xCF30);//4294967295
// DPMode solarinfo("Solarinfo", "boiler", 0x7754);//0
// DPMode speicherladepumpe("Speicherladepumpe", "boiler", 0x6513);//0
DPMode betriebsart("Betriebsart", "state", 0x3323);//2
    DPMode ferienbetrieb("ferienbetrieb", "state", 0x2535);
DPMode betriebsart2("Betriebsart2", "state", 0x2323);//2
    DPMode sparbetrieb("Sparbetrieb", "state", 0x3302);
//-// DPHours brennerlaufzeit("Brennerlaufzeit", "boiler", 0x0886);
//-// DPMode brennerstoerung("Brennerstörung", "boiler", 0x0883);//0

// DPCountS brennerstatus("Brennerstatus", "boiler", 0xA38F);//0
//-// DPPercent brennerleistung("Brennerleistung", "boiler", 0xA38F);//0
// DPMode flamme("Flamme", "boiler", 0x55D3);//0
// DPMode flowswitch("Flowswitch", "boiler", 0x0883);//0
// DPMode umschaltventil("Umschaltventil", "boiler", 0x0A10);//1
// DPMode umwaelzpumpe("Umwälzpumpe", "boiler", 0x7660);//1
// DPMode heizkreispumpe("Heizkreispumpe", "boiler", 0x3906);//0
//-// DPMode stoerung("Störung", "boiler", 0x0A82);//0
DPRaw stoerungsmeldung1("Stoerungsmeldung1", "error", 0x7507); //25.10.22 02:20:54 Uhr#Unbekannter Fehler#a8
DPRaw stoerungsmeldung2("Stoerungsmeldung2", "error", 0x7510); //01.10.22 06:13:22 Uhr#Unbekannter Fehler#e9
DPRaw stoerungsmeldung3("Stoerungsmeldung3", "error", 0x7519);
// DPRaw stoerungsmeldung4("Stoerungsmeldung4", "boiler", 0x7522);
// DPRaw stoerungsmeldung5("Stoerungsmeldung5", "boiler", 0x752B);
// DPRaw stoerungsmeldung6("Stoerungsmeldung6", "boiler", 0x7534);
// DPRaw stoerungsmeldung7("Stoerungsmeldung7", "boiler", 0x753D);
// DPRaw stoerungsmeldung8("Stoerungsmeldung8", "boiler", 0x7546);
// DPRaw stoerungsmeldung9("Stoerungsmeldung9", "boiler", 0x754F);
// DPRaw stoerungsmeldung10("Stoerungsmeldung10", "boiler", 0x7558);

/*https://github.com/Ixtalo/MyVitotronicLogger/blob/master/vitowifimqtt/src/vitowifimqtt.ino
room temp
22	309.4
21	258.1
18	257.8	283.4
reduced room temp
10	615.4
11	615.5
12	615.6

// Raumtemperatur Soll
DPTemp setRoomTemp("setRoomTemp", "temperature", 0x2306); // 1-Byte, 3..37
// Reduizierte Raumtemperatur Soll [RW]
DPTemp setRoomReducedTemp("setRoomReducedTemp", "temperature", 0x2307); // 1-Byte, 3..37
*/

const uint8_t resetBytes[] = {0x04};
const uint8_t initBytes[] = {0x16, 0x00, 0x00};
const uint8_t startBytes[] = {0x41, 0x05, 0x00, 0x01};

void tempCallbackHandler(const IDatapoint& dp, DPValue value);
void globalCallbackHandler(const IDatapoint& dp, DPValue value);
void stoerungsmeldungCallbackHandler(const IDatapoint& dp, DPValue value);

ErrorLogPrint vitoErrorLog;
std::vector<LiveValue> liveValues;

void setupVito() {
    VitoWiFi.setup(&Serial);
    VitoWiFi.setLogger(&vitoErrorLog);
    VitoWiFi.enableLogger();
    aussenTemp.setCallback(&tempCallbackHandler);
    outsideTemp.setCallback(&tempCallbackHandler);
    outsideTempSmooth.setCallback(&tempCallbackHandler);
    ladespeicherObenTemp.setCallback(&tempCallbackHandler);
    ladespeicherUntenTemp.setCallback(&tempCallbackHandler);
    vorlaufSollTemp2.setCallback(&tempCallbackHandler);
    agtsTemp.setCallback(&tempCallbackHandler);
    abgasTemp.setCallback(&tempCallbackHandler);
    kesselIstTemp.setCallback(&tempCallbackHandler);
    kesselSollTemp.setCallback(&tempCallbackHandler);
    boilerTemp.setCallback(&tempCallbackHandler);
    warmwasserIstTemp.setCallback(&tempCallbackHandler);
    VitoWiFi.setGlobalCallback(&globalCallbackHandler);
    stoerungsmeldung2.setCallback(&stoerungsmeldungCallbackHandler);
    stoerungsmeldung3.setCallback(&stoerungsmeldungCallbackHandler);
    stoerungsmeldung1.setCallback(&stoerungsmeldungCallbackHandler);
    // stoerungsmeldung4.setCallback(&stoerungsmeldungCallbackHandler);
    // stoerungsmeldung5.setCallback(&stoerungsmeldungCallbackHandler);
    // stoerungsmeldung6.setCallback(&stoerungsmeldungCallbackHandler);
    // stoerungsmeldung7.setCallback(&stoerungsmeldungCallbackHandler);
    // stoerungsmeldung8.setCallback(&stoerungsmeldungCallbackHandler);
    // stoerungsmeldung9.setCallback(&stoerungsmeldungCallbackHandler);
    // stoerungsmeldung10.setCallback(&stoerungsmeldungCallbackHandler);
    stoerungsmeldung2.setLength(9);
    stoerungsmeldung3.setLength(9);
    stoerungsmeldung1.setLength(9);
    // stoerungsmeldung4.setLength(9);
    // stoerungsmeldung5.setLength(9);
    // stoerungsmeldung6.setLength(9);
    // stoerungsmeldung7.setLength(9);
    // stoerungsmeldung8.setLength(9);
    // stoerungsmeldung9.setLength(9);
    // stoerungsmeldung10.setLength(9);

    liveValues.reserve(IDatapoint::getCollection().size());
    for (IDatapoint* dp : IDatapoint::getCollection()) {
        liveValues.push_back({dp, "", 0});
    }
}

CycleMonitor cycleMonitor;
bool cycleRunning = false;
time_t lastCycleTime = 0;
unsigned long lastCycleMillis = 0;

const std::vector<LiveValue>& getLiveValues() {
    return liveValues;
}

time_t getLastCycleTime() {
    return lastCycleTime;
}

long getNextCycleInMs() {
    return (long)VALUES_INTERVAL_MS - (long)(millis() - lastCycleMillis);
}

// Von allen Callbacks aufgerufen: zählt die Antwort und merkt sich den Wert für die Weboberfläche
void valueReceived(const IDatapoint& dp, const String& text) {
    cycleMonitor.onSuccess();
    for (auto& lv : liveValues) {
        if (lv.dp == &dp) {
            lv.value = text;
            lv.timestamp = time(nullptr);
            return;
        }
    }
}

void publishErrorState() {
    int state = cycleMonitor.state();
    if (state != CycleMonitor::NOTHING) {
        publishMqtt("error", state == 1 ? "1" : "0");
    }
}

int getErrorState() {
    return cycleMonitor.state();
}

void finishCycle() {
    cycleRunning = false;
    int state = cycleMonitor.finishCycle();
    if (state != CycleMonitor::NOTHING) {
        publishErrorState();
        Log(state == 1 ? "Heizung antwortet nicht (error=1)" : "Heizung antwortet (error=0)");
    }
}

void loopVito() {
    VitoWiFi.loop();
    if (cycleRunning && VitoWiFi.queueSize() == 0) {
        finishCycle();
    }
}

void tempCallbackHandler(const IDatapoint& dp, DPValue value) {
    valueReceived(dp, String(value.getFloat(), 1));
    publishMqtt(dp.getName(),(char*)String(value.getFloat()).c_str());
  Log("tempCallbackHandler: " + String(dp.getName()) + " " + String(value.getFloat()).c_str());
}

void globalCallbackHandler(const IDatapoint& dp, DPValue value) {
  char value_str[15] = {0};
  value.getString(value_str, sizeof(value_str));
  valueReceived(dp, value_str);
  publishMqtt(dp.getName(),value_str);
  Log("globalCallbackHandler: " + String(dp.getName()) + " " + String(value_str));
}

void stoerungsmeldungCallbackHandler(const IDatapoint& dp, DPValue value) {
    char dpBuffer[128];
    value.getString(dpBuffer,128);
    //Log((char*) test); // 9e2022051507133625

    String dpString = String(dpBuffer);
    String timeString = dpString.substring(8,10) + "." + dpString.substring(6,8) + "." + dpString.substring(4,6) + " " + dpString.substring(12,14) + ":" + dpString.substring(14,16) + ":" + dpString.substring(16,18) + " Uhr";
    Log(dpString);

    uint8_t errorCode = strtol(dpString.substring(0,2).c_str(), 0, 16);
    //char errorHexString[3] ;
    //sprintf(errorHexString,"%x",errorCode);
    String errorMessage = getErrorMessage(errorCode);
    Log(String(dp.getName()) + " " + errorMessage);
    
    String payload = timeString + "#" + errorMessage + "#" + dpString.substring(0,2);
    valueReceived(dp, payload);
    publishMqtt(dp.getName(), payload.c_str());
}

void getVitoData() {
    if (cycleRunning) {
        finishCycle();      // voriger Zyklus ist nicht fertig geworden
    }
    // Antwortet die Heizung nicht, dauert ein Durchlauf (5 s Timeout je Datenpunkt) länger als
    // das Intervall; dann nicht noch einmal alles einreihen, sonst staut sich die Warteschlange.
    if (VitoWiFi.queueSize() == 0) {
        VitoWiFi.readAll();
    } else {
        Log("Abfrage übersprungen, Warteschlange noch nicht leer");
    }
    cycleRunning = true;
    lastCycleTime = time(nullptr);
    lastCycleMillis = millis();
}

String getErrorMessage(uint8_t errorCode) {
    switch (errorCode) {
        case 0:
            return "Regelbetrieb (kein Fehler)";
            break;
        case 15:
            return "Wartung (für Reset Codieradresse 24 auf 0 stellen)";
            break;
        case 16:
            return "Kurzschluss Außentemperatursensor";
            break;
        case 24:
            return "Unterbrechung Außentemperatursensor";
            break;
        case 32:
            return "Kurzschluss Vorlauftemperatursensor";
            break;
        case 33:
            return "Kurzschluss Rücklauftemperatursensor";
            break;
        case 40:
            return "Unterbrechung Außentemperatursensor";
            break;
        case 41:
            return "Unterbrechung Rücklauftemperatursensor";
            break;
        case 48:
            return "Kurzschluss Kesseltemperatursensor";
            break;
        case 56:
            return "Unterbrechung Kesseltemperatursensor";
            break;
        case 64:
            return "Kurzschluss Vorlauftemperatursensor M2";
            break;
        case 66:
            return "Unterbrechung Vorlauftemperatursensor M2";
            break;
        case 80:
            return "Kurzschluss Speichertemperatursensor";
            break;
        case 88:
            return "Unterbrechung Speichertemperatursensor";
            break;
        case 146:
            return "Solar: Kurzschluss Kollektortemperatursensor";
            break;
        case 147:
            return "Solar: Kurzschluss Sensor S3";
            break;
        case 148:
            return "Solar: Kurzschluss Speichertemperatursensor";
            break;
        case 154:
            return "Solar: Unterbrechung Kollektortemperatursensor";
            break;
        case 155:
            return "Solar: Unterbrechung Sensor S3";
            break;
        case 156:
            return "Solar: Unterbrechung Speichertemperatursensor";
            break;
        case 158:
            return "Solar: Zu geringer oder kein Volumenstrom im Solarkreis oder Temperaturwächter hat ausgelöst";
            break;
        case 159:
            return "Solar: Fehlermeldung Solarteil (siehe Solarregler)";
            break;
        case 167:
            return "Bedienteil defekt";
            break;
        case 168:
            return "Luft in der internen Umwälzpumpe";
            break;
        case 176:
            return "Kurzschluss Abgastemperatursensor";
            break;
        case 177:
            return "Kommunikationsfehler Bedieneinheit";
            break;
        case 180:
            return "Interner Fehler (Elektronik)";
            break;
        case 181:
            return "Interner Fehler (Elektronik)";
            break;
        case 182:
            return "Ungültige Hardwarekennung (Elektronik)";
            break;
        case 183:
            return "Interner Fehler (Kesselkodierstecker)";
            break;
        case 184:
            return "Unterbrechung Abgastemperatursensor";
            break;
        case 185:
            return "Interner Fehler (Dateneingabe wiederholen)";
            break;
        case 186:
            return "Kommunikationsfehler Erweiterungssatz für Mischerkreis M2";
            break;
        case 188:
            return "Kommunikationsfehler Fernbedienung Vitorol, Heizkreis M1";
            break;
        case 189:
            return "Kommunikationsfehler Fernbedienung Vitorol, Heizkreis M2";
            break;
        case 190:
            return "Falsche Codierung Fernbedienung Vitorol";
            break;
        case 193:
            return "Externe Sicherheitseinrichtung (Kessel kühlt aus)";
            break;
        case 194:
            return "Kommunikationsfehler Solarregelung";
            break;
        case 197:
            return "Kommunikationsfehler drehzahlgeregelte Heizkreispumpe, Heizkreis M1";
            break;
        case 198:
            return "Kommunikationsfehler drehzahlgeregelte Heizkreispumpe, Heizkreis M2";
            break;
        case 199:
            return "Falsche Codierung der Heizkreispumpe";
            break;
        case 201:
            return "Störmeldeeingang am Schaltmodul-V aktiv";
            break;
        case 205:
            return "Kommunikationsfehler Vitocom 100 (KM-BUS)";
            break;
        case 206:
            return "Kommunikationsfehler Schaltmodul-V";
            break;
        case 207:
            return "Kommunikationsfehler LON Modul";
            break;
        case 209:
            return "Brennerstörung";
            break;
        case 212:
            return "Sicherheitstemperaturbegrenzer hat ausgelöst oder Störmeldemodul nicht richtig gesteckt";
            break;
        case 218:
            return "Kurzschluss Raumtemperatursensor, Heizkreis M1";
            break;
        case 219:
            return "Kurzschluss Raumtemperatursensor, Heizkreis M2";
            break;
        case 221:
            return "Unterbrechung Raumtemperatursensor, Heizkreis M1";
            break;
        case 222:
            return "Unterbrechung Raumtemperatursensor, Heizkreis M2";
            break;
        case 228:
            return "Fehler Versorgungsspannung";
            break;
        case 229:
            return "Interner Fehler (Ionisationselektrode)";
            break;
        case 230:
            return "Abgas- / Zuluftsystem verstopft";
            break;
        case 233:
            return "Feuerungsautomat";
            break;
        case 240:
            return "Interner Fehler (Regelung tauschen)";
            break;
        case 241:
            return "Abgastemperaturbegrenzer ausgelöst";
            break;
        case 242:
            return "Temperaturbegrenzer ausgelöst";
            break;
        case 243:
            return "Flammensigal beim Brennerstart bereits vorhanden";
            break;
        case 244:
            return "Flammensigal nicht vorhanden";
            break;
        case 247:
            return "Differenzdrucksensor defekt";
            break;
        case 248:
            return "Brennstoffventil schließt zu spät";
            break;
        case 249:
            return "Gebläsedrehzahl beim Brennerstart zu niedrig";
            break;
        case 250:
            return "Gebläsestillstand nicht erreicht";
            break;
        case 253:
            return "Fehler Gasfeurungsautomat"; //e9
            break;
        case 254:
            return "Starkes Störfeld (EMV) in der Nähe oder Elektronik defekt";
            break;
        case 255:
            return "Starkes Störfeld (EMV) in der Nähe oder interner Fehler";
            break;
        default:
            return "Unbekannter Fehler";
    }
}