;==============================================================
; WLA-DX banking setup
;==============================================================
; do not complie still working on ....
.memorymap
defaultslot 0
slotsize $10000
slot 0 $0000
.endme

.rombankmap
bankstotal 1
banksize $10000
banks 1
.endro

.define STACK	     $FFFC
;.define RAM_ADDR     $F800


.bank 0 slot 0
.org $0000
origin:    
di               ;disable interrupt
jp main         ; jump to main program

.org  $0008
RTS0:
ret	

.org  $0010
RTS1:
push bc
push de
push hl
pop  hl
pop  de
pop  bc
ret

.org  $0014
RTS2:
call nc,$002C



.org  $0018  ;port A
RTS4:
push af
ld   a,d
out  ($04),a
rst  $08
ld   a,e
out  ($05),a
rst  $10
pop  af
ret

.org  $001B
RTS5:
ret

.org  $0020  ;port B
RTS6:
push af
ld   a,d
out  ($06),a
rst  $08
ld   a,e
out  ($07),a
rst  $10
pop  af
ret


.org  $0028      ;handle command
RTS7:
ld   a,($FE39)   ;command variable
ld   e,a
ld   d,$00
ld   hl,$0B18  ;handle commande table
add  hl,de
add  hl,de
add  hl,de
jp   (hl)  

.org  $002C ;sync
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

.org  $0038  ; mode 1 INT
IRQ:
di
jp   $1EB5
nop	
nop
ld   d,(hl)
ld   h,l
ld   (hl),d
jr   nz,$0074
ld   l,$35
ld   h,d
jr   nz,$00AA
ld   a,c
jr   nz,$0098
ld   (hl),d
ld   l,$50
ld   h,c
ld   h,e
jr   nz,$00BA
jr   nz,$0096
ld   h,l
ld   (hl),d
ld   l,$30
ld   ($302F),a
ld   sp,$302F
jr   c,$0060	


NMI:
.org $0066
push af
push bc
push de
push hl
in   a,($00)
ld   b,a
cp   $01
jp   z,$0B79   ;0x1 command
cp   $03
jp   z,$0B88   ;0x3 command
cp   $10       ;0x10 command
jp   z,$0B97
or   a
jp   z,$00B9
ld   a,($FE58)
or   a
jr   nz,$00A8
ld   a,$80
call $0B66
ld   a,($FE47)
and  $07
ld   e,a
ld   d,$00
ld   hl,$FE48
add  hl,de
add  hl,de
ld   (hl),$FF
inc  hl
ld   (hl),b
inc  a
and  $07
ld   ($FE47),a
xor  a
ld   ($FE2C),a
jr   $00B0
ld   a,b
ld   ($FE39),a
xor  a
ld   ($FE2C),a
ld   a,($FE69)
or   $80
out  ($0C),a    ;68K return value
out  ($00),a   ;clear sound
pop  hl
pop  de
pop  bc
pop  af
retn	









;==============================================================
; Main program
;==============================================================
.org $00D0
main:
ld   sp,STACK
im 1            ; Interrupt mode 1
xor  a
ld   ($FE66),a  ;init vars
ld   ($F800),a
ld   hl,$F800
ld   de,$F801
ld   bc,$07FF
ldir
ld   a,$FF
ld   ($FE2C),a
ld   ($FE30),a
ld   ($FE31),a
ld   a,$03
ld   ($FE25),a
call $09A3
call $09D5
call $09BC
ld   a,$01
out  ($C0),a
ld   de,$2730 ;FM reset
call $2936
ld   de,$1001 ;ADPCM-B  reset 
call $2936
ld   de,$1C00 ;flags on
call $2936
xor  a
ld   ($FE65),a
ld   ($FC33),a
ld   ($FE2F),a
ld   ($FE29),a
ld   ($FE28),a
ld   a,$03
call $2910
ld   a,$0A
ld   ($FE1E),a
call $15FC
out  ($08),a
infiniteloop:
xor  a
ld   ($FE58),a
ld   ($FE66),a
ei
ld   a,($FE46)
and  $07
ld   ($FE5C),a
ld   e,a
ld   d,$00
ld   hl,$FE48
add  hl,de
add  hl,de
inc  a
ld   ($FE46),a
ld   a,(hl)
or   a
call nz,$0169
ei
ld   a,($FE65)
or   a
jr   z,$0135
ld   a,r
ld   ($FE69),a
or   $80
out  ($0C),a
jp   infiniteloop


.org $0BAF
;0x1 ;BIOS Slot switch
slot_switch:
di
xor  a
out  ($0C),a   ;68K return value
ld   b,$30
djnz $0BB5 
xor  a
out  ($0C),a   ;68K return value
ld   ($FE2F),a ;init ram
ld   ($FE29),a
ld   ($FE28),a
di
ld   de,$B500  ;turn off  1
call writeport_a
call writeport_b
ld   de,$B600  ;turn off  2
call writeport_a
call writeport_b
ld   de,$00BF  ;ADPCM-A reset
call writeport_b
ld   de,$1001  ;ADPCM-B Reset
call writeport_a
ld   de,$1CBF  ;reset   ADPCM-A CH 1-6 and ADPCM-B flags
call writeport_a
ld   de,$1C00  ;reset   ADPCM-A CH 1-6 and ADPCM-B enable
call writeport_a
ld   b,$0D
ld   de,$417F
call writeport_a
call writeport_b
ex   de,hl
ld   de,$0100
add  hl,de
ex   de,hl
djnz $0BF3
ld   de,$2801    ;FM CH 1 off
call writeport_a
ld   de,$2802    ;FM CH 2 off
call writeport_a
ld   de,$2805    ;FM CH 5 off
call writeport_a
ld   de,$2806    ;FM CH 6 off
call writeport_a
ld   de,$0800    ; SSG CH A off
call writeport_a
ld   de,$0900    ; SSG CH B off
call writeport_a
ld   de,$0A00    ; SSG CH C off
call writeport_a
ld   hl,$FFFD  ;dynamic ram loop
ld   (hl),$C3  ;jp
ld   ($FFFE),hl
ld   a,$01
ld   ($FE66),a
out  ($0C),a ;68K return value
jp   $FFFD ;ram loop

