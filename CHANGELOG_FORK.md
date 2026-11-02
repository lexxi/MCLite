# Änderungen des lexxi-Forks

Dieses Dokument beschreibt unsere Änderungen. Das geerbte [CHANGELOG.md](CHANGELOG.md) dokumentiert die Upstream-Entwicklung.

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
