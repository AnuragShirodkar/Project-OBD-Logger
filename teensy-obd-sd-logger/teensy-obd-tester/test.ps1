param([string]$ArduinoCli = 'C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force build | Out-Null
    & g++ -std=c++11 -Wall -Wextra -Werror tests/test_core.cpp -o build/test_core.exe
    if ($LASTEXITCODE) { throw 'Host test compilation failed' }
    & ./build/test_core.exe
    if ($LASTEXITCODE) { throw 'Host tests failed' }
    & g++ -std=c++11 -Wall -Wextra -Werror tests/test_report.cpp -o build/test_report.exe
    if ($LASTEXITCODE) { throw 'Report test compilation failed' }
    & ./build/test_report.exe
    if ($LASTEXITCODE) { throw 'Report tests failed' }
    & $ArduinoCli compile --fqbn teensy:avr:teensy41 --build-path build/teensy41 TeensyOBDTester
    if ($LASTEXITCODE) { throw 'Teensy compilation failed' }
} finally { Pop-Location }
