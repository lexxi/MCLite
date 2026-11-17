# Änderungen des lexxi-Forks

Dieses Dokument beschreibt unsere Änderungen. Das geerbte [CHANGELOG.md](CHANGELOG.md) dokumentiert die Upstream-Entwicklung.

## 0.4.6-ap.8

- Manueller HTTPS-Download des CarinthiaMesh-Knotenverzeichnisses im WLAN-Menü, mit Fortschritt und Abbruch.
- Vollständige API-Paginierung nach `has_more`, Schlüssel-/Positionsprüfung und feste Grenzen; bisheriger Bestand bleibt bei Abruffehlern erhalten.
- PSRAM-Verzeichnis als JSONL auf SD, schrittweise Speicherung und Backup-Wiederherstellung; nach Neustart offline verfügbar.
- Pfadresolver berücksichtigt importierte vollständige Schlüssel inklusive Kandidaten ohne GPS; Datenbankpositionen nur als Fallback.
- Sichtbare Datenbank-Repeater auf der Karte als D/[DB] markiert, ohne Kontakt- oder Funkroutenimport.
- Sprachdateien und Config-Tool synchronisiert; [Bedienung und Grenzen](docs/node-directory.md).
- Host-Tests und Live-API-Parserprüfung; Firmware-Build und Gerätetest noch offen.

## 0.4.6-ap.7

- AirView unter Einstellungen → Radio: Live-RSSI-Kurve (9,6 Sekunden), Pakettypen, RSSI/SNR, RX-/TX-/Fehlerzähler und die vier neuesten Ereignisse.
- Read-only RSSI-Abfrage mit 10 Hz nur im RX-Modus; Erfassung pausiert bei Dimmung/Sperre und endet beim Verlassen. Keine zusätzlichen Sendungen oder SD-Schreibzugriffe.
- MeshCore-Dispatcher-Hooks erfassen auch Duplikate und fremde Pakete; TX ohne erfundenen Empfangspegel. Kein Frequenzscan oder Wasserfall.
- Bounded RAM-Ringpuffer, Reset und Hilfe; [Bedienung und Testschritte](docs/airview.md).
- AirView-Sprachtexte und fehlende Pfadtexte in allen drei Sprachdateien ergänzt; Config-Tool synchronisiert.
- Native Grenzwert-/Ringpuffer-/Timing-/Zählertests; Firmware-Build und Gerätetest noch offen.

## 0.4.6-ap.6

- Empfangspfad neuer Kanal-Flood-Nachrichten mit Hash-Breite im SD-Verlauf speichern; Altverläufe bleiben lesbar.
- Pfad-Link an der Nachricht: Hop-Liste, direkt/unbekannt/mehrdeutig/ohne Position unterscheiden.
- Karte mit nummerierten Relays und Linien nur zwischen unmittelbar aufeinanderfolgenden, lokalisierbaren Hops; keine erfundenen Endpunkte.
- Bekannte vollständige Schlüssel vor dem Präfixabgleich deduplizieren; auch Kandidaten ohne GPS zählen bei Kollisionen.
- History-Lesegrenze auf 256 KiB erweitert, deutsche Texte und [Bedienungsanleitung](docs/message-paths.md) ergänzt.
- Native Pfad-/Grenzwert-/Kollisions-/Clipping-Tests. Fehlendes MCLiteMesh-Include nachgereicht; Build und Pfad-Anzeige mit zwei Kanalnachrichten am T-Deck bestätigt (2026-11-16). Weitere Routenfälle noch offen.

## 0.4.6-ap.5

- WLAN-Karten-Upload im bestehenden Config-AP: Ordnerauswahl, einzelne PNG-Kacheln direkt auf SD, Fortschritt und Stoppen nach der laufenden Kachel.
- Vorhandene Kacheln werden übersprungen; erneute Ordnerauswahl ermöglicht Fortsetzen.
- Prüfung von Kacheladresse, PNG-Signatur, 256×256-Abmessungen und Dateigröße; maximal 1 MiB pro Kachel.
- Inaktivitätslimit wird während Karten-Uploads erneuert.
- Vollständiger Transfer und anschließende Kartenanzeige am Gerät noch offen.

## Weitere Wartung und Dokumentation (2026-11-02)

- Pre-Commit-Hook erkennt PlatformIO aus der aktiven Umgebung und Windows-/Unix-Installationspfaden; nur vorgemerkte Firmwareänderungen lösen native Tests aus.
- Radar-Menüpfad korrigiert: Einstellungen → Radio → Heard adverts → Radar.
- Kartenanleitung für Geofabrik-Rohdaten, lokalen Ausschnitt und QGIS 3.40 ergänzt; Extract läuft, Gestaltung und Export noch offen.
- Bestehender 16-GiB-FAT32-Bereich bleibt erhalten; keine zusätzlichen Partitionen.

## 0.4.6-ap.4

- Radar-Seite über Heard Adverts: getrennte Hop- und geografische Ansicht, Farben nach Knotentyp, Empfangsalter und vorhandene Knotendetails.
- Geografische Ansicht nutzt verfügbare eigene GPS-Position und beworbene Knotenkoordinaten; keine Funkpeilung.
- Neue Übersetzungen und aktualisierte Sprachdateien im Config-Editor.
- Nachträgliche Build-Korrektur: `kPi` ersetzt die mit Arduinos `PI`-Makro kollidierende Konstante.
- Nachträgliche UI-Korrektur: Radar-Button verwendet ein Textlabel statt eines Bilddateinamens; Kopfzeilentitel erhält begrenzten Platz.
- Empfang weiterer Teilnehmer und Radar-Anzeige am T-Deck Plus bestätigt; Knotendetails und Geo-Modus noch offen.

## 0.4.6-ap.3

- `ä ö ü Ä Ö Ü ß` in der ersten Reihe der Emoji-Auswahl zum Schreiben ergänzt.
- Auswahlhöhe angepasst; lateinische Zeichen von der Farb-Emoji-Erzeugung ausgenommen.
- Gerätetest der Eingabe noch offen.

## 0.4.6-ap.2

- Latin-1-Schriften ergänzen unter anderem deutsche Umlaute und ß.
- In bestehende Text- und Emoji-Schriftketten eingebunden.
- Empfang und Anzeige von Umlauten am Gerät bestätigt.

## 0.4.6-ap.1

- Passwortgeschützter WLAN-AP für den Upload von `config.json`.
- Validierung, Sicherung der alten Config und Übernahme mit Neustart.
- OTA-Repository auf `lexxi/MCLite` eingestellt.
- Config-Upload und Nachrichtenverkehr T-Deck Plus ↔ T114 im Kanal `#kf` am Gerät bestätigt.

## Separates Werkzeug: SD-Wartung

- Eigenständige Firmware unter `tools/sd-maintenance`.
- Vorbereiten eines 16-GiB-FAT32-Bereichs ohne Ausbau der SD-Karte.
- Mit unserer eingelegten 64-GB-Karte erfolgreich getestet; anschließend MCLite zurückgeflasht.
- Bestandteil von `main`; ein Wechsel auf den alten Feature-Branch ist nicht erforderlich.

## Dokumentationsbereinigung

- Deutsche README mit Fork-Funktionen, lokalem Build und tatsächlichem Teststand.
- Bisherige ausführliche Anleitung unter `docs/upstream/README.md` erhalten.
- SD-Wartungsanleitung auf `main` und den aktuellen Build-/Upload-Ablauf umgestellt.
- Veraltete Aussagen zum noch ausstehenden Config-Upload- und SD-Gerätetest korrigiert.
