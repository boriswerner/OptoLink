#include <unity.h>
#include "CycleMonitor.h"

void setUp() {}
void tearDown() {}

void test_erster_zyklus_mit_antwort_meldet_0() {
	CycleMonitor m;
	m.onSuccess();
	TEST_ASSERT_EQUAL(0, m.finishCycle());
	TEST_ASSERT_EQUAL(0, m.state());
}

void test_erster_zyklus_ohne_antwort_meldet_1() {
	CycleMonitor m;
	TEST_ASSERT_EQUAL(1, m.finishCycle());
}

void test_gleicher_zustand_wird_nicht_erneut_gemeldet() {
	CycleMonitor m;
	m.onSuccess();
	m.finishCycle();
	m.onSuccess();
	TEST_ASSERT_EQUAL(CycleMonitor::NOTHING, m.finishCycle());
	TEST_ASSERT_EQUAL(1, m.finishCycle());
	TEST_ASSERT_EQUAL(CycleMonitor::NOTHING, m.finishCycle());
}

void test_nach_ausfall_meldet_erste_antwort_0() {
	CycleMonitor m;
	m.finishCycle();	// Ausfall -> 1
	m.onSuccess();
	TEST_ASSERT_EQUAL(0, m.finishCycle());
}

void test_eine_antwort_reicht_fuer_0() {
	CycleMonitor m;
	m.finishCycle();
	m.onSuccess();	// nur 1 von 40 Datenpunkten
	TEST_ASSERT_EQUAL(0, m.finishCycle());
}

void test_zaehler_wird_je_zyklus_zurueckgesetzt() {
	CycleMonitor m;
	m.onSuccess();
	m.onSuccess();
	m.finishCycle();
	TEST_ASSERT_EQUAL(1, m.finishCycle());	// neuer Zyklus ohne Antwort
}

void test_vor_erstem_zyklus_kein_zustand() {
	CycleMonitor m;
	TEST_ASSERT_EQUAL(CycleMonitor::NOTHING, m.state());
}

int main() {
	UNITY_BEGIN();
	RUN_TEST(test_erster_zyklus_mit_antwort_meldet_0);
	RUN_TEST(test_erster_zyklus_ohne_antwort_meldet_1);
	RUN_TEST(test_gleicher_zustand_wird_nicht_erneut_gemeldet);
	RUN_TEST(test_nach_ausfall_meldet_erste_antwort_0);
	RUN_TEST(test_eine_antwort_reicht_fuer_0);
	RUN_TEST(test_zaehler_wird_je_zyklus_zurueckgesetzt);
	RUN_TEST(test_vor_erstem_zyklus_kein_zustand);
	return UNITY_END();
}
