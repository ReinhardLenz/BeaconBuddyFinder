#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

#include "GPSbearing.h"

// --------------------
// ⚠️ Adjust these pins!
// --------------------
static const int LORA_NSS  = 18;   // CS
static const int LORA_DIO1 = 33;   // DIO1 (IRQ)
static const int LORA_RST  = 23;   // RESET
static const int LORA_BUSY = 32;   // BUSY

static const float LORA_FREQ = 868.0;   // change to 915.0 if needed

// LoRa radio
SX1262 radio = SX1262(
  new Module(
    LORA_NSS,
    LORA_DIO1,
    LORA_RST,
    LORA_BUSY
  )
);

// GPS
TinyGPSPlus gps;
HardwareSerial GPS(1);

// All former globals moved into this context struct
GPSBearingContext ctx;

// Uncomment on ONE of the two nodes only
//#define INITIATING_NODE

void setup() {
  Serial.begin(115200);

  GPS.begin(9600, SERIAL_8N1, 34, 12);
  Serial.println("Listening for GPS...");
  delay(1000);

  Serial.println("SX126x Sender starting...");
  SPI.begin(5, 19, 27, 18);

  int state = radio.begin(LORA_FREQ);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("radio.begin() failed, code = ");
    Serial.println(state);
    while (true) { delay(1000); }
  }
  Serial.println("✅ Radio init OK");

  radio.setDio1Action(GPSBearing::setFlag);

#if defined(INITIATING_NODE)
  Serial.print(F("[SX1262] Sending first packet ... "));
  ctx.transmissionState = radio.startTransmit("start transmitting");
  ctx.transmitFlag = true;
#else
  Serial.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
#endif
}

void loop() {
  while (GPS.available()) {
    gps.encode(GPS.read());
  }

  GPSBearing::handleRadioOperation(radio, gps, ctx);

  // ---- Print distance + bearing ----
  GPSBearingOutput out = GPSBearing::computeOutput(gps, ctx);

  if (out.valid) {
    Serial.print("Distance (m): ");
    Serial.print(out.distance_m, 1);     // 1 decimal
    Serial.print(" | Bearing (deg): ");
    Serial.println(out.bearing_deg, 1);
  } else {
    // Optional: show why it's not printing yet
    // Serial.println("Waiting for valid GPS (both nodes)...");
  }

  delay(250); // optional: slow down serial spam
}