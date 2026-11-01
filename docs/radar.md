# Heard-node radar (0.4.6-ap.4)

Open **Device Info -> Heard Adverts -> Radar**. The radar uses the same bounded
64-node RAM cache as Heard Adverts, including nodes not saved as contacts. It
listens passively; opening it does not send discovery requests or other packets.
The cache is lost on reboot and fills as advertisements arrive.

- **Hops:** all cached nodes, with rings for direct reception (0), 1, 2 and 3+
  repeater hops. The angle is a stable arrangement derived from the public key,
  **not a measured direction**. Ring spacing is not geographic distance.
- **Geo / N:** north stays up. Only nodes with advertised coordinates and a
  usable own GPS position are shown. Own position can be live or a last known
  fix accepted by the existing GPS age policy. A last known fix is labelled.
  The outer ring is the displayed range in kilometres, with a linear scale.
  Nodes without coordinates are excluded, not given an invented position.
- The first visit chooses Geo if both own position and a node position are
  available; otherwise Hops. The header button switches modes manually. If GPS
  is lost while Geo is selected, the page explains why it has no points; switch
  to Hops to see all heard nodes.
- The count is `shown / cached`, not the number of nodes guaranteed reachable.
  Advertised positions can be old, and an advert can arrive via repeaters.
- Blue = participant, orange = repeater, purple = room, green = sensor.
  The centre is your device. The selected point has a white outline. A new
  advert briefly brightens its point; old points fade after 10 minutes / 1 hour.
- Tap a point to open its existing Heard Adverts detail dialog. Alternatively
  select with **< / >**, then **Info** (useful for overlapping points). The sidebar
  shows name, distance/bearing or hops, seconds since reception, and node type.
  The detail dialog opens on the Heard Adverts page. Open Radar there to return.
  The back arrow returns to the list.

No map tiles, Wi-Fi, config migration, or SD changes are required. New translated
strings fall back to English when existing SD language files are older. Updated
German/French/Italian files are in `sdcard/mclite/lang/` and in the Config Tool.
The radar is an independent implementation using existing MCLite/LVGL APIs;
no Wadamesh source was copied into this MIT-licensed fork.

## Validation

- Native tests cover cardinal bearings, distances including date-line crossing
  and antipodes, invalid coordinates, hop-ring limits, stable schematic angles,
  and reception-age arithmetic across `millis()` wrap.
- Headless LVGL rendering checks the 320x240 layout and show/hide/show lifecycle.
- Hardware checks: open with an empty cache; receive adverts from T114; inspect
  a point; return to the list; try both modes without GPS and after a GPS fix;
  check received-node counts and a known advertised coordinate. Repeat after
  screen lock/unlock and after leaving the radar for a chat.
