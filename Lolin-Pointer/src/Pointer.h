#pragma once
#include "Ascom.h"
#include "AstroClock.h"
#include "Phy.h"

class Pointer : public Ascom {
 private:
  bool connected;
  AstroClock as;
  Phy phy;
  ////Parking
  boolean parked;
  int parkAlt;
  int parkAz;
  ////Target
  double targetRA;
  double targetDec;
  double nextTargetRA;
  double nextTargetDec;
  ////Tracking
  boolean isTracking;

 public:
  Pointer();
  void tick();
};