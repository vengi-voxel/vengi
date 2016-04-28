TARGET=
VERBOSE=
Q=@
LOCAL_CONFIG_DIR=~/.local/share/engine
BUILDDIR=build
all: run

.PHONY: build
build:
	$(Q)mkdir -p $(BUILDDIR); cd $(BUILDDIR); cmake -G"Eclipse CDT4 - Unix Makefiles" ..; make

clean:
	$(Q)git clean -fdx .

clean-local-config:
	$(Q)rm -r $(LOCAL_CONFIG_DIR)

edit-local-config:
	$(Q)$(EDITOR) $(LOCAL_CONFIG_DIR)/shapetool/shapetool.vars

server: build
	$(Q)./server $(ARGS)

client: build
	$(Q)./client $(ARGS)

shapetool: build
	$(Q)./shapetool -set voxel-plainterrain false $(ARGS)

run: shapetool

runfast: build
	$(Q)./shapetool -set voxel-plainterrain true $(ARGS)

tests: build
	$(Q)./tests -- $(ARGS)

remotery:
	$(Q)xdg-open file://$(CURDIR)/tools/remotery/index.html

.PHONY: tags
tags:
	$(Q)ctags -R src
