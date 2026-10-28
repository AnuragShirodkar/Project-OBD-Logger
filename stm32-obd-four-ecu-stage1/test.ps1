$ErrorActionPreference='Stop'
Push-Location $PSScriptRoot
try {
  New-Item -ItemType Directory -Force build | Out-Null
  & gcc -std=c11 -Wall -Wextra -Werror -ICore/Inc -c Core/Src/obd_sim.c -o build/protocol-test.o
  if ($LASTEXITCODE) { throw 'Protocol compile failed' }
  & g++ -std=c++11 -Wall -Wextra -Werror -ICore/Inc "-I$env:USERPROFILE/Documents/Arduino/libraries/ArduinoJson/src" tests/test_stage1.cpp build/protocol-test.o -o build/test_stage1.exe
  if ($LASTEXITCODE) { throw 'Integration compile failed' }
  & ./build/test_stage1.exe
  if ($LASTEXITCODE) { throw 'Integration tests failed' }
} finally { Pop-Location }
