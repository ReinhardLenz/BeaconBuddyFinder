#include "ReceiveBuddyInfo.h"
#include <math.h>

// Define the globals (storage lives here)
double lat_companion = 0.0;
double lon_companion = 0.0;
double d = 0.0;
double b = 0.0;

static inline double deg2rad(double deg) { return deg * (M_PI / 180.0); }
static inline double rad2deg(double rad) { return rad * (180.0 / M_PI); }

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

double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0;
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

double bearingDegrees(double lat1, double lon1, double lat2, double lon2) {
  double phi1 = deg2rad(lat1);
  double phi2 = deg2rad(lat2);
  double dlambda = deg2rad(lon2 - lon1);

  double y = sin(dlambda) * cos(phi2);
  double x = cos(phi1) * sin(phi2) - sin(phi1) * cos(phi2) * cos(dlambda);

  double theta = atan2(y, x);
  double brng = rad2deg(theta);
  brng = fmod((brng + 360.0), 360.0);
  return brng;
}

static bool companionValid() {
  return !(lat_companion == 0.0 && lon_companion == 0.0);
}

static bool properValid(double lat_proper, double lon_proper) {
  // If you prefer TinyGPSPlus validity, keep that check in main.cpp and pass a bool instead.
  return !(lat_proper == 0.0 && lon_proper == 0.0);
}

void handleTxFinished(SX1262& radio, bool& transmitFlag, int transmissionState) {
  if (transmissionState == RADIOLIB_ERR_NONE) {
//    Serial.println(F("transmission finished!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(transmissionState);
  }

  radio.startReceive();
  transmitFlag = false;
}

void handleRxAndCompute(SX1262& radio, double lat_proper, double lon_proper) {
  String str;
  int state = radio.readData(str);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("readData failed, code ");
    Serial.println(state);
    return;
  }

//  Serial.print("Received raw: ");
//  Serial.println(str);

  double latTmp, lonTmp;
  if (!parseCompanionGps(str, latTmp, lonTmp)) {
    Serial.println("⚠️ Received message is not a valid 'lat,lon' pair (ignored).");
    return;
  }

  lat_companion = latTmp;
  lon_companion = lonTmp;
/*
  Serial.print("✅ Parsed companion GPS -> Latitude = ");
  Serial.print(lat_companion, 6);
  Serial.print(" Longitude = ");
  Serial.println(lon_companion, 6);
*/
  if (properValid(lat_proper, lon_proper) && companionValid()) {
    d = distanceMeters(lat_proper, lon_proper, lat_companion, lon_companion);
    b = bearingDegrees(lat_proper, lon_proper, lat_companion, lon_companion);
/*
    Serial.print("📏 Distance to companion: ");
    Serial.print(d, 1);
    Serial.println(" m");

    Serial.print("🧭 Bearing to companion: ");
    Serial.print(b, 1);
    Serial.println(" deg (0=N, 90=E)");
  */  
  } else {
    Serial.println("⚠️ Cannot compute distance/bearing yet (missing valid GPS fix).");
  }
}