import { useEffect, useState, useCallback } from "react"
import { backend, type SensorReading, type HistorySample } from "@/lib/backend"

export const MAX_SAMPLES = 2048

export interface ChartPoint {
  time: string
  slot0?: number
  slot1?: number
  slot2?: number
  slot3?: number
}

function sampleToChartPoint(s: HistorySample): ChartPoint {
  const d = new Date(s.t * 1000)
  const point: ChartPoint = { time: d.toLocaleTimeString() }
  if (s.s0 !== null) point.slot0 = Math.round(s.s0 * 10) / 10
  if (s.s1 !== null) point.slot1 = Math.round(s.s1 * 10) / 10
  if (s.s2 !== null) point.slot2 = Math.round(s.s2 * 10) / 10
  if (s.s3 !== null) point.slot3 = Math.round(s.s3 * 10) / 10
  return point
}

export function useTemperatures(pollIntervalMs = 5000) {
  const [sensors, setSensors] = useState<SensorReading[]>([])
  const [history, setHistory] = useState<ChartPoint[]>([])

  const refresh = useCallback(async () => {
    try {
      const [temps, hist] = await Promise.all([
        backend.getTemperatures(),
        backend.getHistory(360),
      ])
      setSensors(temps.sensors)
      setHistory(hist.samples.map(sampleToChartPoint))
    } catch {
      // ignore
    }
  }, [])

  useEffect(() => {
    refresh()
    const timer = setInterval(refresh, pollIntervalMs)
    return () => clearInterval(timer)
  }, [refresh, pollIntervalMs])

  return { sensors, history }
}

export function formatDuration(seconds: number): string {
  if (seconds < 60) return `${seconds}s`
  if (seconds < 3600) return `${Math.round(seconds / 60)}m`
  const hours = seconds / 3600
  if (hours < 24) return `${hours.toFixed(1)}h`
  return `${(hours / 24).toFixed(1)}d`
}
