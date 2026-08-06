#pragma once

#include "InitState.h"
#include "BoardConfig.h"
#include "interfaces/BoardProvider.h"
#include "Display_WT32SC01.h"
#include "drivers/MockLed.h"
#include "onewire_bus.h"

// ──────────────────────────────────────────────────────────────
// The board layer's context for the WT32-SC01: owns every hardware driver
// instance (and bus host) and answers BoardProvider.
//
// The bottom layer, and it depends on nothing above it — not the framework,
// not the application. Drivers take their pins and buses as constructor
// arguments, so nothing here needs a provider to find a peer; BoardProvider
// exists for the layer above.
//
// Every board folder provides a class named BoardContext; #include
// "BoardContext.h" resolves to the board selected with -DBOARD=<name>.
//
// Surface rules:
//   • role interfaces (Led&, ...) for devices the application addresses by
//     meaning — declared on BoardProvider, so every board owes every role
//     and binds a Mock* driver when not fitted;
//   • concrete driver accessors are the escape hatch for when the
//     application needs a driver's full API. Those stay OFF BoardProvider
//     and are checked at compile time, which is what stops the role list
//     becoming the union of every board's peripherals.
//
// The display and the 1-Wire bus are both escape-hatch accessors: they are
// this product's hardware, and no sibling board should owe a mock for them.
// ──────────────────────────────────────────────────────────────

class BoardContext : public BoardProvider
{
    static constexpr const char *TAG = "Board";

public:
    BoardContext() = default;

    BoardContext(const BoardContext &) = delete;
    BoardContext &operator=(const BoardContext &) = delete;
    BoardContext(BoardContext &&) = delete;
    BoardContext &operator=(BoardContext &&) = delete;

    void Init();

    /// No user LED is fitted on this board — this is a MockLed, so
    /// application code that speaks the Led role still works.
    Led &GetLed() override { return led_; }

    Display_WT32SC01 &GetDisplay() { return display_; }

    /// The 1-Wire bus the DS18B20 probes hang off. Null if the bus host
    /// failed to come up, which SensorManager treats as "no probes".
    onewire_bus_handle_t GetOneWireBus() const { return oneWireBus_; }

private:
    InitState initState_;

    // Hardware instances — buses first, then the drivers that use them.
    onewire_bus_handle_t oneWireBus_ = nullptr;

    MockLed led_;
    Display_WT32SC01 display_;

    void InitOneWire();
};
