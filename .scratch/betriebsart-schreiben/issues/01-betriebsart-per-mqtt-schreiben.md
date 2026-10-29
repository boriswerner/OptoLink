# Betriebsart per MQTT schreiben

Status: needs-triage

## Ziel

Die Betriebsart der Heizung soll sich von ioBroker aus per MQTT umschalten lassen
(0 = Abschaltbetrieb, 1 = nur Warmwasser, 2 = Heizen + Warmwasser).

## Vorlage im Original-Repo (empty88/OptoLink)

- Topic `<prefix>set_betriebsart`, Payload `0`/`1`/`2`; Callback in `mqtt.cpp` ruft `setOperationMode()`
- `setOperationMode()` in `viessmann.cpp`: `VitoWiFi.writeDatapoint(betriebsart, DPValue(value))`
- Datenpunkte werden mit `setWriteable(true)` beschreibbar gemacht (dort: Betriebsart 0x3323,
  Sparbetrieb 0x3302, Partybetrieb 0x3303)
- Weboberfläche dort mit Knöpfen `/setOff`, `/setWW`, `/setH-WW`

## Offene Punkte vor der Umsetzung

- Das Original nutzt P300, wir KW. `OptolinkKW` kann schreiben (`writeToDP`, Befehl 0xF4),
  das ist an dieser Anlage aber noch nie getestet worden.
- Welche Adresse ist die richtige? Lokal werden `Betriebsart` (0x3323) und `Betriebsart2` (0x2323)
  gelesen, beide liefern aktuell `2`.
- Absicherung: nur gültige Werte annehmen, nach dem Schreiben zurücklesen und im Log protokollieren.
- Test am Gerät im Dauerbetrieb: Zeitpunkt abstimmen, Rückweg (alten Wert wieder schreiben) vorher klären.

## Comments
