# This branch is selected before cartridge configuration or recipes are read.
.DEFAULT_GOAL := cd-image
GAME ?= helloworld
USE_2D_PLUS ?= 0
CD_BIOS ?= stock
CD_MODEL ?= cdz
ifeq ($(OS),Windows_NT)
CD_PYTHON ?= python
else
CD_PYTHON ?= python3
endif
ifneq ($(filter $(CD_BIOS),stock eagle),$(CD_BIOS))
$(error CD_BIOS must be stock or eagle)
endif
.PHONY: all cd-image cd-audit cd-test
all: cd-image
cd-image:
	$(CD_PYTHON) tools/neocd/build.py --game "$(GAME)" $(if $(CD_BUILD_DIR),--out "$(CD_BUILD_DIR)") $(if $(CD_CC),--cc "$(CD_CC)") $(if $(filter 1,$(USE_2D_PLUS)),--plus) $(if $(filter 0,$(CD_CDDA)),--no-cdda)
cd-audit:
	$(CD_PYTHON) tools/neocd/audit_disc.py "$(CD_IMAGE)"
cd-test:
	$(CD_PYTHON) tools/neocd/qa.py --build "$(CD_BUILD_DIR)" --bios "$(CD_BIOS_FILE)" --bios-mode "$(CD_BIOS)" --model "$(CD_MODEL)" --out "$(CD_QA_DIR)" $(if $(CD_MAME),--mame "$(CD_MAME)")
