# SkiDrive logger projects

This workspace contains two independent projects. Each has its own sketch, tests,
configuration, documentation, firmware-package tools and OLED demo.

- [OBD Logger - previous project](teensy-obd-sd-logger/README.md): restored OBD-only
  source from commit `7021f2f`, firmware `1.1.0-dev6-folders`. It includes CONFIG/LOGS
  folders and the original manual OBD tester. Use version-1 OBD configurations.
- [OBD CAN Logger - new project](teensy-obd-can-logger/README.md): current two-mode
  source from commit `7d33b4a`, firmware `1.2.0-dev2-can-names`. A selected configuration
  chooses OBD polling or passive Classical CAN capture; examples are included.

Open the sketch inside the chosen project; both intentionally retain the Arduino
folder/sketch name `TeensyOBDSDLogger`. Do not mix headers or configs between them.
Run each project's own `test.ps1` from its folder. See each README for USB type,
dependencies, recording format and the limits of hardware validation.

Shared hardware material stays in `Schematics/`; the existing third-party reference
datasets under `New folder/` are preserved. These are not either firmware project.
The two retired STM32 simulator directories have been removed as requested.

This reorganization does not flash a logger or alter its SD card. Build outputs,
credentials and private keys are excluded from Git. Original source history is retained.
