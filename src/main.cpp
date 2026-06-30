#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <math.h>

// --------------------
// ⚠️ Adjust these pins!
// --------------------
static const int LORA_NSS  = 18;   // CS
static const int LORA_DIO1 = 33;   // DIO1 (IRQ)
static const int LORA_RST  = 23;   // RESET
static const int LORA_BUSY = 32;   // BUSY

// Own GPS (proper)
double lat_proper = 0.0;
double lon_proper = 0.0;

// Companion GPS (received + parsed)
double lat_companion = 0.0;
double lon_companion = 0.0;

char msg[96];

// LoRa radio
SX1262 radio = SX1262(
  new Module(
    LORA_NSS,
    LORA_DIO1,
    LORA_RST,
    LORA_BUSY
  )
);
//TEST LINE

static const float LORA_FREQ = 868.0;   // change to 915.0 if needed

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// Uncomment on ONE of the two nodes only
//#define INITIATING_NODE

void setFlag(void) {
  operationDone = true;
}

TinyGPSPlus gps;
HardwareSerial GPS(1);

// --------------------
// Helpers: parsing + geo math
// --------------------
bool parseCompanionGps(const String& in, double &latOut, double &lonOut) {
  String s = in;
  s.trim();

  int comma = s.indexOf(',');
  if (comma < 0) return false;

  String latStr = s.substring(0, comma);
  String lonStr = s.substring(comma + 1);

  latStr.trim();
  lonStr.trim();

  if (latStr.length() == 0 || lonStr.length() == 0) return false;

  latOut = latStr.toDouble();
  lonOut = lonStr.toDouble();

  if (latOut < -90.0 || latOut > 90.0) return false;
  if (lonOut < -180.0 || lonOut > 180.0) return false;

  return true;
}

static inline double deg2rad(double deg) { return deg * (M_PI / 180.0); }
static inline double rad2deg(double rad) { return rad * (180.0 / M_PI); }

// Distance in meters (Haversine)
double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0; // Earth radius in meters
  double phi1 = deg2rad(lat1);
  double phi2 = deg2rad(lat2);
  double dphi = deg2rad(lat2 - lat1);
  double dlambda = deg2rad(lon2 - lon1);

  double a = sin(dphi/2.0) * sin(dphi/2.0) +
             cos(phi1) * cos(phi2) *
             sin(dlambda/2.0) * sin(dlambda/2.0);

  double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  return R * c;
}

// Bearing in degrees (0..360), 0=N, 90=E
double bearingDegrees(double lat1, double lon1, double lat2, double lon2) {
  double phi1 = deg2rad(lat1);
  double phi2 = deg2rad(lat2);
  double dlambda = deg2rad(lon2 - lon1);

  double y = sin(dlambda) * cos(phi2);
  double x = cos(phi1) * sin(phi2) - sin(phi1) * cos(phi2) * cos(dlambda);

  double theta = atan2(y, x);          // -pi..pi
  double brng = rad2deg(theta);        // -180..180
  brng = fmod((brng + 360.0), 360.0);  // 0..360
  return brng;
}

bool gpsProperValid() {
  // TinyGPSPlus: location is valid only after a fix
  return gps.location.isValid();
}

bool companionValid() {
  // Basic check: you can improve this if you want
  return !(lat_companion == 0.0 && lon_companion == 0.0);
}

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
  // Continuously feed GPS decoder (recommended)
  while (GPS.available()) {
    gps.encode(GPS.read());
  }

  if (operationDone) {
    operationDone = false;

    if (transmitFlag) {
      // previous operation was transmission
      if (transmissionState == RADIOLIB_ERR_NONE) {
        //Serial.println(F("transmission finished!"));
      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);
      }

      // listen for response
      radio.startReceive();
      transmitFlag = false;

    } else {
      // previous operation was reception
      String str;
      int state = radio.readData(str);

      if (state == RADIOLIB_ERR_NONE) {
        //Serial.print("Received raw: ");
        //Serial.println(str);

        // Try parsing companion GPS
        double latTmp, lonTmp;
        if (parseCompanionGps(str, latTmp, lonTmp)) {
          lat_companion = latTmp;
          lon_companion = lonTmp;

          /*Serial.print("✅ Parsed companion GPS -> Latitude = ");
          Serial.print(lat_companion, 6);
          Serial.print(" Longitude = ");
          Serial.println(lon_companion, 6); */

          // If we also have our own valid GPS, compute distance + bearing
          if (gpsProperValid() && companionValid()) {
            double d = distanceMeters(lat_proper, lon_proper, lat_companion, lon_companion);
            double b = bearingDegrees(lat_proper, lon_proper, lat_companion, lon_companion);

            //Serial.print("Distance to companion: ");
            //Serial.print(d, 1);
            //Serial.println(" m");

            //Serial.print("Bearing to companion: ");
            //Serial.print(b, 1);
            //Serial.println(" deg (0=N, 90=E)");
          
            Serial.print(d, 1);
            Serial.print(",");
            Serial.println(b, 1);
          
          } else {
            Serial.println("⚠️ Cannot compute distance");
          }

        } else {
          Serial.println("⚠️ Received message is not a valid");
        }
      } else {
        Serial.print("readData failed, code ");
        Serial.println(state);
      }

      // Prepare and send own GPS
      delay(1000);

      if (gps.location.isUpdated()) {
        lat_proper = gps.location.lat();
        lon_proper = gps.location.lng();

        snprintf(msg, sizeof(msg),
                 "%.6f,%.6f\r\n",
                 lat_proper, lon_proper);
      } else {
        snprintf(msg, sizeof(msg),
                 "No GPS\r\n");
      }

      //Serial.print("Sending: ");
      //Serial.println(msg);

      transmissionState = radio.startTransmit(msg);
      transmitFlag = true;
    }
  }
}