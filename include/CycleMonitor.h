#pragma once
// Bewertet Abfragezyklen für das MQTT-Topic "error".
// Ohne Arduino-Abhängigkeiten, damit es nativ getestet werden kann (test/test_cycle).

class CycleMonitor {
public:
	static const int NOTHING = -1;	// finishCycle(): nichts zu senden

	// Eine Antwort der Heizung ist eingegangen
	void onSuccess() { _successes++; }

	// Zyklus abschließen. Liefert 1 (keine einzige Antwort), 0 (mindestens eine Antwort)
	// oder NOTHING, wenn sich der Zustand gegenüber der letzten Meldung nicht geändert hat.
	int finishCycle() {
		int state = (_successes > 0) ? 0 : 1;
		_successes = 0;
		if (state == _lastPublished) {
			return NOTHING;
		}
		_lastPublished = state;
		return state;
	}

	// Zuletzt gemeldeter Zustand, NOTHING vor dem ersten abgeschlossenen Zyklus
	int state() const { return _lastPublished; }

private:
	int _successes = 0;
	int _lastPublished = NOTHING;
};
