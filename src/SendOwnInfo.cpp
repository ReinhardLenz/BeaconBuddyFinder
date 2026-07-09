#include "SendOwnInfo.h"

// Define the globals (storage lives here)
double lat_proper = 0.0;
double lon_proper = 0.0;

void prepareAndSendOwnInfo(
  SX1262& radio,
  TinyGPSPlus& gps,
  char* msg,
  size_t msgSize,
  int& transmissionState,
  bool& transmitFlag
) {
  if (gps.location.isUpdated()) {
    lat_proper = gps.location.lat();
    lon_proper = gps.location.lng();

    snprintf(msg, msgSize, "%.6f,%.6f\r\n", lat_proper, lon_proper);
  } else {
    snprintf(msg, msgSize, "No GPS\r\n");
  }

  Serial.print("Sending: ");
  Serial.println(msg);

  transmissionState = radio.startTransmit(msg);
  transmitFlag = true;
}