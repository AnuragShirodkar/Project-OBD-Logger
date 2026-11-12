# USB background programming bench test 01

Date: 2026-11-12 (Asia/Calcutta). Device OBD_1757338, COM8; user confirmed USB-only and pressed Program. Firmware source ca773b6, version 1.1.0-dev.

Official teensy_loader_cli built locally for Windows programmed 191488 bytes with --mcu=TEENSY41 and exited 0 after Booting. Arduino upload / graphical Teensy Loader was not invoked. Device subsequently returned the extended @UPDATE status, confirming the diagnostic image runs. This validates manual Program-button entry plus CLI upload; automatic entry and the branded Windows updater remain untested.

Observed status: state failed; sd_mounted true; oled_ready true; logging_fault true; sd_failures 0; log_drops 0; capture_drops 0; can_esr1 262800; flash_enabled false. The first query produced no response in an eight-second read window. Repeated queries returned three responses in a batch. Reliable response latency and safe-preparation success are NOT validated. No evidence of an SD write failure in these counters; exact fault origin remains to be isolated. No ECU was attached, so CAN transmission without acknowledgements is a candidate, not a confirmed cause.

Next: expose the latched stop/fault cause, isolate MTP and SD-capacity scans from control-response handling, and repeat preparation tests with controlled CAN/idle conditions. Do not enable unattended customer updates based on this test alone.

## Safe preparation with spare ECU

OBD_1757338 on COM8 ran firmware 1.1.0-dev2 (7510bd1). The operator confirmed the spare ECU was connected and logging was active before the host sent prepare_update. The host sent one update_status query while waiting. It observed state ready after 1957 ms from the original request, with stop_reason "Firmware update preparation", logging_fault false, and zero SD failures, dropped log entries, and dropped captured frames. Firmware readiness indicates that the logger completed its shutdown checks. Log contents were not independently inspected. The elapsed time covers preparation and status polling, rather than the latency of an isolated serial query. Flashing was disabled throughout.
