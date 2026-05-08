# Makefile Integration Notes

The current NeoGeoSDK Makefile is mostly a long single-line command chain. Because of that, an automatic patch can be fragile.

Add these new object steps after `sdk/neogeolib.c`:

```make
$(CC) $(CFLAGS) sdk/ng_defs.c -o out/ng_defs0.o
$(CC) $(CFLAGS) sdk/ng_properties.c -o out/ng_properties0.o
$(CC) $(CFLAGS) sdk/ng_game_time.c -o out/ng_game_time0.o
$(CC) $(CFLAGS) sdk/ng_timers.c -o out/ng_timers0.o
$(CC) $(CFLAGS) sdk/ng_progress.c -o out/ng_progress0.o
$(CC) $(CFLAGS) sdk/ng_status.c -o out/ng_status0.o
$(CC) $(CFLAGS) sdk/ng_game_events.c -o out/ng_game_events0.o
$(CC) $(CFLAGS) sdk/ng_sprite_group.c -o out/ng_sprite_group0.o
$(CC) $(CFLAGS) sdk/ng_actions.c -o out/ng_actions0.o
$(CC) $(CFLAGS) sdk/ng_chars.c -o out/ng_chars0.o
$(CC) $(CFLAGS) sdk/ng_border_constraints.c -o out/ng_border_constraints0.o
$(CC) $(CFLAGS) sdk/ng_game_interupt.c -o out/ng_game_interupt0.o
```

Add these objcopy steps:

```make
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_defs0.o out/ng_defs.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_properties0.o out/ng_properties.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_game_time0.o out/ng_game_time.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_timers0.o out/ng_timers.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_progress0.o out/ng_progress.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_status0.o out/ng_status.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_game_events0.o out/ng_game_events.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_sprite_group0.o out/ng_sprite_group.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_actions0.o out/ng_actions.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_chars0.o out/ng_chars.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_border_constraints0.o out/ng_border_constraints.o
$(OBJCP) -R .comment -R .text -R .data -R .bss out/ng_game_interupt0.o out/ng_game_interupt.o
```

Add these to the linker command:

```make
out/ng_defs.o out/ng_properties.o out/ng_game_time.o out/ng_timers.o out/ng_progress.o out/ng_status.o out/ng_game_events.o out/ng_sprite_group.o out/ng_actions.o out/ng_chars.o out/ng_border_constraints.o out/ng_game_interupt.o
```

## NeoGeoSDK integration note

This SDK does not use normal C static storage for the main game objects by default.

For the runtime layer to work correctly here:

1. compile the runtime layer objects as normal `.o` files
2. link their `neogeo_user` text and `.rodata` into the existing ROM area
3. place their `.bss` and `.data` into a RAM-backed linker section
4. keep `WORK_INIT()` or `clearRAM()` zeroing that RAM before the scene starts

In this repository, the runtime objects are linked with a dedicated RAM section
starting at `0x100800`, so the internal tables for characters, actions,
properties, timers, and events stay writable at runtime.
