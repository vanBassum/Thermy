import { moduleConfig } from "../_ui/vite-module"

// Thermy's own module, and the only one in this repo that is not the framework's:
// the four DS18B20 slots, live. Built by the same function as Strux's own modules,
// into the same `www/modules/`, and named by the firmware — see uiModule_ in
// main/app/SensorManager/SensorManager.h.
export default moduleConfig(import.meta.dirname, "temperature")
