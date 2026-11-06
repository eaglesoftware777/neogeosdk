# Additive opt-in diagnostics; no cartridge target depends on these.
ifeq ($(OS),Windows_NT)
CD_PYTHON ?= python
else
CD_PYTHON ?= python3
endif
.PHONY: cd-unit-tests bios-cd cd-bios bios-cd-test
cd-unit-tests:
	$(CD_PYTHON) -m unittest discover -s tests -p "test_cd_*.py" -v
bios-cd cd-bios:
	$(CD_PYTHON) bios/cd/tools/build.py $(if $(CD_BIOS_BUILD_DIR),--out "$(CD_BIOS_BUILD_DIR)") $(if $(CD_CC),--cc "$(CD_CC)")
bios-cd-test:
	$(CD_PYTHON) bios/cd/tools/qa.py --firmware "$(CD_BIOS_BUILD_DIR)" --model "$(or $(CD_MODEL),cdz)" --out "$(CD_QA_DIR)" $(if $(CD_MAME),--mame "$(CD_MAME)")
