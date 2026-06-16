#include <Arduino.h>
#include <RadioLib.h>
//#include <RadioLib.h>
#include <SPI.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// --------------------
// ⚠️ Adjust these pins!
// --------------------
static const int LORA_NSS  = 18;   // CS
static const int LORA_DIO1 = 33;   // DIO1 (IRQ)
static const int LORA_RST  = 23;   // RESET
static const int LORA_BUSY = 32;   // BUSY


double lat = 0.0;
double lon = 0.0;
int sats = 0;
char msg[96]; 
// SPI pins (ESP32 default VSPI is usually SCK=18, MISO=19, MOSI=23, but boards vary)
// If your board uses different SPI pins, you can set them with SPI.begin(SCK, MISO, MOSI, NSS).
// For many T-Beam boards, default SPI pins work if wired accordingly.

SX1262 radio = SX1262(
    new Module(
        LORA_NSS,
        LORA_DIO1,
        LORA_RST,
        LORA_BUSY
    )
);

static const float LORA_FREQ = 868.0;   // change to 915.0 if needed


TinyGPSPlus gps;
HardwareSerial GPS(1);

void setup() {
  Serial.begin(115200);
  GPS.begin(9600, SERIAL_8N1, 34, 12);
  Serial.println("Listening for GPS...");
  delay(1000);

  Serial.println("SX126x Sender starting...");
  SPI.begin(5, 19, 27, 18);
  // If needed, explicitly init SPI pins here:
  // SPI.begin(SCK, MISO, MOSI, LORA_NSS);

  int state = radio.begin(LORA_FREQ);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("radio.begin() failed, code = ");
    Serial.println(state);
    while (true) { delay(1000); }
  }

  // Optional: set LoRa parameters (must match receiver)
  // radio.setSpreadingFactor(7);
  // radio.setBandwidth(125.0);
  // radio.setCodingRate(5);
  // radio.setOutputPower(14); // dBm (check your module/regional limits)

  Serial.println("✅ Radio init OK");
}

void loop() {

  while (GPS.available()) {
    gps.encode(GPS.read());
  	Serial.println("GPS-OK");
  }

  if (gps.location.isUpdated()) {
 
 
   lat = gps.location.lat();
   lon = gps.location.lng();
   sats   = gps.satellites.value();

   snprintf(msg, sizeof(msg),
           "Lat: %.6f\r\nLon: %.6f\r\nSatellites: %d\r\n",
           lat, lon, sats);
 
  }


 
  Serial.print("Sending: ");
  Serial.println(msg);

  int state = radio.transmit(msg);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✅ TX success");
  } else {
    Serial.print("❌ TX failed, code = ");
    Serial.println(state);
  }

  delay(2000);
}