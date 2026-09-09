import type { ShellProvider } from "@shell/contract"
import { Button, Panel } from "../../_ui"
import { Chart } from "./Chart"
import { SLOT_COLORS, SLOT_COUNT, SLOT_NAMES, useSensors, type SensorSlot } from "./sensors"

// The four probes, live — and the screen this device opens on.
//
// Nothing on this page says it is the landing page. The firmware declares it first
// (SensorManager registers its module after the framework's, and UiManager
// head-inserts) and the shell lands on whatever the manifest declares first. There is
// no separate home page any more: a device with one feature has nothing to put on one,
// and a chip name and a heap figure — which is what the old home page showed — are
// reference material that now lives behind the sidebar footer.
//
// The shell rendering this does not know what a temperature probe is.
export function TemperaturePage({ shell }: { shell: ShellProvider }) {
  const { grid, pending, history, busy, error, assign, clearAll } = useSensors(shell.transport)

  // A slot is worth a line once it has been seen active. Keyed on the history rather
  // than on the current poll so a probe that drops off the bus keeps its line and its
  // legend entry instead of vanishing from the chart it is already drawn on.
  const charted = Array.from(
    { length: SLOT_COUNT },
    (_, i) => grid[i]?.active === true || history.some((s) => s.celsius[i] !== null),
  )

  return (
    <div className="flex h-full flex-col gap-4">
      <div className="flex items-center gap-3">
        <Thermometer />
        <h1 className="text-2xl font-bold">Temperature</h1>
      </div>

      {error && (
        <div className="border-destructive/50 bg-destructive/10 rounded-lg border px-3 py-2 text-sm">
          Last poll failed: {error}. Showing the last known reading.
        </div>
      )}

      {/* A probe on the bus that no slot claims. Assigning it here does the same thing
          as tapping a colour on the device's own screen. */}
      {pending && (
        <Panel className="border-amber-500/50 bg-amber-500/10">
          <div className="mb-2 text-sm">
            New probe detected: <span className="font-mono">{pending}</span> — assign it to
            a slot:
          </div>
          <div className="flex flex-wrap gap-2">
            {SLOT_NAMES.map((name, i) => (
              <Button key={i} size="sm" variant="outline" disabled={busy} onClick={() => void assign(i)}>
                {name}
              </Button>
            ))}
          </div>
        </Panel>
      )}

      <div className="grid shrink-0 grid-cols-2 gap-3 sm:grid-cols-4">
        {grid.map((slot, i) => (
          <SlotCard key={i} index={i} slot={slot} />
        ))}
      </div>

      <Panel className="min-h-0 flex-1" flush>
        <div className="h-full min-h-0 p-4">
          <Chart history={history} visible={charted} />
        </div>
      </Panel>

      <div className="flex shrink-0 items-center justify-between gap-4">
        <p className="text-muted-foreground text-xs">
          The chart is a live view assembled in this browser, not device history —
          reloading starts it over. Long-term series live wherever telemetry is shipped.
        </p>
        <Button variant="outline" size="sm" disabled={busy} onClick={() => void clearAll()}>
          Clear all assignments
        </Button>
      </div>
    </div>
  )
}

function SlotCard({ index, slot }: { index: number; slot: SensorSlot | null }) {
  const reading =
    slot?.active && typeof slot.celsius === "number" ? `${slot.celsius.toFixed(1)}°` : "--.-"

  return (
    <div
      className="bg-card rounded-lg border-2 p-3 text-center"
      style={{ borderColor: SLOT_COLORS[index] }}
    >
      <div
        className="text-xs font-medium tracking-wide uppercase"
        style={{ color: SLOT_COLORS[index] }}
      >
        {SLOT_NAMES[index]}
      </div>
      <div className="mt-1 text-2xl font-bold tabular-nums">{reading}</div>
      <div className="text-muted-foreground mt-1 truncate font-mono text-[10px]">
        {!slot || !slot.address ? "unassigned" : slot.active ? slot.address : "offline"}
      </div>
    </div>
  )
}

// Drawn here rather than imported: pulling lucide into a module bundle would ship a
// second copy of it beside the shell's, and the manifest promises one self-contained
// file. This is lucide's own `thermometer`, which is also the icon name the firmware
// declares for the sidebar entry.
function Thermometer() {
  return (
    <svg
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
      className="text-muted-foreground size-6"
      aria-hidden="true"
    >
      <path d="M14 4v10.54a4 4 0 1 1-4 0V4a2 2 0 0 1 4 0Z" />
    </svg>
  )
}
