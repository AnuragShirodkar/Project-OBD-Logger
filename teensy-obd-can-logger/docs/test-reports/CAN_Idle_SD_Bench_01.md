# CAN idle SD access - USB bench installation

Date: 2026-11-25. User confirmed USB-only power with vehicle/ECU CAN disconnected.
Target: OBD_1757338, COM8. No configuration changes were made.

## Update

Before installation, device_info reported 1.2.0-dev1-dual-mode, operating_mode=can,
SD mounted, OLED responding and no logging/storage/drop faults. prepare_update
returned ready with stop_reason="Firmware update preparation". Exactly one Teensy
USB device was confirmed present before entering programming mode at 134 baud.

The hidden command-line loader programmed and booted the compiled normal-Teensy
HEX successfully. Loader reported 202752 programmed bytes. HEX file SHA-256:
D54A4DA7E390A7C6588E00D4BEA8B2F4A929E1CC9708A02F6BEE22309953CDF9.

## Observed after restart

- Same ID OBD_1757338, firmware 1.2.0-dev4-idle-sd, operating_mode=can.
- Automatic state ready; stop_reason="No CAN traffic for more than 3 seconds".
- sd_mounted=true, oled_ready=true, logging_fault=false; sd_failures=0,
  log_drops=0 and capture_drops=0. No stop command was sent after the update.
- Windows exposed This PC > OBD_1757338 > SD card > LOGS automatically.
- Readback of LOGS/01_CAN_1757338.ASC confirmed the new name, firmware/product/device
  metadata, aligned-trace header, trigger block, idle-stop reason and final
  End Triggerblock. Config fingerprint BDFFF497; 500000 bps, listen-only.
- Final file counters: seen=0, drained=0, buffered=0, hw_overruns=0, write_failures=0,
  unwritten_bytes=0. Zero received frames is expected for this USB-only check.
- Downloaded log SHA-256:
  FC84661CB0B2C86B73AA166B3AA736169134C8BEDCF07CB83DAEC9EDC765F1A3.

The log's STOP timestamp is 6317 ms since boot, including initialization. This is
not a direct measurement of the idle interval; its threshold was checked by the
software boundary tests. No calendar timestamp or independent timing instrument
was used. OLED responding confirms I2C availability, not visible SD ACCESS text.

Pending: visual OLED confirmation, traffic-then-idle behavior on a real test bus,
valid frame capture, load/loss comparisons, return of traffic during file access,
physical rotation/storage faults and OBD regression on hardware. This verifies
installation and the no-frames idle path, not the whole CAN logger release.
