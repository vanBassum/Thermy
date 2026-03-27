import { useEffect, useState, useCallback } from "react"
import { backend, type RawLogEntry } from "@/lib/backend"
import { useConnectionStatus } from "@/hooks/use-connection-status"
import { ScrollTextIcon, RefreshCwIcon, TrashIcon } from "lucide-react"
import { Button } from "@/components/ui/button"
import { LogKey, LogCodeName, int32ToFloat } from "@/lib/log-defs"

const PAGE_SIZE = 50

interface DecodedEntry {
  timestamp: number
  logCode: string
  details: string
}

function decodeEntry(raw: RawLogEntry): DecodedEntry {
  const fields = new Map<number, number>()
  for (const [k, v] of raw) fields.set(k, v)

  const timestamp = fields.get(LogKey.TimeStamp) ?? 0
  const codeNum = fields.get(LogKey.LogCode)
  const logCode = codeNum !== undefined ? (LogCodeName[codeNum] ?? `Code ${codeNum}`) : "--"

  const parts: string[] = []

  for (let i = 0; i < 4; i++) {
    const key = LogKey.Temperature_1 + i
    const raw = fields.get(key)
    if (raw === undefined) continue
    const temp = int32ToFloat(raw)
    if (!isNaN(temp)) parts.push(`T${i + 1}: ${temp.toFixed(1)}\u00B0C`)
  }

  const ip = fields.get(LogKey.IpAddress)
  if (ip !== undefined) {
    parts.push(`${ip & 0xff}.${(ip >> 8) & 0xff}.${(ip >> 16) & 0xff}.${(ip >> 24) & 0xff}`)
  }

  const fw = fields.get(LogKey.FirmwareVersion)
  if (fw !== undefined) {
    parts.push(`v${(fw >> 16) & 0xff}.${(fw >> 8) & 0xff}.${fw & 0xff}`)
  }

  return { timestamp, logCode, details: parts.join("  \u00B7  ") }
}

function formatTimestamp(ts: number): string {
  if (!ts) return "--"
  const d = new Date(ts * 1000)
  return d.toLocaleString(undefined, {
    month: "short",
    day: "numeric",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  })
}

export default function LogPage() {
  const connection = useConnectionStatus()
  const [entries, setEntries] = useState<DecodedEntry[]>([])
  const [totalCount, setTotalCount] = useState(0)
  const [oldestLoaded, setOldestLoaded] = useState<number | null>(null)
  const [loading, setLoading] = useState(false)

  const fetchNewest = useCallback(() => {
    setLoading(true)
    backend
      .getLogEntries(0, 1)
      .then((r) => {
        const total = r.entryCount
        setTotalCount(total)
        const offset = Math.max(0, total - PAGE_SIZE)
        return backend.getLogEntries(offset, PAGE_SIZE).then((r2) => {
          setEntries(r2.entries.map(decodeEntry).reverse())
          setOldestLoaded(offset)
        })
      })
      .catch(() => {})
      .finally(() => setLoading(false))
  }, [])

  const loadOlder = useCallback(() => {
    if (oldestLoaded === null || oldestLoaded <= 0) return
    setLoading(true)
    const offset = Math.max(0, oldestLoaded - PAGE_SIZE)
    const limit = oldestLoaded - offset
    backend
      .getLogEntries(offset, limit)
      .then((r) => {
        setEntries((prev) => [...prev, ...r.entries.map(decodeEntry).reverse()])
        setOldestLoaded(offset)
      })
      .catch(() => {})
      .finally(() => setLoading(false))
  }, [oldestLoaded])

  useEffect(() => {
    if (connection !== "connected") return
    fetchNewest()
  }, [connection, fetchNewest])

  useEffect(() => {
    return backend.subscribe((msg) => {
      if (Array.isArray(msg.logEntry)) {
        const entry = decodeEntry(msg.logEntry as RawLogEntry)
        setEntries((prev) => [entry, ...prev])
        setTotalCount((prev) => prev + 1)
      }
    })
  }, [])

  return (
    <div className="flex h-full flex-col">
      <div className="mb-4 flex items-center justify-between">
        <div className="flex items-center gap-2">
          <ScrollTextIcon className="size-5 text-muted-foreground" />
          <h1 className="text-2xl font-bold">Log</h1>
          <span className="text-sm text-muted-foreground">
            ({totalCount} entries on flash)
          </span>
        </div>
        <div className="flex gap-2">
          <Button variant="outline" size="sm" onClick={fetchNewest} disabled={loading}>
            <RefreshCwIcon className={`mr-1.5 size-3.5 ${loading ? "animate-spin" : ""}`} />
            Refresh
          </Button>
          <Button
            variant="outline"
            size="sm"
            onClick={() => {
              backend.eraseLog().then(() => {
                setEntries([])
                setTotalCount(0)
                setOldestLoaded(null)
              }).catch(() => {})
            }}
          >
            <TrashIcon className="mr-1.5 size-3.5" />
            Erase
          </Button>
        </div>
      </div>

      <div className="min-h-0 flex-1 overflow-auto rounded-xl border">
        <table className="w-full text-sm">
          <thead className="sticky top-0 border-b bg-muted/50 text-left">
            <tr>
              <th className="px-4 py-2 font-medium">Time</th>
              <th className="px-4 py-2 font-medium">Event</th>
              <th className="px-4 py-2 font-medium">Details</th>
            </tr>
          </thead>
          <tbody>
            {entries.length === 0 && (
              <tr>
                <td colSpan={3} className="px-4 py-8 text-center text-muted-foreground">
                  {loading ? "Loading..." : "No log entries"}
                </td>
              </tr>
            )}
            {entries.map((entry, i) => (
              <tr key={i} className="border-b border-border/50 last:border-0 hover:bg-muted/30">
                <td className="px-4 py-1.5 font-mono text-xs text-muted-foreground whitespace-nowrap">
                  {formatTimestamp(entry.timestamp)}
                </td>
                <td className="px-4 py-1.5 font-medium whitespace-nowrap">
                  {entry.logCode}
                </td>
                <td className="px-4 py-1.5 text-muted-foreground">
                  {entry.details}
                </td>
              </tr>
            ))}
          </tbody>
        </table>

        {oldestLoaded !== null && oldestLoaded > 0 && (
          <div className="border-t p-3 text-center">
            <Button variant="ghost" size="sm" onClick={loadOlder} disabled={loading}>
              Load older entries
            </Button>
          </div>
        )}
      </div>
    </div>
  )
}
