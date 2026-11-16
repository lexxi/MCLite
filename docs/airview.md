# AirView: lokale Funkdiagnose

Ab 0.4.6-ap.7: **Einstellungen → Radio → AirView**. Erste Version für T-Deck Plus;
die gemeinsamen Screens sind auch im T-Watch-Build enthalten, dort noch ungetestet.
Keine Übernahme des nicht öffentlich vorliegenden MCTerm-Firmwarecodes.

## Bedienung

1. AirView öffnen. Die Messung und die Sitzungszähler beginnen neu.
2. Oben stehen die beim Radiostart angewendeten Einstellungen: Frequenz, SF,
   Bandbreite in kHz, Coding Rate und Sendeleistung. Im Offgrid-Modus werden die
   aufgelösten Offgrid-Werte angezeigt.
3. **Live** zeigt den momentanen Empfangspegel in dBm. Die Kurve läuft von links
   (alt) nach rechts (neu), über 9,6 Sekunden. Skala: unten −140, oben −20 dBm;
   Werte außerhalb werden nur für die Zeichnung an den Rand begrenzt.
4. Farbpunkte markieren empfangene Pakete bei ihrem Paket-RSSI. ADV grün,
   Kanalnachricht CH türkis, DM orange, ACK gelb, andere Typen grau.
   Eigene abgeschlossene Sendungen erscheinen violett am oberen Rand,
   Sendefehler rot. Für TX gibt es keinen Empfangs-RSSI/SNR.
5. Die Liste zeigt die vier neuesten Pakete: Richtung, Typ, RSSI in dBm / SNR in dB,
   Länge in Bytes und Alter in Sekunden. RX/TX/! zählen empfangene Frames,
   abgeschlossene Sendungen und Sendefehler dieser Sitzung, auch wenn ältere
   Ereignisse bereits aus dem begrenzten Puffer gefallen sind.
6. **Reset** leert Kurve, Liste und Sitzungszähler. **?** erklärt die Anzeige.
   Zurück führt zu den Radio-Einstellungen. Beim nächsten Öffnen beginnt eine neue Sitzung.

## Was die Anzeige aussagt

Die SX1262-Abfrage `getRSSI(false)` liest den momentanen Pegel des eingestellten
Kanals, nicht den RSSI des zuletzt empfangenen Pakets. Abfrage alle 100 ms im
Hauptloop und nur im RX-Modus; TX/ungültige Messwerte erzeugen Lücken.
Die UI zeichnet höchstens viermal pro Sekunde. Dimmung oder Sperre pausiert die
Erfassung; Verlassen beendet sie. Es gibt keine zusätzlichen Funkpakete,
Kanalwechsel, CAD-Abfragen oder SD-Schreibzugriffe durch AirView.

Die Empfangsliste nutzt MeshCores `Dispatcher::logRx`, also syntaktisch lesbare
Frames vor Entschlüsselung, Adressprüfung und Duplikatfilter. Auch Wiederholungen,
fremde Kanäle und nicht für uns bestimmte Pakete zählen. Eine DM-Zeile beweist
keine erfolgreiche Zustellung oder entschlüsselbaren Inhalt. CRC-fehlerhafte oder
nicht als MeshCore-Paket parsebare Frames erscheinen nicht in der Liste.

**Kein Frequenzscanner und keine kalibrierte Spektrumanalyse.** Ein Pegelanstieg
kann Funkaktivität zeigen, ohne dass ein Paket decodiert wurde; die Ursache lässt
sich daraus allein nicht bestimmen. Kurze Impulse zwischen zwei Abfragen können
fehlen. Diese erste Version bietet keine Rauschboden-, Empfindlichkeits- oder
Kanalbelegungs-Schätzung und keinen Wasserfall.

## Update und Gerätetest

```powershell
cd C:\pyprojects\MCLite
git pull --ff-only
pio run -d firmware -e tdeck_plus -t upload
```

Für deutsche Hilfetexte zusätzlich `sdcard/mclite/lang/de.json` im Explorer nach
`/mclite/lang/de.json` auf die SD kopieren und ersetzen. Ohne neue Sprachdatei
greifen englische Standardtexte. Konfiguration und Karten bleiben bestehen.

Prüfen: bei ruhigem Kanal einen Live-Wert erwarten, vom T114 ein Advert und eine
Kanalnachricht senden, ADV/CH sowie RSSI/SNR in der Liste prüfen. Falls erreichbar,
eine DM senden und danach eine T-Deck-Sendung/ACK beobachten. Im Normalbetrieb
gesendete Testnachrichten müssen weiterhin ankommen. Reset, Zurück, Wiederöffnen,
Dimmung/Sperre und Wiederaufwecken prüfen. Luftschnittstelle und Gerätetest stehen
bei Einführung noch aus; kein vollständiger Firmware-Build in dieser Sitzung.

Technische Grundlage: MeshCore `companion-v1.16.0` Dispatcher-Logging-Hooks und
RadioLibs SX1262-RSSI-Abfrage; feste Ringpuffer mit 96 Messwerten und 24 Paketen.
