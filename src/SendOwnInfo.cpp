// SendOwnInfo.cpp
#include "SendOwnInfo.h"

OwnInfo prepareAndSendOwnInfo(
  SX1262& radio,
  TinyGPSPlus& gps,
  int& transmissionState,
  bool& transmitFlag
) {
  OwnInfo out;

  if (gps.location.isUpdated()) {
    out.hasFix = true;
    out.lat_proper = gps.location.lat();
    out.lon_proper = gps.location.lng();

    char buf[48];
    snprintf(buf, sizeof(buf), "%.6f,%.6f\r\n", out.lat_proper, out.lon_proper);
    out.payload = buf;
  } else {
    out.payload = "No GPS\r\n";
  }

  transmissionState = radio.startTransmit(out.payload.c_str());
  transmitFlag = true;

  return out;
}