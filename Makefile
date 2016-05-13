-include Makefile.local

Q                 = @
LOCAL_CONFIG_DIR  = ~/.local/share/engine
BUILDDIR         ?= build
GDB              ?=
BUILD_TYPE       ?= Debug

MAKE_PID := $$PPID
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))

all: build

run: shapetool

.PHONY: build
build:
	$(Q)mkdir -p $(BUILDDIR)
	$(Q)cd $(BUILDDIR); cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_INSTALL_PREFIX=./linux -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CURDIR)
	$(Q)cd $(BUILDDIR); make $(JOB_FLAG);
	$(Q)cd $(BUILDDIR); make install

clean:
	$(Q)rm -rf $(BUILDDIR)

clean-local-config:
	$(Q)rm -rf $(LOCAL_CONFIG_DIR)

edit-local-config:
	$(Q)$(EDITOR) $(LOCAL_CONFIG_DIR)/shapetool/shapetool.vars

server client shapetool tests:
	$(Q)cd $(BUILDDIR); make $@ $(JOB_FLAG)
	$(Q)cd $(BUILDDIR); $(GDB) ./$@ $(ARGS)

shapetool2: clean-local-config
	$(Q)cd $(BUILDDIR); make shapetool copy-data-shapetool copy-data-shared $(JOB_FLAG)
	$(Q)cd $(BUILDDIR); $(GDB) ./shapetool

material-color:
	$(Q)cd $(BUILDDIR); make tests $(JOB_FLAG)
	$(Q)cd $(BUILDDIR); $(GDB) ./tests --gtest_filter=MaterialTest* -- $(ARGS)
	$(Q)xdg-open build/material.png

test-ambient-occlusion:
	$(Q)cd $(BUILDDIR); make tests $(JOB_FLAG)
	$(Q)cd $(BUILDDIR); $(GDB) ./tests --gtest_filter=AmbientOcclusionTest* -- $(ARGS)

.PHONY: remotery
remotery:
	$(Q)xdg-open file://$(CURDIR)/tools/remotery/index.html

.PHONY: tags
tags:
	$(Q)ctags -R src
