// ReceiveBuddyInfo.cpp
#include "ReceiveBuddyInfo.h"
#include <math.h>

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

void handleTxFinished(SX1262& radio, bool& transmitFlag, int transmissionState) {
  if (transmissionState != RADIOLIB_ERR_NONE) {
    Serial.print(F("failed, code "));
    Serial.println(transmissionState);
  }
  radio.startReceive();
  transmitFlag = false;
}

BuddyInfo handleRxAndCompute(
  SX1262& radio,
  double lat_proper,
  double lon_proper,
  bool   properFixValid
) {
  BuddyInfo out;
  out.hasProperFix = properFixValid;

  String str;
  int state = radio.readData(str);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("readData failed, code ");
    Serial.println(state);
    return out;
  }

  double latTmp = 0.0, lonTmp = 0.0;
  if (!parseCompanionGps(str, latTmp, lonTmp)) {
    Serial.println("⚠️ Received message is not a valid 'lat,lon' pair (ignored).");
    return out;
  }

  out.hasCompanionFix = true;
  out.lat_companion = latTmp;
  out.lon_companion = lonTmp;

  if (out.hasProperFix && out.hasCompanionFix) {
    out.d_meters  = distanceMeters(lat_proper, lon_proper, out.lat_companion, out.lon_companion);
    out.b_degrees = bearingDegrees(lat_proper, lon_proper, out.lat_companion, out.lon_companion);
    out.hasSolution = true;
  } else {
    Serial.println("⚠️ Cannot compute distance/bearing yet (missing valid GPS fix).");
  }

  return out;
}