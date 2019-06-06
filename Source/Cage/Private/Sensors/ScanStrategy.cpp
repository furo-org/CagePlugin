// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ScanStrategy.h"
#include "Lidar.h"

void UScan2DStrategy::init(ALidar *lidar)
{
  StartHAngle = lidar->StartHAngle;
  EndHAngle = lidar->EndHAngle;
  StepHAngle = lidar->StepHAngle;
  Rpm = lidar->Rpm;
}

float UScan2DStrategy::GetVectors(VecType &vec, float currentTime)
{
  float speed = Rpm / 60. * 360;          // 回転速度 [deg/s]
  float mtime = StepHAngle / speed;       // 1点の計測にかかる時間 [sec]
  float dur = currentTime - LastTime;     // 今回計算が必要な時間 [sec]
  int npts = dur / mtime;                 // 今回計算する計測方位数
  float mdur = npts * mtime;               // 最後に計測する時刻までの時間[sec] (durから1点計測にかかる時間未満の余剰分を差し引いたもの)
  for (int i = 1; i <= npts; ++i) {       // i==0は前回の最後の計測
    auto factor = i * mtime / dur;
    auto yaw = Yaw + StepHAngle * i;
    while (yaw > 180.)yaw -= 360;
    vec.Emplace(FRotator(0, yaw, 0), factor);
  }
  Yaw += StepHAngle * npts;
  while (Yaw > 180)Yaw -= 360.;
  LastTime += mdur;
  return LastTime;
}

void UVLPScanStrategy::init(ALidar *lidar)
{
  StartHAngle = lidar->StartHAngle;
  EndHAngle = lidar->EndHAngle;
  StepHAngle = lidar->StepHAngle;
  Rpm = lidar->Rpm;
  ensure(HOffsets.Num() == 0 || (HOffsets.Num() == VAngles.Num()));
}

float UVLPScanStrategy::GetVectors(VecType &vec, float currentTime)
{
  float speed = Rpm / 60. * 360;          // 回転速度 [deg/s]
  float mtime = StepHAngle / speed;       // 1点の計測にかかる時間 [sec]
  float dur = currentTime - LastTime;     // 今回計算が必要な時間 [sec]
  int npts = dur / mtime;                 // 今回計算する計測方位数
  float mdur = npts * mtime;               // 最後に計測する時刻までの時間[sec] (durから1点計測にかかる時間未満の余剰分を差し引いたもの)
  vec.Reserve(VAngles.Num()*npts);
  for (int i = 1; i <= npts; ++i) {       // i==0は前回の最後の計測
    auto factor = i * mtime / dur;
    auto yawBase = Yaw + StepHAngle * i;
    for (int ch = 0, ec = VAngles.Num(); ch != ec;++ch){
      auto yaw = yawBase;
      if (HOffsets.Num() != 0) {
        yaw += HOffsets[ch];
        while (yaw > 180.)yaw -= 360;
        while (yaw < -180.)yaw += 360;
      }
      vec.Emplace(FRotator(VAngles[ch], yaw, 0), factor);
    }
  }
  Yaw += StepHAngle * npts;
  while (Yaw > 180)Yaw -= 360.;
  LastTime += mdur;
  return LastTime;
}