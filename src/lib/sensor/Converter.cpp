/******************************************************************************
 * Copyright (C) 2011 by Jerome Maye                                          *
 * jerome.maye@gmail.com                                                      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * Lesser GNU General Public License for more details.                        *
 *                                                                            *
 * You should have received a copy of the Lesser GNU General Public License   *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 ******************************************************************************/

#include "sensor/Converter.h"

/******************************************************************************/
/* Methods                                                                    */
/******************************************************************************/

namespace Converter {

void toPointCloud(const DataPacket& dataPacket, const Calibration&
    calibration, VdynePointCloud& pointCloud, float minDistance, float
    maxDistance) {
  pointCloud.setTimestamp(dataPacket.getTimestamp());
  for (size_t i = 0; i < dataPacket.mDataChunkNbr; ++i) {
    size_t idxOffs = 0;
    const DataPacket::DataChunk& data = dataPacket.getDataChunk(i);
    if (data.mHeaderInfo == dataPacket.mLowerBank)
      idxOffs = data.mLasersPerPacket;
    const float rotation =
      calibration.deg2rad(static_cast<float>(data.mRotationalInfo) /
      static_cast<float>(dataPacket.mRotationResolution));
    if (i == 0)
      pointCloud.setStartRotationAngle(rotation);
    else if (i == dataPacket.mDataChunkNbr -1)
      pointCloud.setEndRotationAngle(rotation);
    for (size_t j = 0; j < data.mLasersPerPacket; ++j) {
      size_t laserIdx = idxOffs + j;
      const float distance = (calibration.getDistCorr(laserIdx)
        + static_cast<float>(data.mLaserData[j].mDistance) /
        static_cast<float>(dataPacket.mDistanceResolution)) /
        static_cast<float>(mMeterConversion);
      if ((distance < minDistance) || (distance > maxDistance))
        continue;
      const float sinRot = sin(rotation) *
        calibration.getCosRotCorr(laserIdx) -
        cos(rotation) * calibration.getSinRotCorr(laserIdx);
      const float cosRot = cos(rotation) *
        calibration.getCosRotCorr(laserIdx) +
        sin(rotation) * calibration.getSinRotCorr(laserIdx);
      const float horizOffsCorr =
        calibration.getHorizOffsCorr(laserIdx) /
        static_cast<float>(mMeterConversion);
      const float vertOffsCorr =
        calibration.getVertOffsCorr(laserIdx) /
        static_cast<float>(mMeterConversion);
      const float xyDist = distance *
        calibration.getCosVertCorr(laserIdx) -
        vertOffsCorr * calibration.getSinVertCorr(laserIdx);
      VdynePointCloud::Point3D point;
      point.mX = xyDist * sinRot - horizOffsCorr * cosRot;
      point.mY = xyDist * cosRot + horizOffsCorr * sinRot;
      point.mZ = distance *
        calibration.getSinVertCorr(laserIdx) + vertOffsCorr *
        calibration.getCosVertCorr(laserIdx);
      point.mIntensity = data.mLaserData[j].mIntensity;
      pointCloud.insertPoint(point);
    }
  }
}

void toScanCloud(const DataPacket& dataPacket, const Calibration&
    calibration, VdyneScanCloud& scanCloud, float minDistance, float
    maxDistance) {
  scanCloud.setTimestamp(dataPacket.getTimestamp());
  for (size_t i = 0; i < dataPacket.mDataChunkNbr; ++i) {
    size_t idxOffs = 0;
    const DataPacket::DataChunk& data = dataPacket.getDataChunk(i);
    if (data.mHeaderInfo == dataPacket.mLowerBank)
      idxOffs = data.mLasersPerPacket;
    const float rotation =
      calibration.deg2rad(static_cast<float>(data.mRotationalInfo) /
      static_cast<float>(dataPacket.mRotationResolution));
    if (i == 0)
      scanCloud.setStartRotationAngle(rotation);
    else if (i == dataPacket.mDataChunkNbr -1)
      scanCloud.setEndRotationAngle(rotation);
    for (size_t j = 0; j < data.mLasersPerPacket; ++j) {
      size_t laserIdx = idxOffs + j;
      const float distance = (calibration.getDistCorr(laserIdx)
        + static_cast<float>(data.mLaserData[j].mDistance) /
        static_cast<float>(dataPacket.mDistanceResolution)) /
        static_cast<float>(mMeterConversion);
      if ((distance < minDistance) || (distance > maxDistance))
        continue;
      VdyneScanCloud::Scan scan;
      scan.mRange = distance;
      scan.mHeading = normalizeAngle(-(rotation -
        calibration.getRotCorr(laserIdx)));
      scan.mPitch = calibration.getVertCorr(laserIdx);
      scan.mIntensity = data.mLaserData[j].mIntensity;
      scanCloud.insertScan(scan);
    }
  }
}

float normalizeAngle(float angle) {
  float value = normalizeAnglePositive(angle);
  if (value > M_PI)
    value -= 2.0 * M_PI;
  return value;
}

}
