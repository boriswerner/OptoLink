# Datenpunkte über die Weboberfläche konfigurieren

Status: needs-triage

## Ziel

Datenpunkte sollen ohne neues Flashen angelegt, geändert und entfernt werden können:
je Datenpunkt **Adresse**, **Format** (Datentyp, z. B. DPTemp, DPMode, DPRaw mit Länge) und **MQTT-Topic**.

## Ausgangslage

- Heute sind alle Datenpunkte als globale Objekte in `src/viessmann.cpp` fest einprogrammiert
  (Name = Topic, Adresse, Typ, teils eigener Callback, Störmeldungen mit `setLength(9)`).
- VitoWiFi 1.x registriert Datenpunkte im Konstruktor in einer statischen Liste; zur Laufzeit
  anlegen/löschen ist dort nicht vorgesehen und müsste geprüft werden.

## Offene Punkte

- Speicherort der Konfiguration (LittleFS-Datei, z. B. JSON) und Umgang mit Fehlkonfiguration
- Migration: die heutigen 40 Datenpunkte als Standardkonfiguration übernehmen, Topics und Formate
  müssen dabei exakt gleich bleiben (ioBroker)
- Ein Datenpunkt pro Topic oder mehrere Formate pro Adresse (zum Ausprobieren unbekannter Adressen)?
- Absicherung der Konfigurationsseite (die Weboberfläche hat heute kein Passwort)

## Comments
