#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <TinyGPSPlus.h>

struct OwnInfo {
  bool   hasFix    = false;   // gps.location.isUpdated() was true
  double lat_proper = 0.0;
  double lon_proper = 0.0;

  // what we actually sent (useful for debugging/printing)
  String payload;
};

OwnInfo prepareAndSendOwnInfo(
  SX1262& radio,
  TinyGPSPlus& gps,
  int& transmissionState,
  bool& transmitFlag
);