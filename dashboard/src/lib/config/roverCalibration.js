// Single dashboard calibration surface. These are assumed commissioning
// values, not hidden constants. Replace them after measured floor runs; valid
// live values from rover NVS take precedence when preferLiveTelemetry is true.
export const roverCalibration = Object.freeze({
  assumeCalibrated: true,
  preferLiveTelemetry: true,

  geometry: Object.freeze({
    chassisWidthMm: 220,
    trackWidthMm: 180,
    leftMicrometersPerTick: 10210.176,
    rightMicrometersPerTick: 10210.176,
    leftDistanceScale: 1.0,
    rightDistanceScale: 1.0
  }),

  pose: Object.freeze({
    startXM: 0.0,
    startYM: 0.0,
    startHeadingDeg: 0.0,
    encoderHeadingSign: 1.0,
    imuHeadingSign: -1.0,
    imuHeadingOffsetDeg: 0.0,
    imuFusionWeight: 0.20,
    initialConfidence: 0.72,
    confidenceLossPerMeter: 0.015,
    confidenceLossPerRadian: 0.020,
    minimumConfidence: 0.20,
    maximumConfidence: 0.95,
    discontinuityTicks: 10000
  }),

  scanner: Object.freeze({
    forwardAngleDeg: 90.0,
    bearingOffsetDeg: 0.0,
    minimumRangeMm: 30,
    maximumRangeMm: 1200,
    minimumConfidencePct: 50
  }),

  map: Object.freeze({
    cellSizeM: 0.05,
    footprintMarginM: 0.05,
    pixelsPerMeter: 40
  }),

  imuMount: Object.freeze({
    chassisPitchFrom: 'roll',
    chassisPitchSign: -1.0,
    chassisPitchOffsetDeg: 0.0,
    chassisRollFrom: 'pitch',
    chassisRollSign: 1.0,
    chassisRollOffsetDeg: 0.0,
    chassisYawSign: -1.0,
    chassisYawOffsetDeg: 0.0
  })
});
