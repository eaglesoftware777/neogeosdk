# Portable Shadow League source sheets

This directory contains the original source sheets that previously lived outside
the repository. It replaces an absolute symlink into a workstation home directory.
Normal builds consume the existing indexed exports and do not reimport these sheets.

The approved replacement Jack sheets are in `jack-corrected/`: jack_s1.png through
jack_s7.png. The older Jack sheets in other
source directories are historical references and must not replace his current
boxing animation. To run an explicit new import into an empty scratch directory:

```sh
python3 games/shadowleague/tools/painted_import/import_fighter.py jack \
  --sources games/shadowleague/assets/authored/sdk177/sources/jack-corrected \
  --folder /tmp/shadowleague-jack-review --out "$PWD"
```

The importer reads only the PNG sheets: pass the source directory explicitly,
as above. Preserve the approved indexed frames during normal builds.

`earlier-art/` preserves the earlier external fighter sheets, portraits and stage
masters; `concept-kit/` preserves the original project concept kit. They are
historical sources, not automatic replacements for current approved exports.
