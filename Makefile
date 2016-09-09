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

VALGRIND         ?=
ifeq ($(VALGRIND),)
VALGRIND_CMD     ?=
else
VALGRIND_CMD     ?= valgrind
endif

# TODO lldb command support on osx
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

CMAKE_OPTIONS    ?=

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

OS           := $(shell uname)
MAKE_PID     := $$PPID
JOB_FLAG     := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
MAKE_OPTIONS := --no-print-directory -C $(BUILDDIR)

ifeq ($(OS),Darwin)
CMAKE_GENERATOR ?= "Xcode"
CMAKE_BINARY    ?= /Applications/CMake.app/Contents/bin/cmake
DARWIN          := 1
else
CMAKE_GENERATOR ?= "Eclipse CDT4 - Unix Makefiles"
CMAKE_BINARY    ?= cmake
LINUX           := 1
endif
INSTALL_DIR     ?= $(BUILDDIRPATH)$(OS)

all: build

run: shapetool

.PHONY: cmake
cmake:
	$(Q)mkdir -p $(BUILDDIR)
	$(Q)cd $(BUILDDIR); $(CMAKE_BINARY) -G$(CMAKE_GENERATOR) -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CURDIR) $(CMAKE_OPTIONS)

define COMPILE
$(if $(LINUX),\
	$(Q)$(MAKE) $(MAKE_OPTIONS) $(JOB_FLAG) $(1) \
$(else),\
	$(if $(DARWIN),\
		$(Q)cd $(BUILDDIR); xcodebuild build -target $(1) install -project tests.xcodeproj -configuration $(BUILD_TYPE) \
	$(else),\
		$(Q)$(MAKE) $(MAKE_OPTIONS) $(JOB_FLAG) $(1) \
	)
)
endef

.PHONY: build
build: cmake
	$(call COMPILE, install)

clean:
	$(Q)rm -rf $(BUILDDIR)

clean-local-config:
	$(Q)rm -rf $(LOCAL_CONFIG_DIR)

edit-local-config:
	$(Q)$(EDITOR) $(LOCAL_CONFIG_DIR)/shapetool/shapetool.vars

doc: cmake
	$(call COMPILE, $@)

server client shapetool shadertool noisetool databasetool uitool tests testmesh testcamera testdepthbuffer testtexture flatc: cmake
	$(call COMPILE, $@)
	$(call COMPILE, copy-data-shared)
	$(call COMPILE, copy-data-$@)
	$(Q)cd $(BUILDDIR); $(VALGRIND_CMD) $(GDB_CMD) $(VOGL_CMD) ./$@ $(ARGS)

rcon: cmake
	$(call COMPILE, $@)
	$(Q)cd $(BUILDDIR); $(VALGRIND_CMD) $(GDB_CMD) $(VOGL_CMD) ./$@ $(ARGS)

test-material-color: cmake
	$(call COMPILE, tests)
	$(Q)cd $(BUILDDIR); $(VALGRIND_CMD) $(GDB_CMD) ./tests --gtest_color=yes --gtest_filter=MaterialTest* -- $(ARGS)
	$(Q)xdg-open build/material.png

test-ambient-occlusion: cmake
	$(call COMPILE, tests)
	$(Q)cd $(BUILDDIR); $(VALGRIND_CMD) $(GDB_CMD) ./tests --gtest_color=yes --gtest_filter=AmbientOcclusionTest* -- $(ARGS)

.PHONY: remotery
remotery:
	$(Q)xdg-open file://$(CURDIR)/tools/remotery/index.html

.PHONY: tags
tags:
	$(Q)ctags -R src
