set SDKHOME=C:
..\win\romtool_x64.exe /p 1c.c1 1p.c1 2048 0
..\win\romtool_x64.exe /p 2c.c2 2p.c2 2048 0
..\win\romtool_x64.exe /f 1p.c1 052-c1.c1
..\win\romtool_x64.exe /f 2p.c2 052-c2.c2
copy 052-c1.c1  %SDKHOME%\neogeosdk\roms\ssideki
copy 052-c1.c1  %SDKHOME%\neogeosdk\roms\ssideki

