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

#VOGL_OPTIONS     ?= --vogl_force_debug_context --vogl_exit_after_x_frames 2000
VOGL_OPTIONS     ?= --vogl_force_debug_context
VOGL             ?=
ifeq ($(VOGL),)
VOGL_CMD         ?=
else
VOGL_BIN         ?= vogl
VOGL_CMD         ?= $(VOGL_BIN) trace --vogl_tracepath $(BUILDDIR) --vogl_tracefile $@.trace.bin $(VOGL_OPTIONS)
ARGS_TMP         := $(ARGS)
ARGS              = "--args $(ARGS_TMP)"
endif

MAKE_PID := $$PPID
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))

all: build

run: shapetool

.PHONY: cmake
cmake:
	$(Q)mkdir -p $(BUILDDIR)
	$(Q)cd $(BUILDDIR); cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_INSTALL_PREFIX=./linux -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CURDIR)

.PHONY: build
build: cmake
	$(Q)cd $(BUILDDIR); make $(JOB_FLAG);
	$(Q)cd $(BUILDDIR); make install

clean:
	$(Q)rm -rf $(BUILDDIR)

clean-local-config:
	$(Q)rm -rf $(LOCAL_CONFIG_DIR)

edit-local-config:
	$(Q)$(EDITOR) $(LOCAL_CONFIG_DIR)/shapetool/shapetool.vars

server client shapetool shadertool noisetool tests: cmake
	$(Q)cd $(BUILDDIR); make $@ copy-data-shared copy-data-$@ $(JOB_FLAG)
	$(Q)cd $(BUILDDIR); $(GDB_CMD) $(VOGL_CMD) ./$@ $(ARGS)

shapetool2: shapetool clean-local-config

material-color: cmake
	$(Q)cd $(BUILDDIR); make tests $(JOB_FLAG)
	$(Q)cd $(BUILDDIR); $(GDB_CMD) ./tests --gtest_filter=MaterialTest* -- $(ARGS)
	$(Q)xdg-open build/material.png

test-ambient-occlusion: cmake
	$(Q)cd $(BUILDDIR); make tests $(JOB_FLAG)
	$(Q)cd $(BUILDDIR); $(GDB_CMD) ./tests --gtest_filter=AmbientOcclusionTest* -- $(ARGS)

.PHONY: remotery
remotery:
	$(Q)xdg-open file://$(CURDIR)/tools/remotery/index.html

.PHONY: tags
tags:
	$(Q)ctags -R src
