// Must match LogKeys enum in LogDefs.h
export const LogKey = {
  LogCode: 0,
  TimeStamp: 1,
  Temperature_1: 2,
  Temperature_2: 3,
  Temperature_3: 4,
  Temperature_4: 5,
  IpAddress: 6,
  FirmwareVersion: 7,
} as const

// Must match LogCode enum in LogDefs.h
export const LogCodeValue = {
  SystemBoot: 0,
  TimeSynced: 1,
  StaConnected: 2,
  StaDisconnected: 3,
  StaReconnecting: 4,
  IpAcquired: 5,
  IpLost: 6,
  ApStarted: 7,
  ApFallback: 8,
  TemperatureReading: 9,
} as const

export const LogCodeName: Record<number, string> = {
  0: "SystemBoot",
  1: "TimeSynced",
  2: "StaConnected",
  3: "StaDisconnected",
  4: "StaReconnecting",
  5: "IpAcquired",
  6: "IpLost",
  7: "ApStarted",
  8: "ApFallback",
  9: "TemperatureReading",
}

// Decode IEEE 754 float stored as int32
const f32Buf = new ArrayBuffer(4)
const f32View = new DataView(f32Buf)
export function int32ToFloat(bits: number): number {
  f32View.setInt32(0, bits, true)
  return f32View.getFloat32(0, true)
}
