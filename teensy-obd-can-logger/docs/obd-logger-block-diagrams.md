# OBD Logger design review diagrams

The diagrams combine the verified bench workflow with planned production hardware. Labels marked planned require schematic review and bench validation.

## Product overview

```mermaid
flowchart LR
  O["Vehicle OBD-II connector"] <-->|CANH and CANL| P["CAN protection"]
  P <-->|CAN pair| T["TLE9371VSJ"]
  T <-->|TX and RX| I["ISO6421-Q1"]
  I <-->|CAN1 GPIO 22 and 23| M["Teensy 4.1"]
  O -->|Vehicle power| V["Protected input and main 5 V regulator"]
  V --> D["UCC33420-Q1 isolated supply"]
  D -->|5V_CAN| T
  D -->|CAN-side supply| I
  V --> Q["Power selection and backup cutoff · planned"]
  Q -->|5V_LOGGER| M
  M <-->|config.json and log files| SD["microSD"]
  M -->|I2C GPIO 18 and 19| OLED["128 × 32 OLED"]
  B["Mode 04 button"] -->|GPIO 40| M
  R["Recovery button"] -->|PROGRAM| M
  W["Windows configuration and update app"] <-->|USB serial and MTP| M
  W -->|USB power| Q
```

The isolation barrier crosses the signal isolator and isolated DC/DC converter. Keep GND_CAN and GND_LOGIC separate across it. The vehicle input return and USB power routing must be reviewed together so external wiring does not bypass the barrier. MTP exposes the SD card after recording stops and the file closes. Backup power serves log closure. RTC backup is outside the product scope.

## Power loss and shutdown

```mermaid
flowchart TD
  A["Vehicle supply powers recording"] --> B{"Vehicle supply lost?"}
  B -- No --> A
  B -- Yes --> C["PWR_FAIL signal reaches Teensy"]
  C --> D{"USB can supply shutdown load?"}
  D -- Yes --> E["Use USB power"]
  D -- No --> F["Enable battery hold-up and hardware timeout"]
  E --> G["Stop OBD requests and save queued data"]
  F --> G
  G --> H["Flush SD writes and close log"]
  H --> I["Disable battery hold-up after closure"]
  F -. "Timeout if saving stalls" .-> J["Hardware cuts battery power"]
  I --> K["Keep recording stopped while USB remains powered"]
```

Production checks: verify that the transfer to USB or battery keeps the Teensy and SD within their supply limits. Test vehicle removal with USB present and absent. Measure peak current and worst-case time to file closure before accepting the provisional 200 ms hold-up target. Verify the independent cutoff on a stalled save. Include test points for main 5 V, logger 5 V, isolated CAN 5 V, PWR_FAIL and each local ground.

## Firmware update and recovery

```mermaid
flowchart TD
  A["Windows identifies selected logger over USB"] --> B["Verify package and hardware compatibility"]
  B -->|Valid| C["Ask logger to stop and close the log"]
  B -->|Invalid| X["Report package error"]
  C --> D{"Logger ready?"}
  D -->|Yes| E["Enter programming mode over USB"]
  D -->|Failed or timed out| Y["Show reason and allow retry"]
  E --> F["Program approved image in the background"]
  F --> G{"Same device ID and expected version after reboot?"}
  G -->|Yes| H["Update successful"]
  G -->|No| R["Guide user to briefly press Recovery"]
  E -->|Automatic entry fails| R
  R --> F
```

Bench verified: standard-board HEX update from dev2 to dev3 without a button press or uploader window, with the same device ID and readable SD logs. Planned: package verification in the Windows app, locked-board EHEX installation and physical recovery. The current command-line uploader supports HEX. Interrupted-update automatic rollback is outside the verified design.

## CAN bench setup

```mermaid
flowchart LR
  S["DiagRA S multi-ECU simulation"] <--> A["Simulator CAN interface"]
  A <-->|CANH and CANL| B["Same terminated bench CAN bus"]
  B <-->|OBD requests and ECU replies| L["OBD Logger to SD log"]
  B -->|Listen only| K["Kvaser Leaf v3 to CANking ASC trace"]
```

Use the same simulation, JSON, bitrate and bus wiring for both firmware runs. Start CANking before restarting the logger. Align the two traces using common requests and replies. Record the firmware commit and any stop reason. The intermittent CAN receive overrun remains open.
