$ProgressPreference = 'SilentlyContinue'
$ErrorActionPreference = 'SilentlyContinue'
function S($n, $v) { Write-Output ('{0,-30} {1}' -f $n, $v) }
function Hdr($t) { Write-Output ('SECTION:' + $t) }

Hdr 'OS'
S 'Caption' (Get-CimInstance Win32_OperatingSystem).Caption
S 'Version' (Get-CimInstance Win32_OperatingSystem).Version
$cv = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion'
S 'Build' ($cv.CurrentBuild + '.' + $cv.UBR)
S 'DisplayVersion' $cv.DisplayVersion
S 'EditionID' $cv.EditionID
S 'InstallDate' (Get-CimInstance Win32_OperatingSystem).InstallDate
$cs = Get-CimInstance Win32_ComputerSystem
S 'Computer' $cs.Name
S 'Manufacturer' $cs.Manufacturer
S 'Model' $cs.Model
S 'RAM_GB' ([math]::Round($cs.TotalPhysicalMemory / 1GB, 1))
S 'HypervisorPresent' $cs.HypervisorPresent
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
S 'CPU' $cpu.Name
S 'Cores' $cpu.NumberOfCores
S 'Logical' $cpu.NumberOfLogicalProcessors

Hdr 'GPU_WMI'
Get-CimInstance Win32_VideoController | ForEach-Object {
  S 'VideoController' ($_.Name + ' | drv=' + $_.DriverVersion + ' | vram_mb=' + [math]::Round($_.AdapterRAM / 1MB, 0) + ' | status=' + $_.Status + ' | ' + $_.VideoProcessor)
}
Hdr 'GPU_PNP'
Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match 'NVIDIA|Display|Graphics|Hyper-V Video' } | ForEach-Object { S 'PnP' ($_.Name + ' | ' + $_.Status) }
Hdr 'NVSMI'
$nvsmi = 'C:\Windows\System32\nvidia-smi.exe'
if (Test-Path $nvsmi) {
  S 'path' $nvsmi
  & $nvsmi --query-gpu=name,driver_version,memory.total,compute_cap,vbios_version --format=csv,noheader
  & $nvsmi -L
} else { S 'nvidia-smi' 'NOT FOUND' }
Hdr 'GPU_CONFIG_HKLM'
Get-ChildItem 'HKLM:\SYSTEM\CurrentControlSet\Control\Video' | ForEach-Object { $k = $_; Get-ChildItem $k.PSPath | ForEach-Object { $p = Get-ItemProperty $_.PSPath; if ($p.'DriverDesc') { S 'Adapter' ($p.'DriverDesc' + ' | drv=' + $p.'DriverVersion' + ' | ' + $k.PSChildName) } } }

Hdr 'TOOLS'
foreach ($t in @('windbg', 'windbgx', 'kd', 'cdb', 'procdump', 'procdump64', 'procmon', 'procmon64', 'procexp', 'procexp64', 'wpr', 'wpa', 'xperf', 'git', 'python', 'python3', 'ssh', 'scp', 'dumpbin', 'cl', 'msbuild', 'link', 'sigcheck', 'vsjitdebugger', 'tracelog', 'vswhere', 'code', 'node', 'curl', '7z', 'cmake', 'ninja', 'gcc')) {
  $c = Get-Command $t -ErrorAction SilentlyContinue
  if ($c) { S $t $c.Source }
}
Hdr 'TOOL_DIRS'
foreach ($p in @('C:\Program Files\SysinternalsSuite', 'C:\Sysinternals', 'C:\Tools', 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x64', 'C:\Program Files\Windows Kits\10\Debuggers\x64', 'C:\Program Files\Microsoft VS Code', 'C:\ProgramData\chocolatey\bin', 'C:\Program Files\Git\cmd', 'C:\Program Files\Python313', 'C:\Program Files\Python312', 'C:\Program Files\Python311', 'C:\Program Files\NVIDIA Corporation\Nsight Systems 2024.5.1', 'C:\Program Files\NVIDIA Corporation\Nsight Compute 2024.3.2')) {
  if (Test-Path $p) { S 'DIR' $p }
}
Get-ChildItem 'C:\Program Files\NVIDIA Corporation' | Select-Object -ExpandProperty Name | ForEach-Object { S 'NVCorp' $_ }

Hdr 'MSVC'
$vw = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vw) {
  S 'vswhere' $vw
  S 'installPath' (& $vw -latest -products * -property installationPath)
  S 'displayName' (& $vw -latest -property displayName)
  S 'catalog_productDisplayVersion' (& $vw -latest -property catalog_productDisplayVersion)
  & $vw -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
} else { S 'vswhere' 'NOT FOUND' }
Get-ChildItem 'C:\Program Files\Microsoft Visual Studio' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name | ForEach-Object { S 'VS' $_ }
Get-ChildItem 'C:\Program Files (x86)\Microsoft Visual Studio' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name | ForEach-Object { S 'VSx86' $_ }

Hdr 'WINSDK'
Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\Include' | Select-Object -ExpandProperty Name | ForEach-Object { S 'SDK_Include' $_ }
Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin' | Select-Object -ExpandProperty Name | ForEach-Object { S 'SDK_bin' $_ }
Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x64' | Select-Object -ExpandProperty Name | ForEach-Object { S 'Dbg_x64' $_ }

Hdr 'ADOBE'
Get-ChildItem 'C:\Program Files\Adobe' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name | ForEach-Object { S 'AdobeDir' $_ }
Get-ChildItem 'C:\Program Files (x86)\Adobe' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name | ForEach-Object { S 'AdobeDirx86' $_ }
Get-ChildItem 'C:\Program Files\Common Files\Adobe' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name | ForEach-Object { S 'AdobeCommon' $_ }
Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*', 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*' -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -match 'Adobe|Creative Cloud|Photoshop|Premiere|Media Encoder' } | ForEach-Object { S 'Installed' ($_.DisplayName + ' v' + $_.DisplayVersion) }

Hdr 'SERVICES'
S 'sshd' (Get-Service sshd).Status
S 'LastBoot' (Get-CimInstance Win32_OperatingSystem).LastBootUpTime
S 'Uptime' ((Get-Date) - (Get-CimInstance Win32_OperatingSystem).LastBootUpTime)
Hdr 'NET'
Get-NetIPAddress -AddressFamily IPv4 | ForEach-Object { S 'IP' ($_.InterfaceAlias + ' ' + $_.IPAddress) }
Hdr 'DISK'
Get-PSDrive -PSProvider FileSystem | ForEach-Object { S 'Drive' ($_.Name + ': used=' + [math]::Round($_.Used / 1GB, 1) + 'GB free=' + [math]::Round($_.Free / 1GB, 1) + 'GB') }
