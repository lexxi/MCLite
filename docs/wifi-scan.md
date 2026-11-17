# WLAN-Scan diagnostizieren

Ab 0.4.6-ap.10 unterscheiden wir einen fehlgeschlagenen Scan von einem Scan ohne
auswählbare Treffer. Die vorherige Anzeige fasste negative Arduino-Fehlercodes
und null Treffer zu „Keine Netzwerke gefunden“ zusammen. Auch ein gescheiterter
Treiberstart blieb dadurch unsichtbar.

Die WLAN-Auswahl startet den Treiber neu im STA-Modus, deaktiviert automatisches
Wiederverbinden während der Auswahl und löscht dabei keine gespeicherten
Zugangsdaten. Es folgt ein aktiver Scan auf allen vom Treiber erlaubten
2,4-GHz-Kanälen mit 300 ms Obergrenze pro Kanal. Versteckte APs werden mitgezählt;
ohne ausgesendeten Namen sind sie nicht aus der Liste auswählbar.

Wenn keine auswählbaren Netze erscheinen, steht eine dauerhafte Diagnosezeile
in der Netzliste, beispielsweise:

```text
WiFi scan failed
Scan:-2 Driver:-1 AP:-1
```

| Wert | Bedeutung |
|---|---|
| Scan ≥ 0 | Anzahl der vom Arduino-Scan bereitgestellten AP-Datensätze |
| Scan = −1 | Arduino meldet noch einen laufenden Scan |
| Scan = −2 | Arduino-Scan fehlgeschlagen |
| Scan = −3 | Unser STA-Treiberstart fehlgeschlagen |
| Driver = 0 | IDF meldet den Scan erfolgreich beendet |
| Driver > 0 | IDF meldet einen Scanfehler |
| Driver/AP = −1 | Kein entsprechendes Scan-Abschlussereignis erfasst |
| AP ≥ 0 | AP-Anzahl aus dem IDF-Scan-Abschlussereignis |

`Scan:0 Driver:0 AP:0` bedeutet: Beide Ebenen melden keine APs. `Scan:0` bei
`AP>0` bedeutet dagegen, dass der Treiber APs sah, Arduino aber keine Ergebnisse
bereitstellte (mögliche Ursachen sind Ergebnisabruf oder Speicher). Das wird als
Fehler angezeigt. Bei `Scan>0`, aber ohne auswählbare Namen können versteckte
SSIDs vorliegen. Das ist kein Nachweis eines Hardwaredefekts.

Die gleichen Werte werden im seriellen Log mit `[WiFi] Scan result=...` ausgegeben.
Die Gerätediagnose benötigt jedoch keinen seriellen Monitor: Die Zahlen bleiben
im WLAN-Fenster sichtbar. Bitte diese komplette Zeile melden, falls auch ein
sichtbarer 2,4-GHz-iPhone-Hotspot direkt neben dem T-Deck nicht gefunden wird.

Die Netzliste behält die stärkste Ausprägung jeder SSID und bevorzugt bei der
Begrenzung auf 20 Netze die stärksten Signale. Es wird weiterhin der bestehende
blockierende Scan verwendet; die genaue Laufzeit hängt vom Treiber ab.

Die Änderung ist eine Initialisierungsverbesserung plus Diagnose, keine bestätigte
Behebung des beobachteten Geräteproblems. Fünf Host-Tests mit einem WiFi-Treiberdouble
prüfen Fehlerunterscheidung, Initialisierung und Ergebnisaufbereitung. Der
ESP32-Build und der Scan am betroffenen Gerät stehen noch aus.
