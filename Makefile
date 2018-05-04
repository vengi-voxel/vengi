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
## Run in debugger
#
#  make tests DEBUG=1 ARGS=--gtest_filter=World*
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
OS               := $(shell uname)
LOCAL_CONFIG_DIR  = ~/.local/share/engine
UPDATEDIR        := /tmp

VALGRIND         ?=
ifeq ($(VALGRIND),)
VALGRIND_OPTIONS ?=
VALGRIND_CMD     ?=
else
VALGRIND_OPTIONS ?=
VALGRIND_CMD     ?= valgrind $(VALGRIND_OPTIONS)
endif

PERF             ?=
ifeq ($(PERF),)
PERF_OPTIONS     ?=
PERF_CMD         ?=
PERF_REPORT_CMD  ?=
else
PERF_OPTIONS     ?= --call-graph dwarf
#sudo sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'
PERF_CMD         ?= perf record $(PERF_OPTIONS)
PERF_REPORT_CMD  ?= perf report -g srcline -s dso,sym,srcline --inline -n --stdio
endif

DEBUG            ?=
ifeq ($(DEBUG),)
DEBUG_CMD        ?=
else
DEBUGGER         := $(shell (gdb --help >/dev/null 2>&1 && echo GDB) || (lldb --help >/dev/null 2>&1 && echo LLDB))
ifeq ($(DEBUGGER),GDB)
DEBUG_CMD        ?= gdb -ex run --args
else ifeq ($(DEBUGGER),LLDB)
DEBUG_CMD        ?= lldb -b -o run
else
DEBUG_CMD        ?=
endif
endif

CMAKE_OPTIONS    ?=

GPROF            ?=
GCOV             ?=
BUILD_TYPE       ?= Debug
# override this in your Makefile.local to use a different directory
BUILDDIRPATH     ?= ./
#BUILDDIR         ?= $(BUILDDIRPATH)build-$(shell echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]')
ifeq ($(GPROF),)
ifneq ($(GCOV),)
BUILDDIR         ?= $(BUILDDIRPATH)build/$(BUILD_TYPE)/gcov
CMAKE_OPTIONS    += -DUSE_GCOV=True
else
BUILDDIR         ?= $(BUILDDIRPATH)build/$(BUILD_TYPE)
endif
else
BUILDDIR         ?= $(BUILDDIRPATH)build/$(BUILD_TYPE)/gprof
CMAKE_OPTIONS    += -DUSE_GPROF=True
endif


ifneq ($(THREADS),)
BUILDDIR         ?= $(BUILDDIRPATH)build/$(BUILD_TYPE)/threads
CMAKE_OPTIONS    += -DSANITIZER_THREADS=True
endif

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

MAKE_PID     := $$PPID
JOB_FLAG     := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
MAKE_OPTIONS := --no-print-directory -C $(BUILDDIR)

ifeq ($(OS),Darwin)
CMAKE_GENERATOR ?= "Xcode"
CMAKE_BINARY    ?= $(shell which cmake)
ifeq ($(CMAKE_BINARY),)
CMAKE_BINARY    ?= /Applications/CMake.app/Contents/bin/cmake
endif
DARWIN          := 1
else ifeq ($(OS),Linux)
CMAKE_GENERATOR ?= "Eclipse CDT4 - Unix Makefiles"
CMAKE_BINARY    ?= cmake
LINUX           := 1
else
CMAKE_GENERATOR ?= "MSYS Makefiles"
CMAKE_BINARY    ?= cmake
WINDOWS         := 1
endif
INSTALL_DIR     ?= $(BUILDDIRPATH)$(OS)

all: build

run: shapetool

.PHONY: clangtidy
clangtidy:
	$(Q)mkdir -p $(BUILDDIR)/tidy
	$(Q)cd $(BUILDDIR)/tidy; $(CMAKE_BINARY) -DCMAKE_CXX_CLANG_TIDY:STRING="clang-tidy-4.0;-checks=readability-uniqueptr-delete-release,readability-non-const-parameter,readability-redundant-smartptr-get,performance-unnecessary-value-param,performance-unnecessary-copy-initialization,performance-inefficient-string-concatenation,performance-implicit-cast-in-loop,performance-for-range-copy,performance-faster-string-find,modernize-make-shared,clang-analyzer-security.*;-fix" $(CURDIR) $(CMAKE_OPTIONS) && cmake --build .

.PHONY: cmake
cmake:
	$(Q)mkdir -p $(BUILDDIR)
	$(Q)cd $(BUILDDIR); $(CMAKE_BINARY) -G$(CMAKE_GENERATOR) -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CURDIR) $(CMAKE_OPTIONS)

define COMPILE
$(if $(LINUX),\
	$(Q)$(MAKE) $(MAKE_OPTIONS) $(JOB_FLAG) $(1) \
$(else),\
	$(if $(DARWIN),\
		$(Q)cd $(BUILDDIR); xcodebuild build -target $(1) install -project tests.xcodeproj -configuration $(BUILD_TYPE) CODE_SIGNING_ALLOWED=NO CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO && exit ${PIPESTATUS[0]} \
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
	$(Q)$(EDITOR) $(LOCAL_CONFIG_DIR)/mapedit/mapedit.vars

doc: cmake
	$(call COMPILE, codegen)
	$(call COMPILE, $@)

server client voxedit shapetool mapedit shadertool noisetool databasetool uitool tests tests-math tests-core tests-persistence tests-voxel benchmarks-voxel tests-noise tests-computeshadertool testmesh testcamera testdepthbuffer testturbobadger testnuklear testtexture testvoxelfont testplane testimgui testoctree testglslgeom testglslcomp testluaui testoctreevisit testshapebuilder tests-shadertool flatc computeshadertool: cmake
	$(call COMPILE, $@)
	$(call COMPILE, copy-data-shared)
	$(call COMPILE, copy-data-$@)
	$(Q)cd $(BUILDDIR); $(PERF_CMD) $(VALGRIND_CMD) $(DEBUG_CMD) $(VOGL_CMD) ./$@ $(ARGS)

assimp backward benchmark dearimgui flatbuffers glm gtest libcurl libenet libturbobadger libuv lua53 nativefiledialog restclient-cpp sdl2 selene simplecpp zlib: cmake
	$(call COMPILE, $@)

rcon: cmake
	$(call COMPILE, $@)
	$(Q)cd $(BUILDDIR); $(VALGRIND_CMD) $(DEBUG_CMD) $(VOGL_CMD) ./$@ $(ARGS)

define UPDATE_GIT
	$(Q)if [ ! -d $(UPDATEDIR)/$(1).sync ]; then \
		git clone --depth=1 $(2) $(UPDATEDIR)/$(1).sync; \
	else \
		cd $(UPDATEDIR)/$(1).sync && git pull --depth=1 --rebase; \
	fi;
endef

define UPDATE_HG
	$(Q)if [ ! -d $(UPDATEDIR)/$(1).sync ]; then \
		hg clone $(2) $(UPDATEDIR)/$(1).sync; \
	else \
		cd $(UPDATEDIR)/$(1).sync && hg pull && hg update; \
	fi;
endef

update-restclient-cpp:
	$(call UPDATE_GIT,restclient-cpp,https://github.com/mrtazz/restclient-cpp.git)
	rm -rf contrib/libs/restclient-cpp/restclient-cpp/*.h
	rm -rf contrib/libs/restclient-cpp/*.cc
	cp $(UPDATEDIR)/restclient-cpp.sync/include/restclient-cpp/*.h contrib/libs/restclient-cpp/restclient-cpp
	cp $(UPDATEDIR)/restclient-cpp.sync/source/*.cc contrib/libs/restclient-cpp

update-libuv:
	$(call UPDATE_GIT,libuv,https://github.com/libuv/libuv.git)
	rm -rf contrib/libs/libuv/include/*.[ch]
	rm -rf contrib/libs/libuv/src/unix/*.[ch]
	rm -rf contrib/libs/libuv/src/win32/*.[ch]
	rm -rf contrib/libs/libuv/src/*.[ch]
	cp $(UPDATEDIR)/libuv.sync/include/*.h contrib/libs/libuv/include
	cp $(UPDATEDIR)/libuv.sync/src/unix/*.[ch] contrib/libs/libuv/src/unix
	cp $(UPDATEDIR)/libuv.sync/src/win/*.[ch] contrib/libs/libuv/src/win
	cp $(UPDATEDIR)/libuv.sync/src/*.[ch] contrib/libs/libuv/src

update-stb:
	$(call UPDATE_GIT,stb,https://github.com/nothings/stb.git)
	cp $(UPDATEDIR)/stb.sync/stb_image.h src/modules/image/stb_image.h
	cp $(UPDATEDIR)/stb.sync/stb_image_write.h src/modules/image/stb_image_write.h
	cp $(UPDATEDIR)/stb.sync/stb_truetype.h src/modules/voxel/font/stb_truetype.h
	cp $(UPDATEDIR)/stb.sync/stb_image.h contrib/libs/libturbobadger/tb/thirdparty
	cp $(UPDATEDIR)/stb.sync/stb_truetype.h contrib/libs/libturbobadger/tb/thirdparty
# TODO: dearimgui

update-simplecpp:
	$(call UPDATE_GIT,simplecpp,https://github.com/danmar/simplecpp.git)
	cp $(UPDATEDIR)/simplecpp.sync/simplecpp.* contrib/libs/simplecpp

update-googletest:
	$(call UPDATE_GIT,googletest,https://github.com/google/googletest.git)
	rm -rf contrib/libs/gtest/src
	rm -rf contrib/libs/gtest/include
	cp -r $(UPDATEDIR)/googletest.sync/googletest/src contrib/libs/gtest
	cp -r $(UPDATEDIR)/googletest.sync/googletest/include contrib/libs/gtest/include
	git checkout -f contrib/libs/gtest/include/gtest/internal/custom

update-benchmark:
	$(call UPDATE_GIT,benchmark,https://github.com/google/benchmark.git)
	cp -r $(UPDATEDIR)/benchmark.sync/src/* contrib/libs/benchmark/src
	cp -r $(UPDATEDIR)/benchmark.sync/include/* contrib/libs/benchmark/include

update-backward:
	$(call UPDATE_GIT,backward-cpp,https://github.com/bombela/backward-cpp.git)
	cp $(UPDATEDIR)/backward-cpp.sync/backward.cpp contrib/libs/backward
	cp -f $(UPDATEDIR)/backward-cpp.sync/backward.hpp contrib/libs/backward/backward.h
	sed -i 's/backward.hpp/backward.h/g' contrib/libs/backward/backward.cpp

update-dearimgui:
	$(call UPDATE_GIT,imgui,https://github.com/ocornut/imgui.git)
	cp $(UPDATEDIR)/imgui.sync/imgui*.h $(UPDATEDIR)/imgui.sync/imgui*.cpp $(UPDATEDIR)/imgui.sync/stb_*.h contrib/libs/dearimgui/dearimgui
	mv contrib/libs/dearimgui/dearimgui/imgui_demo.cpp src/tests/testimgui/Demo.cpp
	sed -i 's/"imgui.h"/"ui\/imgui\/IMGUI.h"/g' src/tests/testimgui/Demo.cpp

update-assimp:
	$(call UPDATE_GIT,assimp,https://github.com/assimp/assimp.git)
	rm -rf contrib/libs/assimp/code/* contrib/libs/assimp/include/*
	cp -r $(UPDATEDIR)/assimp.sync/code/* contrib/libs/assimp/code
	cp -r $(UPDATEDIR)/assimp.sync/include/* contrib/libs/assimp/include
	git checkout contrib/libs/assimp/include/assimp/revision.h

update-flatbuffers:
	$(call UPDATE_GIT,flatbuffers,https://github.com/google/flatbuffers.git)
	rm -rf contrib/libs/flatbuffers/flatbuffers/* contrib/libs/flatbuffers/compiler/*
	mkdir -p contrib/libs/flatbuffers/compiler/src
	cp -r $(UPDATEDIR)/flatbuffers.sync/include/flatbuffers/* contrib/libs/flatbuffers/flatbuffers
	cp -r $(UPDATEDIR)/flatbuffers.sync/src/* contrib/libs/flatbuffers/compiler
	cp -r $(UPDATEDIR)/flatbuffers.sync/grpc/src/* contrib/libs/flatbuffers/compiler/src
	rm contrib/libs/flatbuffers/compiler/flathash.cpp

update-enet:
	$(call UPDATE_GIT,libenet,https://github.com/lsalzman/enet.git)
	cp -r $(UPDATEDIR)/libenet.sync/*.[ch] contrib/libs/libenet
	cp -r $(UPDATEDIR)/libenet.sync/include/* contrib/libs/libenet/include

update-glm:
	$(call UPDATE_GIT,glm,https://github.com/g-truc/glm.git)
	rm -rf contrib/libs/glm/glm/*
	cp -r $(UPDATEDIR)/glm.sync/glm/* contrib/libs/glm/glm
	rm contrib/libs/glm/glm/CMakeLists.txt

update-sdl2:
	$(call UPDATE_HG,sdl2,https://hg.libsdl.org/SDL)
	rm -rf contrib/libs/sdl2/src/* contrib/libs/sdl2/include/* contrib/libs/sdl2/cmake/*
	cp -r $(UPDATEDIR)/sdl2.sync/src/* contrib/libs/sdl2/src
	cp -r $(UPDATEDIR)/sdl2.sync/include/* contrib/libs/sdl2/include
	cp -r $(UPDATEDIR)/sdl2.sync/cmake/* contrib/libs/sdl2/cmake

update-turbobadger:
	$(call UPDATE_GIT,turbobadger,https://github.com/fruxo/turbobadger.git)
	rm -rf contrib/libs/libturbobadger/tb/*
	cp -r $(UPDATEDIR)/turbobadger.sync/src/tb/* contrib/libs/libturbobadger/tb
	git checkout master contrib/libs/libturbobadger/tb/tb_clipboard_sdl.cpp
	git checkout master contrib/libs/libturbobadger/tb/tb_system_sdl.cpp
	git checkout master contrib/libs/libturbobadger/tb/tb_file_sdl.cpp
	git checkout contrib/libs/libturbobadger/tb/thirdparty/
	rm contrib/libs/libturbobadger/tb/CMakeLists.txt
	rm -rf contrib/libs/libturbobadger/tb/utf8/test\ files
	rm -rf contrib/libs/libturbobadger/tb/tests
	git diff contrib/libs/libturbobadger/ > $(UPDATEDIR)/turbobadger.sync/upstream.diff
	git checkout contrib/libs/libturbobadger/tb/tb_id.cpp

update-glslang:
	$(call UPDATE_GIT,glslang,https://github.com/KhronosGroup/glslang.git)
	rm -rf src/tools/glslang/External
	cp -r $(UPDATEDIR)/glslang.sync/External src/tools/glslang/
	rm -rf src/tools/glslang/glslang
	cp -r $(UPDATEDIR)/glslang.sync/glslang src/tools/glslang/
	rm -rf src/tools/glslang/OGLCompilersDLL
	cp -r $(UPDATEDIR)/glslang.sync/OGLCompilersDLL src/tools/glslang/
	rm -rf src/tools/glslang/SPIRV
	cp -r $(UPDATEDIR)/glslang.sync/SPIRV src/tools/glslang/
	rm -rf src/tools/glslang/StandAlone
	cp -r $(UPDATEDIR)/glslang.sync/StandAlone src/tools/glslang/

update-nuklear:
	$(call UPDATE_GIT,nuklear,https://github.com/vurtun/nuklear.git)
	cp $(UPDATEDIR)/nuklear.sync/nuklear.h src/modules/ui/nuklear/private
	cp $(UPDATEDIR)/nuklear.sync/demo/overview.c src/tests/testnuklear

update-json:
	$(call UPDATE_GIT,json,https://github.com/nlohmann/json)
	cp $(UPDATEDIR)/json.sync/src/json.hpp src/modules/core

# currently not part of updatelibs - intentional - we adopted the original code.
update-simplexnoise:
	$(call UPDATE_GIT,simplexnoise,https://github.com/simongeilfus/SimplexNoise.git)
	cp $(UPDATEDIR)/simplexnoise.sync/include/Simplex.h src/modules/noise

update-stringview:
	$(call UPDATE_GIT,string_view,https://github.com/satoren/string_view.git)
	cp $(UPDATEDIR)/string_view.sync/string_view.hpp contrib/libs/string_view

update-voxelizer:
	$(call UPDATE_GIT,voxelizer,https://github.com/karimnaaji/voxelizer)
	cp $(UPDATEDIR)/voxelizer.sync/voxelizer.h src/tools/voxedit/ui/editorscene/

# TODO native file dialog support
# TODO simpleai support
updatelibs: update-voxelizer update-nuklear update-stringview update-restclient-cpp update-libuv update-stb update-googletest update-benchmark update-backward update-dearimgui update-flatbuffers update-assimp update-enet update-glm update-sdl2 update-turbobadger update-glslang

updategl:
	cd tools/flextGL && ./flextgl.sh
