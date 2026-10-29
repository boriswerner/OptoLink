// OptoLink – Bestandsaufnahme für die Umbenennung der MQTT-Topics (nur lesend).
//
// Im ioBroker-JavaScript-Adapter als neues Skript (JavaScript) anlegen, einmal starten, danach wieder stoppen.
// Ergebnis: Datei optolink-export.json in 0_userdata.0 (Admin -> Dateien -> 0_userdata.0) herunterladen
// und im Projekt unter docs/migration/lokal/ ablegen.
//
// Inhalt:
//  - alle Objekte unter mqtt.0.optolink.* inkl. common.custom (InfluxDB-Einstellungen, z. B. aliasId)
//    und Zeitpunkt der letzten Aktualisierung (zeigt veraltete Zustände)
//  - alle übrigen Zustände mit InfluxDB-Protokollierung (Name, aliasId), um Alias-IDs und eigene
//    Datenpunkte aus Skripten zu erkennen
//  - Quelltext aller Skripte, die "optolink" enthalten
// Es wird nichts verändert.

const PREFIX = 'mqtt.0.optolink.';
const result = { erstellt: new Date().toISOString(), optolink: {}, influxSonstige: {}, skripte: {} };

function view(type, start, end) {
    return new Promise((resolve, reject) => {
        getObjectView('system', type, { startkey: start, endkey: end }, (err, res) => {
            if (err) reject(err); else resolve(res.rows.map(r => r.value));
        });
    });
}

function influxSettings(obj) {
    const custom = (obj.common && obj.common.custom) || {};
    const out = {};
    for (const key of Object.keys(custom)) {
        if (key.startsWith('influxdb.')) out[key] = custom[key];
    }
    return out;
}

(async () => {
    const states = await view('state', '', '香');
    for (const obj of states) {
        const influx = influxSettings(obj);
        if (obj._id.startsWith(PREFIX)) {
            const st = getState(obj._id);
            result.optolink[obj._id] = {
                name: obj.common && obj.common.name,
                type: obj.common && obj.common.type,
                role: obj.common && obj.common.role,
                unit: obj.common && obj.common.unit,
                influx: influx,
                letzteAenderung: st && st.lc ? new Date(st.lc).toISOString() : null,
                letzteAktualisierung: st && st.ts ? new Date(st.ts).toISOString() : null,
            };
        } else if (Object.keys(influx).length) {
            result.influxSonstige[obj._id] = { name: obj.common && obj.common.name, influx: influx };
        }
    }
    const scripts = await view('script', 'script.js.', 'script.js.香');
    for (const obj of scripts) {
        const src = (obj.common && obj.common.source) || '';
        if (/optolink/i.test(src)) {
            result.skripte[obj._id] = { aktiv: obj.common.enabled, engine: obj.common.engineType, quelltext: src };
        }
    }
    writeFile('0_userdata.0', 'optolink-export.json', JSON.stringify(result, null, 1), err => {
        if (err) log('OptoLink-Export fehlgeschlagen: ' + err, 'error');
        else log(`OptoLink-Export: ${Object.keys(result.optolink).length} optolink-Zustände, ` +
                 `${Object.keys(result.influxSonstige).length} weitere mit InfluxDB, ` +
                 `${Object.keys(result.skripte).length} Skripte -> 0_userdata.0/optolink-export.json`);
    });
})().catch(e => log('OptoLink-Export fehlgeschlagen: ' + e, 'error'));
