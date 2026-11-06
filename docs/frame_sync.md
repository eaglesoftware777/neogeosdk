# Optional completed-frame publication

`2d_engine/ng_frame_sync.h` provides a one-byte publication handshake between
one game-loop producer and its game-owned VBlank interrupt on the single-core
68000. It does not change existing games, interrupt vectors, writer deadlines,
queue sizes, ROM banking or the `NGCharacter` layout.

Zero-initialize an `NGFrameSync`. Finish all sprite, FIX, palette and queue
mutations before `ng_frame_sync_publish(&sync)`. A return of0 means the previous
batch is still pending: do not edit its data. Wait until `sync.pending` clears
before starting another producer batch. An old VBlank wakeup flag alone does not
acknowledge the new batch.

The game-owned interrupt must check reentrancy/busy and the safe write window
**before** `ng_frame_sync_take(&sync)`. A return of0 means no completed batch is
available; do not flush a producer that may still be editing. After taking, commit
the palette and deferred sprite/FIX jobs using their normal measured deadlines.
The main loop resumes after that interrupt returns. Uploads which do not fit
remain queued for a future completed batch; acknowledgement is ownership of the
batch, not a promise that every queued decoration was uploaded.

This permits starting a bounded commit before BIOS input polling spends part of
the blank. BIOS-owned interrupt modes still need the game's established fallback.
It is opt-in: preserve the required BIOS I/O, watchdog, sound, raster and system
bookkeeping. Never publish a partially updated frame, consume while a writer is
busy, bypass a deadline, or consume a batch in the middle of normal gameplay.

The compiler barriers assume a single-core interrupt model. This header is not a
mutex, a multicore/thread synchronization API or a multi-buffer job queue. Native
hardware and emulator write-window validation remain required for each adopter.

Host regression: `python3 -m unittest discover -s tests -p 'test_frame_sync.py'`.
It injects interrupts around each producer mutation, checks duplicate publication,
stale wakeups, busy/closed-window deferral and exactly-once consumption.
