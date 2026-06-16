#include <Arduino.h>
#include <RadioLib.h>
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
// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;
// flag to indicate that a packet was sent or received
volatile bool operationDone = false;
// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!


#define INITIATING_NODE


void setFlag(void) {
  // we sent or received a packet, set the flag
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
  // If needed, explicitly init SPI pins here:
  // SPI.begin(SCK, MISO, MOSI, LORA_NSS);

  int state = radio.begin(LORA_FREQ);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("radio.begin() failed, code = ");
    Serial.println(state);
    while (true) { delay(1000); }
  }
  Serial.println("✅ Radio init OK");

  radio.setDio1Action(setFlag);

  #if defined(INITIATING_NODE)
    // send the first packet on this node
    Serial.print(F("[SX1262] Sending first packet ... "));
    transmissionState = radio.startTransmit("start transmitting");
    transmitFlag = true;
  #else
    // start listening for LoRa packets on this node
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
  if(operationDone) {
    // reset flag
    operationDone = false;

    if(transmitFlag) {
      // the previous operation was transmission, listen for response
      // print the result
      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));

      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);

      }

      // listen for response
      radio.startReceive();
      transmitFlag = false;

    } else {
      // the previous operation was reception
      // print data and send another packet
      String str;
      int state = radio.readData(str);

      if (state == RADIOLIB_ERR_NONE) {
        // print data of the packet
        Serial.println(str);
      }

      delay(1000);
      while (GPS.available()) {
        gps.encode(GPS.read());
      }
      if (gps.location.isUpdated()) {
          lat = gps.location.lat();
          lon = gps.location.lng();
          snprintf(msg, sizeof(msg),
                    "%.6f,%.6f\r\n",
                    lat, lon);
      } else {
          snprintf(msg, sizeof(msg),
                    "No GPS\r\n");
      }
      Serial.print("Sending: ");
      Serial.println(msg);
      transmissionState = radio.startTransmit(msg);
      transmitFlag = true;
    }
  }
}