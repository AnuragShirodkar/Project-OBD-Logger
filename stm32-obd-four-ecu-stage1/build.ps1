param([string]$ToolchainBin = '', [switch]$Crystal)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    if (!$ToolchainBin) {
        $found = Get-ChildItem 'C:\ST' -Filter arm-none-eabi-gcc.exe -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if (!$found) { throw 'Pass -ToolchainBin with the STM32 GCC tools/bin directory.' }
        $ToolchainBin = $found.DirectoryName
    }
    if (!(Test-Path 'vendor/hal/Inc/stm32f4xx_hal.h')) { throw 'Run python fetch_dependencies.py first.' }
    New-Item -ItemType Directory -Force build | Out-Null
    $flags = @('-mcpu=cortex-m4','-mthumb','-mfpu=fpv4-sp-d16','-mfloat-abi=hard','-std=c11','-Og','-g3','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-DSTM32F407xx','-DUSE_HAL_DRIVER','-ICore/Inc','-Ivendor/hal/Inc','-Ivendor/device/Include','-Ivendor/cmsis')
    if ($Crystal) { $flags += '-DVEHICLE_HSE_CRYSTAL' }
    $sources = @(Get-ChildItem 'Core/Src/*.c','vendor/hal/Src/*.c','vendor/device/*.c','vendor/device/*.s')
    $objects = @()
    foreach ($src in $sources) {
        $obj = 'build/' + $src.BaseName + '.o'
        $sourceFlags = @($flags)
        if ($src.FullName -like '*\vendor\*') { $sourceFlags += '-Wno-unused-parameter' }
        & "$ToolchainBin/arm-none-eabi-gcc.exe" @sourceFlags -c $src.FullName -o $obj
        if ($LASTEXITCODE) { throw "Compile failed: $($src.Name)" }
        $objects += $obj
    }
    & "$ToolchainBin/arm-none-eabi-gcc.exe" @flags @objects '-TSTM32F407VG.ld' '--specs=nano.specs' '--specs=nosys.specs' '-Wl,--gc-sections,-Map=build/vehicle.map' -o build/vehicle.elf
    if ($LASTEXITCODE) { throw 'Link failed' }
    & "$ToolchainBin/arm-none-eabi-objcopy.exe" -O ihex build/vehicle.elf build/vehicle.hex
    if ($LASTEXITCODE) { throw 'HEX generation failed' }
    & "$ToolchainBin/arm-none-eabi-objcopy.exe" -O binary build/vehicle.elf build/vehicle.bin
    if ($LASTEXITCODE) { throw 'BIN generation failed' }
    & "$ToolchainBin/arm-none-eabi-size.exe" build/vehicle.elf
} finally { Pop-Location }
