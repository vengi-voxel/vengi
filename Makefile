-include Makefile.local

# Building
## Release or Debug
#
#  make BUILD_TYPE=Debug
#  make BUILD_TYPE=Release
#
# To build either in debug mode, or in release mode
#
# Debugging
## Run in gdb
#
#  make tests GDB=1 ARGS=--gtest_filter=World*
#
# to only build the tests targets, and run it via gdb afterwards.
# It will automatically start the execution and forward the arguments
# that are given via ARGS to the target
#
## Verbose output
#
#  make Q=
#
# This will print out information about the commands that are executed to build
# and run the target(s)

Q                 = @
LOCAL_CONFIG_DIR  = ~/.local/share/engine
GDB              ?=
ifeq ($(GDB),)
GDB_CMD          ?=
else
GDB_CMD          ?= gdb -ex run --args
endif
BUILD_TYPE       ?= Debug
# override this in your Makefile.local to use a different directory
BUILDDIRPATH     ?= ./
BUILDDIR         ?= $(BUILDDIRPATH)build-$(shell echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]')

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
	$(Q)cd $(BUILDDIR); $(GDB_CMD) ./$@ $(ARGS)

shapetool2: clean-local-config
	$(Q)cd $(BUILDDIR); make shapetool copy-data-shapetool copy-data-shared $(JOB_FLAG)
	$(Q)cd $(BUILDDIR); $(GDB_CMD) ./shapetool

material-color:
	$(Q)cd $(BUILDDIR); make tests $(JOB_FLAG)
	$(Q)cd $(BUILDDIR); $(GDB_CMD) ./tests --gtest_filter=MaterialTest* -- $(ARGS)
	$(Q)xdg-open build/material.png

test-ambient-occlusion:
	$(Q)cd $(BUILDDIR); make tests $(JOB_FLAG)
	$(Q)cd $(BUILDDIR); $(GDB_CMD) ./tests --gtest_filter=AmbientOcclusionTest* -- $(ARGS)

.PHONY: remotery
remotery:
	$(Q)xdg-open file://$(CURDIR)/tools/remotery/index.html

.PHONY: tags
tags:
	$(Q)ctags -R src
