// The temperature module's entry point, and the only thing the manifest names.
//
// One export, `activate`, called once by whichever shell imported this file. It never
// constructs a device connection and never imports the shell's backend: everything it
// needs arrives in `shell`, which is what lets the identical bundle run on the
// device's own shell and on the relay's.

import css from "./index.css?inline"
import type { ActivateFn } from "@shell/contract"
import { adoptStyles } from "../../_ui/activate"
import { TemperaturePage } from "./TemperaturePage"

export const activate: ActivateFn = (shell) => {
  adoptStyles("temperature", css)

  // The id must match what the firmware declared in `ui modules` — the shell IGNORES
  // anything undeclared, because honouring it would make navigation depend on running
  // module code. See uiPages_ in main/app/SensorManager/SensorManager.h: same string.
  //
  // It is also the LANDING page, and nothing here arranges that: it is the first page
  // the manifest declares, because UiManager head-inserts and Thermy's app layer
  // initialises after the framework's. The probes are what this device is for.
  shell.routes.register({
    id: "temperature",
    render: () => <TemperaturePage shell={shell} />,
  })
}
