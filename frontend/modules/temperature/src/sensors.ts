// The module's own view of its own commands. These types used to live in the shell's
// `backend.ts`, which was the wrong home for them: the shape of `sensor list`'s reply
// belongs to whoever owns the `sensor` commands, and that is SensorManager on the
// device and this module in the browser. The shell now knows nothing about probes.

import { useCallback, useEffect, useRef, useState } from "react"
import type { DeviceTransport } from "@shell/contract"

/// Four slots, in a fixed order, because a 1-Wire address is not a position: the
/// probe in the tank and the probe on the flow pipe are interchangeable to the bus
/// and very much not to the user. The names and colours match the device's own
/// screen, so "the red one" means the same thing in both places.
export const SLOT_COLORS = ["#ef4444", "#3b82f6", "#22c55e", "#eab308"] as const
export const SLOT_NAMES = ["Red", "Blue", "Green", "Yellow"] as const
export const SLOT_COUNT = 4

export interface SensorSlot {
  slot: number
  /// 16-hex-digit 1-Wire ROM address, or "" when the slot is unassigned.
  address: string
  /// True when the assigned probe was found on the bus in the last scan.
  active: boolean
  /// Present only while active.
  celsius?: number
}

export interface SensorsReply {
  slots: SensorSlot[]
  /// ROM address of a probe found on the bus that no slot claims, or "".
  pending: string
}

/// One polled sample: a timestamp and whatever slots were reading at the time.
export interface Sample {
  at: number
  celsius: (number | null)[]
}

// Slow enough to be free on the LAN, and slow because through the relay every poll
// takes that device's request gate — see the contract. The device reads its probes on
// `sensor.read` (1 s by default) regardless, so this is only how often we look.
const POLL_MS = 2000

/// How many polled samples the in-page chart keeps. At POLL_MS that is ~10 minutes.
///
/// This is a live view assembled in the BROWSER and not device history: the on-flash
/// log is gone, and anything long-term lives wherever TelemetryManager's points are
/// being shipped. Reloading the page starts it over, which is the honest behaviour
/// for a window onto a bus.
const CHART_POINTS = 300

export function errorMessage(e: unknown): string {
  return e instanceof Error ? e.message : String(e)
}

/// Poll `sensor list`, keep a rolling window of samples, and offer the two writes.
export function useSensors(transport: DeviceTransport) {
  const [slots, setSlots] = useState<SensorSlot[]>([])
  const [pending, setPending] = useState("")
  const [history, setHistory] = useState<Sample[]>([])
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState<string | null>(null)

  // A reply can land after unmount — the contract has no cancellation — so every
  // setState is guarded rather than assumed safe.
  const alive = useRef(true)
  useEffect(() => {
    alive.current = true
    return () => {
      alive.current = false
    }
  }, [])

  const refresh = useCallback(async () => {
    try {
      const reply = await transport.request<SensorsReply>("sensor list")
      if (!alive.current) return
      setSlots(reply.slots ?? [])
      setPending(reply.pending ?? "")
      setHistory((prev) => {
        const celsius = Array.from({ length: SLOT_COUNT }, (_, i) => {
          const slot = reply.slots?.find((s) => s.slot === i)
          return slot?.active && typeof slot.celsius === "number" ? slot.celsius : null
        })
        const next = [...prev, { at: Date.now(), celsius }]
        return next.length > CHART_POINTS ? next.slice(-CHART_POINTS) : next
      })
      setError(null)
    } catch (e) {
      // Keep the last reading on the screen: a dropped poll is not news, and blanking
      // the grid on every hiccup would be worse than being two seconds stale.
      if (alive.current) setError(errorMessage(e))
    }
  }, [transport])

  useEffect(() => {
    void refresh()
    const id = setInterval(() => void refresh(), POLL_MS)
    return () => clearInterval(id)
  }, [refresh])

  /// Bind the currently-pending probe to a slot — the same thing as tapping a colour
  /// on the device's own screen.
  const assign = useCallback(
    async (slot: number) => {
      setBusy(true)
      try {
        const reply = await transport.request<{ ok: boolean; error?: string }>(
          "sensor assign",
          { slot },
        )
        if (alive.current && reply && reply.ok === false)
          setError(reply.error ?? "assign failed")
        await refresh()
      } catch (e) {
        if (alive.current) setError(errorMessage(e))
      } finally {
        if (alive.current) setBusy(false)
      }
    },
    [transport, refresh],
  )

  const clearAll = useCallback(async () => {
    setBusy(true)
    try {
      await transport.request("sensor clear")
      if (alive.current) setHistory([])
      await refresh()
    } catch (e) {
      if (alive.current) setError(errorMessage(e))
    } finally {
      if (alive.current) setBusy(false)
    }
  }, [transport, refresh])

  /// Slots the device has not reported yet read as null, so the grid can render four
  /// placeholders instead of popping in as the first poll lands.
  const grid: (SensorSlot | null)[] = Array.from(
    { length: SLOT_COUNT },
    (_, i) => slots.find((s) => s.slot === i) ?? null,
  )

  return { grid, pending, history, busy, error, assign, clearAll, refresh }
}
