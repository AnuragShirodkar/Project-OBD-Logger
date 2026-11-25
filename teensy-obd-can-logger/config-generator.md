# Windows configuration generator

Use [config.template.json](config.template.json) as the starter and [config.json](config.json) as the project's longer example. Export plain UTF-8 JSON as `config.json` at the root of the selected logger's SD card. Finish the transfer before restarting the logger so it can load the new file. Keep app settings outside this file. The firmware rejects unknown keys and files larger than 8192 bytes.

- `version` must be the integer `1`. It identifies the configuration format.
- `bitrate` is one of `125000`, `250000`, `500000`, or `1000000`. `address_bits` is `11` or `29`.
- `tx_id` and `rx_id` are quoted hexadecimal strings without `0x`. For functional requests use `7DF` for 11-bit or `18DB33F1` for 29-bit, with `rx_id` set to `"0"`. For a physical request use distinct, in-range IDs and a nonzero receive ID. The transmit ID cannot be the functional ID in that case.
- `startup_delay_ms` accepts 0–600000. `run_duration_ms` accepts 0 for continuous operation, or 1000–86400000. `response_timeout_ms` accepts 10–10000. Use integers for all timings.
- `requests` must contain 1–64 unique requests. Each item contains only `request` and `interval_ms`. Encode the service and parameters as space-separated hex bytes without CAN padding. Set `interval_ms` to an integer from 10 to 3600000. It is the requested polling interval. Actual polling can be slower while the logger waits for replies.
- Use `01 PID`, `02 PID FRAME`, `06 ID`, `09 INFO_TYPE`, or the single-byte services `03`, `07`, and `0A`. Leave disabled requests out of the export. Mode 04 is controlled by the logger's button and is outside this file format.

The template requests RPM, vehicle speed, stored fault codes, and VIN. The logger checks vehicle support for applicable requests during discovery. The template is a starting point for the generator and does not establish that every vehicle supports these requests.

Missing or rejected config.json triggers default live logging when the SD log can be opened. The display alternates NO CONFIG or BAD CONFIG with the current activity. The fallback discovers Mode 01 support at 500 kbps, 11-bit, functional ID 7DF and polls the union of confirmed live PIDs with a 250 ms target interval per PID. Actual intervals depend on the number of jobs and response timeouts. It skips bitmap queries during polling, leaves the file untouched, and stops if it cannot confirm any live PIDs or the ECU inventory overflows.
