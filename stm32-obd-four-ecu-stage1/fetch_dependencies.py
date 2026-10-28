"""Download official, version-tagged ST/Arm source files into this project only."""
from pathlib import Path
from urllib.request import urlopen
from concurrent.futures import ThreadPoolExecutor

ROOT = Path(__file__).resolve().parent / 'vendor'
jobs = []
def add(repo, tag, remote, local):
    jobs.append((f'https://raw.githubusercontent.com/{repo}/{tag}/{remote}', ROOT/local))

hal = 'STMicroelectronics/stm32f4xx-hal-driver'
modules = ['hal','hal_can','hal_cortex','hal_flash','hal_flash_ex','hal_gpio',
           'hal_pwr','hal_pwr_ex','hal_rcc','hal_rcc_ex']
for mod in modules:
    add(hal,'v1.8.3',f'Src/stm32f4xx_{mod}.c',f'hal/Src/stm32f4xx_{mod}.c')
    add(hal,'v1.8.3',f'Inc/stm32f4xx_{mod}.h',f'hal/Inc/stm32f4xx_{mod}.h')
for name in ['stm32f4xx_hal_def.h','stm32f4xx_hal_gpio_ex.h','stm32f4xx_hal_flash_ramfunc.h','Legacy/stm32_hal_legacy.h']:
    add(hal,'v1.8.3','Inc/'+name,'hal/Inc/'+name)
add(hal,'v1.8.3','LICENSE.md','hal/LICENSE.md')
dev = 'STMicroelectronics/cmsis-device-f4'
for name in ['stm32f4xx.h','stm32f407xx.h','system_stm32f4xx.h']:
    add(dev,'v2.6.10','Include/'+name,'device/Include/'+name)
add(dev,'v2.6.10','Source/Templates/system_stm32f4xx.c','device/system_stm32f4xx.c')
add(dev,'v2.6.10','Source/Templates/gcc/startup_stm32f407xx.s','device/startup_stm32f407xx.s')
add(dev,'v2.6.10','LICENSE.md','device/LICENSE.md')
for name in ['core_cm4.h','cmsis_version.h','cmsis_compiler.h','cmsis_gcc.h','mpu_armv7.h']:
    add('ARM-software/CMSIS_5','5.9.0','CMSIS/Core/Include/'+name,'cmsis/'+name)
add('ARM-software/CMSIS_5','5.9.0','LICENSE.txt','cmsis/LICENSE.txt')
def fetch(job):
    url, path = job
    if path.exists(): return
    data = urlopen(url,timeout=60).read()
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(data)
with ThreadPoolExecutor(max_workers=6) as pool:
    list(pool.map(fetch,jobs))
print(f'Official dependencies ready: {len(jobs)} files')
