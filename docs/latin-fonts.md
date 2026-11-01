# Latin-1 display support

Fork version `0.4.6-ap.2` adds U+00A0–U+00FF to all six UI font sizes,
including `Ä Ö Ü ä ö ü ß`, accented Western European letters, and symbols.
LVGL's bundled Montserrat fonts do not contain these letters. A missing glyph
previously appeared as a rectangle even when the received UTF-8 was correct.

The font chain for messages is now:

`colour emoji (if enabled) -> monochrome emoji -> Latin-1 -> stock Montserrat`

Other UI labels use the Latin-1 supplement with stock Montserrat as fallback.
ASCII, icons, and emoji keep their existing glyphs. No config migration or SD
card changes are needed. Rebuild and flash the firmware.

Generated font sources are committed. To regenerate, use
`firmware/assets/fonts/generate_latin_fonts.sh` with a Montserrat Medium TTF
(weight 500). The original source is Google Fonts' Montserrat variable font;
the Medium instance was produced with fontTools. Montserrat is licensed under
SIL OFL 1.1; see `firmware/assets/fonts/Montserrat-OFL.txt`.

## Device check

Send `Grüße aus Kärnten: ÄÖÜ äöü ß - Test` from the T114 companion app.
Verify the seven German letters in the received bubble and in saved history
after a restart. Repeat with colour emoji enabled and disabled.

## Entering German letters (0.4.6-ap.3)

In a chat, tap the emoji button beside the input field. The first row now
contains `ä ö ü Ä Ö Ü ß`. Tap a letter to append it to the draft; the picker
closes and the keyboard returns to the input field. Nothing is sent until you
press Send. This uses the existing UTF-8 byte-limit check (two bytes per letter).
The picker requires `display.emoji` to be enabled, as before.

Device check: compose and send `Grüße` using the picker, then verify the text
on the T114. Check both lower-case and upper-case letters and an existing emoji.
