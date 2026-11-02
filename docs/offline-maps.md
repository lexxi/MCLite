# Offline-Karten: Kärnten und Umgebung

Stand: 2026-11-02. Zielgerät: T-Deck Plus, MCLite 0.4.6-ap.5.

## Aktueller Teststand

| Schritt | Stand |
|---|---|
| Geofabrik-Alpen-Datei | Download am Windows-PC abgeschlossen |
| Lokaler Kärnten-Extract | Gestartet; Abschluss noch nicht bestätigt |
| QGIS | Version 3.40 vorhanden; Gestaltung und Export noch offen |
| WLAN-Karten-Upload | Implementiert; vollständiger Gerätetest offen |
| Kartenansicht mit unserem Paket | Noch offen |

## Format und Speicher

MCLite erwartet PNG-Rasterkacheln mit 256×256 Pixeln in XYZ-Anordnung:
**/tiles/Zoom/X/Y.png**, Web Mercator (EPSG:3857).
Der Upload akzeptiert Zoom 0–19 und maximal 1 MiB je Kachel.
PBF, Shapefile, GeoPackage, MBTiles und ZIP sind keine direkten Uploadformate.
Rohdaten müssen zuerst gestaltet und in PNG-Kacheln gerendert werden.

Bestätigt ist unsere 64-GB-Karte mit einem 16-GiB-FAT32-Bereich.
16 GiB ist eine bewusste Begrenzung des Wartungstools, keine im MCLite-SD-Code
festgestellte Grenze. Die geerbte Anleitung nennt Karten bis 32 GB; größere
Partitionen sind in unserem Setup nicht bestätigt. Der übrige Kartenplatz
bleibt ungenutzt. MCLite bindet derzeit nur eine Partition ein.

**Für den Kartenimport ist keine Neuformatierung nötig.**
SD-Maintenance ersetzt die Partitionierung und macht bisherige Dateien
unzugänglich; es ist kein Kartenimporter.

## Gebiet und Umfang

| Grenze | Grad (WGS84 / EPSG:4326) |
|---|---:|
| West | 12.5 |
| Süd | 46.3 |
| Ost | 15.2 |
| Nord | 47.3 |

Das Rechteck umfasst Kärnten und angrenzende Bereiche; es ist keine exakte
Landesgrenze. Die Vorschau mit mt-downloader ergab **2.911 geplante Kacheln
bis Zoom 13**. Das ist keine bestätigte Dateigröße und noch kein Kartenpaket.

## Quelle und Arbeitsumgebung

Wir verwenden den kostenlosen [Alpen-Extract von Geofabrik](https://download.geofabrik.de/europe/alps.html)
als OSM-PBF und schneiden lokal aus. Kein API-Key erforderlich.
Die vorher versuchte manuelle Aufteilung bei BBBike ist dafür nicht nötig.

Der öffentliche OSM-Kachelserver ist keine Quelle für diesen Offline-Massendownload.
Thunderforest verlangt für solche Downloads einen passenden Tarif.
Siehe [OSM-Kachelrichtlinie](https://operations.osmfoundation.org/policies/tiles/)
und [Thunderforest-Bedingungen](https://www.thunderforest.com/terms/).

Die Kartenerstellung erfolgt getrennt vom Firmware-Checkout unter
**C:\pyprojects\MCLite-Maps**, mit eigener .venv.
Große Quell-/Ausgabedaten und virtuelle Umgebungen gehören nicht ins Git-Repository.

PowerShell, einmalige Einrichtung:

    New-Item -ItemType Directory -Force C:\pyprojects\MCLite-Maps | Out-Null
    cd C:\pyprojects\MCLite-Maps
    py -m venv .venv
    .\.venv\Scripts\Activate.ps1
    python -m pip install "osmium>=4.0"
    New-Item -ItemType Directory -Force source | Out-Null

Download, auch zum Fortsetzen nach Unterbrechung:

    curl.exe -L --fail -C - -o source\alps-latest.osm.pbf https://download.geofabrik.de/europe/alps-latest.osm.pbf

## Lokaler Ausschnitt

Der derzeit laufende Python-Schritt verwendet osmium.FileProcessor zur Auswahl
der Punkte im Rechteck und osmium.ForwardReferenceWriter mit
back_references=True, forward_relation_depth=0 und remove_tags=False zur
Ergänzung referenzierter Objekte.

- Eingabe: source/alps-latest.osm.pbf
- Ausgabe: source/kaernten-umgebung.osm.pbf
- Die Quelldatei bleibt unverändert; ein vorhandenes Ziel wird nicht überschrieben.
- Die Verarbeitung liest große Teile der Quelle mehrfach; längere Phasen ohne
  neue Konsolenausgabe bedeuten nicht automatisch einen Stillstand.

Das ist kein geometrisch exakt beschnittener Datensatz. Vollständige Wege
können über den Rand reichen. Geometrien ohne ausgewählte Stützpunkte und
komplexe Relationen müssen bei der Vorschau berücksichtigt werden.
Der spätere Renderbereich wird separat auf das Rechteck gesetzt.
Die Vollständigkeit der gerenderten Karte ist noch nicht bestätigt.

[Pyosmium-Dokumentation](https://docs.osmcode.org/pyosmium/latest/)

## QGIS 3.40: noch ausstehender Ablauf

1. Lokalen Extract importieren und geeignete Arbeitslayer erzeugen.
2. Straßen, Gewässer, Flächennutzung und Ortsnamen gestalten.
3. Seen, Multipolygone und Gebietsgrenzen visuell prüfen.
4. Zunächst einen kleinen Bereich rendern und auf dem T-Deck prüfen.
5. Mit **Generate XYZ tiles (Directory)** / **XYZ-Kacheln erzeugen (Verzeichnis)**
   PNG-Kacheln mit 256×256 Pixeln exportieren. XYZ verwenden, keine invertierte
   TMS-Y-Achse. Der erste vollständige Satz ist bis Zoom 13 geplant.
6. Den fertigen Ordner als **tiles** bereitstellen.

QGIS rendert die Darstellung des Projekts. Ein ungeformatierter Rohdatenimport
ergibt noch keine brauchbare Straßenkarte. Ein fertiger QGIS-Stil und die
vollständig geprüften Exporteinstellungen stehen noch aus.

[QGIS-3.40-Dokumentation](https://docs.qgis.org/3.40/en/docs/user_manual/processing_algs/qgis/rastertools.html#generate-xyz-tiles-directory)

Kartendaten: © OpenStreetMap contributors,
[Copyright und ODbL](https://www.openstreetmap.org/copyright).
Die Attribution muss bei der späteren Kartendarstellung berücksichtigt werden.

## WLAN-Upload auf die SD

1. MCLite ab 0.4.6-ap.5 bauen und flashen.
2. Bestehenden Config-AP starten und mit dessen WLAN verbinden.
3. http://192.168.4.1 am Computer in Chrome oder Edge öffnen.
4. Unter **Offline-Karten** den entpackten tiles-Ordner auswählen.
5. **Karten übertragen** starten; Kacheln werden einzeln direkt geschrieben.
6. Nach Abschluss T-Deck neu starten und die Karte über das GPS-Symbol öffnen.

Vorhandene Kacheln werden übersprungen. Nach einer Unterbrechung denselben
Ordner erneut auswählen. **Nach dieser Kachel stoppen** beendet nach der
laufenden Datei. Vorhandene fehlerhafte Kacheln werden nicht automatisch ersetzt.
Signatur, IHDR-Abmessungen und gespeicherte Größe werden geprüft,
keine vollständige PNG-Dekodierung oder CRC-Prüfung.

Während des Transfers wird der Inaktivitätszeitpunkt erneuert.
Nach zehn Minuten ohne Aktivität schaltet der AP ab.
Config-Upload startet das Gerät neu: Config und Karten nacheinander übertragen.

## Radar ist unabhängig von Karten

Radar benötigt keine PNG-Kacheln. Hops funktioniert ohne GPS.
Geo / N benötigt eine akzeptierte eigene Position und Knotenkoordinaten.
Die separate Kartenansicht verwendet die Kacheln auf SD.
