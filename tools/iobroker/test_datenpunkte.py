"""Prüft, dass das ioBroker-Aufräumskript genau die Topics kennt, die die Firmware sendet.

Aufruf: python -m unittest discover -s tools/iobroker
"""
import os
import re
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def firmware_topics():
    """Namen der aktiven Datenpunkte in viessmann.cpp (Zeilen ohne Kommentar am Anfang) plus error."""
    with open(os.path.join(ROOT, "src", "viessmann.cpp"), encoding="utf-8") as f:
        text = f.read()
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    namen = []
    for zeile in text.splitlines():
        m = re.match(r'\s*DP\w+\s+\w+\("([^"]+)",', zeile)
        if m:
            namen.append(m.group(1))
    return namen + ["error"]


def skript_topics():
    with open(os.path.join(ROOT, "tools", "iobroker", "optolink-aufraeumen.js"), encoding="utf-8") as f:
        return re.findall(r"\{ id: '([^']+)'", f.read())


class Datenpunkte(unittest.TestCase):
    def test_gleiche_topics(self):
        self.assertEqual(sorted(firmware_topics()), sorted(skript_topics()))

    def test_keine_doppelten_alias_pfade(self):
        with open(os.path.join(ROOT, "tools", "iobroker", "optolink-aufraeumen.js"), encoding="utf-8") as f:
            pfade = re.findall(r"alias: '([^']+)'", f.read())
        self.assertEqual(len(pfade), len(set(pfade)))
        self.assertEqual(len(pfade), len(skript_topics()))

    def test_anzahl(self):
        self.assertEqual(len(firmware_topics()), 44)


if __name__ == "__main__":
    unittest.main()
