# Deutsche Sprachdatei über WLAN aktualisieren

Ab **0.4.6-ap.9** muss die SD-Karte für `de.json` nicht mehr ausgebaut werden.
Der neue Button ist auch mit einer alten oder fehlenden deutschen Sprachdatei
vorhanden: Dann steht dort zunächst **Download German (de.json)**.

1. Admin mit **0** oder Zahnrad öffnen, dann **WLAN/WiFi** öffnen.
2. Mit einem WLAN mit Internetzugang verbinden. Der Config-AP dient dem Upload
   vom PC und liefert selbst keinen Internetzugang.
3. Nach unten scrollen und **de.json per WLAN laden** bzw.
   **Download German (de.json)** wählen.
4. Auf **de.json gespeichert. Zum Übernehmen neu starten.** warten.
5. **Neu starten** wählen. Wenn Deutsch bereits als Sprache eingestellt ist,
   werden die neuen Texte nach dem Neustart verwendet.

Die Funktion lädt nur die deutsche Sprachdatei. Sie installiert keine Firmware
und ändert keine Spracheinstellung, Konfiguration, Kontakte oder Karten.
War bisher Englisch ausgewählt, lässt sich Deutsch nach dem Download über die
bestehende Spracheinstellung auswählen. Knotenverzeichnis und Sprachdatei können
nacheinander heruntergeladen werden. Während eines Downloads sind der andere
Download, Firmware-Update, Config-AP und der Neustart-Button gesperrt.

Der Abruf läuft im Hintergrund über HTTPS mit dem vorhandenen Root-CA-Bundle.
Quelle ist `sdcard/mclite/lang/de.json` auf `main` im konfigurierten GitHub-Repo
(standardmäßig `lexxi/MCLite`). Anders als der bestehende OTA-Nachlauf benötigt
man dafür kein Release-Tag. Ein Versionsparameter reduziert veraltete CDN-Antworten.

Vor dem Speichern werden JSON-Form, Mindestversion, vollständige Abdeckung aller
Firmware-Texte, printf-Formatangaben und die Zeichenketten-Anzahl geprüft.
Antwortlimit: 32 KiB; die Version darf weder älter als die Firmware-Anforderung
noch älter als die bereits vorhandene Sprachdatei sein. Eine inkompatible Datei
wird nicht installiert. Abbruch, Netzwerk- oder Prüffehler erhalten den bisherigen
Bestand. Die SD-Promotion erfolgt über die vorhandene atomare Schreibfunktion;
bei einer unterbrochenen Promotion kann der Sprachloader die `.bak` verwenden.

**Die laufende Oberfläche wird nicht neu geladen.** Sie darf weiterhin ihre
bisherigen Übersetzungszeiger verwenden. Der bewusste Neustart nach erfolgreichem
Speichern übernimmt die neue Datei.

Firmware einmal aktualisieren:

```powershell
cd C:\pyprojects\MCLite
git pull --ff-only
pio run -d firmware -e tdeck_plus -t upload
```

Für dieses Update die `de.json` nicht mehr im Explorer kopieren: Den neuen
WLAN-Button direkt mit der bisherigen Datei testen. Gerätetest: Download, Neustart,
deutsche neue Texte; anschließend Abbruch bei bestehender Datei prüfen.
Firmware-Build und HTTPS/SD-Test am Gerät stehen bei Einführung noch aus.
