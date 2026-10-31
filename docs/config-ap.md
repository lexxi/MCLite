# Config upload over the device's WiFi access point

Fork addition: `0.4.6-ap.1`.

1. Insert the microSD and boot the T-Deck Plus.
2. Open Admin → WiFi → **Config-AP starten**. Full settings permission is required.
3. Connect your computer/phone to the displayed `MCLite-Config-…` network using the displayed random password. Stay connected even if the OS reports no internet.
4. Open **http://192.168.4.1** and select the `config.json` exported by the config tool. Upload the JSON itself, not the ZIP.
5. Confirm **Prüfen und übernehmen**. On success the device restarts automatically.

The AP stops after ten minutes, when stopped explicitly, or when leaving the WiFi screen. Existing home-WiFi credentials are retained but the old WiFi/companion connection is stopped. Reconnect manually afterwards. If Bluetooth has been initialized, reboot before starting the AP (the existing firmware's RAM restriction).

The portal has no external scripts and requires no internet. Its randomly generated WPA password is session-only, one client is allowed, and an upload token protects the POST endpoint. No configuration download endpoint is exposed. Files are limited to 32768 bytes, matching the boot loader's configuration read limit.

Validation checks JSON/object structure, device name, required radio values, array types, identity encoding/length, and parsing using ConfigManager. This is not a complete schema validator or a cryptographic check that supplied public/private keys match. Omit `identity` to preserve this device's current keys; a supplied identity replaces them. Review it before upload.

The previous file is kept as `/config.pre-upload.json`. The new file is written using the existing staged/rename save method. Live settings stay unchanged until restart, and ordinary saves are blocked after a successful import so they cannot overwrite it. On validation failure the active file is untouched; on storage failure the portal reports an error. If recovery is needed, power off and copy `config.pre-upload.json` to `config.json` using a card reader. Keep this backup private: it contains keys.

## Validation

Native ConfigManager tests cover identity preservation, backup, unchanged live config until reboot, blocked stale saves/re-import, malformed input, invalid keys, and missing storage. Hardware tests still required: join the AP, upload from desktop/iOS, reject corrupt/oversize files, reboot persistence, timeout and stop, and BLE-to-WiFi reboot behaviour.

Build: `pio run -d firmware -e tdeck_plus`. This fork checks OTA releases in `lexxi/MCLite` to avoid replacing the custom firmware with upstream firmware.
