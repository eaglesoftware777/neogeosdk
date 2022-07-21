# neogeosdk
Neo Geo SDK
<br/>
<br/>
https://eaglesoftware.biz
<br/>
https://github.com/eaglesoftware777
<br/>
https://github.com/eaglesoftware777/neogeosdk
<br/><br/>
Requirement:
<br/>
 <br/>
Ubuntu Linux   ==> https://ubuntu.com/
<br/>
m68k compiler  ==> https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.0/x-tools.tar
<br/>
mame           ==> https://www.mamedev.org/
<br/>
srecord        ==> https://packages.ubuntu.com/search?keywords=srecord
<br/>
python 2.7     ==> https://www.python.org/download/releases/2.7/
<br/>
numpy          ==> https://numpy.org/
<br/>
pypng          ==> https://pypi.org/project/pypng/
<br/>
sqlite3        ==> https://www.sqlite.org/index.html
<br/>
<br/>
Optional :
<br/>
<br/>
Geany          ==> https://www.geany.org/
<br/>
<br/>
Installation :
<br/><br/>
sudo apt-get install mame<br/>
sudo apt-get install srecord<br/>
sudo apt-get install python2<br/>
pip install numpy<br/>
pip install pypng<br/><br/>
Put sdk    on ~/neogeosdk<br/>
Put xtools on ~/x-tools<br/>
<br/>
Compilation/Test : <br/><br/>
cd ~/neogeosdk<br/>
compile      : make<br/>
test         : make test<br/>
debug        : make debug<br/>
dump         : make dump<br/>
artbox       : make art<br/>
clean        : make clean<br/>
artbox clean : make art-clean<br/>
<br/>
<br/>
<br/>
#
Windows : 
<br/>
<br/>
install sysprogs m68k compiler for windows
<br/>
m68k compiler : https://sysprogs.com/files/gnutoolchains/m68k-elf/m68k-elf-gcc4.8.0.exe
<br/>
install python 2.7  :https://www.python.org/ftp/python/
<br/>
<br/>
install python required package
<br/>
py -2.7 -m pip install numpy
<br/>
py -2.7 -m pip install pypng
<br/>
py -2.7 -m pip install sqlite3
<br/>
<br/>
Change SDKHOME variable in the makefile and artbox bat files such as :
<br/>
SDKHOME = Windows folder containing  neogeosdk 
<br/>
Default is set to C: driver
<br/>
SDKHOME=C: 
<br/>
Path to neogeo sdk : c:\neogeosdk
<br/>
<br/>
srec_cat.exe and xxd.exe are in the folder neogeosdk\win 
<br/>
srec_cat.exe  : http://srecord.sourceforge.net/download.html
<br/>
xxd.exe is used for  dumping the rom, you can change it with an equivalent tool or another windows version :  (https://ftp.nluug.nl/pub/vim/pc/gvim73_46_s.zip)
<br/>
xxd.exe were taken from :  https://sourceforge.net/projects/xxd-for-windows/ 
<br/>
<br/>
Uncompress/install mame in a folder , the makefile use %SDKHOME%\mame\mame.exe
<br/>
You can change the makefile with your mame.exe path to test the rom
<br/>
<br/>
Compilation/Test : <br/><br/>
cd %SDKHOME%\neogeosdk<br/>
compile : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak
<br/>
test : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak test
<br/>
debug : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak debug
<br/>
dump : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak dump
<br/>
artbox : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak art
<br/>
clean : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak clean
<br/>
artbox clean : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak art-clean





