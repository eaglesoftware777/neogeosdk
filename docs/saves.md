# Saves: backup RAM and the memory card

All of this is in `sdk/ng_system.h`, as `static inline` functions: a game that doesn't call them pays nothing for them.

## The save block

The cartridge header names a block of work RAM for the game: its start at 0x10E and its size at 0x112, 4 KB at most.

| Bytes | Contents |
|---|---|
| 0–1 | the system's debug switches (left alone) |
| 2–5 | the tag `SAVE` |
| 6–7 | the game's NGH number |
| 8 | the layout version |
| 9 | (pad) |
| 10–11 | the data size |
| 12–13 | Fletcher-16 checksum of the data |
| 14… | the game's data |

- `ng_save_format(version, size)` starts the block afresh.
- `ng_save_valid(version, size)` tells the game's own whole data from an empty or foreign block.
- `ng_save_commit()` reseals the block after a change.
- `ng_save_data()` points at the data.

**On an arcade board (MVS)** the system ROM loads the block from backup RAM before it calls the game, and writes it back afterwards. **On a console** it only lasts while the machine is on, unless a memory card keeps it.

## The memory card

Through the system ROM's CARD routine at `$C00468` (neogeodev wiki: "CARD", "Memory card"):

| Call | What it does |
|---|---|
| `ng_card_save(title, buf, buf_size)` | writes the sealed block to the card, behind a 20-character title |
| `ng_card_load(version, size, buf, buf_size)` | reads it back, and puts it in the block only when it holds this game's data at this version and size, whole |
| `ng_card_call(command, data, size)` | the routine itself, for other commands |

`buf` is the caller's room for the card file: `NG_CARD_FILE_SIZE(data size)` bytes.

**How the file is stored:**
- The game's NGH number and sub-number 0 name the card file.
- Its first 20 bytes are the title, which a console's card manager shows. Then come the block's header and data.
- The size is rounded up to whole 64-byte card blocks. With a size that isn't a whole number of blocks, the AES system ROM reads back only the first block (MAME 0.264).

**Formatting:** a card that answers "unformatted" holds nothing, so `ng_card_save` formats it first. A formatted card is never formatted.

**Answers:** `NG_CARD_OK`, `NG_CARD_NONE` (no card in the slot), `NG_CARD_UNFORMATTED`, `NG_CARD_NO_DATA`, `NG_CARD_FAT_ERROR`, `NG_CARD_FULL`, `NG_CARD_PROTECTED`.

**What the calls need from the caller:**
- **Registers:** the routine keeps none, so the call saves them all.
- **Machine:** ask `ng_sys_is_mvs()` first, since an arcade board keeps the block in backup RAM.

**One trap:**
- The copies between the card buffer and the save block are done in assembly (`ng_copy_bytes`).
- As a C loop between two buffers, GCC at -O2 folded the copy into an addressing mode the 68000 works out after the increment, and every byte landed one along.

## Maiya

Arcade boards are as before. On a console:
- **Power-on:** it reads its save from the card. Both its init and its reset do, because a console's system ROM calls the reset at every power-on.
- **Writes:** it writes to the card when something worth keeping is sealed: a game started, a name entered, the journey finished. It never writes just the defaults.
- **The file:** `MAIYA SCORES`, 128 bytes, two card blocks.

Checked in MAME's AES driver (`-memc`) on the AES system ROM (asia) and on UniBIOS 4.0:
- a fresh card is formatted and saved;
- the save is back after a power-on: the count of games played went 1, 1, 2, 2 over four power-ons;
- with no card the game runs on, the card answering "no card".

## Not done

- **EagleBIOS's CARD routine** answers "no card" (`bios/src/bios_card.c`). With it, a console keeps its save for the session only.
- **Hardware:** a real AES with a card is not verified.
