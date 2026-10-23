# EagleBIOS-CD 1.7.7 — independent experimental disc loader

**Native CDZ disc boot, runtime asset loading and CDDA have passed emulator probes.**
This is a separately linked 512 KiB CD system ROM. It is not the cartridge BIOS
padded to a different size. Existing cartridge sources, vectors, ROM names and
outputs are unchanged. No stock firmware, artwork, sound driver or patches are
embedded in this implementation.

From the SDK root:

```sh
make bios-cd CD_BIOS_BUILD_DIR=/new/firmware-build
make bios-cd-test CD_BIOS_BUILD_DIR=/absolute/firmware-build \
  CD_MODEL=cdz CD_QA_DIR=/new/isolated-firmware-test
```

The Windows makefile supports the same aliases. Without an exact output path,
builds go to a new timestamped `out/neocd/bios/` directory. Every build produces
`eagle_cd_front.rom`, `eagle_cd_top.rom`, `eagle_cd_cdz.rom`, each exactly 524,288
bytes, plus ELF/map/hash metadata. Both Linux and native Windows builds were
executed and produced byte-identical firmware.

The three profiles currently differ only in identification/menu text. They do
not establish three optical-drive implementations. `qa.py` maps only our ROM
and our SDK-original LO data into an isolated MAME directory, compares all
loaded ROM bytes, checks input/frame/stack/PC behavior and saves screenshots.
No existing MAME configuration, save, installed game or cartridge BIOS is replaced.

The independent transport implements checksummed drive packets, bounded TOC and
sector reads, CDC/DMA transfer, flat ISO9660/IPL parsing and native PRG, SPR, FIX,
Z80 and PCM uploads. Maiya and Sky Lance reached gameplay with input, sound
effects and CDDA in MAME 0.283 using this ROM. The same native game files also
boot with supplied stock CDZ firmware. Disc packages may have separate volume
labels and launch profiles; neither package contains a system BIOS.

The runtime file service was checked by loading a second sprite bank and
comparing its bytes through the native upload window. Caller registers and
interrupt-mask restoration, missing files, invalid banks and unsupported
executable overlays were also checked. This is partial service conformance,
not a complete replacement BIOS. Standalone UPLOAD, generic HAL file/audio
integration, virtual-card saving, tray/media changes and physical hardware
compatibility remain unfinished or unverified. Front/top optical behavior has
not passed the CDZ game tests. Do not flash this build as finished firmware.

- [Architecture and status](../../docs/EAGLEBIOS_CD.md)
- [ABI and capabilities](../../docs/EAGLEBIOS_CD_ABI.md)
- [Hardware register ledger](../../docs/EAGLEBIOS_CD_HARDWARE.md)
- [Validation evidence](../../docs/roadmap/EAGLEBIOS_CD_VALIDATION_REPORT.md)
