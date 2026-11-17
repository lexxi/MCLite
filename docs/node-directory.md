# Knotenverzeichnis per WLAN laden

Ab 0.4.6-ap.8 kann MCLite das öffentliche Verzeichnis von
[CarinthiaMesh](https://map.carinthiamesh.com/api/nodes) direkt laden und auf SD
speichern. Nach Neustart steht es ohne Internet zur Verfügung.

## Bedienung am T-Deck

1. Firmware aktualisieren und die neue `sdcard/mclite/lang/de.json` nach
   `/mclite/lang/de.json` auf die SD kopieren. Ohne Aktualisierung der Sprachdatei
   erscheinen neue Beschriftungen auf Englisch.
2. Admin-Menü mit **0** oder dem Zahnrad öffnen, dann **WiFi/WLAN** öffnen.
3. Mit einem bestehenden WLAN mit Internetzugang verbinden. Der **Config-AP**
   ist für Uploads vom PC; er stellt selbst keine Internetverbindung her.
4. Im WLAN-Fenster nach unten scrollen und **Knotenverzeichnis laden** wählen.
   Angezeigt wird die Zahl der bisher geladenen eindeutigen Kennungen.
5. Nach dem vollständigen Abruf folgt **wird auf SD gespeichert**, anschließend
   **Verzeichnis gespeichert**. Erst dann ist der neue Bestand aktiv.
6. Das WLAN kann danach ausgeschaltet werden. Pfad und Karte nutzen den Bestand
   auch offline und nach einem Neustart. Der Download startet niemals automatisch.

Während des Downloads kann man den Bildschirm verlassen. WLAN bleibt nach dem
vorhandenen WLAN-Verhalten an; Status beim erneuten Öffnen prüfen. **Download
abbrechen** stoppt nach der laufenden HTTP-Anfrage; bis zum Timeout kann das einige
Sekunden dauern. WLAN-Abbruch, ungültige Daten oder ein erreichtes Limit ersetzen
den bisherigen Bestand nicht. Firmware-Update und Config-AP sind während eines
laufenden Verzeichnis-Downloads gesperrt, um konkurrierende Netzwerkaktionen zu vermeiden.

## Nutzung und Aussagekraft

- Vollständiger 32-Byte-Public-Key, Name, Typ, letzte Sichtung der Quelle und
  gegebenenfalls Position werden gespeichert. Nicht nur Repeater: Auch andere
  Identitäten müssen bei Kollisionen kurzer Pfadkennungen mitzählen.
- Im Pfaddialog werden die importierten Schlüssel zusätzlich abgeglichen.
  Mehrere vollständige Schlüssel zum selben Präfix bleiben **mehrdeutig**.
  Identitäten ohne GPS zählen bei dieser Prüfung ebenfalls.
- Frische Telemetrie und lokale Adverts haben bei Positionsdaten Vorrang.
  Verzeichnispositionen sind ein Fallback, mit **[DB]** im Pfaddialog.
- Die normale Karte ergänzt im sichtbaren Ausschnitt höchstens 128 Repeater aus
  der Datenbank. **D** bzw. **[DB]** kennzeichnet diese Einträge; sie sind keine
  lokal gehörten Nachbarn. Verschieben/Zoomen baut die Auswahl neu auf.
- Das Verzeichnis lernt keine Funkrouten, erzeugt keine LoRa-Pakete und fügt keine
  Mesh-Kontakte hinzu. Die knappen Kontaktplätze und `config.json` bleiben unberührt.
- Namen/Positionen sind Daten einer externen Karte, keine verifizierten Identitäten,
  kein Empfangsnachweis und keine historischen Positionen des jeweiligen Pakets.
  Bei 1-Byte-Kennungen können durch den größeren Datenbestand mehr Kollisionen
  erkannt werden; eine zuvor scheinbar eindeutige Zuordnung kann dann entfallen.
- Das Kärnten-Kartenpaket auf SD bleibt bestehen. Dieser Download lädt keine PNG-Kacheln.

## Datei und manuelle Pflege

`/mclite/nodes.jsonl` enthält ein JSON-Objekt pro Zeile, zum Beispiel:

```json
{"public_key":"0e6cebecf336e0b7de13ee8be9b4a5eb7050620ba7866563fcc20348310c2973","name":"AT-K-Emmersdorf HB","role":"repeater","lat":46.666591,"lon":14.279462,"last_seen":"2026-11-17T09:06:24Z"}
```

Öffentlicher Beispielschlüssel aus der API; kein privater Schlüssel erforderlich.
`role` ist `companion`, `repeater`, `room`, `sensor` oder `unknown`; `lat` und `lon`
dürfen beide `null` sein. Koordinaten sind geografische Gradwerte, keine
Web-Mercator-Meter. Doppelte vollständige Schlüssel werden zusammengeführt,
die letzte Zeile gewinnt. Nur 64-stellige vollständige Hex-Schlüssel werden akzeptiert.

Die Datei ist am PC bearbeitbar und wird beim nächsten Start geladen. **Ein neuer
API-Download ersetzt diese Datei einschließlich manueller Änderungen.** Vorher
sichern, wenn man selbst Einträge ergänzt hat. Eine separate Override-Datei und
automatisches dauerhaftes Sammeln aller lokalen Adverts sind noch nicht enthalten.
Eine `.bak` dient der Wiederherstellung nach unterbrochener Promotion/beschädigter
Datei; ungültige Dateien werden nicht teilweise in den RAM-Bestand übernommen.

## Technische Grenzen und Prüfstand

- HTTPS mit dem bereits für Firmware-Updates verwendeten Root-CA-Bundle,
  kein `setInsecure`. Bei falscher Gerätezeit ggf. GPS/NTP-Synchronisierung abwarten.
- API-Seiten mit 50 angeforderten Datensätzen, Ende nach `has_more=false`.
  `total` und kurze/leer gefilterte Seiten werden nicht als Ende interpretiert.
- Maximal 100 Seiten und 4096 eindeutige Knoten. 96 KiB Antwortlimit je Seite.
  JSON-Zeilen maximal 767 Bytes; lokale Datei maximal 2 MiB. Limitüberschreitung
  bricht ab und erhält den bisherigen Bestand.
- API-Antworten werden geprüft und in PSRAM gesammelt. Ein Hintergrundtask erledigt
  Netzwerk/JSON, ohne Zugriff auf SD/SPI oder LVGL. SD-Schreibzugriffe erfolgen in
  Acht-Zeilen-Blöcken im Hauptloop, zwischen den LoRa-Durchläufen.
- API-Sortierung und Bestand können sich während des Downloads ändern. Gleiche
  Schlüssel werden dedupliziert; die Quelle bietet damit keinen garantierten
  atomaren Schnappschuss. Ein späterer manueller Download aktualisiert ihn.
- 497 Host-Tests in 31 Testsätzen bestehen, darunter acht neue Verzeichnistests.
  Sie prüfen Parser, Schlüssel-/Positionsgrenzen, Mehrdeutigkeit, Kapazität,
  SD-Schreibfehler, Abbruch und Backup-Wiederherstellung. Ein Live-API-Datensatz
  wurde zusätzlich gegen den Parser geprüft. Firmware-Build, HTTPS/SD am ESP32
  und der Betrieb während LoRa-Verkehr müssen am Gerät geprüft werden.

Update am Windows-PC:

```powershell
cd C:\pyprojects\MCLite
git pull --ff-only
pio run -d firmware -e tdeck_plus -t upload
```

Gerätetest: herunterladen, Erfolgsmeldung abwarten, WLAN ausschalten, Pfad mit
bekanntem Repeater erneut öffnen und D-Marker auf der Karte prüfen. Danach neu
starten und die Zuordnung offline wiederholen. Zusätzlich Abbruch mit vorhandenem
Bestand und eine #kf-Nachricht während des Downloads prüfen.
