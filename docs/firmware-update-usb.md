# Firmware update USB interface — stage 1

Development firmware 1.1.0-dev. Hardware revision is explicitly unassigned until the product revision is defined. This is not a complete customer updater.

Send newline-terminated ASCII commands over USB serial: `device_info`, `prepare_update`, `update_status`. Responses start with `@UPDATE ` followed by JSON and newline. Other console output can occur; ignore lines without this prefix. Open with DTR asserted and read continuously. Retry a query if no response; the existing bounded console queue can drop output under load. Commands are case-sensitive; maximum line length is 31 characters. Overlong lines are discarded.

`prepare_update` stops acquisition through the normal shutdown path, drains captured frames and log buffers, flushes and closes the log. It does not reboot or write firmware. Poll `update_status` until state is `ready` or `failed`; `preparing` is not permission to program. Readiness requires the existing clean shutdown checks, including valid configuration and successful storage writes. Missing SD or invalid configuration therefore fails closed. Recovery programming for these faults is a separate future workflow. Repeated preparation is safe; reset is needed to resume logging.

Device information remains queryable after shutdown and does not require an OLED or SD file. `sd_mounted` is the mount result, not a current card-presence probe. `oled_ready` reports communication availability, not visual correctness. `ready` means logging shutdown succeeded, not that an update image has been validated. MTP remains exposed after shutdown: the host must finish and close file transfers before any future programming operation.

Windows team: disconnect vehicle, use one logger, query identity, back up configuration, request preparation, poll status. Hardware compatibility checks, signed package verification, bootloader entry, installation and post-update verification are NOT implemented. `flash_enabled` is false. Do not interpret this API as authorization to bypass package checks.

Bench checks pending: query while recording; prepare during a multi-frame response; verify final log closure; query after stopping; repeat prepare; missing SD; invalid config; OLED disconnected; USB disconnect/reconnect; overlong and fragmented commands. Physical recovery button remains necessary for unresponsive firmware.
