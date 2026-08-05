# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Thermy is a DS18B20 temperature monitor for the WT32-SC01 (ESP32-WROVER-B with a 3.5"
480x320 touchscreen): four probes in named slots, a local LVGL UI, a React web dashboard,
and telemetry to InfluxDB.

It is a **fork of [Strux](https://github.com/vanBassum/Stux)** (`C:\Workspace\Strux`), the
generic ESP32 firmware template. Strux is a **separate repository with no shared history**,
so there is no merge path — every sync is a hand port. Read Strux's own `CLAUDE.md` for the
framework's architecture; it is the authority, and this file only records what Thermy adds
or changes.

**Keep the framework diffable against Strux.** Anything under `main/Application/` except
`SensorManager/` and `DisplayManager/`, plus all of `main/lib/`, should stay byte-identical
to upstream where possible, so the next sync is a diff and not an archaeology exercise. If
a framework file must change for Thermy, say so in a comment at the change.

### What is Thermy's, not Strux's

- `main/Application/SensorManager/` — DS18B20 probes in four slots
- `main/Application/DisplayManager/` — LVGL UI and its six pages
- `main/hardware/boards/wt32_sc01/` — the only board, including its panel driver
- `frontend/src/pages/TemperaturePage.tsx` and the sensor API in `frontend/src/lib/backend.ts`
- `main/lib/common/core_utils.h` — two tick-arithmetic helpers Strux does not have

### Deliberately gone

MQTT, Home Assistant, `LogManager` and the `flash_log` component were **removed**, not
ported (2026-08-05). Devices that exist to live in Home Assistant are better served by
ESPHome, and time-series history is `TelemetryManager`'s job now — a manager records a
point, the relay ships it to InfluxDB. `DeviceManager` is gone too: the per-board `Board`
class replaced its role. Do not reintroduce any of them; the last versions are in git
history.

## Build commands

Requires ESP-IDF v6.0 and Node 22+ with pnpm.

```bash
idf.py set-target esp32
idf.py build                 # also builds the frontend if pnpm is installed
idf.py -p COM3 flash monitor
```

`BOARD` defaults to `wt32_sc01`, which is currently the only board — `-DBOARD=<name>`
selects another if one is ever added. The board folder carries its own
`sdkconfig.defaults` (16 MB flash, PSRAM, LVGL fonts and colour order) and its own
`partitions.csv` (two 3 MB app slots, the rest FAT for the frontend), both of which
override the 4 MB-sized common defaults inherited from Strux.

Frontend, from `frontend/`: `pnpm dev` (proxies its WebSocket to a live device),
`pnpm build`, `pnpm typecheck`.

### This machine's ESP-IDF is installed unusually

`export.ps1` does not work here. It looks for its venv at
`$IDF_TOOLS_PATH\python_env\idf6.0_py3.13_env`, but this install (VS Code extension / eim)
puts venvs at `C:\Espressif\tools\python\v6.0\venv`, and `C:\Espressif\espidf.constraints.v6.0.txt`
was never written. To build, set the environment by hand and call `idf.py` directly:

```powershell
$env:IDF_PATH = "C:\esp\v6.0\esp-idf"
$env:IDF_TOOLS_PATH = "C:\Espressif"
$env:IDF_PYTHON_ENV_PATH = "C:\Espressif\tools\python\v6.0\venv"
$env:ESP_ROM_ELF_DIR = "C:\Espressif\tools\esp-rom-elfs\20241011\"
$env:IDF_PYTHON_CHECK_CONSTRAINTS = "0"   # the constraints file is missing
$env:ESP_IDF_VERSION = "6.0"              # component manager crashes on None
# then prepend to PATH: the venv's Scripts, xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/bin,
# cmake/4.0.3/bin, ninja/1.12.1, idf-exe/1.0.3 — all under C:\Espressif\tools\
```

There are no automated tests. Verification is building, flashing, and driving the device
over its own WebSocket — see Strux's `CLAUDE.md` for the wire protocol and the `help list`
command that enumerates the device's whole RPC surface.

## Thermy-specific architecture notes

### The board owns the hardware, including LVGL's display

`Board` (`main/hardware/boards/wt32_sc01/`) owns the 1-Wire bus host and the
`Display_WT32SC01` panel driver, and `main.cpp` initialises it before any application
manager. Two consequences worth knowing:

- **The panel driver calls `lv_init()` itself**, not `DisplayManager`. Registering an LVGL
  display driver is the whole point of the driver, so it cannot run before LVGL is up, and
  the Board runs first. `DisplayManager` deliberately does not call `lv_init()`.
- **The panel driver lives in the board folder, not `hardware/drivers/`.** `drivers/` is
  for board-*independent* chip drivers; this one is this board's panel, wired one way, and
  no sibling board would share it. It is compiled in via `BOARD_SOURCES` in `board.cmake`.

`SensorManager` borrows the bus with `getBoard().GetOneWireBus()` and never names a GPIO.
A null bus is not fatal — the probes read as inactive and the rest of the device works.

### The LVGL pages reach settings through the registry

Strux made settings private to the manager that owns them: there is no `getInt(key)`. The
touch UI still has to edit `wifi.ssid`, `ntp.server`, `device.name` and friends, so
`DisplayPage` has `FindSetting` / `ReadSettingText` / `WriteSettingText` / `ReadSettingInt`,
which walk `SettingsManager`'s public iterator — the same route the generated web settings
UI takes. A page names a key; it never learns the type, and an unregistered key logs an
error instead of inventing a value.

Each settings page pairs its text rows against a file-local `kKeys[]` table **in creation
order**, because `SaveCb` walks the panel's textareas in that order. Add a row, add its key
in the same position.

`DisplayManager` owns `graph.min` / `graph.max` (the on-device chart's Y range) because the
chart is its own. `GraphPage` no longer configures a history log — that page's old keys
(`history.rate`, `monitor.rate`) died with `LogManager`.

### Telemetry replaces the flash log

`SensorManager::PublishTelemetry` emits one point per active slot, tagged `slot`, on
`sensor.telem` seconds. It runs on SensorManager's existing task rather than a separate
monitor. There is no on-device history: the web chart accumulates in the browser while the
page is open, and anything long-term lives wherever the relay ships points.

### NVS keys are capped at 15 characters

`SettingsManager::Register()` asserts this at **runtime**, so an over-long key compiles
fine and then boot-loops the device. `sensor.telem` (12) fits; a `sensor.telemetry` would
not. This is the single easiest way to brick a boot here.

## Conventions

Strux's apply — C++17, `snprintf` with `sizeof` bounds, JSON via `lib/json/JsonWriter.h`
and `JsonReader`/`JsonScope.h`, commands as per-manager `CommandEntry[]` tables registered
in `Init()`. New Thermy code follows Strux's naming (`serviceProvider_`, `initState_`,
trailing-underscore members), which is why `SensorManager` was renamed that way during the
port even though the old code did not use it.

Switches over `SettingType` have no `default` case on purpose — `-Werror=switch` then
breaks every converter when a type is added.
