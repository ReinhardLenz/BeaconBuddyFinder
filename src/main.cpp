#include <Arduino.h>
#include <RadioLib.h>
#include <Adafruit_BNO08x.h>
#include <SPI.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <config.h>
#include "compass.h"
#include "ReceiveBuddyInfo.h"
#include "SendOwnInfo.h"

// --------------------
// from BNO085 UART
// --------------------
constexpr gpio_num_t PIN_BNO_RESET = GPIO_NUM_13;

Adafruit_BNO08x bno08x(PIN_BNO_RESET);

Compass compass(bno08x);

constexpr char PROGRAM_NAME[] = "Stage 1A UART Diagnostic";
constexpr char VERSION[]      = "0.1.0";
constexpr char BUILD_DATE[]   = __DATE__;
constexpr char BUILD_TIME[]   = __TIME__;

enum class ErrorCode
{
    None,
    UART,
    BNO_NotFound,
    EnableReport,
    ProductID,
};

void fatalError(ErrorCode code)
{
    Serial.println();
    Serial.println("========== FATAL ERROR ==========");

    switch (code)
    {
        case ErrorCode::UART:
            Serial.println("Unable to communicate with BNO085.");
            break;

        case ErrorCode::BNO_NotFound:
            Serial.println("BNO085 not detected.");
            break;

        case ErrorCode::EnableReport:
            Serial.println("Could not enable report.");
            break;

        default:
            Serial.println("Unknown error.");
            break;
    }

    while (true)
    {
        digitalWrite(PIN_STATUS_LED, HIGH);
        delay(BLINK_DELAY);
        digitalWrite(PIN_STATUS_LED, LOW);
        delay(BLINK_DELAY);
    }
}

// --------------------
// pins for Lora
// --------------------
static const int LORA_NSS  = 18;
static const int LORA_DIO1 = 33;
static const int LORA_RST  = 23;
static const int LORA_BUSY = 32;

static const float LORA_FREQ = 868.0;

double lastLatProper = 0.0;
double lastLonProper = 0.0;
bool   lastProperFixValid = false;



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
// --------------------
// start BNO085 UART
// --------------------

    Serial.begin(PC_BAUD);
    Serial2.begin(BNO_BAUD, SERIAL_8N1, PIN_BNO_RX, PIN_BNO_TX);

    while (!Serial)  delay(RESET_TIME_MS);
    
    Serial.println("Adafruit BNO08x Accelerometer test!");

    if (!bno08x.begin_UART(&Serial2))
    {
        fatalError(ErrorCode::BNO_NotFound);
    }

    Serial.println("BNO08x Found!");

    Compass::setReports(&bno08x, SH2_ROTATION_VECTOR, 100000);

    Serial.println("Reading events");
    delay(100);
// --------------------
// end BNO085 UART
// --------------------

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
        bool properFixValid = gps.location.isValid();   // recommended vs (lat==0 && lon==0)

        // 1) When receiving: use the last known own position
        BuddyInfo buddy = handleRxAndCompute(radio, lastLatProper, lastLonProper, lastProperFixValid);

        if (buddy.hasSolution) {
          Serial.print(buddy.d_meters, 1);
          Serial.print(",");
          Serial.println(buddy.b_degrees, 1);
        }

        // 2) When sending: update last known own position from returned struct
        OwnInfo own = prepareAndSendOwnInfo(radio, gps, transmissionState, transmitFlag);

        lastProperFixValid = own.hasFix;
        if (own.hasFix) {
          lastLatProper = own.lat_proper;
          lastLonProper = own.lon_proper;
        }



      }

    }
}