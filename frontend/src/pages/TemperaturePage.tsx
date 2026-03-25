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
import { useTemperatures } from "@/hooks/use-temperatures"

const SLOT_COLORS = ["#ef4444", "#3b82f6", "#22c55e", "#eab308"] as const
const SLOT_NAMES = ["Red", "Blue", "Green", "Yellow"] as const

export default function TemperaturePage() {
  const { sensors, history } = useTemperatures(2000)

  return (
    <div className="flex h-full flex-col gap-4">
      {/* Sensor cards */}
      <div className="grid grid-cols-2 gap-3 sm:grid-cols-4">
        {sensors.map((s) => (
          <div
            key={s.slot}
            className="rounded-lg border-2 bg-card p-3 text-center"
            style={{ borderColor: SLOT_COLORS[s.slot] }}
          >
            <div
              className="text-xs font-medium uppercase tracking-wide"
              style={{ color: SLOT_COLORS[s.slot] }}
            >
              {SLOT_NAMES[s.slot]}
            </div>
            <div className="mt-1 text-2xl font-bold tabular-nums">
              {s.active ? `${s.temperature.toFixed(1)}°` : "--.-"}
            </div>
            {s.address && (
              <div className="mt-1 truncate font-mono text-[10px] text-muted-foreground">
                {s.address}
              </div>
            )}
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
              domain={["auto", "auto"]}
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
            {sensors.map(
              (s) =>
                s.active && (
                  <Line
                    key={s.slot}
                    type="monotone"
                    dataKey={`slot${s.slot}`}
                    name={SLOT_NAMES[s.slot]}
                    stroke={SLOT_COLORS[s.slot]}
                    strokeWidth={2}
                    dot={false}
                    isAnimationActive={false}
                  />
                ),
            )}
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  )
}
