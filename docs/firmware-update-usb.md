# Firmware update USB interface — stage 1

Development firmware 1.1.0-dev. Hardware revision is explicitly unassigned until the product revision is defined. This is not a complete customer updater.

Send newline-terminated ASCII commands over USB serial: `device_info`, `prepare_update`, `update_status`. Responses start with `@UPDATE ` followed by JSON and newline. Other console output can occur; ignore lines without this prefix. Open with DTR asserted and read continuously. Retry a query if no response; the existing bounded console queue can drop output under load. Commands are case-sensitive; maximum line length is 31 characters. Overlong lines are discarded.

`prepare_update` stops acquisition through the normal shutdown path, drains captured frames and log buffers, flushes and closes the log. It does not reboot or write firmware. Poll `update_status` until state is `ready` or `failed`; `preparing` is not permission to program. Readiness requires the existing clean shutdown checks, including valid configuration and successful storage writes. Missing SD or invalid configuration therefore fails closed. Recovery programming for these faults is a separate future workflow. Repeated preparation is safe; reset is needed to resume logging.

Device information remains queryable after shutdown and does not require an OLED or SD file. `sd_mounted` is the mount result, not a current card-presence probe. `oled_ready` reports communication availability, not visual correctness. `ready` means logging shutdown succeeded, not that an update image has been validated. MTP remains exposed after shutdown: the host must finish and close file transfers before any future programming operation.

Windows team: disconnect vehicle, use one logger, query identity, back up configuration, request preparation, poll status. Hardware compatibility checks, signed package verification, bootloader entry, installation and post-update verification are NOT implemented. `flash_enabled` is false. Do not interpret this API as authorization to bypass package checks.

Bench checks pending: query while recording; prepare during a multi-frame response; verify final log closure; query after stopping; repeat prepare; missing SD; invalid config; OLED disconnected; USB disconnect/reconnect; overlong and fragmented commands. Physical recovery button remains necessary for unresponsive firmware.

## Background uploader investigation

Use the official PaulStoffregen/teensy_loader_cli Windows command-line implementation, not Arduino CLI upload (which launches the graphical loader). Source: https://github.com/PaulStoffregen/teensy_loader_cli . Local build command: `gcc -O2 -Wall -s -DUSE_WIN32 -o teensy_loader_cli.exe teensy_loader_cli.c -lhid -lsetupapi -lwinmm`. Source and executable are kept in the ignored build directory. Preserve upstream GPL licence obligations when distributing. Windows integration should launch it without a console window and capture output. Background flashing and recovery still need bench validation; building the utility alone does not validate an update.

Status now includes sd_failures, log_drops, capture_drops and raw can_esr1 for fault investigation. USB-only auto polling can encounter missing CAN acknowledgements; do not assume a logging fault means the SD failed. Delayed responses remain under investigation.

## Diagnostic revision dev2

The response includes stop_reason (first shutdown cause) and firmware 1.1.0-dev2. Serial parsing/output is serviced before OLED work. The five-second OLED SD.usedSize scan is removed because FAT allocation scans can block control service; the displayed SD estimate now reflects the startup snapshot plus successful logger writes, and host file edits require reset to refresh it. This removes one identified blocking operation; MTP file transfers can still delay responses. Bench latency and fault-cause confirmation remain pending. No readiness checks have been bypassed.

## Verified Windows programming-mode entry

With one logger attached, confirm its identity and ready status and finish any host SD transfers. Set the selected COM port's USB serial line coding to 134 baud, then close the port and wait for HalfKay (VID 16C0, PID 0478). Run the command-line loader without a window. After reboot, rediscover the logger and query device_info to check the identity and expected version. The installed Teensyduino core handles the USB trigger separately from the application's flash_enabled field. This was verified by reinstalling dev2 on the bench. Package validation and device matching across re-enumeration remain Windows updater work. Keep the Program button for recovery.

## Locked customer units

The Windows command-line loader used in the dev2-to-dev3 test implements Intel HEX loading and has no EHEX support. Do not use it to program locked units. PJRC documents EHEX support in Teensy Loader. Evaluate an EHEX-capable Windows programming path with PJRC before committing to a popup-free customer workflow. The USB trigger and pre-update readiness protocol are separate from image programming.

Provision a dedicated Lockable Teensy 4.1 on the bench using PJRC's procedure and a test key. Verify a matching EHEX before permanently enabling secure mode. Keep the private key out of the app, update package, and repositories. The app should verify the signed release manifest, hardware compatibility, and image hash before requesting shutdown. Source: https://www.pjrc.com/teensy/td_code_security.html

On the locked test unit, install a newer matching EHEX, verify the same logger ID and expected version after reboot, and read the saved log. Confirm an ordinary HEX and an EHEX made with a different test key are rejected and the installed application remains bootable. Test reinstalling an approved image through the physical Program button. Record whether any window appears. Customer deployment depends on these hardware results.
