import { useEffect, useState, useRef } from "react"
import { backend, type SensorReading } from "@/lib/backend"

export interface TemperatureHistory {
  time: string
  slot0?: number
  slot1?: number
  slot2?: number
  slot3?: number
}

const MAX_HISTORY = 120

export function useTemperatures(pollIntervalMs = 2000) {
  const [sensors, setSensors] = useState<SensorReading[]>([])
  const [history, setHistory] = useState<TemperatureHistory[]>([])
  const historyRef = useRef<TemperatureHistory[]>([])

  useEffect(() => {
    let mounted = true

    const poll = async () => {
      try {
        const data = await backend.getTemperatures()
        if (!mounted) return

        setSensors(data.sensors)

        const now = new Date().toLocaleTimeString()
        const point: TemperatureHistory = { time: now }

        for (const s of data.sensors) {
          if (s.active) {
            const key = `slot${s.slot}` as keyof TemperatureHistory
            ;(point as Record<string, unknown>)[key] = Math.round(s.temperature * 10) / 10
          }
        }

        const updated = [...historyRef.current, point].slice(-MAX_HISTORY)
        historyRef.current = updated
        setHistory(updated)
      } catch {
        // ignore poll failures
      }
    }

    poll()
    const timer = setInterval(poll, pollIntervalMs)
    return () => {
      mounted = false
      clearInterval(timer)
    }
  }, [pollIntervalMs])

  return { sensors, history }
}
