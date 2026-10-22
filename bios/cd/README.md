# EagleBIOS-CD 1.7.7 — original diagnostic firmware

**F1 only: menu, input, VBlank and bounded execution. Disc boot is not implemented.**
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

The first meaningful optical-drive gate is F2: TOC plus sector reads, invalid
media, lid/tray and bounded timeouts. F3 must boot the same native SDK disc;
F4 must perform a subsequent file load and CDDA; F5 must record audio and prove
cold-restart saving. None has passed. Do not use this diagnostic as a hardware
replacement or distribute it as finished CD firmware.

- [Architecture and status](../../docs/EAGLEBIOS_CD.md)
- [ABI and capabilities](../../docs/EAGLEBIOS_CD_ABI.md)
- [Hardware register ledger](../../docs/EAGLEBIOS_CD_HARDWARE.md)
- [Validation evidence](../../docs/roadmap/EAGLEBIOS_CD_VALIDATION_REPORT.md)
