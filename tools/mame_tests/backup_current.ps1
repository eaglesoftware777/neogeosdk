$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$stamp = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-ffffff')
$dest = Join-Path $root "backups\$stamp"
New-Item -ItemType Directory -Path $dest | Out-Null
Get-ChildItem -LiteralPath $root -Force | Where-Object { $_.Name -ne 'backups' } | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $dest -Recurse -Force
}
if ((Test-Path (Join-Path $dest 'tests\mvs\roms\maiya')) -and (Test-Path (Join-Path $dest 'tests\mvs\hash\neogeo.xml'))) {
    Write-Host 'Backup includes current MVS and AES test sets.'
}
Set-Content -LiteralPath (Join-Path $root 'latest-backup.txt') -Value $stamp -Encoding ASCII
Write-Host "Backed up current version to $dest"
