# Sky Lance v1 setup (Windows PowerShell 5.1 or later; no Python needed).
#
# Installs the MVS and AES MAME sets of the NeoGeoSDK v1.7.3 pre-release
# side by side into a MAME folder, with EagleBIOS and the launchers:
#
#   tests\skylance\mvs\roms\skylance   arcade cartridge, its software list in ..\hash
#   tests\skylance\aes\roms\skylance   console cartridge, its software list in ..\hash
#   tests\skylance\bios\neogeo, \aes   EagleBIOS (the free NeoGeoSDK system ROM)
#   run_skylance_mvs.bat, run_skylance_aes.bat (and _scanlines), run_skylance.bat
#
# The two release zips are taken from beside this script, or downloaded from
# the release, and checked against SkyLance-MANIFEST_v1.txt before anything
# is installed. Replaced files go to backups\skylance-setup-<time>; settings
# and saves (cfg-*, nvram-*) stay where they are.
#
#   install_skylance.bat                      asks for the folder (default C:\mame\neogeosdk)
#   install_skylance.bat -Target D:\mame\sdk  installs there
param([string]$Target = "", [switch]$NoPause)
$ErrorActionPreference = "Stop"
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Tag = "v1.7.3"
$Version = "v1"
$Release = "https://github.com/eaglesoftware777/neogeosdk/releases/download/$Tag"
$Zips = [ordered]@{ mvs = "SkyLance-WIP-EagleBIOS_$Version.zip"; aes = "SkyLance-WIP-AES-EagleBIOS_$Version.zip" }
$Launchers = "run_skylance.bat", "run_skylance_mvs.bat", "run_skylance_aes.bat",
             "run_skylance_mvs_scanlines.bat", "run_skylance_aes_scanlines.bat", "skylance_display.bat"

Write-Host "Sky Lance $Version setup (NeoGeoSDK $Tag pre-release)"
if (-not $Target) {
    $default = "C:\mame\neogeosdk"
    $answer = Read-Host "Install into [$default]"
    $Target = if ($answer) { $answer } else { $default }
}
$Target = [IO.Path]::GetFullPath($Target)
New-Item -ItemType Directory -Force -Path $Target | Out-Null

$expected = @{}
foreach ($line in Get-Content (Join-Path $Here "SkyLance-MANIFEST_$Version.txt")) {
    if (-not $line -or $line.StartsWith("#")) { continue }
    $fields = -split $line
    $expected[$fields[3]] = $fields[2]
}

$work = Join-Path ([IO.Path]::GetTempPath()) ("skylance-setup-" + [guid]::NewGuid())
New-Item -ItemType Directory -Path $work | Out-Null
try {
    foreach ($platform in $Zips.Keys) {
        $name = $Zips[$platform]
        $zip = Join-Path $Here $name
        if (-not (Test-Path $zip)) {
            $zip = Join-Path $work $name
            Write-Host "Downloading $name ..."
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -Uri "$Release/$name" -OutFile $zip -UseBasicParsing
        }
        $sha = (Get-FileHash -Algorithm SHA256 -Path $zip).Hash.ToLower()
        if ($sha -ne $expected[$name]) { throw "$name does not match SkyLance-MANIFEST_$Version.txt" }
        Write-Host "  $name verified"
        Expand-Archive -Path $zip -DestinationPath (Join-Path $work $platform) -Force
    }

    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $backup = Join-Path $Target "backups\skylance-setup-$stamp"
    $replaced = @("tests\skylance\mvs\roms", "tests\skylance\mvs\hash", "tests\skylance\aes\roms",
                  "tests\skylance\aes\hash", "tests\skylance\bios", "SKYLANCE-README.txt") + $Launchers
    foreach ($item in $replaced) {
        $source = Join-Path $Target $item
        if (Test-Path $source) {
            $destination = Join-Path $backup $item
            New-Item -ItemType Directory -Force -Path (Split-Path $destination) | Out-Null
            Move-Item -Path $source -Destination $destination
        }
    }

    foreach ($platform in $Zips.Keys) {
        $data = Join-Path $Target "tests\skylance\$platform"
        New-Item -ItemType Directory -Force -Path (Join-Path $data "roms\skylance"), (Join-Path $data "hash") | Out-Null
        Copy-Item -Path (Join-Path $work "$platform\roms\skylance\*") -Destination (Join-Path $data "roms\skylance") -Force
        Copy-Item -Path (Join-Path $work "$platform\hash\neogeo.xml") -Destination (Join-Path $data "hash") -Force
    }
    foreach ($board in "neogeo", "aes") {
        $bios = Join-Path $Target "tests\skylance\bios\$board"
        New-Item -ItemType Directory -Force -Path $bios | Out-Null
        Copy-Item -Path (Join-Path $work "mvs\roms\$board\*") -Destination $bios -Force
    }
    foreach ($name in $Launchers + "SKYLANCE-README.txt") {
        Copy-Item -Path (Join-Path $Here $name) -Destination $Target -Force
    }
    if (-not (Test-Path (Join-Path $Target "..\mame.exe")) -and -not (Test-Path (Join-Path $Target "mame-path.txt"))) {
        Write-Host "No mame.exe next to $Target. Put MAME's full path in $Target\mame-path.txt,"
        Write-Host "or the launchers use the 'mame' on your PATH."
    }
    Write-Host ""
    Write-Host "Sky Lance $Version is installed in $Target"
    Write-Host "  run_skylance_mvs.bat   arcade (MVS), EagleBIOS"
    Write-Host "  run_skylance_aes.bat   console (AES), EagleBIOS"
    if (Test-Path $backup) { Write-Host "Earlier files: $backup" }
}
finally {
    Remove-Item -Recurse -Force -Path $work -ErrorAction SilentlyContinue
}
if (-not $NoPause) { Read-Host "Press Enter to close" | Out-Null }
