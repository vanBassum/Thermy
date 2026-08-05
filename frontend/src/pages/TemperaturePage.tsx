import { useCallback, useEffect, useRef, useState } from "react"
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
import { ThermometerIcon } from "lucide-react"
import { backend, type SensorSlot } from "@/lib/backend"
import { useConnectionStatus } from "@/hooks/use-connection-status"
import { Button } from "@/components/ui/button"

const SLOT_COLORS = ["#ef4444", "#3b82f6", "#22c55e", "#eab308"] as const
const SLOT_NAMES = ["Red", "Blue", "Green", "Yellow"] as const

const POLL_MS = 2000
// How many polled samples the in-page chart keeps. At POLL_MS that is ~10 min.
const CHART_POINTS = 300

interface ChartPoint {
  time: string
  t0?: number
  t1?: number
  t2?: number
  t3?: number
}

export default function TemperaturePage() {
  const connection = useConnectionStatus()
  const [slots, setSlots] = useState<SensorSlot[]>([])
  const [pending, setPending] = useState("")
  const [history, setHistory] = useState<ChartPoint[]>([])
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState<string | null>(null)

  // Kept in a ref so the poll effect doesn't restart on every sample.
  const appendPoint = useRef<(s: SensorSlot[]) => void>(() => {})
  appendPoint.current = (fresh: SensorSlot[]) => {
    const point: ChartPoint = { time: new Date().toLocaleTimeString() }
    for (const s of fresh) {
      if (s.active && typeof s.celsius === "number") {
        point[`t${s.slot}` as "t0" | "t1" | "t2" | "t3"] = Math.round(s.celsius * 10) / 10
      }
    }
    setHistory((prev) => {
      const next = [...prev, point]
      return next.length > CHART_POINTS ? next.slice(-CHART_POINTS) : next
    })
  }

  const poll = useCallback(async () => {
    try {
      const res = await backend.getSensors()
      setSlots(res.slots)
      setPending(res.pending)
      appendPoint.current(res.slots)
      setError(null)
    } catch (e) {
      setError(e instanceof Error ? e.message : "read failed")
    }
  }, [])

  useEffect(() => {
    if (connection !== "connected") return
    void poll()
    const id = setInterval(() => void poll(), POLL_MS)
    return () => clearInterval(id)
  }, [connection, poll])

  const assign = async (slot: number) => {
    setBusy(true)
    try {
      const res = await backend.assignSensor(slot)
      if (!res.ok) setError(res.error ?? "assign failed")
      await poll()
    } catch (e) {
      setError(e instanceof Error ? e.message : "assign failed")
    } finally {
      setBusy(false)
    }
  }

  const clearAll = async () => {
    setBusy(true)
    try {
      await backend.clearSensors()
      setHistory([])
      await poll()
    } catch (e) {
      setError(e instanceof Error ? e.message : "clear failed")
    } finally {
      setBusy(false)
    }
  }

  // Slots the device did not report yet — render placeholders so the grid
  // doesn't pop in as the first poll lands.
  const cards: (SensorSlot | null)[] = [0, 1, 2, 3].map(
    (i) => slots.find((s) => s.slot === i) ?? null,
  )

  return (
    <div className="flex h-full flex-col gap-4">
      <div className="flex items-center gap-2">
        <ThermometerIcon className="size-5 text-muted-foreground" />
        <h1 className="text-2xl font-bold">Temperature</h1>
      </div>

      {error && (
        <div className="rounded-lg border border-destructive/50 bg-destructive/10 px-3 py-2 text-sm">
          {error}
        </div>
      )}

      {/* A probe on the bus that no slot claims. Assigning it here does the same
          thing as tapping a colour on the device's own screen. */}
      {pending && (
        <div className="rounded-lg border border-amber-500/50 bg-amber-500/10 p-3">
          <div className="mb-2 text-sm">
            New probe detected: <span className="font-mono">{pending}</span> — assign it to a slot:
          </div>
          <div className="flex flex-wrap gap-2">
            {SLOT_NAMES.map((name, i) => (
              <Button
                key={i}
                size="sm"
                variant="outline"
                disabled={busy}
                onClick={() => void assign(i)}
              >
                {name}
              </Button>
            ))}
          </div>
        </div>
      )}

      {/* Slot cards */}
      <div className="grid grid-cols-2 gap-3 sm:grid-cols-4">
        {cards.map((slot, i) => (
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
              {slot?.active && typeof slot.celsius === "number"
                ? `${slot.celsius.toFixed(1)}°`
                : "--.-"}
            </div>
            <div className="mt-1 truncate font-mono text-[10px] text-muted-foreground">
              {!slot || !slot.address ? "unassigned" : slot.active ? slot.address : "offline"}
            </div>
          </div>
        ))}
      </div>

      {/* Chart. This is a live view assembled in the browser, not device history:
          the on-flash log is gone, and long-term series live wherever telemetry
          is being shipped to. Reloading the page starts it over. */}
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
            <YAxis tick={{ fill: "#888", fontSize: 11 }} width={40} domain={["auto", "auto"]} />
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
                dataKey={`t${i}`}
                name={name}
                stroke={SLOT_COLORS[i]}
                strokeWidth={2}
                dot={false}
                isAnimationActive={false}
                connectNulls
                hide={!cards[i]?.active}
              />
            ))}
          </LineChart>
        </ResponsiveContainer>
      </div>

      <div className="flex justify-end">
        <Button variant="outline" size="sm" disabled={busy} onClick={() => void clearAll()}>
          Clear all assignments
        </Button>
      </div>
    </div>
  )
}
