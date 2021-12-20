set SDKHOME=C:
..\win\romtool_x64.exe /p 1c.s1 1p.s1 2048 0
..\win\romtool_x64.exe /f 1p.s1 052-s1.s1
copy 052-s1.s1 %SDKHOME%\neogeosdk\roms\ssideki
