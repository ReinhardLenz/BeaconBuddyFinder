#pragma once
#include <Arduino.h>
#include <RadioLib.h>

// Public (global) outputs from this module:
extern double lat_companion;
extern double lon_companion;
extern double d;   // distance (meters)
extern double b;   // bearing (degrees)

// Helpers exposed (used by main.cpp or other modules if needed)
bool parseCompanionGps(const String& in, double &latOut, double &lonOut);
double distanceMeters(double lat1, double lon1, double lat2, double lon2);
double bearingDegrees(double lat1, double lon1, double lat2, double lon2);

// Main entry points for your split logic:
void handleTxFinished(SX1262& radio, bool& transmitFlag, int transmissionState);

void handleRxAndCompute(
  SX1262& radio,
  double lat_proper,
  double lon_proper
);