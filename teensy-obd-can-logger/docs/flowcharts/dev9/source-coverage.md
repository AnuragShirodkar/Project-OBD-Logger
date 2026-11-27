# Source-to-flowchart coverage

Firmware: 1.2.0-dev9-oled-gaps; source reference commit: 125bcc7.

Page IDs are the numbered flowcharts, before any source-index appendix.
Project runtime modules are covered. Bundled MTP branches are grouped by operation/transport; external Arduino, FlexCAN, SD/FAT, USB and graphics library internals are boundaries.
This is a code-reviewed behavioural atlas, not a proof that every machine instruction or dependency branch is modelled. Tests/package tools, old OBD-only code and planned hardware are outside this runtime atlas.

## TeensyOBDSDLogger/CanRecording.h
Charts: 02,09-10,23. Lines: 72. SHA-256: `afea9d467718217104f4fb5e51b52633aa34ab1f8dd06832e65719b6aa7d7eea`.

- `formatCanAscHeader` — line 11
- `canIdleExpired` — line 27
- `passiveAction` — line 31
- `sample` — line 45
- `canTrafficRecent` — line 47
- `formatCanAsc` — line 50

## TeensyOBDSDLogger/CaptureRing.h
Charts: 09. Lines: 23. SHA-256: `b0648cee8e5a6690e87771c1043c0c0e87458cee6b756daab462dc2d59a9db3d`.

- `begin` — line 10
- `push` — line 11
- `pop` — line 16
- `size` — line 21

## TeensyOBDSDLogger/CaptureTimer.h
Charts: 02,08. Lines: 27. SHA-256: `68b2b8aba52b3e9f77a0391e9798e4674b6dc96a3ba019a94b96e9d02812c09b`.

- `begin` — line 10
- `now` — line 25

## TeensyOBDSDLogger/Config.h
Charts: 04-05. Lines: 83. SHA-256: `ee5b361da99df03db52c81d2f6740bfa5d5587eb61d89c8b5765ec84b90159b5`.

- `modeName` — line 7
- `hexId` — line 14
- `readUInt` — line 24
- `parseConfig` — line 29

## TeensyOBDSDLogger/DiagnosticCore.h
Charts: 05,15-20. Lines: 189. SHA-256: `91981ba5926afca16f47a57516a1fb6ffe2be5a7312aacda7154434a68c32c91`.

- `accepts` — line 12
- `flowId` — line 18
- `parseHex` — line 23
- `~Sink` — line 47
- `allExpectedComplete` — line 65
- `endRequest` — line 72
- `emit` — line 82
- `state` — line 83
- `matches` — line 88
- `flow` — line 96
- `complete` — line 102
- `DiagnosticCore` — line 121
- `busy` — line 122
- `setResponseWindow` — line 123
- `cancel` — line 124
- `start` — line 125
- `receive` — line 139
- `tick` — line 172

## TeensyOBDSDLogger/Discovery.h
Charts: 11-13. Lines: 94. SHA-256: `09f9cda355da480721961cb9a0365dbd944e5ee098fcae933b72ab4a0306427e`.

- `modeAt` — line 14
- `continuation` — line 15
- `missingExpected` — line 19
- `begin` — line 27
- `busy` — line 28
- `attemptNumber` — line 29
- `request` — line 30
- `observe` — line 31
- `receive` — line 37
- `finishTransaction` — line 48
- `support` — line 55
- `requestSupport` — line 69
- `expected` — line 84
- `name` — line 92

## TeensyOBDSDLogger/J1979Validation.h
Charts: 18. Lines: 75. SHA-256: `d03b821df3997dd5e4e4d3e9f9bf7384f54337e698d201db2aefadf25dfa9503`.

- `validationName` — line 9
- `invalidData` — line 17
- `supportIdentifier` — line 20
- `reviewedPidLength` — line 24
- `vinCharacter` — line 35
- `hasReviewedLayout` — line 40
- `validateData` — line 48

## TeensyOBDSDLogger/LogReport.h
Charts: 33. Lines: 153. SHA-256: `0bc007e7d962bb00724ee1a031a84bb88987cf55d9829dad59d8275ec2e0b167`.

- `leap` — line 11
- `monthDays` — line 12
- `parseLocalDate` — line 17
- `formatLocalDate` — line 28
- `formatHeader` — line 35
- `formatElapsedHeader` — line 41
- `formatEvent` — line 44
- `modeName` — line 50
- `parameterName` — line 65
- `reportRequest` — line 84
- `reportPositive` — line 92

## TeensyOBDSDLogger/LogRotation.h
Charts: 23. Lines: 8. SHA-256: `edde133792a586e281806ecb4a4d33aa9674ddd98d75063d414f09ec4d97d8b2`.

- `needsLogRotation` — line 5

## TeensyOBDSDLogger/Mode4Feedback.h
Charts: 20-21,26,33. Lines: 48. SHA-256: `d6f84797fdbce6850ded205b24fac362a454d00b9de09ecdc7a6e960cb07765a`.

- `queued` — line 18
- `begin` — line 19
- `observe` — line 24
- `finish` — line 31
- `update` — line 35
- `yellow` — line 38
- `red` — line 43
- `occupied` — line 46

## TeensyOBDSDLogger/ProductIdentity.h
Charts: 02,23,25,27. Lines: 5. SHA-256: `8a415dae1909b3a272db9c0a478af4f48c0659769a0a0f80fb899ace26b6dca1`.


## TeensyOBDSDLogger/Readiness.h
Charts: 28. Lines: 60. SHA-256: `2e48fd065c3fc0bb26a2a58e38de00a45973fb4f214cdfb549b0238033c862db`.

- `mil` — line 11
- `dtcs` — line 12
- `compression` — line 13
- `commonSupported` — line 14
- `commonComplete` — line 15
- `reservedMask` — line 16
- `reservedSet` — line 17
- `monitorSupported` — line 18
- `monitorState` — line 19
- `monitorName` — line 23
- `decodeReadiness` — line 37
- `traceReadiness` — line 43

## TeensyOBDSDLogger/ReceivePipeline.h
Charts: 06,09,24. Lines: 28. SHA-256: `34acbbe2daef2f364d850aec03438d8cb8f91d5b00b46d466728313669dc2fd5`.

- `begin` — line 10
- `stage` — line 14
- `staged` — line 15
- `buffered` — line 16
- `pending` — line 17
- `transfer` — line 18
- `pop` — line 26

## TeensyOBDSDLogger/RunControl.h
Charts: 10,14,21,33. Lines: 43. SHA-256: `ee6205c496334ec97f0ce4e43229ea0d7912ffca6ac983cb3550a6ed8a946cdd`.

- `durationReached` — line 4
- `ready` — line 8
- `submitted` — line 9
- `update` — line 17
- `holding` — line 29
- `StopButton` — line 34
- `update` — line 35

## TeensyOBDSDLogger/Schedule.h
Charts: 14,24. Lines: 34. SHA-256: `e714926c26e9ce9f56e400888b6cd36b7d2c54877608e141ea88c536ee06af57`.

- `begin` — line 13
- `select` — line 17
- `finish` — line 28

## TeensyOBDSDLogger/src/mtp/OBD_MTP.cpp
Charts: 29-31. Lines: 2570. SHA-256: `7b042b2e9a869c22b6beda848931d56756051bea6fd75edfddde66fb963a5d35`.

- `usb_mtp_first_rx_cb` — line 90
- `MTP_class::begin` — line 95
- `MTP_class::loop` — line 112
- `MTP_class::_interval_timer_handler` — line 297
- `MTP_class::processIntervalTimer` — line 302
- `MTP_class::SendObjectInfo` — line 388
- `MTP_class::SendObject` — line 463
- `MTP_class::SendObject` — line 582
- `MTP_class::GetObjectInfo` — line 661
- `MTP_class::GetObject` — line 730
- `MTP_class::GetObject` — line 780
- `MTP_class::GetPartialObject` — line 824
- `MTP_class::deleteObject` — line 868
- `MTP_class::moveObject` — line 879
- `MTP_class::copyObject` — line 894
- `MTP_class::formatStore` — line 906
- `MTP_class::GetStorageIDs` — line 943
- `MTP_class::GetStorageInfo` — line 976
- `MTP_class::GetNumObjects` — line 1024
- `MTP_class::GetObjectHandles` — line 1048
- `MTP_class::GetDevicePropValue` — line 1085
- `MTP_class::GetObjectPropDesc` — line 1105
- `MTP_class::GetObjectPropValue` — line 1241
- `MTP_class::setObjectPropValue` — line 1361
- `MTP_class::GetDevicePropDesc` — line 1387
- `MTP_class::GetObjectPropsSupported` — line 1410
- `MTP_class::GetDeviceInfo` — line 1436
- `MTP_class::OpenSession` — line 1541
- `MTP_class::readDataPhaseHeader` — line 1557
- `MTP_class::readstring` — line 1566
- `MTP_class::readDateTimeString` — line 1599
- `MTP_class::read` — line 1636
- `MTP_class::writeDataPhaseHeader` — line 1664
- `utf8_strlen` — line 1677
- `MTP_class::writestring` — line 1698
- `MTP_class::writestringlen` — line 1729
- `MTP_class::write` — line 1735
- `MTP_class::write_finish` — line 1759
- `MTP_class::free_received_bulk` — line 1794
- `MTP_class::allocate_transmit_bulk` — line 1803
- `MTP_class::transmit_bulk` — line 1818
- `MTP_class::receive_bulk` — line 1837
- `MTP_class::free_received_bulk` — line 1856
- `MTP_class::allocate_transmit_bulk` — line 1861
- `MTP_class::transmit_bulk` — line 1869
- `usb_init_events` — line 1900
- `usb_mtp_sendEvent` — line 1908
- `usb_init_events` — line 1965
- `usb_mtp_wait` — line 1975
- `usb_mtp_recvEvent` — line 1990
- `usb_mtp_sendEvent` — line 2005
- `MTP_class::send_Event` — line 2025
- `MTP_class::send_Event` — line 2037
- `MTP_class::send_Event` — line 2049
- `MTP_class::send_Event` — line 2061
- `MTP_class::send_DeviceResetEvent` — line 2075
- `MTP_class::send_StorageInfoChangedEvent` — line 2080
- `MTP_class::send_addObjectEvent` — line 2085
- `MTP_class::send_removeObjectEvent` — line 2088
- `MTP_class::send_StoreAddedEvent` — line 2092
- `MTP_class::send_StoreRemovedEvent` — line 2098
- `MTP_class::send_addObjectEvent` — line 2103
- `MTP_class::send_removeObjectEvent` — line 2116
- `MTP_class::printFilesystemsInfo` — line 2138
- `MTP_class::printContainer` — line 2166

## TeensyOBDSDLogger/src/mtp/PROVENANCE.md
Charts: 29-31. Lines: 9. SHA-256: `0d1a700e03ed77a408296e014a1fc5c2427223dc9a0ae63c7cc2179045955293`.


## TeensyOBDSDLogger/src/mtp/VisibleObjects.h
Charts: 29. Lines: 34. SHA-256: `45544b9ba7cb626f301c12d246a0b9f98c4cf38c62731ff94e33fca5f3d697b1`.

- `sameName` — line 6
- `objectVisible` — line 18

## TeensyOBDSDLogger/StoragePaths.h
Charts: 03,23. Lines: 36. SHA-256: `f865be1d8dca8c469905110766fdabbe197a17d1e9a9c4e2f7f1d3f09246b136`.

- `formatLogPath` — line 12
- `configPath` — line 23
- `ensureDirectory` — line 29

## TeensyOBDSDLogger/TeensyOBDSDLogger.ino
Charts: 01,03,06-11,13-15,20-27,32. Lines: 899. SHA-256: `6f86539dd9943d47f1fb75aee5ebdb182fffdb3a5b6df2c6bd2c8c818f971637`.

- `canMode` — line 45
- `available` — line 61
- `read` — line 61
- `peek` — line 62
- `flush` — line 62
- `write` — line 63
- `captureNowUs` — line 84
- `capturePending` — line 102
- `transferCapture` — line 103
- `obdWrite` — line 122
- `exchangeWrite` — line 128
- `obdStatus` — line 129
- `queueConsole` — line 136
- `pumpConsole` — line 144
- `recordAt` — line 153
- `note` — line 163
- `raw` — line 164
- `writeCanFooter` — line 170
- `prepareLogBatch` — line 179
- `writeFileBatch` — line 194
- `writeBatch` — line 206
- `fileWriteError` — line 207
- `flushFiles` — line 208
- `closeFiles` — line 209
- `writeHeaders` — line 210
- `pumpLog` — line 232
- `onCan` — line 241
- `popCapture` — line 249
- `txPending` — line 250
- `captureCanInterrupt` — line 256
- `configureCan` — line 287
- `nrcName` — line 298
- `traceExchangeRequest` — line 307
- `send` — line 315
- `event` — line 335
- `startRequest` — line 393
- `exposeSD` — line 405
- `openLogSegment` — line 410
- `updateStatus` — line 422
- `pollStop` — line 429
- `setup` — line 457
- `summary` — line 549
- `stopRun` — line 582
- `finishDisplayTransfer` — line 600
- `drawDisplayBadge` — line 607
- `updateDisplay` — line 614
- `loop` — line 719

## TeensyOBDSDLogger/TraceFormat.h
Charts: 20,22,28. Lines: 27. SHA-256: `df7d23be500ce2782ddc65a97a85ccead4ccb7e3215b1a8188663391971e7ed2`.

- `traceCan` — line 7
- `tracePayload` — line 14
- `traceRequest` — line 21

## TeensyOBDSDLogger/usb_identity.c
Charts: 02,25,29. Lines: 22. SHA-256: `67df23999ac5a2f7e64d0986e572d947ba9b29ba16b88467133343e709e17736`.

- `startup_middle_hook` — line 16
