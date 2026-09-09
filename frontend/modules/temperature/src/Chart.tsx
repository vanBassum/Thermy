// The live chart, drawn as plain SVG.
//
// ── Why not recharts, which the shell page used ───────────────────────────────
// This page was a shell page and imported recharts, which was free there: one copy in
// one bundle. It is not free in a module. A module externalises `react` and
// `react/jsx-runtime` and NOTHING else (see the import map in the shell's index.html),
// so recharts would be bundled in — and recharts 3 brings @reduxjs/toolkit,
// react-redux, immer and victory-vendor's d3 with it, plus a peer dependency on
// react-dom that would become a SECOND react-dom instance beside the shell's. Half a
// megabyte in the FAT partition and a duplicated renderer, for one line chart, on a
// page that is also the device's landing page and so the first thing every load fetches.
//
// The precedent is Strux's settings module dropping prismjs on its way into a bundle:
// 30 KB of syntax highlighting is 30 KB of flash. This is the same trade at fifteen
// times the size, so it goes the same way. What is drawn is what recharts drew — grid,
// both axes, one line per active slot, a legend, and a tooltip on hover — in about a
// hundred lines and no dependencies.
//
// ── Why pixels and a ResizeObserver rather than a scaling viewBox ─────────────
// `viewBox` with `preserveAspectRatio="none"` is the cheap way to make an SVG
// responsive and it stretches the strokes and the text with it, which looks wrong at
// this aspect ratio. Measuring instead keeps 1 unit = 1 px, so a 2 px line is 2 px at
// every width.

import { useEffect, useLayoutEffect, useRef, useState, type CSSProperties } from "react"
import { SLOT_COLORS, SLOT_COUNT, SLOT_NAMES, type Sample } from "./sensors"

const PAD = { top: 12, right: 12, bottom: 24, left: 40 }
/// Y ticks, and the number of horizontal grid lines with them.
const Y_TICKS = 5

interface Box {
  width: number
  height: number
}

/// Nice round step for a range, so the axis reads 20 / 22.5 / 25 rather than
/// 21.37 / 23.94.
function niceStep(span: number, ticks: number): number {
  if (span <= 0) return 1
  const raw = span / ticks
  const magnitude = Math.pow(10, Math.floor(Math.log10(raw)))
  for (const factor of [1, 2, 2.5, 5, 10]) {
    if (magnitude * factor >= raw) return magnitude * factor
  }
  return magnitude * 10
}

function formatClock(at: number): string {
  return new Date(at).toLocaleTimeString()
}

export function Chart({
  history,
  visible,
}: {
  history: Sample[]
  /// Which slots to draw. A slot that has never been active would otherwise
  /// contribute an empty line and a legend entry for nothing.
  visible: boolean[]
}) {
  const host = useRef<HTMLDivElement | null>(null)
  const [box, setBox] = useState<Box>({ width: 0, height: 0 })
  const [hover, setHover] = useState<number | null>(null)

  useLayoutEffect(() => {
    const element = host.current
    if (!element) return
    const observer = new ResizeObserver(([entry]) => {
      const { width, height } = entry.contentRect
      setBox({ width, height })
    })
    observer.observe(element)
    return () => observer.disconnect()
  }, [])

  // A window that shrinks (Clear resets the history) must not keep pointing at a
  // sample that is gone.
  useEffect(() => {
    setHover((h) => (h !== null && h >= history.length ? null : h))
  }, [history.length])

  const series = Array.from({ length: SLOT_COUNT }, (_, i) => i).filter((i) => visible[i])

  const values = history.flatMap((s) =>
    series.map((i) => s.celsius[i]).filter((v): v is number => v !== null),
  )

  const plot = {
    width: Math.max(0, box.width - PAD.left - PAD.right),
    height: Math.max(0, box.height - PAD.top - PAD.bottom),
  }

  // An empty or one-sample window has no range to scale to, and dividing by it would
  // put every point on one pixel row. A fixed ±1 °C band around the value keeps the
  // first sample visible instead of hiding it on an axis edge.
  const rawMin = values.length ? Math.min(...values) : 0
  const rawMax = values.length ? Math.max(...values) : 1
  const step = niceStep(Math.max(rawMax - rawMin, 2), Y_TICKS)
  const min = Math.floor(rawMin / step) * step - (values.length ? 0 : step)
  const max = Math.ceil(rawMax / step) * step + (rawMax === rawMin ? step : 0)

  const xOf = (index: number) =>
    PAD.left + (history.length < 2 ? plot.width / 2 : (index / (history.length - 1)) * plot.width)
  const yOf = (celsius: number) =>
    PAD.top + plot.height - ((celsius - min) / (max - min || 1)) * plot.height

  const yTicks: number[] = []
  for (let t = min; t <= max + 1e-9; t += step) yTicks.push(Number(t.toFixed(4)))

  const ready = plot.width > 0 && plot.height > 0

  return (
    <div className="flex h-full min-h-0 flex-col gap-2">
      <div ref={host} className="relative min-h-0 flex-1">
        {ready && (
          <svg
            width={box.width}
            height={box.height}
            className="absolute inset-0 overflow-visible"
            onPointerMove={(event) => {
              if (history.length === 0) return
              const rect = event.currentTarget.getBoundingClientRect()
              const fraction = (event.clientX - rect.left - PAD.left) / (plot.width || 1)
              const index = Math.round(fraction * (history.length - 1))
              setHover(Math.min(history.length - 1, Math.max(0, index)))
            }}
            onPointerLeave={() => setHover(null)}
          >
            {/* Grid and the Y axis, which share their ticks. */}
            {yTicks.map((tick) => (
              <g key={tick}>
                <line
                  x1={PAD.left}
                  x2={PAD.left + plot.width}
                  y1={yOf(tick)}
                  y2={yOf(tick)}
                  className="stroke-border"
                  strokeDasharray="3 3"
                />
                <text
                  x={PAD.left - 6}
                  y={yOf(tick)}
                  textAnchor="end"
                  dominantBaseline="middle"
                  className="fill-muted-foreground text-[11px]"
                >
                  {tick}
                </text>
              </g>
            ))}

            {/* The time axis labels only the ends: a 300-sample window has no room
                for more, and what a reader wants from it is "how far back does this
                go". */}
            {history.length > 0 && (
              <>
                <text
                  x={PAD.left}
                  y={box.height - 8}
                  textAnchor="start"
                  className="fill-muted-foreground text-[11px]"
                >
                  {formatClock(history[0].at)}
                </text>
                <text
                  x={PAD.left + plot.width}
                  y={box.height - 8}
                  textAnchor="end"
                  className="fill-muted-foreground text-[11px]"
                >
                  {formatClock(history[history.length - 1].at)}
                </text>
              </>
            )}

            {hover !== null && (
              <line
                x1={xOf(hover)}
                x2={xOf(hover)}
                y1={PAD.top}
                y2={PAD.top + plot.height}
                className="stroke-muted-foreground"
                strokeDasharray="2 2"
              />
            )}

            {series.map((slot) => (
              <Series
                key={slot}
                color={SLOT_COLORS[slot]}
                points={history.map((sample, index) => ({
                  x: xOf(index),
                  y: sample.celsius[slot] === null ? null : yOf(sample.celsius[slot]!),
                }))}
                marker={hover === null ? null : history[hover]?.celsius[slot] ?? null}
                markerAt={hover === null ? null : { x: xOf(hover) }}
                yOf={yOf}
              />
            ))}
          </svg>
        )}

        {hover !== null && history[hover] && (
          <Tooltip
            sample={history[hover]}
            series={series}
            // Flip to the left of the cursor past the midpoint so the box never
            // hangs off the panel.
            style={{
              left: xOf(hover) < box.width / 2 ? xOf(hover) + 12 : undefined,
              right: xOf(hover) < box.width / 2 ? undefined : box.width - xOf(hover) + 12,
              top: PAD.top,
            }}
          />
        )}
      </div>

      <Legend series={series} />
    </div>
  )
}

/// One slot's line. Nulls BREAK the path rather than being interpolated over: a probe
/// that dropped off the bus has no reading, and drawing straight through the gap would
/// invent one.
function Series({
  color,
  points,
  marker,
  markerAt,
  yOf,
}: {
  color: string
  points: { x: number; y: number | null }[]
  marker: number | null
  markerAt: { x: number } | null
  yOf: (celsius: number) => number
}) {
  let path = ""
  let open = false
  for (const point of points) {
    if (point.y === null) {
      open = false
      continue
    }
    path += `${open ? "L" : "M"}${point.x.toFixed(1)} ${point.y.toFixed(1)} `
    open = true
  }

  return (
    <>
      <path d={path.trim()} fill="none" stroke={color} strokeWidth={2} strokeLinejoin="round" />
      {marker !== null && markerAt && (
        <circle cx={markerAt.x} cy={yOf(marker)} r={3.5} fill={color} />
      )}
    </>
  )
}

function Tooltip({
  sample,
  series,
  style,
}: {
  sample: Sample
  series: number[]
  style: CSSProperties
}) {
  return (
    <div
      className="bg-card border-border pointer-events-none absolute rounded-lg border px-2.5 py-1.5 text-xs shadow-lg"
      style={style}
    >
      <div className="text-muted-foreground mb-1 font-mono">{formatClock(sample.at)}</div>
      {series.map((slot) => (
        <div key={slot} className="flex items-center gap-2">
          <span
            className="size-2 shrink-0 rounded-full"
            style={{ backgroundColor: SLOT_COLORS[slot] }}
          />
          <span className="text-muted-foreground">{SLOT_NAMES[slot]}</span>
          <span className="ml-auto font-mono tabular-nums">
            {sample.celsius[slot] === null ? "--.-" : `${sample.celsius[slot]!.toFixed(1)}°`}
          </span>
        </div>
      ))}
    </div>
  )
}

function Legend({ series }: { series: number[] }) {
  if (series.length === 0) return null
  return (
    <div className="flex shrink-0 flex-wrap items-center justify-center gap-x-4 gap-y-1 text-xs">
      {series.map((slot) => (
        <span key={slot} className="flex items-center gap-1.5">
          <span
            className="h-0.5 w-4 shrink-0 rounded-full"
            style={{ backgroundColor: SLOT_COLORS[slot] }}
          />
          {SLOT_NAMES[slot]}
        </span>
      ))}
    </div>
  )
}
