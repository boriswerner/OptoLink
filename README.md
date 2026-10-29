# OptoLink

Firmware für einen ESP8266, der über die optische Schnittstelle (Optolink) einer Viessmann-Heizungsregelung
**Vitotronic (Gerätekennung VScotHO1)** Messwerte, Betriebszustände und Störmeldungen ausliest
und an einen **MQTT-Broker** sendet. Fehlercodes werden dabei in deutsche Klartext-Meldungen übersetzt.
Zusätzlich gibt es eine Weboberfläche mit allen Live-Werten, Status und Log sowie Firmware-Updates per WLAN (OTA).

Der Zugriff ist **rein lesend**: Die Firmware schreibt keine Werte in die Heizung.

---

## Inhalt

- [Funktionsüberblick](#funktionsüberblick)
- [Hardware](#hardware)
- [Software-Aufbau](#software-aufbau)
- [Ablauf im Detail](#ablauf-im-detail)
- [Ausgelesene Datenpunkte und MQTT-Topics](#ausgelesene-datenpunkte-und-mqtt-topics)
- [Störmeldungen](#störmeldungen)
- [Weboberfläche](#weboberfläche)
- [Einrichtung](#einrichtung)
- [Bauen und Flashen](#bauen-und-flashen)
- [Bekannte Einschränkungen und Hinweise](#bekannte-einschränkungen-und-hinweise)

---

## Funktionsüberblick

| Funktion | Details |
|---|---|
| Heizung auslesen | Viessmann **KW-Protokoll** über Optolink, 4800 Baud, 8E2, Bibliothek [VitoWiFi](https://github.com/bertmelis/VitoWiFi) 1.x |
| Abfrageintervall | alle **60 Sekunden** werden alle definierten Datenpunkte gelesen (plus einmal direkt nach dem Start); ein Durchlauf dauert ca. 1 s |
| MQTT | jeder gelesene Wert wird unter `optolink/<Name>` veröffentlicht ([PubSubClient](https://github.com/knolleary/pubsubclient)) |
| MQTT-Verbindung | Prüfung und ggf. Neuverbindung alle **10 Sekunden**, Fehlerursache im Log |
| WLAN | automatische Neuverbindung, kein Energiesparmodus, Neustart nach **5 Minuten** ohne WLAN |
| Uhrzeit | per NTP vom Router, Zeitzone Deutschland mit Sommerzeit (für die Log-Zeitstempel) |
| Störmeldungen | die letzten 3 Einträge der Fehlerhistorie werden dekodiert (Zeitstempel + Klartext) |
| Verbindungsstatus | `optolink/error`: `1`, wenn ein ganzer Zyklus ohne Antwort der Heizung bleibt, sonst `0` |
| Weboberfläche | Port 80: Live-Werte mit Zeitstempel, Status, Log, Neustart; JSON-Schnittstelle `/api/status` |
| OTA-Update | ArduinoOTA, Hostname `optolink` |
| Status-LED | blinkt, solange keine WLAN-Verbindung besteht |

---

## Hardware

- **Mikrocontroller:** ESP8266-Board (in `platformio.ini` als `nodemcuv2` konfiguriert, die Umgebung heißt `d1_mini`;
  beide Boards haben ein ESP-12-Modul mit 4 MB Flash).
- **Optolink-Adapter:** Selbstbau, Plan in [adapter.fzz](adapter.fzz) (Fritzing).
  Laut Plan: 4-polige Buchsenleiste (u. a. 3V3, GND, TX), eine Infrarot-LED (880 nm) mit 180-Ω-Vorwiderstand
  als Sender und ein optischer Empfänger mit 10-kΩ-Widerstand. Die Details zur Beschaltung bitte im Fritzing-Plan prüfen.
- **Anschluss:** Der Adapter hängt an der **Hardware-UART** des ESP8266 (`Serial`, GPIO1/TX und GPIO3/RX).
  Das ist dieselbe Schnittstelle, die auch der USB-Seriell-Wandler des Boards nutzt (siehe Hinweise unten).

---

## Software-Aufbau

```
OptoLink/
├── platformio.ini          PlatformIO-Konfiguration (Board, Bibliotheken, Upload-Port)
├── patch_vitowifi.py       Pre-Build-Skript: macht VitoWiFi 1.x mit aktuellem GCC kompilierbar
├── adapter.fzz             Fritzing-Plan des Optolink-Adapters
├── include/
│   ├── secrets.example.h   Vorlage für Zugangsdaten (wird eingecheckt)
│   ├── secrets.h           echte Zugangsdaten (NICHT eingecheckt, .gitignore)
│   ├── viessmann.h         Deklarationen, Live-Werte, eigener Datentyp DPPercent
│   ├── CycleMonitor.h      Bewertung der Abfragezyklen für optolink/error (ohne Arduino, nativ testbar)
│   ├── mqtt.h              MQTT-Funktionen, Topic-Präfix
│   ├── WebServer.h         Webserver-Schnittstellen
│   ├── WebUi.h             Startseite (HTML/CSS/JS, im Flash)
│   └── Logging.h
├── src/
│   ├── main.cpp            setup()/loop(), WLAN, OTA, Timer
│   ├── viessmann.cpp       Datenpunkt-Definitionen, Callbacks, Fehlercode-Übersetzung
│   ├── mqtt.cpp            Broker-Verbindung, Publish
│   ├── WebServer.cpp       HTTP-Handler, JSON-Schnittstelle
│   └── Logging.cpp         Ringpuffer für Log-Meldungen
├── test/test_cycle/        Unit-Tests für CycleMonitor (pio test -e native)
├── .scratch/               lokale Tickets (siehe docs/agents/issue-tracker.md)
└── lib/                    lokal mitgelieferte Bibliotheken
    ├── Time/               TimeLib (Zeitfunktionen für Log-Zeitstempel)
    └── Timer-master/       Timer-Bibliothek (Simon Monk / Jack Christensen) für periodische Aufrufe
```

Externe Bibliotheken (lädt PlatformIO automatisch): `bertmelis/VitoWiFi@^1.0.2` (aktuell 1.1.3), `knolleary/PubSubClient@^2.8`.

---

## Ablauf im Detail

### Start (`setup()` in [src/main.cpp](src/main.cpp))

1. `Serial` wird mit 4800 Baud geöffnet, die LED als Ausgang gesetzt.
2. **WLAN:** Station-Modus ohne Energiesparen, automatische Neuverbindung, Hostname `OptoLink`, Zugangsdaten aus `secrets.h`.
   Bis die Verbindung steht, blinkt die LED (200 ms an/aus). Klappt das 5 Minuten lang nicht, startet der ESP neu.
   Im Betrieb prüft `checkWifi()` alle 10 s die Verbindung; nach 5 Minuten ohne WLAN folgt ebenfalls ein Neustart.
   Die Zugangsdaten werden dabei nie gelöscht.
3. **OTA** wird mit dem Hostnamen `optolink` gestartet (ohne Passwort).
4. **NTP** (`setupNtp()`): Zeitserver ist der Router (Gateway-Adresse), Zeitzone `CET-1CEST,M3.5.0,M10.5.0/3`.
   Der ESP8266-Core gleicht die Zeit danach selbstständig regelmäßig ab. Bis zur ersten Antwort zeigt das Log `01.01.`.
5. **MQTT:** Broker-Adresse setzen, sofort verbinden, danach alle 10 s die Verbindung prüfen.
   Client-ID: `optolink`, Benutzer und Passwort aus `secrets.h`.
6. Die eigene IP-Adresse wird geloggt, dann startet der **Webserver** auf Port 80.
7. Zusätzlich wird ein TCP-Server auf **Port 81** geöffnet. Er war für eine WLAN-Seriell-Brücke gedacht
   (`wifiSerialLoop()`), die aber auskommentiert ist. Der Port ist also offen, tut aber nichts.
8. **VitoWiFi** wird eingerichtet (`setupVito()`): Die Bibliothek stellt `Serial` auf 4800 Baud, 8E2 um,
   registriert alle Datenpunkte und hängt die Callbacks an.
9. Alle 60 s wird `getValues()` aufgerufen. Es läuft einmal sofort, protokolliert den freien Heap und stößt
   `VitoWiFi.readAll()` an. Ist die Warteschlange vom vorigen Zyklus noch nicht leer (Heizung antwortet nicht,
   5 s Timeout je Datenpunkt), wird nichts neu eingereiht.

### Hauptschleife (`loop()`)

Bei jedem Durchlauf werden der Reihe nach bedient: Webserver-Anfragen, OTA, MQTT-Client, die VitoWiFi-Warteschlange
(`loopVito()`: sendet Anfragen an die Heizung, verarbeitet Antworten und schließt einen fertigen Zyklus ab) und der Timer.

### Verbindungsstatus `optolink/error`

Jede Antwort der Heizung wird gezählt. Ein Zyklus endet, wenn die Warteschlange leer ist, spätestens aber mit dem
nächsten `getValues()`. Gab es im Zyklus **keine einzige** Antwort, wird `error = 1` gesendet, sonst `0`.
Gesendet wird nur bei einer Änderung, nach dem ersten Zyklus und nach jeder MQTT-Neuverbindung.
Ein Ausfall fällt damit nach spätestens etwa 2 Minuten auf. Einzelne fehlgeschlagene Datenpunkte erscheinen nur im Web-Log.

### Vom Datenpunkt zur MQTT-Nachricht

`readAll()` stellt alle definierten Datenpunkte in eine Warteschlange. VitoWiFi arbeitet sie nacheinander ab.
Sobald eine Antwort der Heizung da ist, ruft die Bibliothek **entweder** den eigenen Callback des Datenpunkts **oder**,
falls keiner gesetzt ist, den globalen Callback auf:

| Callback | Verwendet für | MQTT-Payload |
|---|---|---|
| `tempCallbackHandler` | ausgewählte Temperaturen (siehe Tabelle) | Gleitkommazahl mit **2** Nachkommastellen, z. B. `21.30` |
| `stoerungsmeldungCallbackHandler` | Störmeldungen 1–3 | `TT.MM.JJ hh:mm:ss Uhr#<Meldung>#<Code hex>` |
| `globalCallbackHandler` | alle übrigen Datenpunkte | Textdarstellung je nach Typ (siehe unten), max. 14 Zeichen |

Jede Veröffentlichung wird zusätzlich ins Log geschrieben.

---

## Ausgelesene Datenpunkte und MQTT-Topics

Topic = `optolink/` + Name (Präfix in [include/mqtt.h](include/mqtt.h), `topic_prefix`).
Nachrichten werden **ohne Retain-Flag** gesendet.
Zusätzlich zu den Datenpunkten gibt es `optolink/error` (`0`/`1`, siehe [Verbindungsstatus](#verbindungsstatus-optolinkerror)).

**Datentypen (VitoWiFi):**

| Typ | Bytes | Umrechnung | Payload beim globalen Callback |
|---|---|---|---|
| `DPTemp` | 2 | Wert / 10 | `21.3` (1 Nachkommastelle) |
| `DPTempS` / `DPMode` | 1 | Ganzzahl ohne Vorzeichen | `20` |
| `DPStat` | 1 | Boolesch | `true` / `false` |
| `DPCount` | 4 | Ganzzahl ohne Vorzeichen | `123456` |
| `DPHours` | 4 | Sekunden / 3600 | `1234.5` |
| `DPCoP` | 1 | Wert / 10 | `1.4` |
| `DPRaw` | n | Rohbytes | Hex-String |

**Datenpunkte** (Adresse = Speicheradresse in der Vitotronic):

| Topic-Name | Adresse | Typ | Callback | Bedeutung (laut Kommentaren/Namen) |
|---|---|---|---|---|
| `AußenTemp` | 0x0800 | DPTemp | temp | Außentemperatur Sensor 1 |
| `KesselIstTemp` | 0x0802 | DPTemp | temp | Kesseltemperatur Sensor 3 |
| `WarmwasserIstTemp` | 0x0804 | DPTemp | temp | Warmwasser-Ist |
| `AbgasTempOderRuecklauf` | 0x0808 | DPTemp | temp | Abgas- oder Rücklauftemperatur (unsicher) |
| `rücklauftemperatur17A` | 0x080A | DPTemp | global | Rücklauftemperatur (17A) |
| `vorlauftemperatur` | 0x080C | DPTemp | global | Vorlauftemperatur (17B) |
| `boilerTemp` | 0x0810 | DPTemp | temp | Kesseltemperatur Tiefpass |
| `LadespeicherObenTemp` | 0x0812 | DPTemp | temp | Speicher oben |
| `LadespeicherUntenTemp` | 0x0814 | DPTemp | temp | Speicher unten |
| `abgastemperaturTiefpass` | 0x0816 | DPTemp | temp | Abgastemperatur Tiefpass |
| `x0818` | 0x0818 | DPTemp | global | Rücklauftemperatur Tiefpass? (unbekannt) |
| `heizkreisRaumtemperatur` | 0x0896 | DPTemp | global | Raumtemperatur A1M1 Tiefpass |
| `raumtemperaturSoll` | 0x2306 | DPTempS | global | Raumtemperatur-Soll normal |
| `redRaumtemperaturSoll` | 0x2307 | DPTempS | global | Raumtemperatur-Soll reduziert |
| `partyTemperaturSoll` | 0x2308 | DPTempS | global | Party-Temperatur-Soll |
| `ferienbetrieb2` | 0x2303 | DPMode | global | Ferienbetrieb |
| `Betriebsart2` | 0x2323 | DPMode | global | Betriebsart |
| `currentOperatingMode` | 0x2500 | DPMode | global | aktuelle Betriebsart A1M1 (0 = Standby, 1 = reduziert, 2/3 = normal) |
| `ferienbetrieb` | 0x2535 | DPMode | global | Ferienbetrieb |
| `VorlaufSollTemp2` | 0x2544 | DPTemp | temp | Vorlauf-Soll |
| `heizkennlinieNeigung` | 0x27D3 | DPCoP | global | Neigung der Heizkennlinie |
| `heizkennlinieNiveau` | 0x27D4 | DPMode | global | Niveau der Heizkennlinie |
| `x2900` | 0x2900 | DPTemp | global | Vorlauftemperatur-Ist? (unbekannt) |
| `a1m1Pump` | 0x2906 | DPStat | global | Heizkreispumpe A1M1 |
| `Sparbetrieb` | 0x3302 | DPMode | global | Sparbetrieb |
| `Betriebsart` | 0x3323 | DPMode | global | Betriebsart |
| `VorlaufSollTemp` | 0x3544 | DPTemp | global | Vorlauf-Soll (zweiter Heizkreis?) |
| `VorlaufIstTemp` | 0x3900 | DPTemp | global | Vorlauf-Ist (zweiter Heizkreis?) |
| `aussenTemperaturTiefpass` | 0x5525 | DPTemp | temp | Außentemperatur Tiefpass |
| `aussenTemperaturGedaempft` | 0x5527 | DPTemp | temp | Außentemperatur gedämpft |
| `KesselSollTemp` | 0x555A | DPTemp | temp | Kessel-Soll |
| `WarmwasserSollTemp` | 0x6300 | DPTempS | global | Warmwasser-Soll |
| `Nachladeunterdrückung` | 0x6551 | DPMode | global | Solar: Nachladeunterdrückung |
| `Solarpumpe` | 0x6552 | DPMode | global | Solar: Pumpe |
| `Wärmemenge` | 0x6560 | DPCount | global | Solar: Wärmemenge |
| `KollektorTemp` | 0x6564 | DPTemp | global | Solar: Kollektortemperatur |
| `Betriebsstunden` | 0x6568 | DPHours | global | Solar: Betriebsstunden |
| `Stoerungsmeldung1` | 0x7507 | DPRaw (9 Byte) | stoerung | neuester Eintrag der Fehlerhistorie |
| `Stoerungsmeldung2` | 0x7510 | DPRaw (9 Byte) | stoerung | 2. Eintrag |
| `Stoerungsmeldung3` | 0x7519 | DPRaw (9 Byte) | stoerung | 3. Eintrag |

Weitere Adressen (Brennerstarts, Brennerlaufzeit, Störmeldungen 4–10 usw.) sind in
[src/viessmann.cpp](src/viessmann.cpp) auskommentiert und lassen sich bei Bedarf aktivieren.
Ein Datenpunkt wird gelesen, sobald er als globales Objekt definiert ist. Ohne eigenen Callback landet er
automatisch beim globalen Callback.

---

## Störmeldungen

Ein Eintrag der Fehlerhistorie besteht aus 9 Bytes, die VitoWiFi als Hex-String liefert, z. B.:

```
9e 20 22 05 15 07 13 36 25
│  └──┬──┘ │  │  │  │  │  └ Sekunde
│     │    │  │  │  │  └─── Minute
│     │    │  │  │  └────── Stunde
│     │    │  │  └───────── Wochentag (wird ignoriert)
│     │    │  └──────────── Tag
│     │    └─────────────── Monat
│     └──────────────────── Jahr (BCD, 2022)
└────────────────────────── Fehlercode (hex)
```

Daraus wird die MQTT-Nachricht `15.05.22 13:36:25 Uhr#Solar: Zu geringer oder kein Volumenstrom …#9e`.

Die Übersetzung erledigt `getErrorMessage()` in [src/viessmann.cpp](src/viessmann.cpp). Die Tabelle deckt die
gängigen Codes der Vitotronic 200 HO1 ab (Sensor-Kurzschluss/-Unterbrechung, Solar, Kommunikation, Brenner,
Feuerungsautomat …). Code `0x00` bedeutet „Regelbetrieb (kein Fehler)“, unbekannte Codes erscheinen als „Unbekannter Fehler“.

---

## Weboberfläche

Erreichbar unter `http://optolink/` oder `http://optolink.local/`.

Die Startseite ([include/WebUi.h](include/WebUi.h)) ist eine einzelne Seite ohne externe Abhängigkeiten
(funktioniert also auch ohne Internet), passt sich hell/dunkel der Systemeinstellung an und läuft auch auf dem Handy:

- **Statusleiste:** Uhrzeit des Boards, Verbindung zur Heizung (`error`), letzte und nächste Abfrage,
  MQTT-Status, WLAN-Pegel, freier Heap, Knopf **Neustart** (mit Sicherheitsabfrage)
- **Live-Werte** gruppiert nach Temperaturen, Zuständen & Betriebsarten, Zählern und Störmeldungen,
  mit Einheit (`°C`, `Std.`) und dem Zeitpunkt der letzten erfolgreichen Abfrage je Wert.
  Werte, die älter als zwei Abfrageintervalle sind, werden gelb hervorgehoben.
- **Log** (einklappbar)
- Die Seite lädt ihre Daten ca. 3 s nach jeder Abfrage neu, die Uhr läuft im Browser weiter.

Gruppe und Einheit ergeben sich aus dem zweiten Parameter der Datenpunkt-Definition in
[src/viessmann.cpp](src/viessmann.cpp) (`temperature`, `state`, `counter`, `hours`, `error`).

| Pfad | Methode | Funktion |
|---|---|---|
| `/` | GET | Startseite |
| `/api/status` | GET | JSON: Uhrzeit, Laufzeit, Heap, WLAN, MQTT, `error`, Intervall, letzte/nächste Abfrage, alle Werte mit Gruppe und Zeitstempel |
| `/api/logs` | GET | Log als Text |
| `/api/restart` | POST | startet den ESP neu (ein GET bewirkt nichts) |
| `/api/scan/start` | POST | Scan-Modus: reguläre Abfrage pausiert (endet 10 Minuten nach der letzten Scan-Anfrage) |
| `/api/scan/stop` | POST | Scan-Modus beenden |
| `/api/raw?list=0x0800:2,0x0802:2,…` | GET | bis zu 16 Adressen roh lesen (Länge 1..9), nur im Scan-Modus; liefert `{"id":n}` |
| `/api/raw/result` | GET | Ergebnis der letzten `/api/raw`-Anfrage: je Adresse Zustand und Rohbytes als Hex |

Das Log ist ein Ringpuffer mit den letzten **120** Meldungen im RAM, nach einem Neustart ist es leer.
Ein voller Abfragezyklus braucht ca. 90 Zeilen. Seite, JSON und Log werden stückweise gesendet, damit kein großer
String den Heap fragmentiert.

---

## Datenpunkte ermitteln (Scan)

Welche Adressen die Heizung wirklich liefert und mit welchem Typ, lässt sich mit
[tools/datenpunkte/datenpunkte.py](tools/datenpunkte/datenpunkte.py) prüfen. Der Scan **liest nur**, er schreibt nie.

```
python tools/datenpunkte/datenpunkte.py kandidaten <adressliste.xlsx>   # Kandidaten sammeln
python tools/datenpunkte/datenpunkte.py scan                            # an der Heizung lesen (Sekunden)
python tools/datenpunkte/datenpunkte.py auswerten                       # Excel-Liste erzeugen
```

- **Kandidaten:** alle Adressen aus der Excel-Adressliste (Blatt „Tabelle1“ mit allen Modellblöcken, Blatt
  „Addressen“ mit eigenen Tests) und die aktiven Datenpunkte aus [src/viessmann.cpp](src/viessmann.cpp).
- **Scan:** liest über `/api/raw` je 16 Adressen in einem Rutsch. Das ist wichtig: Einzeln gesendete Anfragen müssen
  beim KW-Protokoll jeweils ca. 2 s auf das Sync-Signal der Heizung warten, gemeinsam eingereiht laufen sie direkt
  hintereinander. Ein abgebrochener Scan wird beim nächsten Aufruf fortgesetzt (`--neu` beginnt von vorn).
- **Auswerten:** deutet jeden Rohwert nach dem Typ der Quellen und in gängigen Varianten (u8, s8, u16, s16/10,
  u16/100, u32), bewertet die Plausibilität (plausibel / fraglich / Fehlwert, z. B. `FF…` = nicht belegt) und schreibt
  eine Excel-Liste. Die Spalten „Genutzt in ioBroker“, „Bestätigter Typ“ und „Kommentar“ sind für eigene Angaben und
  bleiben bei jeder Neuauswertung erhalten.

Alle Ausgaben enthalten echte Messwerte und landen deshalb in `docs/datenpunkte/lokal/`, das per `.gitignore` nicht
eingecheckt wird. Abhängigkeit: `openpyxl` (`pip install openpyxl`).

**Hinweis zum KW-Protokoll:** Praktisch jede Adresse liefert irgendwelche Bytes. „Antwortet“ heißt also nicht
„existiert“; ob ein Datenpunkt sinnvoll ist, zeigt nur die Plausibilität und am sichersten ein Vorher/Nachher-Vergleich
(Wert am Bedienteil ändern, erneut lesen).

---

## Einrichtung

### 1. Werkzeuge

- [VS Code](https://code.visualstudio.com/) mit der Erweiterung **PlatformIO IDE**
- oder nur PlatformIO Core (`pio`) auf der Kommandozeile

Beim ersten Build lädt PlatformIO die ESP8266-Plattform, die Toolchain und die Bibliotheken automatisch herunter.

### 2. Zugangsdaten

Die Zugangsdaten stehen nicht im Quellcode, sondern in `include/secrets.h`. Diese Datei ist in `.gitignore` eingetragen
und wird nicht eingecheckt.

```
include/secrets.example.h  →  kopieren nach  include/secrets.h
```

und die Werte eintragen:

| Konstante | Bedeutung |
|---|---|
| `WIFI_SSID`, `WIFI_PW` | WLAN-Name und -Passwort |
| `MQTT_HOST`, `MQTT_PORT` | IP/Hostname und Port des MQTT-Brokers |
| `MQTT_USER`, `MQTT_PW` | Zugangsdaten für den Broker |

Fehlt `secrets.h`, bricht der Build mit „secrets.h: No such file or directory“ ab.

### 3. Tests

Die Logik für `optolink/error` hat Unit-Tests, die ohne Hardware auf dem PC laufen:

```
pio test -e native
```

Dafür braucht es einen C++-Compiler auf dem PC (unter Windows z. B. MinGW-w64/GCC im `PATH`).

### 4. Optional anpassen

- **Topic-Präfix:** `topic_prefix` in [include/mqtt.h](include/mqtt.h) (Standard `optolink/`)
- **Abfrageintervall:** `t.every(60000, getValues)` in [src/main.cpp](src/main.cpp) (Millisekunden)
- **Datenpunkte:** in [src/viessmann.cpp](src/viessmann.cpp) ein- oder auskommentieren

---

## Bauen und Flashen

Upload-Weg und serieller Monitor sind in [platformio.ini](platformio.ini) eingestellt:

```ini
monitor_speed = 4800
upload_protocol = espota        ; OTA über WLAN (Standard)
upload_port = optolink.local
#upload_protocol = esptool      ; USB als Rückfall
#upload_port = COM8
```

| Aktion | VS Code (Statusleiste unten) | Kommandozeile |
|---|---|---|
| Bauen | ✓ (Build) | `pio run` |
| Flashen | → (Upload) | `pio run -t upload` |
| Serieller Monitor | 🔌 | `pio device monitor` |

**Per WLAN (OTA, Standard):** Das Board muss als `optolink.local` erreichbar sein. Beim Upload baut der ESP eine
TCP-Verbindung zurück zum PC auf. Deshalb muss die Windows-Firewall eingehende Verbindungen für das Python von
PlatformIO (`%USERPROFILE%\.platformio\penv\Scripts\python.exe`) erlauben. Sonst endet der Upload mit „No Answer“.

**Per USB (Rückfall):** In `platformio.ini` die beiden OTA-Zeilen aus- und die beiden USB-Zeilen einkommentieren.
Board an `COM8` (Port im Geräte-Manager prüfen und ggf. anpassen). Den Optolink-Adapter vorher abziehen, weil er an
denselben RX/TX-Leitungen hängt wie der USB-Seriell-Wandler.

---

## Bekannte Einschränkungen und Hinweise

- **Keine seriellen Ausgaben auf der UART.** Die UART ist die Optolink-Leitung zur Heizung. Früher schrieb `Log()` dort
  hinein, die Heizung verschluckte sich daran, und nur jeder zweite Datenpunkt kam an. Deshalb gehen Log-Ausgaben nur noch
  in den Log-Puffer der Weboberfläche. **Keine neuen `Serial.print`-Aufrufe einbauen!**
- **Fehler beim Lesen** meldet VitoWiFi im Web-Log als `VitoWiFi: DP <Name> error: …` (Filter `ErrorLogPrint` in
  [src/Logging.cpp](src/Logging.cpp)). Ein Durchlauf aller Datenpunkte dauert etwa eine Sekunde.
- **Serieller Monitor:** Über die UART laufen nur Protokoll-Bytes (8E2). Für die Diagnose ist die Log-Seite im Browser gedacht.
- **Uhrzeit:** Bis die erste NTP-Antwort da ist (normalerweise wenige Sekunden nach dem Start), zeigen die Log-Zeitstempel
  `01.01.`. Antwortet der Router nicht als Zeitserver, bleibt das so. Die Zeitstempel der Störmeldungen kommen aus der Heizung.
- **MQTT-Verbindungsaufbau blockiert:** Ist der Broker nicht erreichbar, kann jeder Verbindungsversuch (alle 10 s) die Hauptschleife
  einige Sekunden lang aufhalten (Weboberfläche und OTA reagieren dann verzögert).
- **Keine Absicherung:** Weboberfläche (inkl. Neustart) und OTA-Update sind ohne Passwort erreichbar. Das Gerät gehört nur in ein
  vertrauenswürdiges Netz.
- **Port 81** ist geöffnet, aber ohne Funktion (Rest der deaktivierten WLAN-Seriell-Brücke).
- **MQTT:** kein Retain, kein Last Will. Ist der Broker beim Lesen nicht verbunden, gehen die Werte dieses Zyklus verloren.
  Die Neuverbindung erfolgt spätestens nach 10 s.
- **Umlaute in Topics** (`AußenTemp`, `Wärmemenge`, `Nachladeunterdrückung`, `rücklauftemperatur17A`): Erlaubt (UTF-8),
  manche Werkzeuge kommen damit aber schlecht zurecht.
- **Unsichere Adressen:** Einige Datenpunkte (`x2900`, `x0818`, `AbgasTempOderRuecklauf`, 0x35xx/0x39xx) sind
  laut Kommentaren nicht sicher zugeordnet.
- **VitoWiFi 1.x und neuer Compiler:** VitoWiFi 1.1.3 kompiliert mit der aktuellen ESP8266-Plattform nicht ohne Anpassung
  (abweichender Rückgabetyp `const size_t getLength()`). Das Skript [patch_vitowifi.py](patch_vitowifi.py) korrigiert die
  heruntergeladene Bibliothek vor jedem Build automatisch.
- `DPPercent` (eigener Typ in [include/viessmann.h](include/viessmann.h)) ist definiert, wird aber derzeit nicht verwendet.
