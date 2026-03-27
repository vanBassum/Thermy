import { useEffect, useState, useCallback } from "react"
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  Legend,
} from "recharts"
import { backend, type RawLogEntry } from "@/lib/backend"
import { useConnectionStatus } from "@/hooks/use-connection-status"
import { LogKey, LogCodeValue, int32ToFloat } from "@/lib/log-defs"

const SLOT_COLORS = ["#ef4444", "#3b82f6", "#22c55e", "#eab308"] as const
const SLOT_NAMES = ["Red", "Blue", "Green", "Yellow"] as const
const CHART_ENTRIES = 360

interface ChartPoint {
  time: string
  t1?: number
  t2?: number
  t3?: number
  t4?: number
}

function decodeTemps(raw: RawLogEntry): (number | null)[] {
  const fields = new Map<number, number>()
  for (const [k, v] of raw) fields.set(k, v)
  return [
    LogKey.Temperature_1,
    LogKey.Temperature_2,
    LogKey.Temperature_3,
    LogKey.Temperature_4,
  ].map((key) => {
    const bits = fields.get(key)
    if (bits === undefined) return null
    const v = int32ToFloat(bits)
    return isNaN(v) ? null : Math.round(v * 10) / 10
  })
}

function rawToChartPoint(raw: RawLogEntry): ChartPoint | null {
  const fields = new Map<number, number>()
  for (const [k, v] of raw) fields.set(k, v)
  if (fields.get(LogKey.LogCode) !== LogCodeValue.TemperatureReading) return null

  const ts = fields.get(LogKey.TimeStamp) ?? 0
  const temps = decodeTemps(raw)
  const point: ChartPoint = {
    time: ts ? new Date(ts * 1000).toLocaleTimeString() : "",
  }
  if (temps[0] !== null) point.t1 = temps[0]
  if (temps[1] !== null) point.t2 = temps[1]
  if (temps[2] !== null) point.t3 = temps[2]
  if (temps[3] !== null) point.t4 = temps[3]
  return point
}

export default function TemperaturePage() {
  const connection = useConnectionStatus()
  const [history, setHistory] = useState<ChartPoint[]>([])
  const [current, setCurrent] = useState<(number | null)[]>([null, null, null, null])
  const [graphRange, setGraphRange] = useState<{ min: number; max: number }>({ min: 0, max: 100 })

  const fetchHistory = useCallback(() => {
    backend
      .getLogEntries(0, 1)
      .then((r) => {
        const total = r.entryCount
        const offset = Math.max(0, total - CHART_ENTRIES)
        return backend.getLogEntries(offset, CHART_ENTRIES)
      })
      .then((r) => {
        const points: ChartPoint[] = []
        for (const raw of r.entries) {
          const p = rawToChartPoint(raw)
          if (p) points.push(p)
        }
        setHistory(points)
        // Set current from the last temperature entry
        for (let i = r.entries.length - 1; i >= 0; i--) {
          const fields = new Map<number, number>()
          for (const [k, v] of r.entries[i]) fields.set(k, v)
          if (fields.get(LogKey.LogCode) === LogCodeValue.TemperatureReading) {
            setCurrent(decodeTemps(r.entries[i]))
            break
          }
        }
      })
      .catch(() => {})

    backend.getSettings().then((s) => {
      const min = s.settings.find((x) => x.key === "graph.min")
      const max = s.settings.find((x) => x.key === "graph.max")
      if (min && max) setGraphRange({ min: Number(min.value), max: Number(max.value) })
    }).catch(() => {})
  }, [])

  useEffect(() => {
    if (connection !== "connected") return
    fetchHistory()
  }, [connection, fetchHistory])

  // Live updates from broadcast
  useEffect(() => {
    return backend.subscribe((msg) => {
      if (!Array.isArray(msg.logEntry)) return
      const raw = msg.logEntry as RawLogEntry
      const point = rawToChartPoint(raw)
      if (!point) return

      setCurrent(decodeTemps(raw))
      setHistory((prev) => {
        const next = [...prev, point]
        return next.length > CHART_ENTRIES ? next.slice(-CHART_ENTRIES) : next
      })
    })
  }, [])

  return (
    <div className="flex h-full flex-col gap-4">
      {/* Sensor cards */}
      <div className="grid grid-cols-2 gap-3 sm:grid-cols-4">
        {current.map((temp, i) => (
          <div
            key={i}
            className="rounded-lg border-2 bg-card p-3 text-center"
            style={{ borderColor: SLOT_COLORS[i] }}
          >
            <div
              className="text-xs font-medium uppercase tracking-wide"
              style={{ color: SLOT_COLORS[i] }}
            >
              {SLOT_NAMES[i]}
            </div>
            <div className="mt-1 text-2xl font-bold tabular-nums">
              {temp !== null ? `${temp.toFixed(1)}°` : "--.-"}
            </div>
          </div>
        ))}
      </div>

      {/* Chart */}
      <div className="min-h-0 flex-1 rounded-lg border bg-card p-4">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={history}>
            <CartesianGrid strokeDasharray="3 3" stroke="#333" />
            <XAxis
              dataKey="time"
              tick={{ fill: "#888", fontSize: 11 }}
              interval="preserveStartEnd"
              minTickGap={60}
            />
            <YAxis
              tick={{ fill: "#888", fontSize: 11 }}
              domain={[graphRange.min, graphRange.max]}
              width={40}
            />
            <Tooltip
              contentStyle={{
                backgroundColor: "#1a1a1a",
                border: "1px solid #333",
                borderRadius: 6,
              }}
              labelStyle={{ color: "#aaa" }}
            />
            <Legend />
            {SLOT_NAMES.map((name, i) => (
              <Line
                key={i}
                type="monotone"
                dataKey={`t${i + 1}`}
                name={name}
                stroke={SLOT_COLORS[i]}
                strokeWidth={2}
                dot={false}
                isAnimationActive={false}
                hide={current[i] === null}
              />
            ))}
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  )
}
