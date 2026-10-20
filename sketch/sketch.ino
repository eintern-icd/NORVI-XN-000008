/*
 * NPD-NORVI X-AI4 V2 (N16R2)
 * 2026.08.24
 * UPGRADED WITH LOVYANGFX
*/

#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_ADS1X15.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// If you have your image data in an external file, include it here:
// #include "your_image_file.h" 

#define SDA   8
#define SCL   9

#define MISO  13
#define MOSI  11
#define SCLK  12

// SPI Isolation Pins (To prevent screen flashing on boot)
#define ETH_CS 1
#define SD_chipSelect 42
#define DSP_CS 45

#define PORT_RST 38

/*
 ADS1115 ADDRESS
 DIP1(ON) - 0x49
 DIP2(ON) - 0x4B
 DIP3(ON) - 0x4A
 DIP4(ON) - 0x48
*/

#define ADS_ADDR 0x49
Adafruit_ADS1115 ads1;

const float mA_Factor = 4.096 / 3269.826;

// ---------------- LOVYANGFX SETUP ----------------
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;
public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 12; 
      cfg.pin_mosi = 11; 
      cfg.pin_miso = 13; 
      cfg.pin_dc   = 46; 
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = 45; 
      cfg.pin_rst          = 47; 
      cfg.pin_busy         = -1;
      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = true; 
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true; 
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};

LGFX tft;

unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 100;

float lastDisplay0 = -999.0;
float lastDisplay1 = -999.0;
float lastDisplay2 = -999.0;
float lastDisplay3 = -999.0;

// FUNCTION PROTOTYPES
void I2C_SCAN();
void LOGO_PRINT();

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("NPD-NORVI X-AI4 TEST");

  // 1. Wake up expansion bus
  pinMode(PORT_RST, OUTPUT);
  digitalWrite(PORT_RST, HIGH); 
  delay(100);

  // 2. Isolate shared SPI pins to protect the display
  pinMode(ETH_CS, OUTPUT); digitalWrite(ETH_CS, HIGH); 
  pinMode(SD_chipSelect, OUTPUT); digitalWrite(SD_chipSelect, HIGH); 
  pinMode(DSP_CS, OUTPUT); digitalWrite(DSP_CS, HIGH); 
  
  // i2c
  Wire.begin(SDA, SCL);

  if (!ads1.begin(ADS_ADDR)) {
    Serial.println("Failed to initialize ADS 1.");
  } else {
    Serial.println("ADS1115 OK");
  }

  ads1.setGain(GAIN_ONE);

  // FAST ADC
  ads1.setDataRate(RATE_ADS1115_860SPS);

  Serial.print("ADS1115 ADDRESS: 0x");
  if (ADS_ADDR < 0x10) {
    Serial.print("0");
  }
  Serial.println(ADS_ADDR, HEX);
  Serial.println("ADS1115 DATA RATE = 860 SPS");

  // SPI
  SPI.begin(SCLK, MISO, MOSI);
  delay(100);

  // TFT Initialize
  tft.init();
  tft.setRotation(0);
  
  // LOGO_PRINT(); // Uncomment this when you add your image array data back in
  
  tft.fillScreen(TFT_BLACK);

  // TITLE
  tft.setTextColor(TFT_GREEN);
  tft.setFont(&fonts::FreeSansBold12pt7b); // LovyanGFX native font equivalent to FSB12
  
  tft.setCursor(60, 20);
  tft.print("NORVI");

  tft.setCursor(40, 50);
  tft.print("X-AI4 TEST");

  // CURRENT LABELS
  tft.setFont(&fonts::FreeSansBold9pt7b); // LovyanGFX native font equivalent to FSB9
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  
  tft.setCursor(10, 100);
  tft.print("CURRENT 1 :");

  tft.setCursor(10, 130);
  tft.print("CURRENT 2 :");

  tft.setCursor(10, 160);
  tft.print("CURRENT 3 :");

  tft.setCursor(10, 190);
  tft.print("CURRENT 4 :");

  // INITIAL CURRENT VALUES
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  tft.setCursor(145, 100);
  tft.print("0.00 mA");

  tft.setCursor(145, 130);
  tft.print("0.00 mA");

  tft.setCursor(145, 160);
  tft.print("0.00 mA");

  tft.setCursor(145, 190);
  tft.print("0.00 mA");

  // ADS1115 ADDRESS AT BOTTOM
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, 240);
  tft.print("ADS ADDR : 0x");

  if (ADS_ADDR < 0x10) {
    tft.print("0");
  }
  tft.print(ADS_ADDR, HEX);

  Serial.println("TEST START");
}


void loop() {
  int16_t adc0;
  int16_t adc1;
  int16_t adc2;
  int16_t adc3;

  adc0 = ads1.readADC_SingleEnded(0);
  adc1 = ads1.readADC_SingleEnded(1);
  adc2 = ads1.readADC_SingleEnded(2);
  adc3 = ads1.readADC_SingleEnded(3);

  // ADC TO CURRENT
  float current0 = adc0 * mA_Factor;
  float current1 = adc1 * mA_Factor;
  float current2 = adc2 * mA_Factor;
  float current3 = adc3 * mA_Factor;

  // SERIAL MONITOR
  Serial.printf(
    "CH1: %.2f mA  "
    "CH2: %.2f mA  "
    "CH3: %.2f mA  "
    "CH4: %.2f mA\n",
    current0,
    current1,
    current2,
    current3
  );

  // TFT UPDATE
  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = millis();

    // Using TFT_WHITE for text and TFT_BLACK for background 
    // automatically overwrites the old text cleanly without needing fillRect()!
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    // CURRENT 1
    if (current0 != lastDisplay0) {
      tft.setCursor(145, 100);
      tft.printf("%5.2f mA", current0);
      lastDisplay0 = current0;
    }
    
    // CURRENT 2
    if (current1 != lastDisplay1) {
      tft.setCursor(145, 130);
      tft.printf("%5.2f mA", current1);
      lastDisplay1 = current1;
    }

    // CURRENT 3
    if (current2 != lastDisplay2) {
      tft.setCursor(145, 160);
      tft.printf("%5.2f mA", current2);
      lastDisplay2 = current2;
    }

    // CURRENT 4
    if (current3 != lastDisplay3) {
      tft.setCursor(145, 190);
      tft.printf("%5.2f mA", current3);
      lastDisplay3 = current3;
    }
  }
}

// I2C SCANNER
void I2C_SCAN() {
  byte error;
  byte address;
  int deviceCount = 0;

  Serial.println();
  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println(" !");
      deviceCount++;
      delay(1);
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }

  if (deviceCount == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.println("Scanning complete");
  }
}

//logo
void LOGO_PRINT() {
  tft.setSwapBytes(true);

  // WHITE BACKGROUND
  tft.fillScreen(TFT_WHITE);

  // DISPLAY IMAGE 
  // (Assuming you have IMAGE_WIDTH, IMAGE_HEIGHT, and image_data defined somewhere)
  /*
  tft.pushImage(
    0,
    0,
    IMAGE_WIDTH,
    IMAGE_HEIGHT,
    image_data
  );
  */
  
  delay(2000);
  tft.fillScreen(TFT_BLACK);
}