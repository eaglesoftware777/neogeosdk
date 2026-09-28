MAIYA: SUPER NATURE GIRL  --  Eagle Software, 2026
===================================================

!! WORK IN PROGRESS !!  This is a preview build, not a finished game.
Expect rough edges, placeholder art in places, and balance that is still
being tuned. Thank you for playing and testing it!

THE STORY (a few hints)
-----------------------
Lord Smoggar and his guardians have poisoned twelve valleys: forests cut,
falls choked with sludge, coasts drowned in plastic, skies full of smog.
Their machines -- saw bots, drill bots, torch bots, smoke stacks -- and
the trash beasts do the dirty work for them.
Maiya (or Luna) sets out to heal each valley: free the captives held in
chests, find the Sun Key, and pass through the ancient gate at the end of
the road to face its guardian. Each healed valley blooms again, and the
elder has a word for the road ahead. Sunboy waits in his crystal...
Look for hidden vaults: spirit orbs there teach new Secret Arts.

CONTROLS
--------
A  jump (Up + A in the air: a second jump while a sky lily lasts;
   kneel, then Up + A: the high leap)
B  thorn / whip up close (hold: run)
C  dash
D  Secret Art (uses a rose charge)
Forward, Down, Down-Forward + B : Rising Bloom
Down, Forward + B               : Rose Blossom Surge

WHAT'S IN THIS PACKAGE
----------------------
Maiya-WIP-EagleBIOS.zip   the game's ROMs plus EagleBIOS, the free
                          open-source replacement system firmware, for MAME:
                            mame neogeo -rompath roms -hashpath hash -bios euro -cart1 maiya
                          (checksum warnings for the replacement firmware
                          are expected)
Maiya-WIP-NeoSD.neo       for the NeoSD flash cart: copy it to the SD card.
                          It runs on the cart's own system BIOS support.

No SNK system BIOS is included in anything here.

ENGINE WORK IN THIS BUILD
-------------------------
- Eagle Software NeoGeoSDK 2D engine: camera, physics, sprite groups,
  palette effects (valleys that brighten as they heal, colour-shaking arts)
- painted landmarks and creatures, particle showers, formation flight
- synthesized voices for both heroines, ADPCM music and effects
- one ROM that runs on arcade (MVS) and console (AES)
Coming next: a platform layer for MVS / AES / Neo Geo CD, a locked frame
rate, raster effects (rippling water), more polish everywhere.
