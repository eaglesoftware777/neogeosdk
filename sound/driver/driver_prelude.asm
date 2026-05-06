;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Neo Geo YM2610 sound driver prelude for split C build
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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

.define FIFO_BUF     $F800
.define FIFO_WRITE   $F821
.define READY_VALUE  $01

.bank 0 slot 0
.org $0000
.include "driver_prelude.inc"
