@echo off
setlocal
set "HERE=%~dp0"
cd /d "%HERE%"

echo ============================================================
echo   Eagle Software - Flashcart ^& SD Card Image Downloader
echo   NeoSD, Darksoft Multi, and BackBit Platinum Images
echo ============================================================
echo.

set "PY_EXE="
for %%P in (python.exe py.exe) do (
    if not defined PY_EXE (
        where %%P >nul 2>&1 && set "PY_EXE=%%P"
    )
)

if not defined PY_EXE (
    if exist "C:\Users\%USERNAME%\AppData\Local\Programs\Python\Python311\python.exe" (
        set "PY_EXE=C:\Users\%USERNAME%\AppData\Local\Programs\Python\Python311\python.exe"
    ) else if exist "C:\Users\%USERNAME%\AppData\Local\Programs\Python\Python312\python.exe" (
        set "PY_EXE=C:\Users\%USERNAME%\AppData\Local\Programs\Python\Python312\python.exe"
    ) else if exist "C:\Users\%USERNAME%\AppData\Local\Programs\Python\Python313\python.exe" (
        set "PY_EXE=C:\Users\%USERNAME%\AppData\Local\Programs\Python\Python313\python.exe"
    ) else if exist "C:\Python311\python.exe" (
        set "PY_EXE=C:\Python311\python.exe"
    ) else if exist "C:\Python312\python.exe" (
        set "PY_EXE=C:\Python312\python.exe"
    )
)

if defined PY_EXE (
    if exist "%HERE%download_sd.py" (
        "%PY_EXE%" "%HERE%download_sd.py" %*
        if errorlevel 1 pause
        exit /b %ERRORLEVEL%
    )
)

echo [INFO] Python not found, launching PowerShell downloader...
echo.
if not exist "%HERE%sd_images\neosd" mkdir "%HERE%sd_images\neosd"
if not exist "%HERE%sd_images\darksoft" mkdir "%HERE%sd_images\darksoft"
if not exist "%HERE%sd_images\backbit" mkdir "%HERE%sd_images\backbit"

powershell -ExecutionPolicy Bypass -Command "& { Write-Host 'Downloading NeoSD, Darksoft, and BackBit images from official GitHub release...'; $base = 'https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.2'; Invoke-WebRequest -Uri ($base + '/Maiya-WIP-NeoSD_MVS_v3.neo') -OutFile '%HERE%sd_images\neosd\Maiya-NeoSD-MVS.neo'; Invoke-WebRequest -Uri ($base + '/Maiya-WIP-NeoSD_AES_v3.neo') -OutFile '%HERE%sd_images\neosd\Maiya-NeoSD-AES.neo'; Invoke-WebRequest -Uri ($base + '/Maiya-Darksoft-MVS_v3.zip') -OutFile '%HERE%sd_images\darksoft\Maiya-Darksoft-MVS_v3.zip'; Invoke-WebRequest -Uri ($base + '/Maiya-Darksoft-AES_v3.zip') -OutFile '%HERE%sd_images\darksoft\Maiya-Darksoft-AES_v3.zip'; Invoke-WebRequest -Uri ($base + '/Maiya-BackBit-MVS_v3.zip') -OutFile '%HERE%sd_images\backbit\Maiya-BackBit-MVS_v3.zip'; Invoke-WebRequest -Uri ($base + '/Maiya-BackBit-AES_v3.zip') -OutFile '%HERE%sd_images\backbit\Maiya-BackBit-AES_v3.zip'; Write-Host 'All flashcart images downloaded to %HERE%sd_images\'; }"

pause
exit /b 0
