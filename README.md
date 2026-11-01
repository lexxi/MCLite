# MCLite – lexxi-Fork für den T-Deck Plus

Dieser Fork basiert auf [laserir/MCLite](https://github.com/laserir/MCLite), einer eigenständig bedienbaren MeshCore-Firmware. Unser Schwerpunkt ist der **LilyGo T-Deck Plus**: Nachrichten über Tastatur und Touchscreen schreiben, Konfiguration ohne Kartenleser übertragen und gehörte Mesh-Knoten anzeigen.

**Aktueller Firmwarestand: `0.4.6-ap.4`**, einschließlich der Korrektur des Arduino-`PI`-Namenskonflikts.

## Unsere Erweiterungen

| Erweiterung | Bedienung / Zweck | Bestätigter Stand |
|---|---|---|
| WLAN-Config-Upload | Geschützter Access Point; `config.json` im Browser hochladen | Upload am T-Deck Plus erfolgreich getestet |
| SD-Wartungsfirmware | Eingelegte große SD-Karte direkt am Gerät für MCLite vorbereiten | Mit unserer 64-GB-Karte und einem 16-GiB-FAT32-Bereich erfolgreich getestet |
| Umlaute anzeigen | Latin-1-Schrift für `ä ö ü Ä Ö Ü ß` und weitere Zeichen | Empfang und Anzeige bestätigt |
| Umlaute schreiben | Erste Reihe der Emoji-Auswahl enthält `ä ö ü Ä Ö Ü ß` | Implementiert; Bestätigung am Gerät noch offen |
| Radar | Gehörte Knoten nach Hops oder geografischer Position anzeigen | Implementiert; Gerätetest noch offen |

Zusätzlich bestätigt: **T-Deck Plus ↔ T114**, Nachrichten im Kanal **`#kf`** in beide Richtungen.

Die üblichen MCLite-Funktionen bleiben die Grundlage: Direktnachrichten, Kanäle, Kontakte, Nachrichtenverlauf, GPS, Offline-Karten und Companion-Verbindungen. Diese Funktionen stammen überwiegend aus Upstream; ihre Verfügbarkeit im Quellcode bedeutet nicht, dass wir jede davon in diesem Fork am Gerät getestet haben.

## Bauen und Flashen unter Windows

Einmalig klonen und eine eigene Python-Umgebung anlegen:

```powershell
cd C:\pyprojects
git clone https://github.com/lexxi/MCLite.git
cd MCLite
py -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install platformio
```

Bei einem bestehenden Checkout:

```powershell
cd C:\pyprojects\MCLite
.\.venv\Scripts\Activate.ps1
git switch main
git pull --ff-only
```

Nur bauen:

```powershell
python -m platformio run -d firmware -e tdeck_plus
```

Bauen und über USB flashen:

```powershell
python -m platformio run -d firmware -e tdeck_plus -t upload
```

Falls die automatische Erkennung scheitert, den aktuellen Port ergänzen, beispielsweise `--upload-port COM10`. Den Port im Windows-Gerätemanager prüfen. Falls nötig: T-Deck ausschalten und beim Einschalten den Trackball gedrückt halten, um den USB-Bootloader zu starten.

Die erste Einrichtung lädt Toolchain und Bibliotheken herunter. Spätere Builds verwenden den lokalen Cache.

**Wichtig:** Der Webflasher unter `laserir.github.io` installiert Upstream-Firmware. Er enthält unsere Erweiterungen nicht. Auch die geerbten Webflasher-Dateien und alten Binärdateien in diesem Repository sind kein Ersatz für einen aktuellen Fork-Build.

## Konfiguration über WLAN hochladen

1. T-Deck mit eingelegter, nutzbarer SD-Karte starten.
2. **Sym + 0 → Device Info → WiFi Companion → Start config AP** öffnen. Menüeinträge per Touchscreen auswählen; die Bezeichnung hängt von der Sprache ab.
3. Handy oder Computer mit dem angezeigten WLAN und Passwort verbinden.
4. [http://192.168.4.1](http://192.168.4.1) öffnen.
5. `config.json` hochladen und die Übernahme bestätigen. Das Gerät startet danach neu.

Die SD-Karte bleibt dabei eingebaut. Die bisherige Konfiguration wird als `/config.pre-upload.json` gesichert.

Zum Bearbeiten den [Config-Editor dieses Forks](tools/config-tool/mclite_config_tool.html) lokal im Browser öffnen. Beim Export das ZIP entpacken und die **JSON-Datei** hochladen.

Details, Voraussetzungen und Wiederherstellung: [Config-AP](docs/config-ap.md).

## Umlaute schreiben

Im Chat den **Emoji-Button neben dem Eingabefeld** antippen. Die erste Reihe bietet **ä ö ü Ä Ö Ü ß**. Ein Zeichen antippen und anschließend weiterschreiben. Gesendet wird erst mit „Senden“.

Die Auswahl setzt `display.emoji = true` voraus. Details: [Schriften und Zeicheneingabe](docs/latin-fonts.md).

## Radar öffnen

**Device Info → Heard Adverts → Radar**

- **Hops:** alle gespeicherten gehörten Knoten. Die Ringe stehen für 0, 1, 2 und 3+ Hops. Die Winkel sind schematisch; sie zeigen keine gemessene Richtung.
- **Geo / N:** Richtung und Entfernung werden aus den Koordinaten des eigenen Geräts und des jeweiligen Knotens berechnet. Norden bleibt oben. Knoten ohne ausreichende Positionsdaten werden hier ausgelassen.
- Ein Punkt öffnet per Antippen die vorhandenen Knotendetails. Über **< / >** und **Info** lassen sich auch überlappende Punkte auswählen.
- Farben: Blau = Teilnehmer, Orange = Repeater, Lila = Room, Grün = Sensor.

Der Empfangsspeicher enthält höchstens 64 Knoten und wird beim Neustart geleert. Eine empfangene Aussendung kann über Repeater gekommen sein; sie beweist weder unmittelbare Nähe noch aktuelle Erreichbarkeit. Der Radar benötigt keine Kartenkacheln und sendet keine zusätzlichen Suchpakete.

Details: [Radar-Bedienung und Grenzen](docs/radar.md).

## SD-Wartungsfirmware

Das separate Projekt unter [tools/sd-maintenance](tools/sd-maintenance/README.md) ersetzt MCLite vorübergehend, um die eingelegte Karte vorzubereiten.

**Die Formatierung entfernt die vorhandene Partitionierung und macht die bisherigen Dateien unzugänglich.** Nur mit gesicherten beziehungsweise entbehrlichen Daten verwenden. Danach MCLite wieder flashen. Der verbleibende Kartenplatz außerhalb des vorbereiteten Bereichs wird nicht verwendet.

## Hardware und Teststand

- **T-Deck Plus:** unser Entwicklungs- und Testgerät.
- **T114:** Gegenstelle für die bestätigten MeshCore-Nachrichtentests.
- **T-Watch Ultra:** aus Upstream im Quellcode enthalten; die Fork-Erweiterungen sind darauf von uns nicht am Gerät bestätigt.

Builds, Flashen und Gerätetests erfolgen in der lokalen PlatformIO-Umgebung. Ein erfolgreicher Softwaretest ersetzt keinen Gerätetest. Noch offene Bestätigungen stehen oben in der Funktionstabelle.

## Dokumentation und Herkunft

- [Änderungen dieses Forks](CHANGELOG_FORK.md)
- [Config-AP](docs/config-ap.md)
- [Umlaute und Zeichenauswahl](docs/latin-fonts.md)
- [Radar](docs/radar.md)
- [SD-Wartungsfirmware](tools/sd-maintenance/README.md)
- [Bisherige ausführliche Anleitung mit Upstream-Funktionen](docs/upstream/README.md)
- [Geerbtes Upstream-Changelog](CHANGELOG.md)
- [Originalprojekt laserir/MCLite](https://github.com/laserir/MCLite)
- [MeshCore](https://github.com/meshcore-dev/MeshCore)

Upstream-Code, Bilder und Werkzeuge bleiben für die vorhandenen Funktionen und spätere Übernahmen erhalten. Externe Upstream-Links in der archivierten Anleitung beziehen sich auf das Originalprojekt.

## Lizenz

Die Firmware steht unter der [MIT-Lizenz](LICENSE). Urheber- und Lizenzhinweise des Originalprojekts sowie der eingebundenen Bibliotheken und Schriftdateien bleiben erhalten. Weitere Hinweise: [LICENSES.md](LICENSES.md).
