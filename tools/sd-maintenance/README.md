# T-Deck Plus SD maintenance

Standalone firmware for preparing a large microSD card **inside the T-Deck Plus**.
No card reader, mounted filesystem or MCLite config is required.

**Destructive:** after typing `FORMAT` and pressing Enter on the physical T-Deck
keyboard, the existing partition table is replaced with one FAT32 partition in
a 16 GiB area. Remaining capacity is unused. This is a quick format, not secure
erasure. Existing files will no longer be accessible. Do not use for valuable
unbacked-up data. Keep USB power connected while formatting.

## Build (PowerShell, existing project venv)

```powershell
Set-Location C:\pyprojects\MCLite
git fetch origin
git switch main
git pull --ff-only
.\.venv\Scripts\Activate.ps1
python -m platformio run -d tools/sd-maintenance -e tdeck_sd_maintenance
```

This separate PlatformIO project only builds the maintenance utility. It does
not include the MCLite application, network, credentials or config uploader.
Dependencies: ESP32 platform 6.11.0, LovyanGFX 1.2.19 and SdFat 2.3.0.

## Flash and use

Switch the T-Deck off and leave the SD card inserted. Enter the USB bootloader
(hold trackball while powering on). Check the current COM port; it can change.

```powershell
python -m serial.tools.list_ports
python -m esptool --chip esp32s3 --port COM10 --baud 460800 write_flash 0x0 .\tools\sd-maintenance\.pio\build\tdeck_sd_maintenance\tdeck-sd-maintenance-v1.bin
```

Power-cycle normally, without pressing trackball.

1. The screen shows card capacity. Detection does not modify the partition table.
2. If detection fails, formatting is unavailable: formatting cannot fix electrical
   detection problems. Power off before reseating the card.
3. On a card of at least 16 GiB, type `FORMAT`, then Enter within 60 seconds of
   the last keystroke. Lowercase is accepted. Any different character cancels.
4. Do not remove power/card during formatting. There is no automatic retry.
5. `SUCCESS` requires FAT32 mounting plus a test-file write, sync, close, reopen,
   read comparison and removal. Errors remain visible; USB serial runs at 115200.
6. Switch off and flash your **MCLite** merged firmware again. This utility does
   not create config.json. MCLite should create its default config on the FAT32
   card, after which Admin → WiFi → Config-AP provides config upload.

To rebuild and restore the current application from `main`:

```powershell
python -m platformio run -d firmware -e tdeck_plus -t upload
```

If necessary add the current port, for example `--upload-port COM10`. This
uses PlatformIO's normal firmware upload layout. When flashing a merged image
manually instead, use the actual versioned `mclite-*.bin` at offset 0x0;
never flash the unmerged firmware.bin at 0x0.

## Design / validation limits

The card is initialized at 4 MHz shared SPI using the existing board pin map.
A bounds-checked block-device adapter exposes only the first 16 GiB to SdFat's
FatFormatter. The formatter generates the MBR, FAT32 boot sectors, FATs and root
directory; partition size is slightly below 16 GiB because of alignment/metadata.
The filesystem test mounts the physical card without the size adapter.

The user successfully prepared the inserted 64 GB card on the T-Deck Plus,
then reflashed MCLite and used the card. This confirms that particular setup;
other card models and capacities have not been established by that test.
