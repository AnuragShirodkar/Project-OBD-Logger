param(
  [string]$ArduinoCli = 'C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe',
  [string]$ArduinoJson = "$env:USERPROFILE\Documents\Arduino\libraries\ArduinoJson\src"
)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
  New-Item -ItemType Directory -Force build | Out-Null
  & g++ -std=c++11 -Wall -Wextra -Werror tests/test_core.cpp -o build/test_core.exe
  if ($LASTEXITCODE) { throw 'Core test compilation failed' }
  & ./build/test_core.exe
  if ($LASTEXITCODE) { throw 'Core tests failed' }
  & g++ -std=c++11 -Wall -Wextra -Werror "-I$ArduinoJson" tests/test_config.cpp -o build/test_config.exe
  if ($LASTEXITCODE) { throw 'Config test compilation failed' }
  & ./build/test_config.exe
  if ($LASTEXITCODE) { throw 'Config tests failed' }
  & g++ -std=c++11 -Wall -Wextra -Werror tests/test_capture.cpp -o build/test_capture.exe
  if ($LASTEXITCODE) { throw 'Capture test compilation failed' }
  & ./build/test_capture.exe
  if ($LASTEXITCODE) { throw 'Capture tests failed' }
  & g++ -std=c++11 -Wall -Wextra -Werror tests/test_run_control.cpp -o build/test_run_control.exe
  if ($LASTEXITCODE) { throw 'Run-control test compilation failed' }
  & ./build/test_run_control.exe
  if ($LASTEXITCODE) { throw 'Run-control tests failed' }
  & g++ -std=c++11 -Wall -Wextra -Werror tests/test_discovery.cpp -o build/test_discovery.exe
  if ($LASTEXITCODE) { throw 'Discovery test compilation failed' }
  & ./build/test_discovery.exe
  if ($LASTEXITCODE) { throw 'Discovery tests failed' }
  & g++ -std=c++11 -Wall -Wextra -Werror tests/test_trace.cpp -o build/test_trace.exe
  if ($LASTEXITCODE) { throw 'Trace test compilation failed' }
  & ./build/test_trace.exe
  if ($LASTEXITCODE) { throw 'Trace tests failed' }
  & g++ -std=c++11 -Wall -Wextra -Werror tests/test_mode4_feedback.cpp -o build/test_mode4_feedback.exe
  if ($LASTEXITCODE) { throw 'Mode 04 feedback compilation failed' }
  & ./build/test_mode4_feedback.exe
  if ($LASTEXITCODE) { throw 'Mode 04 feedback tests failed' }
  & $ArduinoCli compile --fqbn teensy:avr:teensy41:usb=serialmtp --build-path build/teensy41 TeensyOBDSDLogger
  if ($LASTEXITCODE) { throw 'Teensy compilation failed' }
} finally { Pop-Location }
