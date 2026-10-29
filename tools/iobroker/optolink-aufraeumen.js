////////////////////////////////////////////////
// Script Name: optolink-aufraeumen
// Version: 0.1
// Creation Date: 29.10.2026
//
// Description: Räumt die MQTT-Zustände der OptoLink-Firmware in ioBroker auf.
//   1. Löscht Zustände unter mqtt.0.optolink, die die Firmware nicht mehr sendet.
//      Die Historie in InfluxDB bleibt erhalten (Measurements werden nicht angefasst).
//   2. Setzt Klarname, Einheit und Rolle an den gesendeten Zuständen.
//      IDs bleiben gleich, InfluxDB, Grafana, vis2 und Skripte sind nicht betroffen.
//   3. Legt Aliase unter alias.0.Heizung an (nur lesend), z. B. alias.0.Heizung.kessel.temperatur.
//
// Voraussetzungen:
//   - Firmware mit allen 44 Topics ist eingespielt und hat mindestens einen Zyklus gesendet
//     (sonst fehlen die neuen Zustände und es werden keine Aliase für sie angelegt).
//   - In der javascript-Instanz ist "setObject erlauben" (enableSetObject) aktiviert.
//   - Vorher mqtt.0.optolink als Sicherung exportieren (Objekte → Ordner → Export).
//
// Ablauf: mit PROBELAUF = true starten und das Log prüfen, dann PROBELAUF = false und erneut starten.
// Das Skript kann danach gestoppt werden; es läuft nur einmal beim Start.
////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////

const PROBELAUF = true;                 // true: nur ins Log schreiben, nichts ändern
const CFG_QUELLE = 'mqtt.0.optolink';
const CFG_ALIAS = 'alias.0.Heizung';
const CFG_LOESCHEN = true;              // Schritt 1
const CFG_NAMEN = true;                 // Schritt 2
const CFG_ALIASE = true;                // Schritt 3

// Topic -> Klarname, Einheit, Rolle, Alias-Pfad (unter CFG_ALIAS), optional Wertetexte und Umrechnung
const DATENPUNKTE = [
    { id: 'AußenTemp',                 name: 'Außentemperatur',                          unit: '°C', role: 'value.temperature', alias: 'aussen.temperatur' },
    { id: 'aussenTemperaturTiefpass',  name: 'Außentemperatur Tiefpass',                 unit: '°C', role: 'value.temperature', alias: 'aussen.temperatur_tiefpass' },
    { id: 'aussenTemperaturGedaempft', name: 'Außentemperatur gedämpft',                 unit: '°C', role: 'value.temperature', alias: 'aussen.temperatur_gedaempft' },

    { id: 'KesselIstTemp',             name: 'Kesseltemperatur',                         unit: '°C', role: 'value.temperature', alias: 'kessel.temperatur' },
    { id: 'boilerTemp',                name: 'Kesseltemperatur Tiefpass',                unit: '°C', role: 'value.temperature', alias: 'kessel.temperatur_tiefpass' },
    { id: 'KesselSollTemp',            name: 'Kesseltemperatur Soll',                    unit: '°C', role: 'value.temperature', alias: 'kessel.temperatur_soll' },
    { id: 'Umschaltventil',            name: 'Umschaltventil',                           unit: '',   role: 'value',             alias: 'kessel.umschaltventil',
      states: { 0: 'undefiniert', 1: 'Heizen', 2: 'Mittelstellung', 3: 'Warmwasser' } },
    { id: 'Pumpendrehzahl',            name: 'Interne Pumpe Solldrehzahl',               unit: '%',  role: 'value',             alias: 'kessel.pumpe_drehzahl' },

    { id: 'Brennerlaufzeit',           name: 'Brennerlaufzeit',                          unit: 'h',  role: 'value',             alias: 'brenner.laufzeit' },
    { id: 'Brennerstarts',             name: 'Brennerstarts',                            unit: '',   role: 'value',             alias: 'brenner.starts' },
    { id: 'Brennerleistung',           name: 'Brennerleistung',                          unit: '%',  role: 'value',             alias: 'brenner.leistung' },
    { id: 'Brennerstörung',            name: 'Brennerstörung',                           unit: '',   role: 'value',             alias: 'brenner.stoerung' },
    { id: 'Flamme',                    name: 'Flamme (Bedeutung nicht bestätigt)',       unit: '',   role: 'value',             alias: 'brenner.flamme' },

    { id: 'x2900',                     name: 'Vorlauftemperatur Heizkreis 1',            unit: '°C', role: 'value.temperature', alias: 'heizkreis1.vorlauf_temperatur' },
    { id: 'VorlaufSollTemp2',          name: 'Vorlauftemperatur Soll Heizkreis 1',       unit: '°C', role: 'value.temperature', alias: 'heizkreis1.vorlauf_temperatur_soll' },
    { id: 'raumtemperaturSoll',        name: 'Raumtemperatur Soll',                      unit: '°C', role: 'value.temperature', alias: 'heizkreis1.raumtemperatur_soll' },
    { id: 'redRaumtemperaturSoll',     name: 'Raumtemperatur Soll reduziert',            unit: '°C', role: 'value.temperature', alias: 'heizkreis1.raumtemperatur_soll_reduziert' },
    { id: 'partyTemperaturSoll',       name: 'Raumtemperatur Soll Party',                unit: '°C', role: 'value.temperature', alias: 'heizkreis1.raumtemperatur_soll_party' },
    { id: 'heizkennlinieNeigung',      name: 'Heizkennlinie Neigung',                    unit: '',   role: 'value',             alias: 'heizkreis1.heizkennlinie_neigung' },
    // Die Firmware sendet das Niveau als Byte ohne Vorzeichen (247 = -9); der Alias rechnet um
    { id: 'heizkennlinieNiveau',       name: 'Heizkennlinie Niveau (Rohwert, 247 = -9)', unit: '',   role: 'value',             alias: 'heizkreis1.heizkennlinie_niveau',
      aliasName: 'Heizkennlinie Niveau', aliasUnit: 'K', read: 'val > 127 ? val - 256 : val' },
    { id: 'Betriebsart2',              name: 'Betriebsart Heizkreis 1',                  unit: '',   role: 'value',             alias: 'heizkreis1.betriebsart',
      states: { 0: 'Abschaltbetrieb', 1: 'Nur Warmwasser', 2: 'Heizen und Warmwasser', 3: 'Dauernd reduziert', 4: 'Dauernd normal' } },
    { id: 'currentOperatingMode',      name: 'Betriebszustand Heizkreis 1',              unit: '',   role: 'value',             alias: 'heizkreis1.betriebszustand',
      states: { 0: 'Standby', 1: 'Reduziert', 2: 'Normal', 3: 'Normal' } },
    { id: 'Sparbetrieb2',              name: 'Sparbetrieb Heizkreis 1',                  unit: '',   role: 'value',             alias: 'heizkreis1.sparbetrieb' },
    { id: 'ferienbetrieb2',            name: 'Partybetrieb Heizkreis 1',                 unit: '',   role: 'value',             alias: 'heizkreis1.partybetrieb' },
    { id: 'ferienbetrieb',             name: 'Ferienbetrieb Heizkreis 1 (nicht bestätigt)', unit: '', role: 'value',            alias: 'heizkreis1.ferienbetrieb' },
    { id: 'a1m1Pump',                  name: 'Heizkreispumpe Heizkreis 1',               unit: '',   role: 'indicator',         alias: 'heizkreis1.pumpe' },

    { id: 'WarmwasserIstTemp',         name: 'Speichertemperatur oben',                  unit: '°C', role: 'value.temperature', alias: 'warmwasser.speicher_oben' },
    { id: 'LadespeicherObenTemp',      name: 'Speichertemperatur oben Tiefpass',         unit: '°C', role: 'value.temperature', alias: 'warmwasser.speicher_oben_tiefpass' },
    { id: 'LadespeicherUntenRohTemp',  name: 'Speichertemperatur unten',                 unit: '°C', role: 'value.temperature', alias: 'warmwasser.speicher_unten' },
    { id: 'LadespeicherUntenTemp',     name: 'Speichertemperatur unten Tiefpass',        unit: '°C', role: 'value.temperature', alias: 'warmwasser.speicher_unten_tiefpass' },
    { id: 'WarmwasserSollTemp',        name: 'Warmwassertemperatur Soll',                unit: '°C', role: 'value.temperature', alias: 'warmwasser.temperatur_soll' },

    { id: 'AbgasTempOderRuecklauf',    name: 'Abgastemperatur',                          unit: '°C', role: 'value.temperature', alias: 'abgas.temperatur' },
    { id: 'abgastemperaturTiefpass',   name: 'Abgastemperatur Tiefpass',                 unit: '°C', role: 'value.temperature', alias: 'abgas.temperatur_tiefpass' },

    { id: 'Stoerungsmeldung1',         name: 'Störmeldung 1 (neueste)',                  unit: '',   role: 'text',              alias: 'stoerung.meldung_1' },
    { id: 'Stoerungsmeldung2',         name: 'Störmeldung 2',                            unit: '',   role: 'text',              alias: 'stoerung.meldung_2' },
    { id: 'Stoerungsmeldung3',         name: 'Störmeldung 3',                            unit: '',   role: 'text',              alias: 'stoerung.meldung_3' },
    { id: 'Stoerungsmeldung4',         name: 'Störmeldung 4',                            unit: '',   role: 'text',              alias: 'stoerung.meldung_4' },
    { id: 'Stoerungsmeldung5',         name: 'Störmeldung 5',                            unit: '',   role: 'text',              alias: 'stoerung.meldung_5' },
    { id: 'Stoerungsmeldung6',         name: 'Störmeldung 6',                            unit: '',   role: 'text',              alias: 'stoerung.meldung_6' },
    { id: 'Stoerungsmeldung7',         name: 'Störmeldung 7',                            unit: '',   role: 'text',              alias: 'stoerung.meldung_7' },
    { id: 'Stoerungsmeldung8',         name: 'Störmeldung 8',                            unit: '',   role: 'text',              alias: 'stoerung.meldung_8' },
    { id: 'Stoerungsmeldung9',         name: 'Störmeldung 9',                            unit: '',   role: 'text',              alias: 'stoerung.meldung_9' },
    { id: 'Stoerungsmeldung10',        name: 'Störmeldung 10 (älteste)',                 unit: '',   role: 'text',              alias: 'stoerung.meldung_10' },

    { id: 'error',                     name: 'Verbindungsfehler (1 = Heizung antwortet nicht)', unit: '', role: 'indicator.maintenance', alias: 'status.verbindungsfehler' },
];

const BEREICHE = {
    aussen: 'Außen', kessel: 'Kessel', brenner: 'Brenner', heizkreis1: 'Heizkreis 1',
    warmwasser: 'Warmwasser', abgas: 'Abgas', stoerung: 'Störmeldungen', status: 'Status',
};

////////////////////////////////////////////////
// DO NOT CHANGE ANYTHING AFTER THIS LINE
////////////////////////////////////////////////

const SCRIPT_NAME = '[optolink-aufraeumen]';
const PRAEFIX = PROBELAUF ? 'PROBELAUF: ' : '';

function info(text) {
    log(`${SCRIPT_NAME} ${PRAEFIX}${text}`, 'info');
}

function warnung(text) {
    log(`${SCRIPT_NAME} ${PRAEFIX}${text}`, 'warn');
}

async function loescheVeraltete() {
    const aktuell = new Set(DATENPUNKTE.map(d => `${CFG_QUELLE}.${d.id}`));
    const vorhanden = [];
    $(`state[id=${CFG_QUELLE}.*]`).each(id => { vorhanden.push(id); });
    const veraltet = vorhanden.filter(id => !aktuell.has(id)).sort();
    info(`${vorhanden.length} Zustände vorhanden, ${veraltet.length} veraltet:`);
    for (const id of veraltet) {
        const obj = await getObjectAsync(id);
        const influx = obj && obj.common && obj.common.custom && obj.common.custom['influxdb.0'];
        const hinweis = influx && influx.enabled ? ' (mit InfluxDB-Protokollierung, Historie bleibt)' : '';
        info(`  löschen: ${id}${hinweis}`);
        if (!PROBELAUF) {
            await deleteObjectAsync(id);
        }
    }
}

async function setzeNamen() {
    for (const d of DATENPUNKTE) {
        const id = `${CFG_QUELLE}.${d.id}`;
        const obj = await getObjectAsync(id);
        if (!obj) {
            warnung(`  fehlt (noch nicht gesendet?): ${id}`);
            continue;
        }
        const common = { name: d.name, role: d.role, unit: d.unit };
        if (d.states) {
            common.states = d.states;
        }
        info(`  Name: ${id} -> "${d.name}"${d.unit ? ' [' + d.unit + ']' : ''}`);
        if (!PROBELAUF) {
            await extendObjectAsync(id, { common });
        }
    }
}

async function legeAliaseAn() {
    const ordner = { [CFG_ALIAS]: { type: 'device', name: 'Heizung (OptoLink)' } };
    for (const [bereich, name] of Object.entries(BEREICHE)) {
        ordner[`${CFG_ALIAS}.${bereich}`] = { type: 'channel', name };
    }
    for (const [id, o] of Object.entries(ordner)) {
        info(`  Ordner: ${id} ("${o.name}")`);
        if (!PROBELAUF) {
            await setObjectAsync(id, { type: o.type, common: { name: o.name }, native: {} });
        }
    }
    for (const d of DATENPUNKTE) {
        const quelle = `${CFG_QUELLE}.${d.id}`;
        const ziel = `${CFG_ALIAS}.${d.alias}`;
        const obj = await getObjectAsync(quelle);
        if (!obj) {
            warnung(`  Alias übersprungen, Quelle fehlt: ${quelle}`);
            continue;
        }
        const common = {
            name: d.aliasName || d.name,
            type: d.read ? 'number' : obj.common.type,
            role: d.role,
            unit: d.aliasUnit !== undefined ? d.aliasUnit : d.unit,
            read: true,
            write: false,
            alias: { id: quelle },
        };
        if (d.read) {
            common.alias.read = d.read;
        }
        if (d.states) {
            common.states = d.states;
        }
        info(`  Alias: ${ziel} -> ${quelle}${d.read ? ' (Umrechnung: ' + d.read + ')' : ''}`);
        if (!PROBELAUF) {
            await setObjectAsync(ziel, { type: 'state', common, native: {} });
        }
    }
}

// Prüft nach zwei Abfragezyklen, ob der MQTT-Adapter die gesetzten Namen wieder überschrieben hat
function pruefeNamenSpaeter() {
    setTimeout(async () => {
        let ueberschrieben = 0;
        for (const d of DATENPUNKTE) {
            const obj = await getObjectAsync(`${CFG_QUELLE}.${d.id}`);
            const name = obj && obj.common ? obj.common.name : undefined;
            if (obj && name !== d.name) {
                ueberschrieben++;
                warnung(`Name überschrieben: ${CFG_QUELLE}.${d.id} heißt jetzt "${JSON.stringify(name)}"`);
            }
        }
        info(ueberschrieben ? `${ueberschrieben} Namen wurden überschrieben` : 'Namen nach 2 Minuten unverändert');
    }, 2 * 60 * 1000);
}

async function main() {
    info(`Start (${DATENPUNKTE.length} Datenpunkte)`);
    if (CFG_LOESCHEN) {
        info('Schritt 1: veraltete Zustände löschen');
        await loescheVeraltete();
    }
    if (CFG_NAMEN) {
        info('Schritt 2: Klarnamen setzen');
        await setzeNamen();
    }
    if (CFG_ALIASE) {
        info('Schritt 3: Aliase anlegen');
        await legeAliaseAn();
    }
    info('Fertig');
    if (!PROBELAUF && CFG_NAMEN) {
        pruefeNamenSpaeter();
    }
}

main().catch(e => log(`${SCRIPT_NAME} Fehler: ${e}`, 'error'));
