#pragma once

#include "ServiceProvider.h"
#include "InitState.h"
#include "BoardConfig.h"
#include "Display_WT32SC01.h"
#include "drivers/MockLed.h"
#include "onewire_bus.h"

// ──────────────────────────────────────────────────────────────
// Board for the WT32-SC01. Owns every hardware driver instance (and bus
// host) and exposes the capability surface the application compiles against.
//
// Every board folder provides a class named Board with the same surface;
// #include "Board.h" resolves to the board selected with -DBOARD=<name>.
// There is deliberately no IBoard base class: a board that misses a method
// the application uses fails to compile for that board.
//
// Surface rules:
//   • role interfaces (Led&, ...) for devices the application addresses by
//     meaning — bind a Mock* driver when not fitted;
//   • concrete driver accessors are allowed as an escape hatch when the
//     application needs a driver's full API.
//
// The display is an escape-hatch accessor: DisplayManager needs the LVGL
// display handle and the backlight, which is the driver's full API and not
// something a 1-3 method role interface would capture.
// ──────────────────────────────────────────────────────────────

class Board
{
    static constexpr const char *TAG = "Board";

public:
    explicit Board(ServiceProvider &serviceProvider);

    Board(const Board &) = delete;
    Board &operator=(const Board &) = delete;
    Board(Board &&) = delete;
    Board &operator=(Board &&) = delete;

    void Init();

    /// No user LED is fitted on this board — this is a MockLed, so
    /// application code that speaks the Led role still works.
    Led &GetLed() { return led_; }

    Display_WT32SC01 &GetDisplay() { return display_; }

    /// The 1-Wire bus the DS18B20 probes hang off. Null if the bus host
    /// failed to come up, which SensorManager treats as "no probes".
    onewire_bus_handle_t GetOneWireBus() const { return oneWireBus_; }

private:
    ServiceProvider &serviceProvider_;
    InitState initState_;

    // Hardware instances — buses first, then the drivers that use them.
    onewire_bus_handle_t oneWireBus_ = nullptr;

    MockLed led_;
    Display_WT32SC01 display_;

    void InitOneWire();
};
