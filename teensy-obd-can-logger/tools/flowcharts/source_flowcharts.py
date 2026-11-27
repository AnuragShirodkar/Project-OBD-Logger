"""Code-reviewed flowchart specification for the installed SkiDrive dev9 build.

Runtime logic only. Diagram text is deliberately explicit about inactive helpers
and dependency boundaries. Run from the OBD/CAN project folder.
"""
import hashlib,json,pathlib,re
ROOT=pathlib.Path(__file__).resolve().parents[2]
SRC=ROOT/'TeensyOBDSDLogger'
OUT=ROOT/'docs/flowcharts/dev9'
PAGES=[]
def chart(title,refs,notes,nodes,edges):
    PAGES.append(dict(number=len(PAGES)+1,title=title,refs=refs,notes=notes,
                      nodes=[dict(id=a,kind=b,label=c) for a,b,c in nodes],
                      edges=[dict(source=a,target=b,label=c) for a,b,c in edges]))

chart('Firmware at a glance', ['TeensyOBDSDLogger.ino: setup, loop, stopRun'],[
 'Reference: installed 1.2.0-dev9-oled-gaps. One mode per boot; this is not simultaneous OBD polling and passive CAN recording.',
 'Solid arrows show control order. Yes/No labels identify decisions. A call box points to another detailed chart by its title.',
 'The CAN interrupt runs independently of the repeating main loop. The loop must drain its queues before exposing SD storage.',
 'This atlas covers project-owned runtime modules and the bundled MTP responder. External Arduino, SD/FAT, USB, CAN and graphics driver internals are dependency boundaries, not reconstructed here.'
],[('a','start','Power on or reset'),('b','call','Identity and timer startup'),('c','call','SD and JSON configuration'),('d','decision','Configuration, timer, PSRAM and log ready?'),('e','call','Configure CAN1 and enter main loop'),('f','decision','Selected operating mode?'),('g','call','OBD: discover support, filter jobs, request and collect responses'),('h','call','CAN: listen only and record received frames'),('i','call','Common: buffers, SD, OLED, serial and health checks'),('j','decision','Stop, duration, idle or fault?'),('k','call','Stop capture, drain, save and close'),('l','end','Expose available SD over MTP; restart required to record again')],
 [('a','b',''),('b','c',''),('c','d',''),('d','e','Yes'),('d','k','No'),('e','f',''),('f','g','OBD'),('f','h','CAN'),('g','i',''),('h','i',''),('i','j',''),('j','f','No: next loop'),('j','k','Yes'),('k','l','')])

chart('Identity and independent timer', ['usb_identity.c: startup_middle_hook','CaptureTimer.h: begin, now','CanRecording.h: CaptureClock'],[
 'Numeric identity = HW_OCOTP_MAC0 & 0xFFFFFF. Permanent ID remains OBD_<number>; friendly USB/MTP name is SkiDrive_<number>. Firmware updates do not rewrite the fuse ID.',
 'GPT2 is reserved. PERCLK must be the undivided 24 MHz crystal source. A running GPT2 or unexpected clock makes begin() return false.',
 'Reset wait is bounded by 100,000 polls. GPT2 prescaler 23 gives 1 MHz; free-running mode uses no timer IRQ. Initial offset is sampled from micros().',
 'captureNowUs() briefly serializes a 64-bit extension of unsigned 32-bit GPT2 differences. Main-loop sampling must occur at least once per approximately 71-minute wrap. OBD millisecond traces/scheduling still use millis().'
],[('a','start','Core calls startup_middle_hook after RAM initialization'),('b','process','Read fuse number; build stable ID and SkiDrive USB descriptor'),('c','process','setup(): enable GPT2 clock gates'),('d','decision','GPT2 free and PERCLK expected?'),('e','process','Request GPT2 software reset'),('f','decision','Reset clears within bounded polling?'),('g','process','Set PR=23; disable IRQ; clear flags; set epoch offset'),('h','process','Enable free-running GPT2 at 1 MHz'),('i','end','Return timer ready; setup continues'),('j','fault','Return timer unavailable; setup later disables CAN'),('k','call','During loop/ISR: extend counter and sample timestamps')],
 [('a','b',''),('b','c',''),('c','d',''),('d','e','Yes'),('d','j','No'),('e','f',''),('f','g','Yes'),('f','j','No'),('g','h',''),('h','i',''),('i','k','During runtime')])

chart('Setup: interfaces, storage and config selection', ['TeensyOBDSDLogger.ino: setup','StoragePaths.h: configPath, ensureDirectory'],[
 'Serial is initialized at 115200. GPIO40 uses INPUT_PULLUP. MTP starts with a silent debug stream; the SD filesystem is not yet registered for host access.',
 'OLED probes I2C 0x3C then 0x3D. The first responding address is tried; it does not fall through to the second address if oled.begin fails. Wire is set to 400 kHz.',
 'SD usage is scanned once at startup. CONFIG/LOGS must be directories; a regular file with either name is an error and is not overwritten.',
 'Canonical /CONFIG/config.json wins whenever present, even if invalid. Root /config.json is tried only when canonical is absent.',
 'Selected file must be a regular file <=8192 bytes. Compute FNV1a over its bytes, rewind, parse with nesting limit 4, reject trailing non-whitespace, then close it.'
],[('a','start','Begin setup; initialize timer, serial, button and MTP'),('b','process','Probe OLED; initialize if found; continue even if unavailable'),('c','decision','SD.begin succeeds?'),('d','process','Read capacity/usage; ensure CONFIG and LOGS directories'),('e','process','Choose canonical config or legacy root fallback'),('f','decision','Regular config file within size limit?'),('g','process','Fingerprint file; deserialize; validate schema; reject trailing data'),('h','decision','JSON and schema valid?'),('i','process','Set configGood and modeKnown; attempt new log and header'),('j','fault','CONFIG_ERROR; reject config; mark fault and stop reason'),('k','process','No mounted SD: leave config/log unavailable'),('l','call','Continue to memory and CAN startup')],
 [('a','b',''),('b','c',''),('c','d','Yes'),('c','k','No'),('d','e',''),('e','f',''),('f','g','Yes'),('f','j','No'),('g','h',''),('h','i','Yes'),('h','j','No'),('i','l',''),('j','l',''),('k','l','')])

chart('JSON: version, mode and common options', ['Config.h: Config, parseConfig, readUInt, hexId'],[
 'Only schema versions 1 and 2 are implemented. Version 1 means OBD. Version 2 requires exact lowercase operating_mode "obd" or "can".',
 'Defaults: 500000 bit/s, startup 5000 ms for OBD or 0 for CAN, run duration 0, response timeout 70 ms. Missing optional unsigned values preserve defaults.',
 'Allowed bitrates: 125000, 250000, 500000, 1000000. Startup: 0..600000 ms. Duration: 0 (continuous), or 1000..86400000 ms.',
 'Unknown/mode-incompatible keys reject the entire file. CAN has no requests/address/timeout fields. The proposed filters object and version 3 are NOT implemented.',
 'Missing/invalid config does not trigger default polling in this build.'
],[('a','start','parseConfig resets Config defaults'),('b','decision','Top-level JSON object and version 1 or 2?'),('c','decision','Version 2?'),('d','decision','Valid required operating_mode?'),('e','process','Set selected mode; set CAN startup default to 0'),('f','process','Check every key against mode allowlist'),('g','decision','Keys, bitrate and timing values valid?'),('h','decision','CAN mode?'),('i','end','Accept CAN configuration; no diagnostic jobs'),('j','call','Continue OBD address/job validation'),('x','fault','Reject configuration and return reason')],
 [('a','b',''),('b','c','Yes'),('b','x','No'),('c','d','Yes'),('c','e','No: OBD'),('d','e','Yes'),('d','x','No'),('e','f',''),('f','g',''),('g','h','Yes'),('g','x','No'),('h','i','Yes'),('h','j','No')])

chart('JSON: OBD addresses and request jobs', ['Config.h: parseConfig','DiagnosticCore.h: parseHex, Address'],[
 'address_bits is 11 or 29. Defaults: 7DF or 18DB33F1 TX; rx_id=0 selects functional responses. Supplied TX/RX IDs are hex strings WITHOUT 0x.',
 'IDs must fit the address width. RX=0 requires functional TX; physical RX requires non-functional TX; TX and RX cannot be equal.',
 'response_timeout_ms remains accepted from 10..10000 ms; 70 is the default. It is not fixed by firmware, despite the intended software UI default.',
 '1..64 unique jobs. Request bytes are 1-2 hex digits separated by spaces/tabs, maximum 3 bytes. Required interval_ms: 10..3600000.',
 'Allowed request shapes: 01 PID, 02 PID frame, 03, 06 MID, 07, 09 INFO, 0A. Mode 04 is button-only. Modes 05/08 are rejected.'
],[('a','start','Continue OBD config parsing'),('b','process','Validate width; apply functional TX/RX defaults'),('c','decision','Supplied hex IDs and address pair valid?'),('d','decision','Timing/timeout and requests array valid?'),('e','process','For each job: only request and interval_ms keys'),('f','decision','Hex bytes, service shape and interval valid?'),('g','decision','Duplicate request bytes already present?'),('h','process','Store job; increment count'),('i','decision','More jobs?'),('j','end','Accept OBD configuration'),('x','fault','Reject whole configuration')],
 [('a','b',''),('b','c',''),('c','d','Yes'),('c','x','No'),('d','e','Yes'),('d','x','No'),('e','f',''),('f','g','Yes'),('f','x','No'),('g','x','Yes'),('g','h','No'),('h','i',''),('i','e','Yes'),('i','j','No')])

chart('Setup: memory, log and CAN enable gates', ['TeensyOBDSDLogger.ino: setup, configureCan','ReceivePipeline.h: begin'],[
 'The first file/header is attempted only with valid config, mounted SD and valid directories. Failure leaves sdGood=false and reports SD_INIT_FAILED.',
 'Timer failure sets CAPTURE_TIMER_ERROR, configGood=false and a fault. PSRAM allocations are attempted when configGood is true, even if the log failed.',
 'Allocate 65536 Captured slots and 4096 LogLine slots externally; both pointers must have high address nibble 7. Failure frees allocations and prevents CAN startup.',
 'Initialize slots, migrate startup text from the 64-line emergency queue, then bind 2048 DTCM staging slots plus PSRAM backlog.',
 'CAN1: TX22/RX23, 16 mailboxes, FIFO ACCEPT_ALL, callback + wrapper IRQ priority64. Start listen-only first; OBD switches to TX mode. OBD capture begins immediately; CAN capture waits for startup delay.'
],[('a','start','After config/file setup'),('b','decision','Config valid but GPT2 unavailable?'),('c','fault','Mark timer fault; invalidate config'),('d','decision','Config still valid?'),('e','process','Allocate and verify external capture/log memory'),('f','decision','Both pointers are actual PSRAM?'),('g','process','Initialize queues and staging pipeline'),('h','fault','Free memory; PSRAM_ERROR; invalidate config'),('i','decision','configGood AND sdGood?'),('j','process','Set address/response window; configure CAN1'),('k','process','Set setupAt; leave startup/polling to loop'),('l','process','Set closing=true if config or log unavailable'),('m','end','Enter main loop')],
 [('a','b',''),('b','c','Yes'),('b','d','No'),('c','d',''),('d','e','Yes'),('d','i','No'),('e','f',''),('f','g','Yes'),('f','h','No'),('g','i',''),('h','i',''),('i','j','Yes'),('i','k','No'),('j','k',''),('k','l',''),('l','m','')])

chart('Main loop: exact service order', ['TeensyOBDSDLogger.ino: loop'],[
 'This loop repeats continuously. CAN interrupts can run between these main-loop operations.',
 'transferCapture moves at most 512 records per call. Decode drains at most 128 records while logCount < logCapacity-512, preserving space for decoded text.',
 'CAN errors are read up to eight per iteration. Only actual error bits/counters/fault-state changes produce CAN_ERROR notes; hardware overruns or bus-off trigger stop.',
 'A finished logger services USB/MTP and OLED but does not automatically start another recording. Configuration edits require a restart.'
],[('a','start','Next main-loop iteration'),('b','process','Extend clock; update loop-gap maximum; transfer staging'),('c','process','pollStop: button + serial; pump console; update OLED'),('d','decision','finished?'),('e','process','Pump console; MTP.loop; pump console; return'),('f','call','If config good: transfer and process capture backlog'),('g','call','If config good and not closing: health gates, then CAN or OBD control'),('h','decision','Closing and both receive queues empty; no final summary yet?'),('i','process','Write final summary and CAPTURE_DRAINED marker'),('j','process','Pump up to 128 log lines + console; call MTP.loop'),('k','call','If due and diagnostics idle: write/flush every 1000 ms'),('l','call','If closing, summary done and text queue empty: final file close and expose SD'),('m','end','Return; next iteration')],
 [('a','b',''),('b','c',''),('c','d',''),('d','e','Yes'),('d','f','No'),('f','g',''),('g','h',''),('h','i','Yes'),('h','j','No'),('i','j',''),('j','k',''),('k','l',''),('l','m','')])

chart('CAN interrupt and receive callback', ['TeensyOBDSDLogger.ino: captureCanInterrupt, onCan'],[
 'IRQ priority64 is above default128 and below SysTick32. Wrapper duration includes preemption, not only useful work.',
 'Check FIFO overflow before each bundled library ISR pass. On first observed overrun save GPT2 time, flags and ESR. Always clear the overflow flag.',
 'The bundled library removes one FIFO frame per invocation and calls onCan. The wrapper repeats up to eight passes while FIFO data remains.',
 'onCan never writes SD, formats strings or touches the PSRAM frame backlog. It copies to internal DTCM only. A failed stage increments RXdrops.',
 'Compare DWT duration converted at F_CPU_ACTUAL with GPT2 duration; differences >100 us increment clock_disagreements.'
],[('a','start','CAN1 interrupt'),('b','process','Sample GPT2 + DWT; increment ISR entry count'),('c','decision','FIFO overflow bit set?'),('d','process','If capture enabled: save first details; count overrun; clear flag'),('e','call','Run bundled FlexCAN ISR; callback on each delivered frame'),('f','decision','onCan: captureEnabled?'),('g','process','Timestamp frame; sequence++; CAN activity update; count frame overrun'),('h','process','Push DTCM staging; count failed insert or update peak'),('i','decision','FIFO still pending and passes < 8?'),('j','process','Update maximum cycles/us/passes and clock disagreement count'),('k','end','Return from interrupt')],
 [('a','b',''),('b','c',''),('c','d','Yes'),('c','e','No'),('d','e',''),('e','f','RX callback'),('e','i','No RX callback'),('f','g','Yes'),('f','i','No'),('g','h',''),('h','i',''),('i','c','Yes'),('i','j','No'),('j','k','')])

chart('Receive queues, processing and raw export', ['ReceivePipeline.h','CaptureRing.h','TeensyOBDSDLogger.ino: transferCapture, loop'],[
 'DTCM staging: 2048 x 40 bytes =81920 bytes. PSRAM frame backlog:65536 x40 =2.5 MiB. Text queue:4096 x320 =1.25 MiB.',
 'Single ISR producer; main-loop consumer. Atomic publish/load ordering prevents reading incomplete records. A blocked external transfer leaves staged records intact.',
 'CAN ASC exports all controller-delivered standard/extended data/remote frames. Remote frames carry DLC but no invented payload bytes.',
 'OBD raw RX export checks configured response IDs and width, including late/unmatched/queued-at-stop frames. Only non-remote frames feed active diagnostic decoding.',
 'RX_QUEUE_OVERFLOW or hardware overrun is a fault. A sequence number starts at callback delivery; it cannot count frames lost before the callback.'
],[('a','start','Main calls transferCapture'),('b','decision','Move limit not reached; backlog space and staged frame available?'),('c','process','Copy one record into PSRAM; repeat up to 512'),('d','process','Update PSRAM peak; enter bounded decoder drain'),('e','decision','CAN mode?'),('f','process','Validate Classical CAN ID/DLC; format ASC; fault if unrepresentable'),('g','process','OBD: trace accepted RX IDs, including RTR; others not exported'),('h','process','Increment captureDrained; flag frame overrun'),('i','decision','OBD, not closing, non-remote?'),('j','call','DiagnosticCore.receive using captured milliseconds'),('k','process','After drain: report new drops; mark capture loss as fault'),('l','end','Return to loop health gates')],
 [('a','b',''),('b','c','Yes'),('c','b',''),('b','d','No'),('d','e','For each drained frame'),('e','f','Yes'),('e','g','No'),('f','h',''),('g','h',''),('h','i',''),('i','j','Yes'),('i','k','No / drain done'),('j','k','After bounded drain'),('k','l','')])

chart('Passive CAN recording control', ['CanRecording.h: passiveAction, canIdleExpired','TeensyOBDSDLogger.ino: loop CAN branch'],[
 'Priority: user stop; startup; duration; idle; otherwise wait. No OBD requests, ACK transmission or CAN error-frame transmission in listen-only mode.',
 'At start: save the 64-bit elapsed origin, runStart and running=true; enable capture. Frames before capture enable are not queued.',
 'Idle expiry is strictly more than 3000 ms since last valid callback frame, or capture start if none arrived. Startup delay is excluded.',
 'Action/ISR activity snapshot and idle capture cutoff are serialized briefly, preventing a late frame from silently restarting SD-access mode.',
 'run_duration_ms=0 removes duration limit but does not disable idle stop. No auto-resume once stopped.'
],[('a','start','CAN branch after healthy loop gates'),('b','decision','stopRequested?'),('c','call','stopRun: user requested stop'),('d','decision','Not running and startup delay elapsed?'),('e','process','Set capture origin/runStart; running=true; captureEnabled=true'),('f','decision','Running and nonzero duration reached?'),('g','call','stopRun: configured duration finished'),('h','decision','Running and no callback traffic for >3000 ms?'),('i','process','Disable capture atomically; stopRun: no traffic'),('j','process','Wait/continue; summary every 1000 ms while running'),('k','end','Common log/USB service and next loop')],
 [('a','b',''),('b','c','Yes'),('b','d','No'),('d','e','Yes'),('d','f','No'),('e','j',''),('f','g','Yes'),('f','h','No'),('h','i','Yes'),('h','j','No'),('c','k',''),('g','k',''),('i','k',''),('j','k','')])

chart('OBD startup and discovery sequencing', ['TeensyOBDSDLogger.ino: loop OBD startup branch','Discovery.h'],[
 'Capture and CAN ACK capability are already enabled in OBD setup; diagnostic requests wait for startup delay.',
 'Stop before running and while idle invokes stopRun. tick() runs only after both receive queues are empty.',
 'At every ended/cancelled transaction, pending TX mailbox is a fatal stop; there is no reset-and-continue that could erase receive data.',
 'Discovery starts once, with modes01/02(frame00)/06/09. A new request requires no active diagnostic transaction, no captured backlog and >=10 ms since last accepted request.',
 'After discovery, conservative job filtering starts the fair schedule. With no enabled jobs stop; with no observed ECU keep unknown support rather than inventing unsupported evidence.'
],[('a','start','OBD control branch'),('b','process','Handle early stop; tick diagnostics only when capturePending=0'),('c','decision','Transaction ended?'),('d','decision','Any hardware TX still pending?'),('e','fault','Stop: TX pending at transaction end'),('f','process','If discovery active and not closing: finishTransaction'),('g','decision','Not running/stopping/closing and startup elapsed?'),('h','process','Begin discovery once'),('i','decision','Discovery busy?'),('j','process','When request gates permit: get support-map request and transmit'),('k','call','Discovery done: inventory, support filtering and expected-ECU maps'),('l','end','Return to running OBD control / next loop')],
 [('a','b',''),('b','c',''),('c','d','Yes'),('c','g','No'),('d','e','Yes'),('d','f','No'),('f','g',''),('g','h','Yes'),('g','l','No'),('h','i',''),('i','j','Yes'),('i','k','No'),('j','l',''),('k','l',''),('e','l','')])

chart('Discovery maps, retries and range continuation', ['Discovery.h: request, observe, receive, finishTransaction'],[
 'Inventory has at most 8 ECUs, four service map sets and eight ranges each (00..E0). Mode02 support maps apply only to freeze frame00.',
 'HardwareSink observes IDs from positive, negative or pending payloads while discovery is active; only valid fully assembled positive map payloads enter maps.',
 'Receive requires current positive service, current range, exact map length and frame00 when applicable. Merge map bits using OR across retries.',
 'Repeat base range once to catch a missed responder. Higher range retries once only when a preceding continuation promised a map that did not arrive.',
 'Continue a range when any known ECU map sets its continuation bit, up to E0; then advance service. Eight-ECU overflow makes support filtering conservative.'
],[('a','start','Discovery.begin(false): reset scan state'),('b','process','Build mode/range request (02 adds frame00)'),('c','process','Observe eligible responder ID; add if space, otherwise overflow'),('d','decision','Positive payload matches exact current map layout?'),('e','process','Store/OR 32 support bits; mark range known'),('f','process','At transaction completion: inspect retry conditions'),('g','decision','First attempt and map may be missing?'),('h','process','Retry same request once'),('i','decision','Range < E0 and any continuation bit?'),('j','process','Advance range by 0x20'),('k','process','Reset range; advance next service'),('l','decision','All four services complete?'),('m','end','Discovery inactive; use collected evidence')],
 [('a','b',''),('b','c','On payload'),('c','d',''),('d','e','Yes'),('d','f','No'),('e','f',''),('f','g',''),('g','h','Yes'),('h','b',''),('g','i','No'),('i','j','Yes'),('j','b',''),('i','k','No'),('k','l',''),('l','b','No'),('l','m','Yes')])

chart('Supported jobs and expected responders', ['Discovery.h: support, requestSupport, expected','TeensyOBDSDLogger.ino: PID_FILTER'],[
 'support() returns SUPPORTED, UNSUPPORTED, UNKNOWN or NO_PID_BITMAP. A known terminal map rules out later ranges. PID00 is supported only when base range is known.',
 'Mode02 frame other than00 stays unknown. Modes03/07/0A have no PID bitmap and remain enabled; a negative response does not currently suppress repeated polling.',
 'Disable a configured job only when EVERY inventoried ECU explicitly marks it unsupported and there is no inventory overflow. Unknown/silent evidence is retained.',
 'expected() returns supported ECU IDs only when every inventoried ECU has a known applicable answer; otherwise zero keeps the full response collection window.',
 'Physical addressing always supplies one expected response ID. This is per-request expected-response checking, not per-ECU retry/backoff or automatic ECU recovery.'
],[('a','start','For each configured request'),('b','decision','No bitmap, unknown inventory, discovery active or overflow?'),('c','process','KEEP job conservatively'),('d','process','Evaluate map/terminal-range evidence per ECU'),('e','decision','Any ECU supports requested identifier?'),('f','decision','Every ECU explicitly unsupported?'),('g','process','SKIP job; log unsupported reason'),('h','process','Log per-job KEEP/SKIP and final counts'),('i','decision','At least one enabled job?'),('j','fault','Stop: no enabled requests'),('k','process','Begin schedule; set runStart and lastContact'),('l','call','Before each request: construct known expected responder list')],
 [('a','b',''),('b','c','Yes'),('b','d','No'),('d','e',''),('e','c','Yes'),('e','f','No'),('f','g','Yes'),('f','c','No: unknown'),('c','h',''),('g','h',''),('h','i','After all jobs'),('i','j','No'),('i','k','Yes'),('k','l','')])

chart('OBD polling priority and fair scheduling', ['TeensyOBDSDLogger.ino: running OBD branch','Schedule.h','RunControl.h: RequestSpacing'],[
 'Exactly one diagnostic transaction is active. Request submission gap is at least10 ms; this is not a guarantee that every configured job runs every10 ms.',
 'Priority when idle: connection probe, queued Mode04, then fair round-robin scheduled job. No retry queue accumulates missed polling periods.',
 'Schedule selects the first enabled due job from its cursor, tracks lateness/skipped periods, advances next by (skipped+1)*interval and moves cursor past the job.',
 'Two missed dedicated connection probes stop the whole OBD run. Normal individual request timeouts alone are not an immediate whole-run stop.',
 'User/duration stop stops scheduling and waits for the active exchange to finish (up to the diagnostic hard deadline). Serial prepare_update takes the immediate stopRun path instead.'
],[('a','start','OBD running'),('b','decision','missedProbes >= 2?'),('c','call','Stop: ECU disconnected'),('d','decision','User stop or configured duration reached?'),('e','process','If diagnostics idle stop; otherwise wait for current exchange'),('f','decision','Diagnostics idle, capture empty and spacing ready?'),('g','decision','probeNeeded?'),('h','process','Submit 01 00 connection probe'),('i','decision','Mode04 queued?'),('j','call','Build Mode04 confirmation inventory and submit clear'),('k','process','Round-robin find enabled due job; update schedule counters'),('l','process','Submit selected request; increment sent only on success'),('m','fault','Any submission failure: stop logging'),('n','end','Summary if due; common service; next loop')],
 [('a','b',''),('b','c','Yes'),('b','d','No'),('d','e','Yes'),('d','f','No'),('f','g','Yes'),('f','n','No'),('g','h','Yes'),('g','i','No'),('i','j','Yes'),('i','k','No'),('k','l','Due job'),('k','n','None due'),('h','n','Success'),('j','n','Success'),('l','n','Success'),('h','m','Failed'),('j','m','Failed'),('l','m','Failed'),('e','n',''),('c','n',''),('m','n','')])

chart('Start a request and submit hardware TX', ['TeensyOBDSDLogger.ino: startRequest, HardwareSink.send','DiagnosticCore.h: start, flow','DiagnosticCore.h: Address'],[
 'startRequest rejects CAN mode, invalid config and a submission gap below10 ms. It remembers trace request bytes/time and expected ECU IDs.',
 'DiagnosticCore.start rejects an active exchange, bad bytes/length/service or expected-count >8; resets all 8 reassembly states; physical RX overrides expected list.',
 'Build an 8-byte padded ISO-TP Single Frame: byte0=payload length, then service/parameters. Request active becomes true only after sink.send succeeds.',
 'Sink rejects CAN mode/not-ready/log fault/SD failure. It masks interrupts briefly, finds TX_INACTIVE mailbox8..15 and writes directly; no library software TX queue.',
 'Success records raw TX including flow controls. No available mailbox emits TX_FAILED. Functional flow IDs: 7E8->7E0, or 18DAF1xx->18DAxxF1. Physical addressing uses configured TX.'
],[('a','start','startRequest'),('b','decision','OBD/config/spacing gates pass?'),('c','process','Get expected ECU IDs; remember exchange trace'),('d','decision','DiagnosticCore.start guards pass?'),('e','process','Reset request state; build padded Single Frame'),('f','decision','Sink mode/ready/SD/fault gates pass?'),('g','process','Briefly mask IRQs; scan TX mailboxes8..15; submit'),('h','decision','Exactly one frame accepted?'),('i','process','Log TX; mark request active; update submission gap clock'),('j','end','Return success; await responses'),('x','fault','Return failure; caller stops when required')],
 [('a','b',''),('b','c','Yes'),('b','x','No'),('c','d',''),('d','e','Yes'),('d','x','No'),('e','f',''),('f','g','Yes'),('f','x','No'),('g','h',''),('h','i','Yes'),('h','x','No'),('i','j','')])

chart('ISO-TP: receive frame dispatch', ['DiagnosticCore.h: receive, matches, state, flow'],[
 'Ignore inactive requests, frames older than request start, rejected IDs/widths, DLC0 or DLC>8. Raw accepted OBD frames are exported separately, even when decoding ignores them.',
 'Single Frame: declared length must be1..7 and fit DLC. First Frame: DLC must be8 and total length8..4095. A payload must match service/PID/frame or negative service before allocation.',
 'One reassembly state per ECU, at most8; extra responders emit TOO_MANY_RESPONDERS. First Frame copies6 bytes, resets sequence to1 and block count0.',
 'Flow Control is 30 08 0A: clear-to-send, block size8, minimum separation10 ms. A failed control TX clears receiving/pending and emits FLOW_CONTROL_TX_FAILED.',
 'Incoming PCI3 or other types have no receive branch here. Repeated matching SF/FF can replace the same ECU state; this is not an overlapping-request matching engine.'
],[('a','start','DiagnosticCore.receive(frame, captured ms)'),('b','decision','Active, not older than start, accepted ID and valid DLC?'),('c','decision','PCI nibble?'),('d','decision','SF length valid and payload matches current request?'),('e','process','Update lastResponse; get ECU state; copy SF payload'),('f','call','Complete assembled payload'),('g','decision','FF DLC/total length valid and first bytes match?'),('h','process','Update lastResponse; state length/have=6; reset sequence/block'),('i','process','Copy first6 bytes; mark receiving; send Flow Control'),('j','call','PCI2: continue existing ECU assembly'),('k','end','Ignore/return; malformed recognized frames emit reason')],
 [('a','b',''),('b','c','Yes'),('b','k','No'),('c','d','0: Single'),('c','g','1: First'),('c','j','2: Consecutive'),('c','k','Other'),('d','e','Yes'),('d','k','No'),('e','f','State available'),('e','k','No free state'),('g','h','Yes'),('g','k','No'),('h','i','State available'),('h','k','No free state'),('i','k',''),('f','k',''),('j','k','')])

chart('ISO-TP: consecutive frames and payload completion', ['DiagnosticCore.h: receive PCI2, complete'],[
 'Consecutive frames require an existing receiving state for that ECU. Expected sequence wraps modulo16. Copy min(remaining,7), and require enough DLC bytes.',
 'Sequence/short-frame errors stop that ECU assembly and emit ISOTP_SEQUENCE_ERROR or SHORT_CONSECUTIVE_FRAME. An unrelated CF is ignored.',
 'If incomplete, send another Flow Control after8 CFs. Complete payloads are matched again before validation.',
 'Invalid length/value payloads are preserved in the trace but do not mark the ECU complete. Unreviewed definitions emit UNVERIFIED_DEFINITION and are still processed.',
 'Positive means complete. Negative NRC78 means pending; other valid negative replies count as completion, not as positive data. A fresh pending reply updates lastResponse through the SF/FF path.'
],[('a','start','Consecutive frame PCI2'),('b','decision','ECU assembly active?'),('c','decision','CF sequence/DLC valid?'),('d','process','Copy bytes; increment have; sequence modulo16; last=now'),('e','decision','have == total length?'),('f','process','If block count reaches8: reset count and send Flow Control'),('g','process','Complete: receiving=false; last=now; complete=false'),('h','decision','Request matches?'),('i','call','Validate data layout/value'),('j','decision','Invalid validation result?'),('k','process','Preserve invalid payload event; pending=false; not complete'),('l','decision','Negative payload 7F?'),('m','process','NRC78: pending=true; other NRC: complete=true; emit event'),('n','process','Positive: pending=false, complete=true; emit POSITIVE'),('x','end','Return to active request collector')],
 [('a','b',''),('b','c','Yes'),('b','x','No'),('c','d','Yes'),('c','x','No: emit error'),('d','e',''),('e','g','Yes'),('e','f','No'),('f','x',''),('g','h',''),('h','i','Yes'),('h','x','No: UNMATCHED_PAYLOAD'),('i','j',''),('j','k','Yes'),('j','l','No'),('k','x',''),('l','m','Yes'),('l','n','No'),('m','x',''),('n','x','')])

chart('Data validation: reviewed layouts and VIN', ['J1979Validation.h: validateData, reviewedPidLength, hasReviewedLayout'],[
 'This is a partial OCT2011 layout checker, not a complete J1979-DA catalogue, semantic decoder or certification proof.',
 'Reviewed data byte counts: PID01=4; 02/0C/10/1F/21/23/42=2; 04/05/0B/0D/0F/11/1C/2F/33/49=1; 88=13. Continuation-map PIDs00..E0 in steps20 use4.',
 'Offsets exclude response service/PID and, for Mode02, freeze-frame identifier. Support PID E0 final bit is reserved and must be0.',
 'Mode09 PID02 VIN: total20 bytes, record count1, exactly17 characters. Allowed digits and uppercase A-Z except I/O/Q.',
 'Unknown reviewed-layout coverage returns Unverified, not WrongLength. Readiness reserved-bit checks are a separate decoder warning.'
],[('a','start','Fully assembled matched payload'),('b','decision','Null or empty?'),('c','decision','Service7F or44?'),('d','process','Require length3 for negative; length1 for clear acknowledgement'),('e','decision','Service41/42/46/49 with reviewed identifier?'),('f','end','Return Unverified'),('g','decision','Support-map identifier?'),('h','process','Require offset+4; reject E0 reserved continuation bit'),('i','decision','Service41 or42?'),('j','process','Require offset + reviewed PID data length'),('k','decision','49 02 VIN?'),('l','process','Require20 bytes, count1, valid17-character VIN'),('m','end','Return LayoutChecked or explicit WrongLength / InvalidValue'),('x','fault','WrongLength')],
 [('a','b',''),('b','x','Yes'),('b','c','No'),('c','d','Yes'),('c','e','No'),('d','m',''),('e','f','No'),('e','g','Yes'),('g','h','Yes'),('g','i','No'),('h','m',''),('i','j','Yes'),('i','k','No'),('j','m',''),('k','l','Yes'),('k','f','No'),('l','m','')])

chart('Response windows, pending and transaction end', ['DiagnosticCore.h: tick, endRequest, allExpectedComplete','TeensyOBDSDLogger.ino: transactionEnded'],[
 'The main loop invokes tick only when captured records in BOTH queues are drained, so queued responses are processed before timeout decisions.',
 'For each used ECU: receiving with no CF for >=1000 ms emits ISOTP_TIMEOUT and clears receiving. pending remains a separate waiting condition.',
 'Priority: all expected complete with no wait -> normal end; otherwise >=10000 ms since start -> hard deadline; otherwise quiet initial window and no waiting -> timeout.',
 'Initial quiet window defaults70 ms and is measured from lastResponse, not always request start. Config accepts10..10000. No fixed sleep is inserted after every response.',
 'Timeout emits NO_RESPONSE_TIMEOUT only when no payload matched at all, and EXPECTED_RESPONSE_MISSING for each expected ECU not validly complete. REQUEST_END always follows. Invalid matching data sets matched=true but not complete.'
],[('a','start','tick(now), active request only'),('b','process','Expire receiving states with >=1000 ms inactivity'),('c','process','waiting = any receiving or pending state'),('d','decision','Every expected ECU complete and not waiting?'),('e','process','endRequest(false)'),('f','decision','Elapsed from start >=10000 ms?'),('g','process','Emit RESPONSE_DEADLINE for still pending/receiving ECUs'),('h','decision','Quiet >=responseWindow and not waiting?'),('i','process','endRequest(true): missing/no-response events'),('j','process','active=false; emit REQUEST_END'),('k','call','Sink marks transactionEnded; main checks pending hardware TX'),('l','end','Otherwise remain active; next loop')],
 [('a','b',''),('b','c',''),('c','d',''),('d','e','Yes'),('d','f','No'),('e','j',''),('f','g','Yes'),('f','h','No'),('g','i',''),('h','i','Yes'),('h','l','No'),('i','j',''),('j','k',''),('k','l','')])

chart('Diagnostic events, traces and connection probes', ['TeensyOBDSDLogger.ino: HardwareSink.event, traceExchangeRequest, nrcName'],[
 'Any event with payload bytes refreshes lastContact, clears missedProbes/probeNeeded and refreshes OLED reply activity, including invalid payload events.',
 'On NO_RESPONSE_TIMEOUT, increment missedProbes only for a dedicated connectionProbe. If no contact for >=5 s, set probeNeeded. REQUEST_END clears connectionProbe.',
 'Payload events produce request context, assembled TX and assembled RX traces. Invalid data is labelled. Matching negative payloads print NRC names; unknown NRC keeps raw value.',
 'During discovery, positive/negative/pending events observe ECU IDs; only positive map responses store validated bitmaps. Inventory overflow is reported.',
 'Only POSITIVE invokes active readiness tracing. REQUEST_END/CANCELLED sets transactionEnded without resetting CAN. responseSeen is set for positive/negative/pending payload events but has no scheduling decision of its own.'
],[('a','start','Sink receives diagnostic event'),('b','decision','Payload bytes present?'),('c','process','Refresh contact/activity and clear probe misses'),('d','process','Timeout/end events update probe state'),('e','call','If Mode04 feedback active: observe or finalize clear result'),('f','decision','Payload event?'),('g','process','Trace request context + complete RX payload; flag invalid layout'),('h','process','No payload: trace timeout/end context and route event/status'),('i','process','During discovery observe eligible ID; on positive store valid map'),('j','process','If negative: print NRC; if positive: trace readiness when applicable'),('k','process','End/cancel: mark transactionEnded'),('l','end','Return; main loop chooses probe or next job')],
 [('a','b',''),('b','c','Yes'),('b','d','No'),('c','d',''),('d','e',''),('e','f',''),('f','g','Yes'),('f','h','No'),('g','i',''),('h','i',''),('i','j',''),('j','k',''),('k','l','')])

chart('Mode04: debounce, queue and confirmation', ['RunControl.h: Mode4Button','Mode4Feedback.h','TeensyOBDSDLogger.ino: pollStop, Mode04 branch'],[
 'GPIO40 is active-low. Both edges debounce for30 ms. A button held at boot must first be released to arm. A continuous armed press of2000 ms triggers once per press.',
 'Only valid OBD mode polls this button. Queue only while not finished/closing/stopping, not already queued, and feedback phase is Idle. CAN mode never requests Mode04.',
 'Queued clear waits through startup/discovery and active transaction; connection probe has priority. No JSON Mode04 request is accepted.',
 'Expected confirmation targets: physical RX or all discovered ECU IDs. Unknown/empty/overflow inventory cannot yield success. Diagnostic collection still uses its normal window.',
 'All expected ECU targets must return positive one-byte44; negative, malformed, cancelled, or missing replies yield failure. RESPONSE_PENDING alone is not a failure. Success holds3 s; failure900 ms, then returns Idle.'
],[('a','start','pollStop in configured OBD mode'),('b','process','Debounce pin; release arms; track held duration'),('c','decision','Armed held >=2000 ms and not fired?'),('d','decision','Queue/recording/feedback gates permit?'),('e','process','Set mode4Requested; feedback Waiting; log MODE04_QUEUED'),('f','process','Otherwise log ignored; no transmission'),('g','call','At next permitted slot after probes: construct target inventory'),('h','process','Begin feedback; submit04; failure stops run'),('i','process','Observe responses and per-ECU feedback'),('j','decision','At request end: all targets replied44 and no failure?'),('k','process','MODE04_CONFIRMED; Success for3000 ms'),('l','process','MODE04_NOT_CONFIRMED; Failure for900 ms'),('m','end','Return to loop; expired feedback becomes Idle; release arms next press')],
 [('a','b',''),('b','c',''),('c','d','Yes'),('c','m','No event'),('d','e','Yes'),('d','f','No'),('e','g','When eligible'),('f','m',''),('g','h',''),('h','i','Submitted'),('i','j',''),('j','k','Yes'),('j','l','No'),('k','m',''),('l','m','')])

chart('Text routing, console and SD batching', ['TeensyOBDSDLogger.ino: obdWrite, recordAt, pumpLog, writeFileBatch'],[
 'Before PSRAM migration:64 emergency text slots. Normal text queue:4096 lines of320 bytes. obdWrite uses bounded snprintf; queue-full increments logDrops and sets loggingFault.',
 'recordAt sends normal scheduler/debug notes to USB only. During closing or for ERROR/FAILED/OVERFLOW/OVERRUN/BUS_OFF/MODE04_/PID_FILTER kinds it queues the sole SD log.',
 'Raw TX/RX, assembled exchange traces, readiness output and obdStatus use the log queue directly; they are not all subject to recordAt filtering.',
 'Console queue:32768 bytes, best effort. Disconnected Serial clears it; pump sends only available contiguous bytes. Console drops do not mean SD drops.',
 'Text batches use8192 bytes, writing after >=4096 bytes or before a line would overflow the batch. Record lines are mirrored to console as pumped. SD short writes set failure and count unwritten bytes; transfer staging before/after writes.'
],[('a','start','Produce event, frame or decoded text'),('b','decision','recordAt or direct log output?'),('c','decision','Closing or persistent event kind?'),('d','process','Console-only note; best-effort queue'),('e','process','obdWrite: queue bounded text or flag log overflow'),('f','process','pumpLog up to128 lines per loop'),('g','decision','Next line would exceed8192 batch bytes?'),('h','call','Rotate if needed, then write current batch'),('i','process','Append line; mirror to USB; dequeue'),('j','decision','Batch >=4096 bytes?'),('k','call','Write batch; update usage/duration/error counters'),('l','end','Return; periodic/final flush writes remaining partial batch')],
 [('a','b',''),('b','c','recordAt'),('b','e','Direct'),('c','e','Yes'),('c','d','No'),('e','f',''),('f','g',''),('g','h','Yes'),('g','i','No'),('h','i',''),('i','j',''),('j','k','Yes'),('j','l','No'),('k','l',''),('d','l','')])

chart('File selection, header and 250 MB rotation', ['StoragePaths.h: formatLogPath','LogRotation.h','CanRecording.h: formatCanAscHeader','TeensyOBDSDLogger.ino: openLogSegment, prepareLogBatch, writeCanFooter'],[
 'File index scans1..999999 and skips existing paths. Names: /LOGS/01_OBD_<number>.TXT or /LOGS/01_CAN_<number>.ASC. At least2 index digits; independent mode paths preserve existing files.',
 'OBD header: SkiDrive title, actual firmware, ID, path, config fingerprint, bitrate/address/request count, power-on time base. CAN header has comment metadata and Begin Triggerblock.',
 'Limit is250000000 decimal bytes including header. CAN reserves End Triggerblock bytes before batch admission. Rotate BEFORE current+pending+footer would exceed cap; equality is allowed.',
 'Rotation writes CAN footer once, checks flush errors, closes, opens next unused segment and repeats header. Capture stays enabled; CAN elapsed origin is not reset.',
 'Footer/flush/open/header failure marks SD/logging failure. Runtime uses no synchronized RTC/calendar date and does not silently delete old recordings.'
],[('a','start','Write batch with pending bytes'),('b','decision','SD good, pending>0 and cap would be crossed?'),('c','process','No rotation: keep current segment'),('d','process','Write CAN footer once if needed'),('e','decision','Footer and flush successful?'),('f','process','Close current; scan next unused indexed path'),('g','decision','Open valid path and write header succeed?'),('h','process','Announce segment; retain receive queues and time origin'),('i','process','Write batch; update SD usage'),('j','fault','Count failure; SD bad; loggingFault=true'),('k','end','Return to common loop/stop handling')],
 [('a','b',''),('b','c','No'),('b','d','Yes'),('c','i',''),('d','e',''),('e','f','Yes'),('e','j','No'),('f','g',''),('g','h','Yes'),('g','j','No'),('h','i',''),('i','k',''),('j','k','')])

chart('Stop boundary, draining and SD handover', ['TeensyOBDSDLogger.ino: stopRun, loop closing tail, exposeSD'],[
 'stopRun is idempotent when closing/finished. Set reason; finalize OBD skipped-period counts, bounded by a configured duration when applicable.',
 'Cancel queued clear and diagnostic transaction; running/ready=false. Atomically disable capture and snapshot FIFO pending. Disable FIFO IRQ and switch CAN to listen-only.',
 'Persist boundary and stop reason. CAN sets closing before notes so they appear in ASC; OBD stop reason also uses direct obdStatus. Unread FIFO is outside the drained software queue.',
 'Continue processing BOTH receive queues. When empty, emit summary + CAPTURE_DRAINED. This marker precedes final text write/flush; alone it is not proof of successful final media persistence.',
 'After text queue empties: write partial batch, footer, flush, check errors and close. completedClean requires valid config, good SD, no logging fault and no final write error. Even failed runs expose mounted SD after close; update state stays failed.'
],[('a','start','stopRun(reason)'),('s','start','Startup failed: setup already set closing'),('b','decision','Already closing/finished?'),('c','end','No-op for repeated stopRun'),('d','process','Finalize schedule; cancel clear/exchange; disable run/TX readiness'),('e','process','Disable capture atomically; snapshot FIFO; disable IRQ; listen-only'),('f','process','Set closing and persist stop/boundary notes'),('g','call','Main loop drains staging + PSRAM, then final health summary'),('h','decision','Final summary done and text queue empty?'),('i','process','Write partial batch/footer; flush/check; close file'),('j','process','Set completedClean; sdGood=false; finished=true'),('k','process','If mounted and not exposed: MTP.addFilesystem and reset'),('l','end','LOG_CLOSED or RUN_FAILED; USB/MTP remains; no auto-restart')],
 [('a','b','Normal stopRun'),('s','g','No capture started'),('b','c','Yes'),('b','d','No'),('d','e',''),('e','f',''),('f','g',''),('g','h',''),('h','g','No: next loop'),('h','i','Yes'),('i','j',''),('j','k',''),('k','l','')])

chart('USB serial commands and update boundary', ['TeensyOBDSDLogger.ino: pollStop, updateStatus','usb_identity.c','docs/firmware-update-usb.md'],[
 'Read at most32 input bytes per main loop. Command buffer accepts31 characters. CR or LF terminates; an overlong command is discarded until terminator. Unknown/blank commands do nothing.',
 'stop/close set stopRequested. prepare_update sets it and invokes immediate normal stopRun if not already closing/finished, then replies with current state.',
 'Reply begins @UPDATE and includes protocol, product/display name, legacy device_type, stable ID, firmware, hardware unassigned, operating_mode, state, SD/OLED flags, faults/drop counters, ESR, reason and flash_enabled=false.',
 'State: not_prepared until closing; preparing while closing; finished -> ready only if completedClean, otherwise failed. A mounted SD is not a live card-presence probe; OLED ready is communication, not visual inspection.',
 'The app verifies a firmware package externally. Teensy core134-baud bootloader entry and external CLI flashing are outside this command handler. No in-application flash writer, signed-image rejection or dual-bank rollback is implemented here.'
],[('a','start','pollStop serial input'),('b','process','Read bounded bytes; assemble case-sensitive line'),('c','decision','Line terminated without overflow?'),('d','decision','Command?'),('e','process','stop / close: set stopRequested'),('f','process','device_info / update_status: return @UPDATE JSON'),('g','process','prepare_update: set stop; stopRun if eligible; return state'),('h','process','Unknown or overlong line: ignore; reset parser state'),('i','call','External updater waits for safe closure / explicit recovery process'),('j','process','Core bootloader entry; external loader installs image'),('k','end','After reboot app rediscovers same permanent ID and expected version'),('q','end','Return to serial/main-loop service')],
 [('a','b',''),('b','c',''),('c','d','Yes'),('c','h','Overflow/unknown'),('d','e','Stop'),('d','f','Query'),('d','g','Prepare'),('d','h','Other'),('e','i','If update follows close'),('g','i','After state/closure checks'),('f','q','Query only'),('h','q','No action'),('i','j','External boundary'),('j','k','')])

chart('OLED status selection and precedence', ['TeensyOBDSDLogger.ino: updateDisplay','Mode4Feedback.h: update','CanRecording.h: canTrafficRecent'],[
 'Mode04 feedback timers update even if OLED is unavailable. OLED failure never stops logging by itself.',
 'Recent CAN activity is <=3000 ms since last callback; recent OBD payload activity is <5000 ms. OBD lost also means two missed probes or ECU-disconnected stop reason.',
 'flowing = recent reply AND not closing/finished/fault. Centre badge is ERROR for fault, otherwise LIVE if flowing, else WAIT.',
 'Status precedence, in code order: running/default; OBD button/feedback/link; CAN starting/no-traffic; closing; finished; fault; configurationRejected last.',
 'No SD -> NO SD CARD on fault; other fault -> LOG ERROR; invalid JSON -> CONFIG ERROR. Normal mounted file access uses SD ACCESS, unless link-lost/fault status overrides it.'
],[('a','start','Each updateDisplay call'),('b','process','Advance Mode04 feedback timers'),('c','decision','oledReady?'),('d','end','Return without affecting logging'),('e','process','If previous frame sending: send one16-byte chunk then return'),('f','decision','New frame due (first or >=100 ms)?'),('g','process','Compute recent/lost/fault/flowing flags'),('h','process','Choose LIVE/WAIT/ERROR badge'),('i','process','Apply ordered main-status precedence'),('j','call','Draw startup or steady layout, then send display commands')],
 [('a','b',''),('b','c',''),('c','d','No'),('c','e','Yes: frame pending'),('c','f','Yes: no pending frame'),('f','d','No'),('f','g','Yes'),('g','h',''),('h','i',''),('i','j','')])

chart('OLED frame, equal gaps and I2C isolation', ['TeensyOBDSDLogger.ino: updateDisplay, drawDisplayBadge, finishDisplayTransfer'],[
 'First frame after setup starts a1500 ms non-blocking splash: SkiDrive, then ID:<numeric ID>. Fault overrides splash on next refresh; no extra acquisition delay is inserted.',
 'Header: mode x0 width25; storage x85 width43; activity width34 at x38 for ERROR (13-pixel gaps), width28 at x41 for LIVE/WAIT (16-pixel gaps). Existing6x8 cells remain.',
 'Main text at y12: scale2 if <=10 chars, else1. Bottom track y30, pulse y29..31. Pulse advances4 pixels per redraw only while flowing; holds last position on lost/stop.',
 'SD percentage is estimated used capacity: startup scan plus successful logger writes, clamped100. Missing card shows SD:NO; unknown total SD:--. Host file edits need restart to refresh snapshot.',
 'Keep framebuffer unchanged until all512 bytes transfer. Set horizontal-address window, then transfer16 bytes per subsequent loop call. I2C failure disables display until restart and reports OLED_IO_FAILED, while logging continues.'
],[('a','start','Ready to draw new frame'),('b','process','Clear buffer; set font/color; estimate SD used label'),('c','decision','Within1500 ms of first draw and no fault?'),('d','process','Draw SkiDrive and numeric ID'),('e','process','Draw OBD/CAN, activity and SD with equal badge gaps'),('f','process','Draw main status and dotted track; move/freeze pulse'),('g','process','Set SSD1306 full-window addressing'),('h','decision','I2C command transaction succeeds?'),('i','process','Set sent=0; on following calls transmit16-byte chunks'),('j','decision','Chunk succeeds?'),('k','process','Advance sent until512; allow next redraw'),('x','fault','Disable OLED; console/error status only; recording continues')],
 [('a','b',''),('b','c',''),('c','d','Yes'),('c','e','No'),('e','f',''),('d','g',''),('f','g',''),('g','h',''),('h','i','Yes'),('h','x','No'),('i','j',''),('j','k','Yes'),('j','x','No'),('k','i','More bytes'),('k','a','Frame done and refresh due')])

chart('Active readiness decoding and raw trace helpers', ['Readiness.h','TraceFormat.h','TeensyOBDSDLogger.ino: HardwareSink.event'],[
 'Readiness tracing is called only for POSITIVE events and accepts exactly six bytes:41 01 A B C D. PID41 has different semantics and is not passed through this decoder.',
 'A: MIL command bit7 and confirmed emissions DTC count bits0..6. B bit3 selects compression vs spark monitor names. B bits0..2 are common support; bits4..6 are incomplete flags.',
 'C contains monitor support, D contains incomplete flags. Apply ignition-specific reserved masks (compression0x14; spark0x10); unsupported monitors print N/A. Reserved bits warn without discarding raw data.',
 'Readiness is completion since DTC clearing, not a pass result. No automatic ignition type is guessed from vehicle name.',
 'traceCan emits raw ID/type/DLC/time, and RTR without bytes. tracePayload emits a single full payload line in bounded fragments, supporting up to4095 bytes. traceRequest labels mode/PID/MID/freeze frame.'
],[('a','start','Positive payload event'),('b','decision','Exactly41 01 plus4 bytes?'),('c','end','No readiness output; preserve existing raw trace'),('d','process','Extract A/B/C/D; MIL and DTC count'),('e','process','Common support + completion flags'),('f','decision','Compression ignition bit set?'),('g','process','Use diesel/compression monitor names and reserved mask'),('h','process','Use spark monitor names and reserved mask'),('i','process','For valid monitor bits: print N/A, COMPLETE or INCOMPLETE'),('j','decision','Reserved bits set?'),('k','process','Print READINESS_WARNING; retain raw response'),('l','end','Print meaning note; return to event handler')],
 [('a','b',''),('b','c','No'),('b','d','Yes'),('d','e',''),('e','f',''),('f','g','Yes'),('f','h','No'),('g','i',''),('h','i',''),('i','j',''),('j','k','Yes'),('j','l','No'),('k','l','')])

chart('MTP exposure, enumeration and hidden objects', ['TeensyOBDSDLogger.ino: exposeSD','src/mtp/VisibleObjects.h','src/mtp/OBD_MTP.cpp: begin, loop'],[
 'MTP.begin starts a50,000 us IntervalTimer fallback while setup has not yet called MTP.loop. It serves safe identity/session/storage metadata and returns DeviceBusy for other timer-time commands.',
 'First main MTP.loop cancels that fallback. While recording, SD is not registered as an exposed filesystem; protocol metadata can still be served.',
 'Only final close exposes mounted SD with addFilesystem and reset, once. SD access includes normal host file mutation commands; this is not a read-only vault.',
 'VisibleObjects filters GetNumObjects and GetObjectHandles, walking ancestors up to256 levels. Hide System Volume Information, mtpindex.dat and descendants; reject empty/invalid/self-parent handles.',
 'This is enumeration filtering, not an authentication/security barrier. USB/MTP friendly name SkiDrive_<ID>; model SkiDrive OBD/CAN Logger; manufacturer Skillicon Technologies.'
],[('a','start','MTP.begin during setup'),('b','process','Start20 Hz metadata-only fallback timer'),('c','process','First main MTP.loop stops fallback timer'),('d','decision','Recording file safely closed and SD mounted?'),('e','process','Do not expose SD filesystem while recording'),('f','process','exposeSD once: add SD card + reset host view'),('g','process','Host requests count/handles: inspect each object/ancestor'),('h','decision','Valid named ancestry, no hidden system item?'),('i','process','Include object in count/handle stream'),('j','process','Omit hidden/invalid object'),('k','call','Host can use standard file/property operations'),('l','end','Continue MTP service; no automatic recording restart')],
 [('a','b',''),('b','c',''),('c','d',''),('d','e','No'),('d','f','Yes'),('e','l',''),('f','g',''),('g','h',''),('h','i','Yes'),('h','j','No'),('i','k',''),('j','k',''),('k','l','')])

chart('Bundled MTP command dispatcher', ['src/mtp/OBD_MTP.cpp: MTP_class::loop, processIntervalTimer'],[
 'Receive command container length12..32; preserve transaction ID. Wrong container type yields operation-not-supported. Bad length frees packet without a full application-defined recovery protocol.',
 'Identity/session:1001 GetDeviceInfo,1002 OpenSession,1003 CloseSession. Storage:1004 IDs,1005 info,1006 count,1007 handles.',
 'Files:1008 info,1009 download,100B delete,100C upload metadata,100D upload data,100F format,1019 move,101A copy,101B partial download. Reset1010 and unsupported ops return2005.',
 'Properties:1014 device descriptor,1015 device value,9801 supported object props,9802 prop descriptor,9803 prop value,9804 set prop. Host failure codes propagate; format success requests DeviceReset event.',
 'Reply reuses transaction ID and encodes response parameters from return_code high bits. Only send if USB MTP status is ready. Restore protocol status and call storage_.loop afterwards.'
],[('a','start','MTP.loop polling'),('b','decision','Bulk packet available?'),('c','decision','Length12..32 and command container?'),('d','process','Copy container; free RX; keep transaction ID'),('e','decision','Operation group?'),('f','process','Identity/session/storage metadata'),('g','call','File upload/download/delete/format/move/copy'),('h','process','Read/write device or object properties'),('i','process','Unknown/reset: operation not supported'),('j','process','Build response and optional parameters; send if USB ready'),('k','process','After successful format: device reset event'),('l','process','Normalize USB status; storage_.loop'),('m','end','Return to application main loop'),('x','process','Malformed packet: free RX / return appropriate unsupported code')],
 [('a','b',''),('b','c','Yes'),('b','l','No'),('c','d','Yes'),('c','x','No'),('d','e',''),('e','f','Metadata'),('e','g','File'),('e','h','Property'),('e','i','Other'),('f','j',''),('g','j',''),('h','j',''),('i','j',''),('j','k',''),('k','l',''),('x','l',''),('l','m','')])

chart('MTP file/data transport and error boundaries', ['src/mtp/OBD_MTP.cpp: SendObjectInfo, SendObject, GetObject, GetPartialObject','src/mtp/OBD_MTP.cpp: read/write/string/bulk/event helpers'],[
 'SendObjectInfo reads metadata, resolves store/parent, UTF strings and timestamps, then creates a file/directory handle. SendObject streams host data to storage; the format and SD engine are core dependencies.',
 'Downloads look up object/store, send metadata/data phase, then read/write bounded buffers. Partial download honors requested offset/count. Deletes and copies propagate storage errors; format can fail/cancel.',
 'readDataPhaseHeader checks container framing; read/readstring/readDateTimeString handle byte streams, UTF-16/UTF-8 and optional dates. write/writestring/write_finish assemble USB packets and terminate phases.',
 'Active Teensy4 USB branch uses receive/transmit buffers, endpoint packet sizes and cache/DMA synchronization. Waits/timeouts/USB disconnects can abort transfers. Teensy3 conditional alternatives are present but not active on this hardware.',
 'Event functions package notifications with event code/transaction parameters. No firmware HEX upload/install command exists in the MTP dispatcher. Putting a HEX on SD does not program flash.'
],[('a','start','Host requests file operation'),('b','decision','Upload or download/mutation?'),('c','process','Upload metadata: parse store/parent/name/time; create object handle'),('d','process','Receive data phase in bounded chunks; write storage'),('e','process','Download/info/partial read: locate object; apply offset/count'),('f','process','Send data header and file chunks through USB bulk'),('g','decision','Frame, USB and storage operations succeed?'),('h','process','Finish data/response phase; update object information'),('i','process','Return operation/storage/partial-deletion error'),('j','process','Delete/format/move/copy through storage engine; map result'),('k','end','Return response; service notifications and storage loop')],
 [('a','b',''),('b','c','Upload'),('c','d',''),('d','g',''),('b','e','Read'),('e','f',''),('f','g',''),('b','j','Mutation'),('j','g',''),('g','h','Yes'),('g','i','No'),('h','k',''),('i','k','')])

chart('Fault gates and health counters', ['TeensyOBDSDLogger.ino: loop health gates, summary','TeensyOBDSDLogger.ino: HardwareSink and write paths'],[
 'Immediate run-fault sources include hardware overflow, staging/capture loss, text queue overflow, SD short/flush/footer/open errors, malformed CAN ASC format and bus-off.',
 'Diagnostic submission failure or any pending hardware TX at transaction end stops the run. A Mode04 negative reply is a clear failure, not by itself a storage fault.',
 'Config, timer and PSRAM failures prevent startup. OLED communication failure disables OLED only; console drops do not stop SD logging. Negative/invalid/unverified diagnostic replies stay visible without globally treating all as fatal.',
 'summary reports STATUS, CAPTURE_STATUS, RX_STAGING, SD_STATUS, CAN_TIMING, CAPTURE_TIMING, first overrun details and every configured job schedule. Periodic summary may route mainly to console; closing preserves all summary kinds.',
 'Zero software drops do not prove no wire loss. GPT2-vs-DWT disagreement is diagnostic, not itself a stop. No watchdog, battery power-fail save, auto-reset recovery or full certification claim is implemented.'
],[('a','start','Health check in main loop'),('b','process','Drain up to8 CAN error snapshots; record real error bits/counters'),('c','decision','Hardware overrun or bus-off?'),('d','process','Set loggingFault; preserve first overrun details'),('e','decision','loggingFault or SD not good?'),('f','call','stopRun: logging/CAN failure'),('g','process','Continue mode control; normal/invalid/negative replies are not all fatal'),('h','process','If due/running or final: summary counters and per-job stats'),('i','end','Common log/USB/display service')],
 [('a','b',''),('b','c',''),('c','d','Yes'),('c','e','No'),('d','e',''),('e','f','Yes'),('e','g','No'),('f','h',''),('g','h',''),('h','i','')])

chart('Present but inactive helper code', ['LogReport.h: date/report helpers','RunControl.h: StopButton','Mode4Feedback.h: yellow, red'],[
 'These helper implementations are in source, but the current main sketch does not call them. Showing them separately avoids inventing live features.',
 'LogReport date helpers parse/format civil time2000..2099, validate leap days and build calendar headers. Current runtime does not call these helpers or synchronize a calendar clock. Live labels come from ProductIdentity.h.',
 'reportRequest/reportPositive can label support maps, selected physical-unit values, VIN/ECU ASCII and counted DTC entries. They are not the active general PID-value decoder in dev9; active payload traces remain raw plus the readiness decoder.',
 'StopButton supports a generic held-stop input but no SD-access push button is wired into the sketch. Mode4Feedback.yellow/red blink-pattern helpers are likewise not called; display phase feedback is active.',
 'Unused constants/state such as MAX_LIVE_PIDS sizing and responseSeen do not imply a new feature. Compile-time Teensy3 MTP alternatives are inactive; Teensy4 core transport is the active dependency.'
],[('a','start','Helper exists in code'),('b','decision','Called from current runtime path?'),('c','process','Use active chart for that function'),('d','decision','Inactive helper family?'),('e','process','Calendar parse/format and generic header utilities'),('f','process','Generic request/positive report and selected value/DTC formatting'),('g','process','Generic stop button and old LED feedback patterns'),('h','end','Document as present but inactive; no live product claim')],
 [('a','b',''),('b','c','Yes'),('b','d','No'),('d','e','Date'),('d','f','Report'),('d','g','Control/LED'),('e','h',''),('f','h',''),('g','h','')])

def export():
    OUT.mkdir(parents=True,exist_ok=True)
    version=re.search(r'FIRMWARE_VERSION\[\]="([^"]+)"',(SRC/'TeensyOBDSDLogger.ino').read_text()).group(1)
    if version!='1.2.0-dev9-oled-gaps':raise RuntimeError('Code changed: review chart specification before regenerating')
    files=[]
    for f in sorted(SRC.rglob('*')):
        if f.is_file():files.append(dict(file=f.relative_to(ROOT).as_posix(),sha256=hashlib.sha256(f.read_bytes()).hexdigest(),lines=len(f.read_text(errors='replace').splitlines())))
    spec=dict(title='SkiDrive firmware code flowcharts',version=version,source_commit='125bcc7',pages=PAGES,source_files=files)
    for page in PAGES:
        ids={n['id'] for n in page['nodes']}
        assert len(ids)==len(page['nodes']),page['title']
        assert all(e['source'] in ids and e['target'] in ids for e in page['edges']),page['title']
    modules={
      'TeensyOBDSDLogger.ino':'01,03,06-11,13-15,20-27,32',
      'DiagnosticCore.h':'05,15-20','Config.h':'04-05','Discovery.h':'11-13',
      'Schedule.h':'14,24','RunControl.h':'10,14,21,33','Mode4Feedback.h':'20-21,26,33',
      'CanRecording.h':'02,09-10,23','CaptureTimer.h':'02,08','CaptureRing.h':'09',
      'ReceivePipeline.h':'06,09,24','LogRotation.h':'23','StoragePaths.h':'03,23',
      'J1979Validation.h':'18','Readiness.h':'28','TraceFormat.h':'20,22,28',
      'LogReport.h':'33','ProductIdentity.h':'02,23,25,27','usb_identity.c':'02,25,29',
      'VisibleObjects.h':'29','OBD_MTP.cpp':'29-31','PROVENANCE.md':'29-31'}
    inventory=['# Source-to-flowchart coverage','',f'Firmware: {version}; source reference commit: 125bcc7.','',
      'Page IDs are the numbered flowcharts, before any source-index appendix.',
      'Project runtime modules are covered. Bundled MTP branches are grouped by operation/transport; external Arduino, FlexCAN, SD/FAT, USB and graphics library internals are boundaries.',
      'This is a code-reviewed behavioural atlas, not a proof that every machine instruction or dependency branch is modelled. Tests/package tools, old OBD-only code and planned hardware are outside this runtime atlas.','']
    function_re=re.compile(r'(?m)^[ \t]*(?:[A-Za-z_][\w:<>,& \t]*[ \t]+)?[*& \t]*([A-Za-z_~][\w:]*)\s*\([^;{}#]*\)\s*(?:const\s*)?(?:override\s*)?(?::[^{}\n]*)?\{')
    for file in files:
        name=pathlib.Path(file['file']).name
        if name not in modules:raise RuntimeError('No coverage mapping: '+name)
        file['charts']=modules[name]
        inventory += [f"## {file['file']}",f"Charts: {modules[name]}. Lines: {file['lines']}. SHA-256: `{file['sha256']}`.",'']
        funcs=[]
        f=ROOT/file['file']
        for m in function_re.finditer(f.read_text(errors='replace')):
            if m[1] not in {'if','for','while','switch','catch','defined','ATOMIC_BLOCK'}:
                funcs.append(dict(name=m[1],line=f.read_text(errors='replace').count('\n',0,m.start())+1))
        if name=='TeensyOBDSDLogger.ino':
            for method in ['read','flush']:
                m=re.search(r'\b'+method+r'\(\) override',f.read_text())
                funcs.append(dict(name=method,line=f.read_text().count('\n',0,m.start())+1))
            funcs.sort(key=lambda x:x['line'])
        file['function_index']=funcs
        inventory += [f"- `{x['name']}` — line {x['line']}" for x in funcs]
        inventory += ['']
    (OUT/'source-coverage.md').write_text('\n'.join(inventory).rstrip()+'\n',encoding='utf-8')
    (OUT/'flowcharts.source.json').write_text(json.dumps(spec,indent=2)+'\n',encoding='utf-8')
    print(f'{len(PAGES)} charts; {len(files)} source files fingerprinted')

if __name__=='__main__':export()
