# Offline-Karten: Kärnten und Umgebung

Stand: 2026-11-02. Zielgerät: T-Deck Plus, MCLite 0.4.6-ap.5.

## Aktueller Teststand

| Schritt | Stand |
|---|---|
| Geofabrik-Alpen-Datei | Download am Windows-PC abgeschlossen |
| Lokaler Kärnten-Extract | Abgeschlossen; GeoPackage-Layer erzeugt |
| QGIS | 3.40: Skript ausgeführt, Villach-Darstellung und Testexport bestätigt |
| WLAN-Karten-Upload | Villach-Testsatz erfolgreich auf SD übertragen |
| Kartenansicht mit unserem Paket | Villach nach Neustart sichtbar; Zoom 12 ↔ 13 bestätigt |
| Gesamtgebiet Zoom 8–13 | Anleitung bereit; vollständiger Export/Transfer noch nicht bestätigt |

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

Der am Windows-PC erfolgreich abgeschlossene Python-Schritt verwendet osmium.FileProcessor zur Auswahl
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
Die Darstellung um Villach wurde visuell geprüft. Eine vollständige Kontrolle aller Regionen ist damit nicht erfolgt.

[Pyosmium-Dokumentation](https://docs.osmcode.org/pyosmium/latest/)

## QGIS 3.40: Schritt für Schritt

### 1. Ausschnitt öffnen

QGIS starten und ein leeres Projekt öffnen. Diese Datei aus dem Windows-Explorer
ins QGIS-Hauptfenster ziehen:

    C:\pyprojects\MCLite-Maps\source\kaernten-umgebung.osm.pbf

Im Dialog zur Layerauswahl zunächst alle Layer hinzufügen. QGIS zeigt
points, lines, multilinestrings, multipolygons und other_relations.
Das Einlesen und erste Zeichnen kann lange dauern. Währenddessen nicht immer
neue Zeichen- oder Exportvorgänge starten.

Bleibt die Karte nach Abschluss weiß: Häkchen beim gewünschten Layer setzen
und per Rechtsklick **Auf Layer zoomen** wählen. Ggf. andere Layer vorübergehend
ausschalten. Die Attributtabelle zeigt, ob Objekte vorhanden sind.

### 2. GeoPackage als Arbeitsdatei erzeugen

Direkter PBF-Zugriff war bei uns langsam. Deshalb werden drei Layer einmalig
in ein gemeinsames GeoPackage exportiert:

| Quelllayer | Zweck | Ziellayername |
|---|---|---|
| lines | Straßen, Wege, Bahnlinien und Bäche | lines |
| multipolygons | Seen, Wald, Siedlungen und Gebäude | multipolygons |
| points | Ortsnamen und Punkte | points |

Für **jeden** dieser drei Layer:

1. Alle Layer zum Entlasten der Anzeige vorübergehend ausschalten.
2. Rechtsklick auf den Quelllayer → **Exportieren → Objekte speichern als …**
3. Format **GeoPackage** wählen.
4. Datei: **C:\pyprojects\MCLite-Maps\source\kaernten.gpkg**
5. Layername laut Tabelle eintragen.
6. **Nur ausgewählte Objekte speichern** ausgeschaltet lassen.
7. **Gespeicherte Datei zur Karte hinzufügen** einschalten.
8. Mit **OK** exportieren und Abschluss abwarten.

Beim zweiten/dritten Export dieselbe GeoPackage-Datei wählen, aber den
jeweiligen neuen Layernamen verwenden. Wenn QGIS nachfragt:
**neuen Layer hinzufügen**, niemals die gesamte bestehende Datei überschreiben.
Keine Umprojektion erforderlich; die exportierten Layer können EPSG:4326 behalten.

Danach die ursprünglichen PBF-Layer aus dem **Projekt** entfernen.
Das löscht nicht die PBF-Datei auf der Festplatte. multilinestrings und
other_relations werden für diese erste Kartenfassung nicht benötigt.

Die GeoPackage-Layer von oben nach unten anordnen:
**points → lines → multipolygons**, alle drei anhaken.
Projekt über **Projekt → Speichern unter …** speichern:

    C:\pyprojects\MCLite-Maps\kaernten.qgz

### 3. Automatische Kartengestaltung anwenden

In **PowerShell** im Firmware-Repository das Skript aktualisieren:

    cd C:\pyprojects\MCLite
    git pull --ff-only

In **QGIS**, nicht in PowerShell, mit **Strg+Alt+P** die Python-Konsole öffnen.
Diesen Befehl als **eine Zeile** einfügen und Enter drücken:

    exec(compile(open(r"C:\pyprojects\MCLite\tools\maps\style_qgis.py", encoding="utf-8").read(), "style_qgis.py", "exec"))

[Skript im Repository](../tools/maps/style_qgis.py)

Es erkennt die drei geladenen GeoPackage-Layer und richtet Darstellung,
Ortsbeschriftung, Reihenfolge, Hintergrund und die anfängliche Ansicht ein.
Quelldaten werden nicht gelöscht. Der points-Layer erhält einen Filter für
benannte Städte, Orte, Dörfer und Weiler. Verwaltungsflächen werden nicht gefüllt.
Straßen und kleinere Details erscheinen abhängig vom Maßstab.

Optional lässt sich der lines-Layer über **Rechtsklick → Filter …** begrenzen:

    "highway" IS NOT NULL OR "waterway" IS NOT NULL OR "railway" IS NOT NULL

Das ist ein Darstellungsfilter, keine Datenlöschung. Ein zusätzlicher manueller
points-Filter ist nicht nötig; das Skript setzt ihn selbst.

Nach Skriptausführung das Zeichnen abwarten. Um Villach/Klagenfurt bis ungefähr
**1:50.000** oder näher zoomen. Erwartet werden blaue Gewässer, grüne Flächen,
Straßen, Gebäude bei hinreichendem Zoom und Ortsnamen.
Bei uns wurde die Villach-Darstellung mit Drau, Straßen, Bahn und Ortsname bestätigt.
Anschließend **Strg+S** drücken, um den Stil im Projekt zu speichern.

### 4. Grenzen und Konsolenfehler verstehen

Das Skript setzt die Ansicht auf das geplante Rechteck. QGIS erweitert die
sichtbare Fläche jedoch passend zum Fensterformat. Referenzierte OSM-Objekte
können außerdem außerhalb des Rechtecks liegen. **Auf Layer zoomen** ist deshalb
keine zuverlässige Methode, den gewünschten Exportbereich festzulegen.

Zum Zurücksetzen der Ansicht, nach Ausführung des Gestaltungsskripts, diese
**eine Zeile in der QGIS-Konsole** eingeben:

    iface.mapCanvas().setExtent(QgsCoordinateTransform(QgsCoordinateReferenceSystem("EPSG:4326"), iface.mapCanvas().mapSettings().destinationCrs(), QgsProject.instance()).transformBoundingBox(QgsRectangle(12.5, 46.3, 15.2, 47.3))); iface.mapCanvas().refresh()

Bei einem SyntaxError nach mehrzeiligem Einfügen: zweimal **Strg+C** drücken,
bis wieder der normale Eingabeprompt erscheint, dann nur den vollständigen
Einzeiler einfügen. Keine Konsolen-Ausgaben oder Promptzeichen mitkopieren.

**Die Ansicht beschneidet keine Daten.** Für den Export immer die unten explizit
gesetzten Grenzen verwenden. Die Reihenfolge im Export lautet
**West, Ost, Süd, Nord**, nicht West, Süd, Ost, Nord.

### 5. Kleinen Villach-Testsatz exportieren

Das Projekt mit den gestalteten drei GeoPackage-Layern geöffnet lassen.
Diesen Einzeiler in der **QGIS-Python-Konsole** ausführen:

    import processing; processing.execAlgorithmDialog("native:tilesxyzdirectory", {"EXTENT": "13.78,13.92,46.57,46.66 [EPSG:4326]", "ZOOM_MIN": 12, "ZOOM_MAX": 13, "DPI": 96, "BACKGROUND_COLOR": "#f5f1e8", "ANTIALIAS": True, "TILE_FORMAT": 0, "TILE_WIDTH": 256, "TILE_HEIGHT": 256, "METATILESIZE": 4, "TMS_CONVENTION": False, "OUTPUT_DIRECTORY": r"C:\pyprojects\MCLite-Maps\test-villach\tiles"})

Der Befehl **öffnet den Exportdialog**, startet aber noch keinen Export.
Im Dialog **Starten/Ausführen** anklicken und auf erfolgreichen Abschluss warten.
Die erzeugten PNG-Dateien liegen unter Zoom/X/Y.png im Ausgabeordner.

### 6. Testsatz am Gerät prüfen

1. Config-AP am T-Deck starten: **Sym+0 → Device Info → WiFi Companion → Start config AP**.
   Die Beschriftung kann je nach Sprache abweichen.
2. Computer mit dem angezeigten WLAN und Passwort verbinden.
3. http://192.168.4.1 in Chrome oder Edge öffnen.
4. Unter **Offline-Karten** genau diesen Ordner auswählen:

       C:\pyprojects\MCLite-Maps\test-villach\tiles

5. **Karten übertragen** starten und Abschluss abwarten.
6. T-Deck neu starten.
7. GPS-Symbol in der Statusleiste antippen, um die Karte zu öffnen.
8. Ansicht auf Villach bringen, z. B. über den Karten-/GPS-Button eines dort
   gehörten Knotens mit Koordinaten, falls das Gerät selbst woanders steht.
9. **+** wechselt von Zoom 12 zu 13, **−** zurück.

Nur für Villach und die Zoomstufen 12/13 sind in diesem Testsatz Kacheln enthalten.
Außerhalb der Abdeckung kann die Ansicht grau sein, ohne dass der Import defekt ist.

**Bestätigt:** Upload abgeschlossen, Neustart, Karte sichtbar und Zoomwechsel
12 ↔ 13 auf dem T-Deck erfolgreich.

### 7. Gesamtgebiet Kärnten samt Umgebung exportieren

Erst nach erfolgreichem kleinen Test. Im selben QGIS-Projekt diese **eine Zeile**
in der Python-Konsole ausführen:

    import processing; processing.execAlgorithmDialog("native:tilesxyzdirectory", {"EXTENT": "12.5,15.2,46.3,47.3 [EPSG:4326]", "ZOOM_MIN": 8, "ZOOM_MAX": 13, "DPI": 96, "BACKGROUND_COLOR": "#f5f1e8", "ANTIALIAS": True, "TILE_FORMAT": 0, "TILE_WIDTH": 256, "TILE_HEIGHT": 256, "METATILESIZE": 4, "TMS_CONVENTION": False, "OUTPUT_DIRECTORY": r"C:\pyprojects\MCLite-Maps\kaernten\tiles"})

Im Exportdialog **Starten/Ausführen** anklicken und Abschluss abwarten.

| Einstellung | Wert |
|---|---|
| Gebiet | West 12.5, Ost 15.2, Süd 46.3, Nord 47.3 in EPSG:4326 |
| Zoom | 8–13; kleinere Zahlen sind Übersichtsstufen |
| Kacheln | PNG, 256×256 Pixel |
| DPI | 96 |
| Hintergrund | #f5f1e8, nicht transparent |
| TMS / invertierte Y-Achse | Aus, damit XYZ entsteht |
| Metatile | 4 |
| Ausgabe | C:\pyprojects\MCLite-Maps\kaernten\tiles |

Die Ausgabe wird auf vollständige Kacheln erweitert; einzelne Randkacheln können
daher über die Koordinatengrenzen reichen. Es wird der Projektinhalt gerendert,
kein öffentlicher OSM-Kachelserver abgerufen.

Danach wie beim Testsatz im Config-AP den **neuen Gesamtordner** auswählen:

    C:\pyprojects\MCLite-Maps\kaernten\tiles

Übertragung vollständig abwarten und T-Deck neu starten. Vorhandene Villach-Kacheln
werden übersprungen. Das gilt auch für spätere Stiländerungen: vorhandene
Kacheln werden derzeit nicht ersetzt, daher ist bei einer Neugestaltung ein
bewusster Austausch der alten Dateien nötig.

Der vollständige Gesamtgebiet-Transfer ist zum dokumentierten Stand noch
nicht bestätigt. Den erfolgreichen Villach-Test nicht mit einer vollständigen
Prüfung des gesamten Kartenpakets gleichsetzen.

[QGIS-3.40-Dokumentation](https://docs.qgis.org/3.40/en/docs/user_manual/processing_algs/qgis/rastertools.html#generate-xyz-tiles-directory)

Kartendaten: © OpenStreetMap contributors,
[Copyright und ODbL](https://www.openstreetmap.org/copyright).
Die Attribution muss bei der Kartendarstellung berücksichtigt werden.

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
