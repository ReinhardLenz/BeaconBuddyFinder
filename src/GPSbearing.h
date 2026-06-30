#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <TinyGPSPlus.h>

// Holds all state that used to be global in main.cpp
struct GPSBearingContext {
  // Own GPS (proper)
  double lat_proper = 0.0;
  double lon_proper = 0.0;

  // Companion GPS (received + parsed)
  double lat_companion = 0.0;
  double lon_companion = 0.0;

  // Radio state machine variables
  int  transmissionState = RADIOLIB_ERR_NONE;
  bool transmitFlag = false;

  // Message buffer
  char msg[96] = {0};
};

// ✅ Clean output for consumers (belt/UI/etc.)
struct GPSBearingOutput {
  bool  valid = false;
  float distance_m = 0.0f;
  float bearing_deg = 0.0f; // 0..360, 0=N, 90=E
};



class GPSBearing {
public:
  // This is the IRQ flag set by DIO1 callback
  static volatile bool operationDone;

  // DIO1 callback
  static void setFlag(void);

  // Parsing + geo math
  static bool parseCompanionGps(const String& in, double &latOut, double &lonOut);

  static double distanceMeters(double lat1, double lon1, double lat2, double lon2);
  static double bearingDegrees(double lat1, double lon1, double lat2, double lon2);

  static bool gpsProperValid(const TinyGPSPlus& gps);
  static bool companionValid(const GPSBearingContext& ctx);

  // Handles the whole "if (operationDone) { ... }" block from your loop.
  // Returns true if it processed an operation (TX done or RX done), else false.
  static bool handleRadioOperation(SX1262& radio, TinyGPSPlus& gps, GPSBearingContext& ctx);

  //  compute a clean output snapshot from current context
  static GPSBearingOutput computeOutput(const TinyGPSPlus& gps, const GPSBearingContext& ctx);

};