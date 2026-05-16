if "%GAME_ID%"=="" set GAME_ID=777
if "%GAME%"=="" set GAME=demo
for %%i in ("%~dp0..") do set REPO_ROOT=%%~fi
set GAME_ARTBOX=%REPO_ROOT%\games\%GAME%\artbox
set PY_SCRIPT=%TEMP%\ng_pad_%RANDOM%_%RANDOM%.py
> "%PY_SCRIPT%" echo import os, sys
>> "%PY_SCRIPT%" echo game_id = os.environ.get("GAME_ID","777")
>> "%PY_SCRIPT%" echo game_artbox = os.environ["GAME_ARTBOX"]
>> "%PY_SCRIPT%" echo target = 8192 * 1024
>> "%PY_SCRIPT%" echo def pad_copy(src, dst):
>> "%PY_SCRIPT%" echo^    with open(src, "rb") as f:
>> "%PY_SCRIPT%" echo^        data = f.read()
>> "%PY_SCRIPT%" echo^    if len(data) ^< target:
>> "%PY_SCRIPT%" echo^        data = data + ^(b"\x00" * ^(target - len(data)^)^)
>> "%PY_SCRIPT%" echo^        action = "padded"
>> "%PY_SCRIPT%" echo^    else:
>> "%PY_SCRIPT%" echo^        action = "kept"
>> "%PY_SCRIPT%" echo^        if len(data) ^> target:
>> "%PY_SCRIPT%" echo^            over = len(data) - target
>> "%PY_SCRIPT%" echo^            tiles_over = over // 64
>> "%PY_SCRIPT%" echo^            print(f"WARNING: {src} exceeds 8MB target by {over} bytes (~{tiles_over} Cx tile rows). Keeping full size.")
>> "%PY_SCRIPT%" echo^    with open(dst, "wb") as f:
>> "%PY_SCRIPT%" echo^        f.write(data)
>> "%PY_SCRIPT%" echo^    print(f"{action}: {src} -^> {dst} ({len(data)} bytes)")
>> "%PY_SCRIPT%" echo c1_in = os.path.join(game_artbox, "1c.c1")
>> "%PY_SCRIPT%" echo c2_in = os.path.join(game_artbox, "2c.c2")
>> "%PY_SCRIPT%" echo c1_out = os.path.join(game_artbox, f"{game_id}-c1.c1")
>> "%PY_SCRIPT%" echo c2_out = os.path.join(game_artbox, f"{game_id}-c2.c2")
>> "%PY_SCRIPT%" echo pad_copy(c1_in, c1_out)
>> "%PY_SCRIPT%" echo pad_copy(c2_in, c2_out)
set GAME_ARTBOX=%GAME_ARTBOX%
py "%PY_SCRIPT%" || (del /Q "%PY_SCRIPT%" & exit /b 1)
del /Q "%PY_SCRIPT%"
if not exist "%REPO_ROOT%\roms\%GAME%" mkdir "%REPO_ROOT%\roms\%GAME%"
copy /Y "%GAME_ARTBOX%\%GAME_ID%-c1.c1" "%REPO_ROOT%\roms\%GAME%\%GAME_ID%-c1.c1"
copy /Y "%GAME_ARTBOX%\%GAME_ID%-c2.c2" "%REPO_ROOT%\roms\%GAME%\%GAME_ID%-c2.c2"
