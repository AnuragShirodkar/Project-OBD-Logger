# SkiDrive firmware code flowcharts

**Firmware:** 1.2.0-dev9-oled-gaps. **Code reference:** commit 125bcc7.
Prepared 27 November 2026 from the current separate OBD/CAN project's source.
No firmware or SD configuration was changed to create these documents.

## Open and edit

- [Editable Canva design](https://www.canva.com/design/DAHcFhnyB0E/edit)
- [PDF review copy](SkiDrive_Code_Flowcharts.pdf)
- [Editable source presentation](SkiDrive_Code_Flowcharts.pptx)
- [Source-file/function navigation and fingerprints](source-coverage.md)
- [Content verification results](verification.json)

Canva contains **33 pages**. Text, boxes, decisions and routed lines are separate
editable elements, not a single image per page. Imported routed lines may need
manual adjustment if boxes are moved. Canva edits do not automatically update
the repository files. The original source is the reviewed
[flowchart specification](flowcharts.source.json).

## Page index

1. Firmware at a glance
2. Identity and independent timer
3. Setup: interfaces, storage and config selection
4. JSON: version, mode and common options
5. JSON: OBD addresses and request jobs
6. Setup: memory, log and CAN enable gates
7. Main loop: exact service order
8. CAN interrupt and receive callback
9. Receive queues, processing and raw export
10. Passive CAN recording control
11. OBD startup and discovery sequencing
12. Discovery maps, retries and range continuation
13. Supported jobs and expected responders
14. OBD polling priority and fair scheduling
15. Start a request and submit hardware TX
16. ISO-TP: receive frame dispatch
17. ISO-TP: consecutive frames and payload completion
18. Data validation: reviewed layouts and VIN
19. Response windows, pending and transaction end
20. Diagnostic events, traces and connection probes
21. Mode04: debounce, queue and confirmation
22. Text routing, console and SD batching
23. File selection, header and 250 MB rotation
24. Stop boundary, draining and SD handover
25. USB serial commands and update boundary
26. OLED status selection and precedence
27. OLED frame, equal gaps and I2C isolation
28. Active readiness decoding and raw trace helpers
29. MTP exposure, enumeration and hidden objects
30. Bundled MTP command dispatcher
31. MTP file/data transport and error boundaries
32. Fault gates and health counters
33. Present but inactive helper code

## Scope and validation

The diagrams map project-owned runtime modules, decisions, timers, buffers,
protocol paths, errors and normal/failed shutdown. The bundled MTP responder is
grouped by command family and transport. External Arduino startup, CAN/SD/FAT,
USB and graphics driver internals remain explicit dependency boundaries.
Inactive helpers and compile-time alternatives are identified separately.
Tests, packaging/build tools, the old OBD-only project and planned hardware are
outside this runtime atlas. No instruction-level completeness, certification
or lossless-capture claim is made by a diagram.

Important implemented/proposed distinctions are retained: no automatic default
polling for invalid JSON; version 1/2 only; configurable CAN ID filters remain a
proposal; no internal firmware installer or automatic rollback; no RTC or
backup-power save path. The 70 ms response window is the default, while the code
still accepts its existing configured range. Pending/multiframe timing differs
from the initial quiet window.

Checked 22 source-file fingerprints. All **587 titles, nodes and detail notes**
are present in the PDF, source presentation and Canva text extraction. The
presentation has zero flattened slide pictures. Package, geometry/font and
re-import validators passed for all 33 pages. Every local page was rendered;
dense receive/shutdown/display pages and Canva-generated overview/ISO-TP previews
were visually reviewed. Canva editability inspection found separate text and
shape elements; the read-only inspection transaction was cancelled without edits.
These checks establish artifact integrity/content retention, not independent
proof that the firmware behaviour or every external driver branch is correct.

## Regeneration

Authoring utilities are in `tools/flowcharts/`. Use the Codex bundled Node/Python
runtime and its supplied Artifact Tool and Graphviz packages; do not install or
modify runtime dependencies. The builder currently uses Windows Arial fonts.

1. Review code changes before updating `source_flowcharts.py`; it is pinned to
   dev9 and deliberately refuses a different version without review.
2. Run `source_flowcharts.py` to write the specification, coverage and hashes.
3. Run `build_flowcharts.mjs` with the bundled node_modules path to create native
   editable elements, page previews and the PDF.
4. Run `finalize_flowcharts.mjs` with the presentation-skill and bundled Python
   paths. Its output filename must be unused; keep prior reviewed exports.
5. Import the validated source presentation into Canva, inspect the result and
   run `verify_flowcharts.py`. Canva extraction is an optional extra check when
   `build/flowcharts-dev9/canva-content.txt` is available.

Transient previews, signed thumbnail URLs, validation receipts and runtime
intermediates stay under the ignored build folder.
