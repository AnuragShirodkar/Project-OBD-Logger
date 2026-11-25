# Recent logger run review

Reviewed on 2026-11-18 after the user authorized stopping the active recording. OBD_1757338 was queried over COM8, stopped with the stop command and reported ready. Logs 43 through 52 were copied via Windows MTP into a temporary local review directory. All share configuration fingerprint AC300FF5. The comparison firmware from 386052b had previously been flashed, but the USB version string 1.1.0-dev3 is shared by both builds, so the string alone cannot prove source identity.

Logs 47 through 51 stopped at 2844 to 2856 ms with TX pending at transaction end and no CAN frames received. Each has zero hardware overruns, zero software capture drops, zero SD write failures and a final CAPTURE_DRAINED record. This is consistent with starting while the simulator or CAN peer is unavailable. Exact wiring and power state at those instants cannot be established from these logs.

Log 46 ended at 1500426 ms (about 25 minutes) with ECU disconnected. It reports zero hardware overruns and zero SD write failures. Log 52 was stopped by this inspection at 711094 ms (about 11 minutes 51 seconds), with all 52025 captured frames drained, no hardware overrun and no SD write failure.

Logs 43, 44 and 45 contain readable traces through approximately 24 minutes 41 seconds, 55 minutes 34 seconds and 9 minutes 56 seconds respectively. They end with ordinary response records and lack a final shutdown summary. They may have been interrupted or power-cycled. These files do not establish zero loss at shutdown or provide final overrun counters.

These recent files do not reproduce the earlier hardware overrun. They identify startup transmission faults and a communication-loss stop. No simultaneous CANking capture exists for these runs. Total 5–8 hours of continuous error-free recording is not established by this ten-file subset. Keep simulator active before logger startup and compare builds using identical replay conditions. Logger is left stopped after this inspection.
