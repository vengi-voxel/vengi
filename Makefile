Q              ?= @
UPDATEDIR      := /tmp
BUILDTYPE      ?= Debug
BUILDDIR       ?= ./build/$(BUILDTYPE)
INSTALL_DIR    ?= $(BUILDDIR)
GENERATOR      := Ninja
CMAKE          ?= cmake
CMAKE_OPTIONS  ?= -DCMAKE_BUILD_TYPE=$(BUILDTYPE) -G$(GENERATOR) --graphviz=$(BUILDDIR)/deps.dot

all:
	$(Q)if [ ! -f $(BUILDDIR)/CMakeCache.txt ]; then $(CMAKE) -H$(CURDIR) -B$(BUILDDIR) $(CMAKE_OPTIONS); fi
	$(Q)$(CMAKE) --build $(BUILDDIR) --target $@

release:
	$(Q)$(MAKE) BUILDTYPE=Release

clean:
	$(Q)rm -rf $(BUILDDIR)

distclean:
	$(Q)git clean -fdx

deb:
	$(Q)debuild -b -ui -uc -us

crosscompile:
	$(Q)dockcross $(CMAKE) -H. -B$(BUILDDIR) $(CMAKE_OPTIONS)
	$(Q)dockcross $(CMAKE) --build $(BUILDDIR) --target all

windows:
	$(Q)$(MAKE) crosscompile TARGET_OS=$@ BUILDDIR=$(BUILDDIR)/$@

release-%:
	$(Q)$(MAKE) BUILDTYPE=Release $(subst release-,,$@)

%:
	$(Q)if [ ! -f $(BUILDDIR)/CMakeCache.txt ]; then $(CMAKE) -H$(CURDIR) -B$(BUILDDIR) $(CMAKE_OPTIONS); fi
	$(Q)$(CMAKE) --build $(BUILDDIR) --target $@
	$(Q)$(CMAKE) --install $(BUILDDIR) --component $@ --prefix $(INSTALL_DIR)/install-$@

dependency-%:
	$(Q)$(CMAKE) -H$(CURDIR) -B$(BUILDDIR) $(CMAKE_OPTIONS)
	$(Q)dot -Tsvg $(BUILDDIR)/deps.dot.$(subst dependency-,,$@) -o $(BUILDDIR)/deps.dot.$(subst dependency-,,$@).svg;
	$(Q)xdg-open $(BUILDDIR)/deps.dot.$(subst dependency-,,$@).svg;

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

update-libuv:
	$(call UPDATE_GIT,libuv,https://github.com/libuv/libuv.git)
	rm -rf contrib/libs/libuv/include/uv/*.[ch]
	rm -rf contrib/libs/libuv/include/*.[ch]
	rm -rf contrib/libs/libuv/src/unix/*.[ch]
	rm -rf contrib/libs/libuv/src/win32/*.[ch]
	rm -rf contrib/libs/libuv/src/*.[ch]
	cp $(UPDATEDIR)/libuv.sync/include/*.h contrib/libs/libuv/include
	cp $(UPDATEDIR)/libuv.sync/include/uv/*.h contrib/libs/libuv/include/uv
	cp $(UPDATEDIR)/libuv.sync/src/unix/*.[ch] contrib/libs/libuv/src/unix
	cp $(UPDATEDIR)/libuv.sync/src/win/*.[ch] contrib/libs/libuv/src/win
	cp $(UPDATEDIR)/libuv.sync/src/*.[ch] contrib/libs/libuv/src

update-stb:
	$(call UPDATE_GIT,stb,https://github.com/nothings/stb.git)
	cp $(UPDATEDIR)/stb.sync/stb_image.h src/modules/image/stb_image.h
	cp $(UPDATEDIR)/stb.sync/stb_image_write.h src/modules/image/stb_image_write.h
	cp $(UPDATEDIR)/stb.sync/stb_truetype.h src/modules/voxelfont/stb_truetype.h
	cp $(UPDATEDIR)/stb.sync/stb_truetype.h src/modules/ui/turbobadger/tb/thirdparty

update-googletest:
	$(call UPDATE_GIT,googletest,https://github.com/google/googletest.git)
	rm -rf contrib/libs/gtest/src
	rm -rf contrib/libs/gtest/include
	mkdir -p contrib/libs/gtest/src
	mkdir -p contrib/libs/gtest/include
	cp -r $(UPDATEDIR)/googletest.sync/googletest/src/ contrib/libs/gtest
	cp -r $(UPDATEDIR)/googletest.sync/googletest/include/ contrib/libs/gtest
	cp -r $(UPDATEDIR)/googletest.sync/googlemock/src/ contrib/libs/gtest
	cp -r $(UPDATEDIR)/googletest.sync/googlemock/include/ contrib/libs/gtest
	git checkout -f contrib/libs/gtest/include/gtest/internal/custom
	git checkout -f contrib/libs/gtest/include/gmock/internal/custom

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
	cp $(UPDATEDIR)/imgui.sync/im*.h $(UPDATEDIR)/imgui.sync/im*.cpp $(UPDATEDIR)/imgui.sync/misc/cpp/* contrib/libs/dearimgui/dearimgui
	mv contrib/libs/dearimgui/dearimgui/imgui_demo.cpp src/tests/testimgui/Demo.cpp
	sed -i 's/"imgui.h"/"ui\/imgui\/IMGUI.h"/g' src/tests/testimgui/Demo.cpp

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
	cp -r $(UPDATEDIR)/sdl2.sync/wayland-protocols/* contrib/libs/sdl2/wayland-protocols
	cp -r $(UPDATEDIR)/sdl2.sync/include/* contrib/libs/sdl2/include
	cp -r $(UPDATEDIR)/sdl2.sync/cmake/* contrib/libs/sdl2/cmake

update-sdl2mixer:
	$(call UPDATE_HG,sdl2_mixer,https://hg.libsdl.org/SDL_mixer)
	rm -rf contrib/libs/sdl2_mixer/*
	cp -r $(UPDATEDIR)/sdl2_mixer.sync/src/* contrib/libs/sdl2_mixer
	cp -r $(UPDATEDIR)/sdl2_mixer.sync/include/* contrib/libs/sdl2_mixer
	cp -r $(UPDATEDIR)/sdl2_mixer.sync/external/libogg* contrib/libs/sdl2_mixer
	cp -r $(UPDATEDIR)/sdl2_mixer.sync/external/libvorbis* contrib/libs/sdl2_mixer
	git checkout -f contrib/libs/sdl2_mixer/CMakeLists.txt

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

update-simplecpp:
	$(call UPDATE_GIT,simplecpp,https://github.com/danmar/simplecpp.git)
	cp $(UPDATEDIR)/simplecpp.sync/simplecpp.* contrib/libs/simplecpp

update-miniz:
	$(call UPDATE_GIT,miniz,https://github.com/richgel999/miniz.git)
	cp $(UPDATEDIR)/miniz.sync/miniz.[ch] src/modules/core

update-nuklear:
	$(call UPDATE_GIT,nuklear,https://github.com/Immediate-Mode-UI/Nuklear)
	cp $(UPDATEDIR)/nuklear.sync/nuklear.h src/modules/ui/nuklear/private
	cp $(UPDATEDIR)/nuklear.sync/demo/overview.c src/tests/testnuklear

# currently not part of updatelibs - intentional - we adopted the original code.
update-simplexnoise:
	$(call UPDATE_GIT,simplexnoise,https://github.com/simongeilfus/SimplexNoise.git)
	cp $(UPDATEDIR)/simplexnoise.sync/include/Simplex.h src/modules/noise

update-flextgl:
	$(call UPDATE_GIT,flextgl,https://github.com/mosra/flextgl.git)
	cp $(UPDATEDIR)/flextgl.sync/*.py tools/flextGL
	cp $(UPDATEDIR)/flextgl.sync/README.md tools/flextGL
	cp $(UPDATEDIR)/flextgl.sync/COPYING tools/flextGL
	rm -rf tools/flextGL/templates/sdl
	rm -rf tools/flextGL/templates/vulkan
	cp -r $(UPDATEDIR)/flextgl.sync/templates/sdl tools/flextGL/templates
	cp -r $(UPDATEDIR)/flextgl.sync/templates/vulkan tools/flextGL/templates

# TODO simpleai support
# TODO lua support
updatelibs: update-nuklear update-libuv update-stb update-googletest update-benchmark update-backward update-dearimgui update-flatbuffers update-enet update-glm update-sdl2 update-glslang update-simplecpp
	$(MAKE) -C $(BUILDDIR) update-libs
