# J1979-DA OCT2011 implementation review

Review started: 2026-10-30. Baseline: `3ae95ee58860b0131e09bb095126baf0fa20be0b`.
Product scope confirmed by the owner: current CAN-only OBD logger.

## Decision and claim boundary

Use October 2011 as the data-definition target. The project is **not demonstrated
fully conformant**. This is an initial implementation review, not a completed
clause-by-clause review of all 143 pages, an independent assessment, or a certificate.
No overall compliance percentage can be justified until the applicable requirements
inventory and verification evidence are complete. A passing build is not that evidence.

Reference inspected through the web reader:
[third-party-hosted OCT2011 PDF](https://f30.bimmerpost.com/forums/attachment.php?attachmentid=1800282&d=1523076762).
The full PDF is not copied into this repository. Direct local download timed out.
Authenticate the edition and resolve discrepancies with an authoritative source before
using the review for a certification submission.

## Document coverage index

This index identifies the remaining review scope; it does not mark every page reviewed.

- Page 1: scope and edition.
- Page 2, Appendix A: identifier support maps.
- Pages 3-102, Appendix B: live/freeze-frame definitions. Selected tables reviewed;
  exhaustive per-PID interpretation, dependencies and display rules remain open.
- Pages 103-104, Appendix C: distinguish non-CAN and CAN test definitions.
- Pages 105-108, Appendix D: CAN monitor identifiers; detailed review open.
- Pages 109-129, Appendix E: monitor scaling; detailed review open.
- Page 130, Appendix F: controlled tests; Mode 08 remains outside automatic polling.
- Pages 131-143, Appendix G: vehicle information; selected definitions reviewed.

Separate data interpretation from ECU-side generation requirements. Separate the
annex review from base J1979, J1978 and applicable ISO 15765 transport/test-tool
requirements. The lab's required editions and acceptance checklist are not known.

## Existing product evidence

The automatic sketch already has request scheduling, per-ECU discovery, response
matching, ISO-TP reassembly, raw and assembled tracing, bounded queues, SD rotation,
manual Mode 04, and controlled USB file access. Host regression tests exercise these.
Historical vehicle captures demonstrate selected exchanges, not exhaustive conformance.

`Config.h` accepts 1-64 jobs with service-specific request sizes. `Discovery.h`
maintains separate support maps per responder and retains unknown support. The
scheduler does not request arbitrary new identifiers automatically. The shipped
JSON has 51 jobs; these are neither 51 unique PIDs nor a complete standard catalogue.

Before this change, ordinary cyclic positive replies were accepted after matching
their identifiers. Discovery checked bitmap lengths, but most runtime response
lengths and data semantics were not checked. `LogReport.h` contains some decoding
helpers, but the automatic sketch does not call its `reportPositive` function; their
presence must not be counted as an implemented automatic display feature.

## First corrective change: explicit data-validation boundary

`J1979Validation.h` installs selected fixed-length checks for the configured live and
freeze-frame identifiers, support-map layouts, and VIN layout/characters. It also
checks the existing negative-response and manual-clear envelopes. These envelope
checks are implementation safeguards, not proof of base-standard conformance.

- `INVALID_LENGTH` or `INVALID_VALUE`: preserve raw frames and assembled bytes; do
  not emit a positive completion or illuminate the response indicator for that reply.
- `UNVERIFIED_DEFINITION`: keep the response and explicitly record that its definition
  is outside the installed validator. A later `POSITIVE` event identifies its service
  response only; it does not certify its contents.
- `LAYOUT_CHECKED`: an internal result for selected structural checks, not a claim
  that every field, dependency or ECU behavior is valid.

Padding outside the ISO-TP payload length is not treated as additional PID data.
Invalid matching replies can consume the collection window; a later valid reply
from the same ECU is still accepted. No malformed bytes are silently repaired.

Source locators for installed PID lengths: Appendix A; Tables B2, B3, B5, B6,
B12-B14, B16-B18, B23, B26-B29, B35, B39, B47, B54 and B115. VIN: Table G2.
These are locators for a review, not a reproduction of the source tables.

## Open implementation work

1. Finish the applicable-requirements inventory, including each claimed identifier,
   dependencies, exceptions, source locator, implementation location and test evidence.
2. Readiness interpretation is now implemented for Mode 01 PID 01 by `Readiness.h`
   and called by the automatic trace. Regression tests cover both monitor families,
   per-ECU isolation, unsupported monitors and reserved-bit warnings. Independent
   ECU/reference-tool comparison and the remaining semantic checks are still open.
3. Complete data lengths and semantic rules outside the initial validator. Do not
   treat a generic successful response as proof that its definition is implemented.
4. Add per-ECU scaling context, including the dependency between PID 10 and PID 50.
   The existing fixed MAF helper is insufficient for all vehicles.
5. Implement supported-item masks and unavailable/reserved-value handling for the
   newer sensor groups, without converting unsupported fields into real zero readings.
6. Complete Mode 06 record interpretation, units, signed values and limit handling.
7. Complete Mode 09 calibration IDs, verification numbers, ECU names and performance
   counters, including record counts and display rules.
8. Review DTC formats/descriptions against their applicable source; the annex alone
   is not a complete DTC dictionary.
9. Audit base-protocol behavior: request timing, pending-response handling, functional
   collection windows, flow control, padding, addressing and supported bitrates.
   The existing 10 ms spacing, 70 ms collection and 10 s deadline are implementation
   settings, not verified normative values.
10. Finish independent bench verification for multiple responders, delayed/invalid
    replies, boundary data, CAN variants and sustained capture under load. Host tests
    and historical vehicle logs do not replace these measurements.

Mode 05/non-CAN interfaces are outside the confirmed hardware scope. Mode 08 remains
an explicitly excluded product function pending an applicability decision, not a
requirement silently marked passed. No automatic actuator-control requests were added.

## Source issue requiring resolution

The introductory PID 66 example in B.4 conflicts with detailed Table B82 about data
length. The detailed table indicates five bytes. PID 66 remains explicitly unverified
in the installed validator until that discrepancy is independently resolved.

## Measuring progress honestly

Overall requirement coverage = verified applicable requirements / all applicable
requirements, with the inventory version and exclusions published alongside it.
Open, partial, untested and ambiguous items are not passes. Until the denominator
is complete, do not publish an overall percentage or describe the product as 100%.

A separate, narrower metric may count configured jobs with installed layout rules.
It measures only the current JSON and says nothing about unconfigured identifiers,
semantic correctness, transport behavior or certification.

## Verification and release gate

Run `teensy-obd-sd-logger/test.ps1`, including the new validation regression and the
Teensy 4.1 Serial+MTP build. Tests cover short/long payloads, VIN characters, the
terminal support-map reserved bit, preservation of malformed payloads, unknown
definitions and completion gating. Transport fixtures deliberately use unvalidated
synthetic data instead of claiming that arbitrary byte arrays are valid VINs.

No hardware was flashed or certification test performed as part of this change.
Before release, retain the flashed commit, configuration, reference-tool capture,
test conditions and expected/actual results. Close every applicable requirement and
source ambiguity before considering a full-conformance claim.

The initial validation revision passed all host tests and the Teensy 4.1 build.
The build emitted an existing FlexCAN_T4 `strncpy` truncation warning; it was not
introduced or resolved by the data-validation changes.

## Readiness trace behavior

Mode 01 PID 01 now adds human-readable lines after the preserved RX payload.
The interpretation is local to each ECU reply. Unsupported non-continuous monitors
are N/A, and reserved bits produce a warning rather than fabricated monitor names.
Completion is not displayed as a passed emissions test. The ignition-selector bit
selects the labels internally; it is not presented as a separate vehicle-fuel result.
The decoder does not reuse these rules for PID 41 or freeze-frame responses.
