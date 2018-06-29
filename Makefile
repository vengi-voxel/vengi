Q           ?= @
UPDATEDIR   := /tmp
BUILDDIR    ?= ./build
INSTALL_DIR ?= $(BUILDDIRPATH)$(shell uname)

$(BUILDDIR)/CMakeCache.txt:
	$(Q)mkdir -p $(BUILDDIR)
	$(Q)cd $(BUILDDIR); cmake -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR) $(CURDIR)

%: force $(BUILDDIR)/CMakeCache.txt
	$(Q)$(MAKE) --no-print-directory -C $(BUILDDIR) $@

force: ;

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
	cp $(UPDATEDIR)/stb.sync/stb_image.h contrib/libs/libturbobadger/tb/thirdparty
	cp $(UPDATEDIR)/stb.sync/stb_truetype.h contrib/libs/libturbobadger/tb/thirdparty

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

# currently not part of updatelibs - intentional - we adopted the original code.
update-simplexnoise:
	$(call UPDATE_GIT,simplexnoise,https://github.com/simongeilfus/SimplexNoise.git)
	cp $(UPDATEDIR)/simplexnoise.sync/include/Simplex.h src/modules/noise

update-curl:
	$(call UPDATE_GIT,curl,https://github.com/curl/curl.git)
	cp $(UPDATEDIR)/curl.sync/lib/*.[ch]* contrib/libs/libcurl/lib
	cp -r $(UPDATEDIR)/curl.sync/CMake/* contrib/libs/libcurl/cmake
	cp $(UPDATEDIR)/curl.sync/lib/vauth/*.[ch]* contrib/libs/libcurl/lib/vauth
	cp $(UPDATEDIR)/curl.sync/lib/vtls/*.[ch]* contrib/libs/libcurl/lib/vtls
	cp $(UPDATEDIR)/curl.sync/include/curl/*.[ch]* contrib/libs/libcurl/include/curl

# TODO native file dialog support
# TODO simpleai support
# TODO lua support
updatelibs: update-nuklear update-restclient-cpp update-libuv update-stb update-googletest update-benchmark update-backward update-dearimgui update-flatbuffers update-assimp update-enet update-glm update-sdl2 update-turbobadger update-curl update-glslang
	$(MAKE) -C $(BUILDDIR) update-libs
