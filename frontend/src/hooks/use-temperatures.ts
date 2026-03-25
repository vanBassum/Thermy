import { useEffect, useState, useCallback } from "react"
import { backend, type SensorReading, type HistorySample } from "@/lib/backend"

export const MAX_SAMPLES = 1024

export interface ChartPoint {
  time: string
  slot0?: number
  slot1?: number
  slot2?: number
  slot3?: number
}

function samplesToChartPoints(samples: HistorySample[], rateSec: number): ChartPoint[] {
  const now = Date.now()
  return samples.map((s, i) => {
    const ageMs = (samples.length - 1 - i) * rateSec * 1000
    const t = new Date(now - ageMs)
    const point: ChartPoint = { time: t.toLocaleTimeString() }
    if (s.s0 !== null) point.slot0 = Math.round(s.s0 * 10) / 10
    if (s.s1 !== null) point.slot1 = Math.round(s.s1 * 10) / 10
    if (s.s2 !== null) point.slot2 = Math.round(s.s2 * 10) / 10
    if (s.s3 !== null) point.slot3 = Math.round(s.s3 * 10) / 10
    return point
  })
}

export function useTemperatures(pollIntervalMs = 5000) {
  const [sensors, setSensors] = useState<SensorReading[]>([])
  const [history, setHistory] = useState<ChartPoint[]>([])
  const [graphRange, setGraphRange] = useState<{ min: number; max: number }>({ min: 0, max: 100 })

  const refresh = useCallback(async () => {
    try {
      const [temps, hist, settings] = await Promise.all([
        backend.getTemperatures(),
        backend.getHistory(360),
        backend.getSettings(),
      ])
      setSensors(temps.sensors)
      setHistory(samplesToChartPoints(hist.samples, hist.rate))

      const minSetting = settings.settings.find((s) => s.key === "graph.min")
      const maxSetting = settings.settings.find((s) => s.key === "graph.max")
      if (minSetting && maxSetting) {
        setGraphRange({
          min: Number(minSetting.value),
          max: Number(maxSetting.value),
        })
      }
    } catch {
      // ignore
    }
  }, [])

  useEffect(() => {
    refresh()
    const timer = setInterval(refresh, pollIntervalMs)
    return () => clearInterval(timer)
  }, [refresh, pollIntervalMs])

  return { sensors, history, graphRange }
}

export function formatDuration(seconds: number): string {
  if (seconds < 60) return `${seconds}s`
  if (seconds < 3600) return `${Math.round(seconds / 60)}m`
  const hours = seconds / 3600
  if (hours < 24) return `${hours.toFixed(1)}h`
  return `${(hours / 24).toFixed(1)}d`
}
