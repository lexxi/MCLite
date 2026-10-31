// Standalone SD maintenance. No MCLite configuration or filesystem required.
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <LovyanGFX.hpp>
#include <SdFat.h>
#include <initializer_list>
#include "hal/boards/tdeck_plus.h"

class TDeckDisplay : public lgfx::LGFX_Device {
public:
    lgfx::Panel_ST7789 _panel;
    lgfx::Bus_SPI      _bus;
    lgfx::Light_PWM    _light;

    TDeckDisplay() {
        auto busConfig = _bus.config();
        busConfig.spi_host   = SPI2_HOST;
        busConfig.spi_mode   = 0;
        busConfig.freq_write = 40000000;
        busConfig.freq_read  = 16000000;
        busConfig.pin_mosi   = TDECK_SPI_MOSI;
        busConfig.pin_miso   = TDECK_SPI_MISO;
        busConfig.pin_sclk   = TDECK_SPI_SCK;
        busConfig.pin_dc     = TDECK_TFT_DC;
        _bus.config(busConfig);
        _panel.setBus(&_bus);

        auto panelConfig = _panel.config();
        panelConfig.pin_cs       = TDECK_TFT_CS;
        panelConfig.pin_rst      = TDECK_TFT_RST;
        panelConfig.panel_width  = 240;   // Native portrait width
        panelConfig.panel_height = 320;   // Native portrait height
        panelConfig.offset_x     = 0;
        panelConfig.offset_y     = 0;
        panelConfig.invert       = true;
        panelConfig.rgb_order    = false;
        _panel.config(panelConfig);
        setPanel(&_panel);

        auto lightConfig = _light.config();
        lightConfig.pin_bl   = TDECK_TFT_BL;
        lightConfig.invert   = false;
        lightConfig.freq     = 12000;
        lightConfig.pwm_channel = 0;
        _light.config(lightConfig);
        _panel.setLight(&_light);
    }
};


// Limit the formatter's view, while retaining real-card LBA addresses.
// FatFormatter creates an MBR and one FAT32 partition inside this range.
class LimitedCard : public FsBlockDeviceInterface {
 public:
  SdCard* card = nullptr;
  static constexpr uint32_t LIMIT = 16UL * 1024 * 1024 * 2; // 16 GiB
  bool range(uint32_t sector, size_t n) { return sector < LIMIT && n <= LIMIT - sector; }
  bool isBusy() override { return card->isBusy(); }
  uint32_t sectorCount() override { return LIMIT; }
  bool syncDevice() override { return card->syncDevice(); }
  bool readSector(uint32_t s, uint8_t* p) override { return range(s,1) && card->readSector(s,p); }
  bool readSectors(uint32_t s, uint8_t* p, size_t n) override { return range(s,n) && card->readSectors(s,p,n); }
  bool writeSector(uint32_t s, const uint8_t* p) override { return range(s,1) && card->writeSector(s,p); }
  bool writeSectors(uint32_t s, const uint8_t* p, size_t n) override { return range(s,n) && card->writeSectors(s,p,n); }
};

TDeckDisplay screen;
SdFat32 sd;
LimitedCard limited;
bool armed = false;
bool finished = false;
String confirmation;
uint32_t lastInput;

void message(const String& text) {
  screen.fillScreen(TFT_BLACK);
  screen.setCursor(8, 8);
  screen.setTextSize(1);
  screen.setTextColor(TFT_WHITE);
  screen.println("T-Deck SD Maintenance v1");
  screen.println();
  screen.println(text);
  Serial.println(text);
}

bool verifyFilesystem() {
  // Mount through the actual device, not the size-limited wrapper: this
  // verifies the MBR is usable by the normal SD stack as well.
  if (!sd.volumeBegin() || sd.fatType() != 32) return false;
  const char* path = "/sd-maintenance-test.txt";
  const char payload[] = "T-Deck FAT32 read/write test\n";
  File32 file = sd.open(path, O_RDWR | O_CREAT | O_TRUNC);
  if (!file) return false;
  bool ok = file.write((const uint8_t*)payload, sizeof(payload)-1) == sizeof(payload)-1;
  ok = file.sync() && ok;
  file.close();
  file = sd.open(path, O_RDONLY);
  char actual[sizeof(payload)] = {};
  ok = file && file.read(actual, sizeof(payload)-1) == sizeof(payload)-1 && ok;
  ok = memcmp(actual, payload, sizeof(payload)-1) == 0 && ok;
  file.close();
  ok = sd.remove(path) && ok;
  return sd.card()->syncDevice() && ok;
}

void formatCard() {
  armed = false; // Never retry on a second Enter or after a failure.
  finished = true;
  message("Formatting...\nDo not remove card or power!\nPlease wait.");
  uint8_t sector[512];
  FatFormatter formatter;
  bool ok = formatter.format(&limited, sector, &Serial) && limited.syncDevice();
  if (ok) ok = verifyFilesystem();
  if (ok) {
    message("SUCCESS: FAT32 + read/write OK\n16 GiB area, remaining unused.\n\nPower off. Leave SD inserted.\nFlash MCLite again via USB.");
  } else {
    message("FAILED. Card may be unusable.\nSee USB serial log (115200).\nNo automatic retry.\nPower off before removing SD.");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TDECK_POWER_EN, OUTPUT);
  digitalWrite(TDECK_POWER_EN, HIGH);
  delay(500);
  for (int pin : {TDECK_TFT_CS, TDECK_LORA_CS, TDECK_SD_CS}) {
    pinMode(pin, OUTPUT); digitalWrite(pin, HIGH);
  }
  screen.init();
  screen.setRotation(1);
  screen.setBrightness(180);
  Wire1.begin(TDECK_KB_SDA, TDECK_KB_SCL, 400000);
  SPI.begin(TDECK_SPI_SCK, TDECK_SPI_MISO, TDECK_SPI_MOSI, TDECK_SD_CS);
  message("Detecting SD (no writes)...");
  // Slow shared SPI bus for initial diagnosis and reliable formatting.
  if (!sd.cardBegin(SdSpiConfig(TDECK_SD_CS, SHARED_SPI, SD_SCK_MHZ(4), &SPI))) {
    message("SD not detected. Nothing written.\nCheck seating / contacts.\nPower off before removing SD.");
    finished = true;
    return;
  }
  limited.card = sd.card();
  uint32_t sectors = sd.card()->sectorCount();
  if (sectors < LimitedCard::LIMIT) {
    message("Card smaller than 16 GiB.\nNothing written.\nUse FAT32 card in MCLite.");
    finished = true;
    return;
  }
  message("SD: " + String((unsigned long)((uint64_t)sectors * 512 / (1024*1024))) +
          " MiB\n\nALL CARD DATA WILL BE LOST!\nCreate 16 GiB FAT32 area.\n\nType FORMAT + Enter to erase.\nOther input cancels. Timeout 60s.");
  armed = true;
  lastInput = millis();
}

void loop() {
  delay(20);
  if (!armed || finished) return;
  if ((uint32_t)(millis() - lastInput) >= 60000) {
    armed = false;
    message("Timeout. Nothing written.\nRestart to try again.");
    return;
  }
  Wire1.requestFrom((uint8_t)TDECK_KB_I2C_ADDR, (uint8_t)1);
  if (!Wire1.available()) return;
  char c = Wire1.read();
  if (!c) return;
  lastInput = millis();
  if (c == '\r' || c == '\n') {
    if (confirmation == "FORMAT") formatCard();
    else { armed = false; message("Cancelled. Nothing written."); }
    return;
  }
  if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
  const char* expected = "FORMAT";
  if (confirmation.length() >= 6 || c != expected[confirmation.length()]) {
    armed = false;
    message("Cancelled. Nothing written.\nRestart to try again.");
    return;
  }
  confirmation += c;
  screen.print(c);
}
