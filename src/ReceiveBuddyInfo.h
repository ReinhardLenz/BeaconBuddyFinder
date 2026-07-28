// ReceiveBuddyInfo.h
#pragma once
#include <Arduino.h>
#include <RadioLib.h>

struct BuddyInfo {
  bool hasCompanionFix = false;   // parsed companion lat/lon successfully
  bool hasProperFix    = false;   // caller says own GPS is valid
  bool hasSolution     = false;   // distance/bearing computed

  double lat_companion = 0.0;
  double lon_companion = 0.0;
  double d_meters      = 0.0;
  double b_degrees     = 0.0;
};

// Helpers exposed (optional)
bool parseCompanionGps(const String& in, double &latOut, double &lonOut);
double distanceMeters(double lat1, double lon1, double lat2, double lon2);
double bearingDegrees(double lat1, double lon1, double lat2, double lon2);

// Main entry points
void handleTxFinished(SX1262& radio, bool& transmitFlag, int transmissionState);

// Now returns the computed values instead of writing globals
BuddyInfo handleRxAndCompute(
  SX1262& radio,
  double lat_proper,
  double lon_proper,
  bool   properFixValid
);