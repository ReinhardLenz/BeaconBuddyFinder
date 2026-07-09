#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <TinyGPSPlus.h>

// Public (global) outputs from this module:
extern double lat_proper;
extern double lon_proper;

// Main entry point:
void prepareAndSendOwnInfo(
  SX1262& radio,
  TinyGPSPlus& gps,
  char* msg,
  size_t msgSize,
  int& transmissionState,
  bool& transmitFlag
);