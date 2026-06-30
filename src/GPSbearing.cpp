#include "GPSbearing.h"
#include <math.h>

volatile bool GPSBearing::operationDone = false;

void GPSBearing::setFlag(void) {
  operationDone = true;
}

bool GPSBearing::parseCompanionGps(const String& in, double &latOut, double &lonOut) {
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

double GPSBearing::distanceMeters(double lat1, double lon1, double lat2, double lon2) {
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

double GPSBearing::bearingDegrees(double lat1, double lon1, double lat2, double lon2) {
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

bool GPSBearing::gpsProperValid(const TinyGPSPlus& gps) {
  return gps.location.isValid();
}

bool GPSBearing::companionValid(const GPSBearingContext& ctx) {
  return !(ctx.lat_companion == 0.0 && ctx.lon_companion == 0.0);
}

GPSBearingOutput GPSBearing::computeOutput(const TinyGPSPlus& gps, const GPSBearingContext& ctx) {
  GPSBearingOutput out;

  if (!(gpsProperValid(gps) && companionValid(ctx))) {
    out.valid = false;
    return out;
  }

  const double d = distanceMeters(ctx.lat_proper, ctx.lon_proper,
                                  ctx.lat_companion, ctx.lon_companion);
  const double b = bearingDegrees(ctx.lat_proper, ctx.lon_proper,
                                  ctx.lat_companion, ctx.lon_companion);

  out.valid = true;
  out.distance_m = (float)d;
  out.bearing_deg = (float)b;
  return out;
}



bool GPSBearing::handleRadioOperation(SX1262& radio, TinyGPSPlus& gps, GPSBearingContext& ctx) {
  if (!operationDone) return false;

  operationDone = false;

  if (ctx.transmitFlag) {
    // previous operation was transmission
    if (ctx.transmissionState == RADIOLIB_ERR_NONE) {
      // Serial.println(F("transmission finished!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(ctx.transmissionState);
    }

    // listen for response
    radio.startReceive();
    ctx.transmitFlag = false;

  } else {
    // previous operation was reception
    String str;
    int state = radio.readData(str);

    if (state == RADIOLIB_ERR_NONE) {
      // Try parsing companion GPS
      double latTmp, lonTmp;
      if (parseCompanionGps(str, latTmp, lonTmp)) {
        ctx.lat_companion = latTmp;
        ctx.lon_companion = lonTmp;

        // If we also have our own valid GPS, compute distance + bearing
        if (gpsProperValid(gps) && companionValid(ctx)) {
          double d = distanceMeters(ctx.lat_proper, ctx.lon_proper, ctx.lat_companion, ctx.lon_companion);
          double b = bearingDegrees(ctx.lat_proper, ctx.lon_proper, ctx.lat_companion, ctx.lon_companion);

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
      ctx.lat_proper = gps.location.lat();
      ctx.lon_proper = gps.location.lng();

      snprintf(ctx.msg, sizeof(ctx.msg),
               "%.6f,%.6f\r\n",
               ctx.lat_proper, ctx.lon_proper);
    } else {
      snprintf(ctx.msg, sizeof(ctx.msg),
               "No GPS\r\n");
    }

    ctx.transmissionState = radio.startTransmit(ctx.msg);
    ctx.transmitFlag = true;
  }

  return true;
}