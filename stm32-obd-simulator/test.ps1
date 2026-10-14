$ErrorActionPreference='Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force build | Out-Null
    & gcc -std=c11 -Wall -Wextra -Werror -ICore/Inc Core/Src/obd_sim.c tests/test_obd.c -o build/test_protocol.exe
    if ($LASTEXITCODE) { throw 'Protocol test compilation failed' }
    & ./build/test_protocol.exe
    if ($LASTEXITCODE) { throw 'Protocol tests failed' }
    & gcc -std=c11 -Wall -Wextra -Werror -Itests/hal_mock -ICore/Inc Core/Src/obd_sim.c Core/Src/vehicle_app.c tests/test_vehicle.c -o build/test_vehicle.exe
    if ($LASTEXITCODE) { throw 'Adapter test compilation failed' }
    & ./build/test_vehicle.exe
    if ($LASTEXITCODE) { throw 'Adapter tests failed' }
} finally { Pop-Location }
