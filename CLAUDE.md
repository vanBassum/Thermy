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

### Three layers, and the sync boundary runs between them

Since 2026-08-06 the tree is split into the three layers Strux uses, bottom to top. Last
synced against Strux on **2026-09-09**.

| Folder | Layer | Owns |
| --- | --- | --- |
| `main/hardware/` | board | the WT32-SC01's drivers; depends on nothing above it |
| `main/strux/` | framework | every Strux manager, and their init order |
| `main/app/` | application | Thermy: `SensorManager`, `DisplayManager` |
| `main/lib/` | — | the substrate all three stand on; not a layer |

Each layer is a **context** that owns the instances and a **provider** that says what the
layer above may reach for: `BoardContext`/`BoardProvider`, `StruxContext`/`StruxProvider`,
`AppContext`/`AppProvider`. A manager takes exactly one reference — its own layer's
provider — and finds everything through it. `main.cpp` is four calls; the init **order**
within a layer lives in that layer's context, so pulling a new Strux manager brings its
position along with it.

Nothing in `main/strux/` may reach up into `main/app/`. When the framework needs something
from Thermy, Thermy **registers** it (a command, a setting, a telemetry point) from its own
`Init()`. `StruxProvider` deliberately has no `getBoard()`.

**Keep the framework diffable against Strux.** `main/strux/` and `main/lib/` (bar
`core_utils.h`) are currently **byte-identical** to upstream and should stay that way, so
the next sync is `diff -r` and not an archaeology exercise. If a framework file must change
for Thermy, say so in a comment at the change.

Every folder is on the include path, so headers are included by name alone — which means an
include cannot show a layering violation. Reviewing the tree is the only audit.

### What is Thermy's, not Strux's

- `main/app/` — `AppContext`/`AppProvider`, `SensorManager/` (DS18B20 probes in four
  slots), `DisplayManager/` (LVGL UI and its six pages). Strux's `app/LedManager` example
  was not ported.
- `main/hardware/boards/wt32_sc01/` — the only board, including its panel driver
- `frontend/modules/temperature/` — the web UI module for the probes. Strux's own
  `modules/led` demo was not ported; its `_ui`, `console`, `settings` and `firmware`
  modules were, unchanged.
- `main/lib/common/core_utils.h` — two tick-arithmetic helpers Strux does not have

Everything else under `main/strux/`, `main/lib/`, `frontend/src/`, `frontend/modules/`
(bar `temperature/`), `frontend/shell-contract/` and `frontend/scripts/` is byte-identical
to Strux, with **one** deliberate exception: `frontend/src/App.tsx`'s empty-state text
points at `SensorManager` rather than Strux's `LedManager` as the worked example. Keep
that list at one entry.

### Deliberately gone

MQTT, Home Assistant, `LogManager` and the `flash_log` component were **removed**, not
ported (2026-08-05). Devices that exist to live in Home Assistant are better served by
ESPHome, and time-series history is `TelemetryManager`'s job now — a manager records a
point, the relay ships it to InfluxDB. `DeviceManager` is gone too: the per-board
`BoardContext` replaced its role. Do not reintroduce any of them; the last versions are in
git history.

`relay-server/` went the same way (2026-09-09), for a different reason: it was a stale
copy of Strux's, imported once and never touched here, and the relay is now its own
repository — [vanBassum/strux-relay](https://github.com/vanBassum/strux-relay). Thermy
dials out to a deployment of that; nothing in this tree builds or serves one. Point
`relay.url` at it and turn `relay.enabled` on.

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
override the common defaults inherited from Strux.

**The root `sdkconfig.defaults` must not assert anything this board contradicts.**
Strux's root `CMakeLists.txt` now refuses a build whose *generated* `sdkconfig`
disagrees with the composed defaults — a real guard, because a generated file wins
over the defaults and an option sitting at its default is still recorded there, so a
config change can look landed and do nothing. The guard cannot tell a board
legitimately overriding a common default from a stale `sdkconfig`, so Thermy dropped
the two lines its only board overrides (`CONFIG_ESPTOOLPY_FLASHSIZE_4MB`, and the
partition-table filename) from the root file. That is the only place Thermy's root
`sdkconfig.defaults` diverges from Strux's; a comment at the top says so. Override a
default deliberately with `-DSTRUX_ALLOW_SDKCONFIG_DRIFT=ON`.

Frontend, from `frontend/`: `pnpm dev` (shell only — see below), `pnpm build`,
`pnpm typecheck`. `pnpm build` builds the shell, then every module bundle, then
checks the module/shell seam, then gzips `www/`.

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

### The web UI is a shell plus firmware-shipped modules, and Thermy owns one

Since the 2026-09-09 sync the frontend is Strux's shell + UI-module architecture, and
the rule behind it is short: **a shell contributes nothing to a device's navigation.**
Every page in the web UI comes from a module the *firmware* declares in the `ui
modules` command, and `UiManager` is the framework manager that answers it.

So Thermy's web page is `frontend/modules/temperature/`, a self-contained ES module
built to `www/modules/temperature.js`, and `SensorManager` declares it — id,
bundle path and one `UiPage` — next to its command table. Three things follow:

- **The temperature page is the landing page, and nothing says so explicitly.** The
  first page the *manifest* declares wins; `UiManager` head-inserts and the app layer
  initialises after the framework, so Thermy's module lands ahead of Strux's console,
  settings and firmware. There is no separate web home page any more — a device with
  one feature has nothing to put on one, and the old page's device readout now lives
  in a dialog behind the sidebar footer. (The device's own LVGL `HomePage` was always
  the probe screen, and is untouched.)
- **The page ids must match on both sides.** `uiPages_` in `SensorManager.h` and the
  `shell.routes.register({ id: … })` in the module's `index.tsx` are the same string.
  Registered-but-undeclared is ignored; declared-but-unregistered keeps its nav entry
  and says so when opened.
- **A module gets `shell.transport` and nothing under it.** The sensor API left
  `frontend/src/lib/backend.ts` entirely: the shape of `sensor list`'s reply belongs
  to whoever owns the `sensor` commands, so it lives in
  `modules/temperature/src/sensors.ts`. `backend.ts` is byte-identical to Strux's
  again, and should stay that way.

Two build-time facts that bite:

- **A module externalises `react` and `react/jsx-runtime` only** (the import map in
  `index.html`), so anything else it imports is bundled into flash. That is why the
  temperature chart is hand-drawn SVG rather than recharts: recharts 3 brings
  `@reduxjs/toolkit`, `react-redux`, `immer` and d3 with it, plus a `react-dom` peer
  that would become a second renderer beside the shell's.
  `modules/temperature/src/Chart.tsx` has the full argument, and `recharts` is no longer
  a dependency.
- **`pnpm dev` serves the shell, not the modules.** A module bundle is whatever
  `pnpm build:modules` last wrote into `www/modules/`; the dev server just serves it
  from there. Editing a module means rebuilding it. Editing the shell stays live.

### The board owns the hardware, including LVGL's display

`BoardContext` (`main/hardware/boards/wt32_sc01/`) owns the 1-Wire bus host and the
`Display_WT32SC01` panel driver, and `main.cpp` initialises the whole board layer first.
Three consequences worth knowing:

- **The panel driver calls `lv_init()` itself**, not `DisplayManager`. Registering an LVGL
  display driver is the whole point of the driver, so it cannot run before LVGL is up, and
  the board runs first. `DisplayManager` deliberately does not call `lv_init()`.
- **The panel driver lives in the board folder, not `hardware/drivers/`.** `drivers/` is
  for board-*independent* chip drivers; this one is this board's panel, wired one way, and
  no sibling board would share it. It is compiled in via `BOARD_SOURCES` in `board.cmake`.
- **`GetDisplay()` and `GetOneWireBus()` stay off `BoardProvider`.** That interface carries
  roles every board owes (`GetLed()`, bound here to a `MockLed` — no LED is fitted). The
  two concrete accessors are the escape hatch, checked at compile time, which is what stops
  the role list becoming the union of every board's peripherals.

`SensorManager` borrows the bus with `app_.getBoard().GetOneWireBus()` and never names a
GPIO. A null bus is not fatal — the probes read as inactive and the rest of the device
works.

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
page is open (300 samples at a 2 s poll, ~10 minutes, and a reload starts it over), and
anything long-term lives wherever the relay ships points.

### NVS keys are capped at 15 characters

`SettingsManager::Register()` asserts this at **runtime**, so an over-long key compiles
fine and then boot-loops the device. `sensor.telem` (12) fits; a `sensor.telemetry` would
not. This is the single easiest way to brick a boot here.

## Conventions

Strux's apply — C++17, `snprintf` with `sizeof` bounds, commands as per-manager
`CommandEntry[]` tables registered in `Init()`, trailing-underscore members
(`initState_`, and `app_` for an app manager's `AppProvider&`).

A command handler names the **shape** of its reply and never the wire format:
`auto resp = ctx.reply.object();`, then `resp.field(...)`. `JsonScope.h` is gone — see
`lib/protocol/ReplyWriter.h`. `ctx.out` is still reachable for raw bytes alongside a
reply, which is how `getWebFile` writes a header record and then streams a file body.

Switches over `SettingType` have no `default` case on purpose — `-Werror=switch` then
breaks every converter when a type is added.
