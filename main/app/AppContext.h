#pragma once

#include "AppProvider.h"
#include "BoardContext.h"
#include "StruxProvider.h"
#include "SensorManager/SensorManager.h"
#include "DisplayManager/DisplayManager.h"

// The application layer's context: owns this product's managers and answers AppProvider.
//
// This is the file Thermy edits. Strux's own managers, their init order and their wiring
// all live one layer down in StruxContext, so pulling an improvement from the template
// does not touch anything here — which is the whole reason the layers were split.
//
// Adding an application manager means: create the class taking AppProvider&, add an
// accessor to AppProvider, add the member here, and call its Init() below. The framework
// does not need to be told it exists; the manager registers its own commands and
// settings into Strux from its Init().
class AppContext : public AppProvider
{
public:
    AppContext(BoardContext& board, StruxProvider& strux)
        : board_(board), strux_(strux) {}

    ~AppContext() = default;
    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;

    /// Bring the application up. Called last: every manager here registers into the
    /// framework, so the framework has to be ready before any of this runs.
    ///
    /// Sensors before the display: the display binds to the sensor slots and would
    /// otherwise paint before there is anything to paint.
    void Init()
    {
        sensorManager_.Init();
        displayManager_.Init();
    }

    StruxProvider& getStrux() override { return strux_; }
    BoardContext& getBoard() override { return board_; }
    SensorManager& getSensorManager() override { return sensorManager_; }
    DisplayManager& getDisplayManager() override { return displayManager_; }

private:
    BoardContext& board_;
    StruxProvider& strux_;

    // SensorManager is declared before DisplayManager because DisplayManager's
    // constructor binds a reference to it.
    SensorManager sensorManager_{*this};
    DisplayManager displayManager_{*this};
};
