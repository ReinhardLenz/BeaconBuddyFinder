#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

#include "ReceiveBuddyInfo.h"
#include "SendOwnInfo.h"

// --------------------
// ⚠️ Adjust these pins!
// --------------------
static const int LORA_NSS  = 18;
static const int LORA_DIO1 = 33;
static const int LORA_RST  = 23;
static const int LORA_BUSY = 32;

static const float LORA_FREQ = 868.0;

char msg[96];

SX1262 radio = SX1262(
  new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY)
);

int transmissionState = RADIOLIB_ERR_NONE;
bool transmitFlag = false;
volatile bool operationDone = false;

// Uncomment on ONE of the two nodes only
//#define INITIATING_NODE

void setFlag(void) {
  operationDone = true;
}

TinyGPSPlus gps;
HardwareSerial GPS(1);

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

  radio.setDio1Action(setFlag);

#if defined(INITIATING_NODE)
  Serial.print(F("[SX1262] Sending first packet ... "));
  transmissionState = radio.startTransmit("start transmitting");
  transmitFlag = true;
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

  
 if (operationDone) {
    operationDone = false;

      if (transmitFlag) {
        handleTxFinished(radio, transmitFlag, transmissionState);

      } else {
        handleRxAndCompute(radio, lat_proper, lon_proper);

        // ✅ Now these are "public" globals from the modules:
        /*'
        Serial.print("MAIN sees lat_proper/lon_proper: ");
        Serial.print(lat_proper, 6);
        Serial.print(", ");
        Serial.println(lon_proper, 6);

        Serial.print("MAIN sees lat_companion/lon_companion: ");
        Serial.print(lat_companion, 6);
        Serial.print(", ");
        Serial.println(lon_companion, 6);
*/
  //      Serial.print("MAIN sees d/b: ");
        Serial.print(d, 1);
        Serial.print(",");
        Serial.println(b, 1);
 //       Serial.println(" deg");

        delay(1000);

        prepareAndSendOwnInfo(radio, gps, msg, sizeof(msg), transmissionState, transmitFlag);
      }

    }
}