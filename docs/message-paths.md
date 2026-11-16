# Empfangspfad von Kanalnachrichten

Ab 0.4.6-ap.6 wird der Flood-Pfad neu empfangener Kanalnachrichten mit dem
Nachrichtenverlauf auf SD gespeichert. Das ist der Pfad der empfangenen Kopie,
kein Advert-Pfad und keine Übersicht aller möglichen Zustellwege.

## Bedienung am T-Deck

1. Neue Nachricht in einem Kanal empfangen und den Kanal öffnen.
2. In der Fußzeile der Nachricht **Pfad** antippen. Der Link ist unabhängig von
   der Einstellung „Show Hop Count“ sichtbar.
3. Der Dialog zeigt die Relay-Kennungen in ihrer Paket-Reihenfolge, nummeriert
   ab 1. Direkt empfangene Nachrichten werden ausdrücklich als direkt angezeigt.
4. **Relays auf Karte** ist verfügbar, sobald mindestens ein Relay eindeutig unter
   den bekannten Schlüsseln zugeordnet werden kann, dessen Position bekannt ist,
   und Kartenkacheln auf SD vorhanden sind.
5. Die Karte zeigt nummerierte Relays. Antippen zeigt Name und Kennung.
   Zoom und Verschieben funktionieren wie auf der normalen Karte. Zentrieren
   springt zum ersten lokalisierbaren Relay zurück. Schließen führt zum Chat.

Eine Linie verbindet nur direkt aufeinanderfolgende, lokalisierbare Relays.
Unbekannte oder mehrdeutige Kennungen und fehlende Positionen unterbrechen sie.
Absender- und Empfängerposition werden nicht ergänzt. Ein einzelnes bekanntes
Relay wird daher als einzelner Punkt angezeigt.

## Aussagekraft und Grenzen

- Die Kennungen sind kurze Public-Key-Präfixe (1–4 Byte), keine verifizierten
  Identitäten. „Eindeutig“ heißt nur: genau ein Treffer unter aktuell bekannten
  vollständigen Schlüsseln. Auch unbekannte Knoten können denselben Präfix haben.
- Beim Abgleich zählen bekannte Schlüssel auch ohne GPS; fehlende Koordinaten
  dürfen eine Kollision nicht scheinbar auflösen.
- Namen und Positionen stammen beim Öffnen aus Heard Adverts, Mesh-Kontakten und
  verfügbaren Standortdaten. Sie sind keine historischen Positionsmessungen des
  Nachrichtenempfangs. Die geöffnete Karte hält diese Zuordnung als Momentaufnahme.
- Heard Adverts ist ein begrenzter RAM-Cache. Nach Neustart oder Verdrängung können
  Zuordnungen fehlen, bis wieder Adverts eintreffen. Gespeicherte Pfadkennungen
  der Nachricht bleiben erhalten.
- Alte Nachrichten haben nur die bisherige Hop-Anzahl. Ihr tatsächlicher Pfad
  kann nicht rekonstruiert werden; sie erhalten keinen Pfad-Link.
- Vorerst nur empfangene Kanal-Flood-Nachrichten. Keine DM-, Raum-, Sende- oder
  ACK-Routen. Nicht-Flood-Pakete bekommen keinen nachträglich erfundenen Pfad.
- Nachrichten-Historien dürfen nun bis zu 256 KiB statt 32 KiB eingelesen werden,
  damit die zusätzlichen Pfaddaten im normalen 100-Nachrichten-Verlauf Platz haben.

## Aktualisieren und prüfen

Firmware aus `firmware` mit `pio run -e tdeck_plus -t upload` bauen/flashen.
Für deutsche neue Beschriftungen auch `sdcard/mclite/lang/de.json` auf der SD
unter `/mclite/lang/de.json` ersetzen. Andernfalls greifen englische Standardtexte.
Konfiguration und Karten müssen nicht neu erstellt werden.

Hardwareprüfung: eine neue direkte und eine weitergeleitete Kanalnachricht
empfangen, Pfad öffnen, bekannte/unbekannte Relays prüfen, Karte öffnen, zoomen,
verschieben und schließen. Nach Neustart die gespeicherte Hop-Liste erneut öffnen.
Firmware-Build und diese Hardwareprüfung stehen bei Einführung noch aus.
