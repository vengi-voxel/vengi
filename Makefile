Q              ?= @
UPDATEDIR      := /tmp
BUILDTYPE      ?= Debug
BUILDDIR       ?= ./build
INSTALL_DIR    ?= $(BUILDDIR)
ifneq (,$(wildcard $(BUILDDIR)/vengi.xcodeproj))
GENERATOR      ?= Xcode
else
GENERATOR      ?= Ninja
endif
ifeq ($(GENERATOR),Xcode)
ALLTARGET      ?= ALL_BUILD
else
ALLTARGET      ?= all
endif
CMAKE          ?= cmake
EMSDK_DIR      ?= $(HOME)/dev/oss/emsdk
EMSDK_UPSTREAM ?= $(EMSDK_DIR)/upstream/emscripten/
EMCMAKE        ?= $(EMSDK_UPSTREAM)/emcmake
EMRUN          ?= $(EMSDK_UPSTREAM)/emrun
CMAKE_OPTIONS  ?= -DUSE_GLSLANG_VALIDATOR=ON -DUSE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=$(BUILDTYPE) -G$(GENERATOR) --graphviz=$(BUILDDIR)/deps.dot
ifneq ($(Q),@)
	CTEST_FLAGS ?= -V
else
	CTEST_FLAGS ?=
endif
ifeq ($(OS),)
	OS := $(shell uname -s)
endif

ifeq ($(OS),Darwin)
define APP_PATH
	$(BUILDDIR)/$1/vengi-$1.app
endef
define EXEC_PATH
	open $(call APP_PATH $1)
endef
else
define EXEC_PATH
	$(BUILDDIR)/$1/vengi-$1
endef
endif


$(ALLTARGET): $(BUILDDIR)/CMakeCache.txt
	$(Q)$(CMAKE) --build $(BUILDDIR) --config $(BUILDTYPE) --target $@
ifneq ($(OS),Windows_NT)
	$(Q)$(CMAKE) -E create_symlink $(BUILDDIR)/compile_commands.json compile_commands.json
endif

$(BUILDDIR)/CMakeCache.txt:
	$(CMAKE) -H$(CURDIR) -B$(BUILDDIR) $(CMAKE_OPTIONS)

release:
	$(Q)$(MAKE) BUILDTYPE=Release

clean:
	$(Q)git clean -fdx $(BUILDDIR)

distclean:
	$(Q)git clean -fdx

analysebuild:
	$(Q)ccache -cC
	$(Q)rm -rf $(BUILDDIR)/analyse
	$(Q)mkdir $(BUILDDIR)/analyse
	$(Q)CC=clang CXX=clang++ $(CMAKE) -H$(CURDIR) -B$(BUILDDIR)/analyse $(CMAKE_OPTIONS) -DUSE_SANITIZERS=OFF
	$(Q)ClangBuildAnalyzer --start $(BUILDDIR)/analyse
	$(Q)$(CMAKE) --build $(BUILDDIR)/analyse --target $(ALLTARGET)
	$(Q)ClangBuildAnalyzer --stop $(BUILDDIR)/analyse $(BUILDDIR)/analyse/capture_file
	$(Q)ClangBuildAnalyzer --analyze $(BUILDDIR)/analyse/capture_file

%.png: data/voxedit/%.vengi
	$(Q)$(call EXEC_PATH,thumbnailer) -s 128 --use-scene-camera -a 0:-45:0 -p 100:0:200 --input $< --output data/voxedit/$@
	$(Q)pngquant -f --ext .png data/voxedit/$@

thumbnails: thumbnailer $(patsubst data/voxedit/%.vengi,%.png,$(wildcard data/voxedit/*.vengi))

pot:
	$(Q)git grep -l -w "_(" src/ > $(BUILDDIR)/POTFILES
	$(Q)xgettext --directory=. --output=data/vengi.pot --omit-header --package-name=vengi --no-location --sort-by-file --keyword=_ --files-from=$(BUILDDIR)/POTFILES
	$(Q)msgmerge --update data/shared/de_DE.po data/vengi.pot

doc-images:
	$(Q)pngquant -f --ext .png docs/img/*.png

deb-changelog:
	$(Q)contrib/installer/linux/changelog.py docs/CHANGELOG.md > debian/changelog

.PHONY: debian/vengi-voxconvert.bash-completion
debian/vengi-voxconvert.bash-completion: voxconvert
	$(Q)$(call EXEC_PATH,voxconvert) --completion bash > $@

.PHONY: debian/vengi-voxedit.bash-completion
debian/vengi-voxedit.bash-completion: voxedit
	$(Q)$(call EXEC_PATH,voxedit) --completion bash > $@

.PHONY: debian/vengi-thumbnailer.bash-completion
debian/vengi-thumbnailer.bash-completion: thumbnailer
	$(Q)$(call EXEC_PATH,thumbnailer) --completion bash > $@

.PHONY: deb-bash-completion
deb-bash-completion: debian/vengi-voxconvert.bash-completion debian/vengi-voxedit.bash-completion debian/vengi-thumbnailer.bash-completion

deb: deb-changelog deb-bash-completion
	$(Q)debuild -b -ui -uc -us

tests:
	$(Q)ctest --test-dir $(BUILDDIR) $(CTEST_FLAGS)

package: voxedit voxbrowser thumbnailer voxconvert $(BUILDDIR)/CMakeCache.txt
ifeq ($(OS),Windows_NT)
	$(Q)cd $(BUILDDIR) & cpack
else
	$(Q)cd $(BUILDDIR); cpack
endif
ifeq ($(OS),Darwin)
	$(MAKE) mac-sign-dmg
endif

ifeq ($(OS),Darwin)
# create an app password for notarization
# https://appleid.apple.com/ - needed for the mac-notarize target
mac-app-password:
	$(Q)xcrun notarytool store-credentials "KC_PROFILE" --apple-id martin.gerhardy@gmail.com --team-id "Apple Distribution"

mac-verify-signatures-app:
	$(Q)codesign --verify --verbose=2 $(call APP_PATH,voxbrowser)
	$(Q)codesign --verify --verbose=2 $(call APP_PATH,voxedit)
	$(Q)codesign --verify --verbose=2 $(call APP_PATH,thumbnailer)
	$(Q)codesign --verify --verbose=2 $(call APP_PATH,voxconvert) # doesn't work

mac-sign-dmg:
	$(Q)codesign --force --verbose=2 --sign "Apple Distribution" $(BUILDDIR)/*.dmg

mac-verify-signatures-dmg:
	$(Q)codesign --verify --verbose=2 $(BUILDDIR)/*.dmg

mac-verify-signatures: mac-verify-signatures-app mac-verify-signatures-dmg

# https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution/customizing_the_notarization_workflow
mac-notarize: mac-sign-dmg
	$(Q)xcrun notarytool submit $(BUILDDIR)/*.dmg --keychain-profile "KC_PROFILE" --wait

# If you distribute your software via a custom third-party installer, you need two rounds of notarization.
# First you notarize the installer’s payload (everything the installer will install). You then package the
# notarized (and stapled, as described in Staple the ticket to your distribution) items into the installer
# and notarize it as you would any other executable. If you use a network installer, separately notarize
# both the installer and the items it downloads.
mac-staple: mac-notarize
	$(Q)xcrun stapler staple $(BUILDDIR)/*.dmg

mac-verify-notarize:
	$(Q)xcrun spctl --assess --type open --context context:primary-signature --ignore-cache --verbose=2 $(BUILDDIR)/*.dmg
endif

.PHONY: cmake
cmake:
	$(Q)$(CMAKE) -H$(CURDIR) -B$(BUILDDIR) $(CMAKE_OPTIONS)

.PHONY: ccmake
ccmake:
	$(Q)ccmake -B$(BUILDDIR) -S.

release-%:
	$(Q)$(MAKE) BUILDTYPE=Release $(subst release-,,$@)

shelltests: all
	$(Q)cd $(BUILDDIR) && ctest -V -C $(BUILDTYPE) -R shelltests-

formatprinter voxbrowser thumbnailer voxedit voxconvert update-videobindings codegen: $(BUILDDIR)/CMakeCache.txt
	$(Q)$(CMAKE) --build $(BUILDDIR) --target $@
	$(Q)$(CMAKE) --install $(BUILDDIR) --component $@ --prefix $(INSTALL_DIR)/install-$@/usr
ifneq ($(OS),Windows_NT)
	$(Q)$(CMAKE) -E create_symlink $(BUILDDIR)/compile_commands.json compile_commands.json
endif

%-run %-memcheckxml %-memcheck %-debug %-perf tests-%: $(BUILDDIR)/CMakeCache.txt
	$(Q)$(CMAKE) --build $(BUILDDIR) --target $@

docs/Formats.md: formatprinter
	$(Q)$(call EXEC_PATH,formatprinter) --markdown > $@

tools/html/data.js: formatprinter
	$(Q)printf "const jsonData = " > $@
	$(Q)$(call EXEC_PATH,formatprinter) --palette --image --voxel | jq >> $@

contrib/installer/linux/x-voxel.xml: formatprinter
	$(Q)$(call EXEC_PATH,formatprinter) --mimeinfo > $@
	$(Q)contrib/installer/linux/mimetypes.sh

contrib/installer/osx/voxedit.plist.in: formatprinter
	$(Q)$(call EXEC_PATH,formatprinter) --plist > $@

formats: tools/html/data.js contrib/installer/linux/x-voxel.xml contrib/installer/osx/application.plist.in docs/Formats.md

dependency-%:
	$(Q)$(CMAKE) -H$(CURDIR) -B$(BUILDDIR) $(CMAKE_OPTIONS)
	$(Q)dot -Tsvg $(BUILDDIR)/deps.dot.$(subst dependency-,,$@) -o $(BUILDDIR)/deps.dot.$(subst dependency-,,$@).svg;
	$(Q)xdg-open $(BUILDDIR)/deps.dot.$(subst dependency-,,$@).svg;

define UPDATE_GIT
	$(Q)if [ ! -d $(UPDATEDIR)/$(1).sync ]; then \
		git clone --recursive --depth=1 $(2) $(UPDATEDIR)/$(1).sync; \
	else \
		cd $(UPDATEDIR)/$(1).sync && git pull --depth=1 --rebase; \
	fi;
endef

tracy:
	$(Q)git submodule update --init --recursive
	$(Q)$(MAKE) -C src/modules/core/tracy/profiler/build/unix release
	$(Q)src/modules/core/tracy/profiler/build/unix/Tracy-release

update-stb:
	$(call UPDATE_GIT,SOIL2,https://github.com/SpartanJ/SOIL2.git)
	cp $(UPDATEDIR)/SOIL2.sync/src/SOIL2/* contrib/libs/stb_image
	rm contrib/libs/stb_image/SOIL2.*
	find contrib/libs/stb_image -type f -exec dos2unix {} \;
	find contrib/libs/stb_image -type f -exec sed -i 's/[ \t]*$$//' {} \;
	$(call UPDATE_GIT,stb,https://github.com/nothings/stb.git)
	cp $(UPDATEDIR)/stb.sync/stb_truetype.h src/modules/voxelfont/external/stb_truetype.h
	cp $(UPDATEDIR)/stb.sync/stb_rect_pack.h src/modules/scenegraph/external/stb_rect_pack.h

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

update-imguizmo:
	$(call UPDATE_GIT,imguizmo,https://github.com/CedricGuillemet/ImGuizmo.git)
	cp $(UPDATEDIR)/imguizmo.sync/ImGuizmo.* src/modules/ui/dearimgui
	dos2unix src/modules/ui/dearimgui/ImGuizmo*
	sed -i 's/[ \t]*$$//' src/modules/ui/dearimgui/ImGuizmo*

update-im-neo-sequencer:
	$(call UPDATE_GIT,im-neo-sequencer,https://gitlab.com/GroGy/im-neo-sequencer.git)
	cp $(UPDATEDIR)/im-neo-sequencer.sync/imgui*.cpp $(UPDATEDIR)/im-neo-sequencer.sync/imgui*.h src/modules/ui/dearimgui
	cp $(UPDATEDIR)/im-neo-sequencer.sync/LICENSE src/modules/ui/dearimgui/LICENSE-sequencer
	clang-format -i src/modules/ui/dearimgui/imgui_neo*

update-implot:
	$(call UPDATE_GIT,implot,https://github.com/epezent/implot.git)
	cp $(UPDATEDIR)/implot.sync/implot* src/modules/ui/dearimgui
	mv src/modules/ui/dearimgui/implot_demo.cpp src/tests/testimgui/implot_demo.cpp
	sed -i 's/[ \t]*$$//' src/modules/ui/dearimgui/implot*

update-dearimgui:
	$(call UPDATE_GIT,imgui,https://github.com/ocornut/imgui.git -b docking)
	cp $(UPDATEDIR)/imgui.sync/im*.h $(UPDATEDIR)/imgui.sync/im*.cpp $(UPDATEDIR)/imgui.sync/misc/cpp/* src/modules/ui/dearimgui
	cp $(UPDATEDIR)/imgui.sync/backends/imgui_impl_sdl2.* src/modules/ui/dearimgui/backends
	cp $(UPDATEDIR)/imgui.sync/backends/imgui_impl_opengl3* src/modules/ui/dearimgui/backends
	cp $(UPDATEDIR)/imgui.sync/examples/example_sdl2_opengl3/main.cpp src/modules/ui/dearimgui/backends/example_sdl2_opengl3.cpp
	cp $(UPDATEDIR)/imgui.sync/misc/fonts/binary_to_compressed_c.cpp tools/binary_to_compressed_c
	cp $(UPDATEDIR)/imgui.sync/misc/freetype/* src/modules/ui/dearimgui/misc/freetype
	mv src/modules/ui/dearimgui/imgui_demo.cpp src/tests/testimgui/Demo.cpp
	$(call UPDATE_GIT,imgui_test_engine,https://github.com/ocornut/imgui_test_engine.git)
	cp -r $(UPDATEDIR)/imgui_test_engine.sync/imgui_test_engine src/modules/ui/dearimgui

update-glm:
	$(call UPDATE_GIT,glm,https://github.com/g-truc/glm.git)
	rm -rf contrib/libs/glm/glm/*
	cp -r $(UPDATEDIR)/glm.sync/glm/* contrib/libs/glm/glm
	rm contrib/libs/glm/glm/CMakeLists.txt

update-sdl2:
	$(call UPDATE_GIT,sdl2,https://github.com/libsdl-org/SDL.git -b SDL2)
	rm -rf contrib/libs/sdl2/src/* contrib/libs/sdl2/include/* contrib/libs/sdl2/cmake/*
	cp -r $(UPDATEDIR)/sdl2.sync/CMakeLists.txt contrib/libs/sdl2
	cp -r $(UPDATEDIR)/sdl2.sync/*.cmake.in contrib/libs/sdl2
	cp -r $(UPDATEDIR)/sdl2.sync/src/* contrib/libs/sdl2/src
	cp -r $(UPDATEDIR)/sdl2.sync/wayland-protocols/* contrib/libs/sdl2/wayland-protocols
	cp -r $(UPDATEDIR)/sdl2.sync/include/* contrib/libs/sdl2/include
	cp -r $(UPDATEDIR)/sdl2.sync/cmake/* contrib/libs/sdl2/cmake

update-lzfse:
	$(call UPDATE_GIT,lzfse,git@github.com:lzfse/lzfse.git)
	cp $(UPDATEDIR)/lzfse.sync/src/lzvn_decode_base.h contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzfse_encode_tables.h contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzvn_encode_base.h contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzfse.h contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzfse_fse.h contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzfse_tunables.h contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzfse_internal.h contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzfse_decode.c contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzfse_decode_base.c contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzfse_encode.c contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzfse_encode_base.c contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzfse_fse.c contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzvn_decode_base.c contrib/libs/lzfse
	cp $(UPDATEDIR)/lzfse.sync/src/lzvn_encode_base.c contrib/libs/lzfse

update-simplecpp:
	$(call UPDATE_GIT,simplecpp,https://github.com/danmar/simplecpp.git)
	cp $(UPDATEDIR)/simplecpp.sync/simplecpp.* contrib/libs/simplecpp

update-miniz:
	$(call UPDATE_GIT,miniz,https://github.com/richgel999/miniz.git)
	cd $(UPDATEDIR)/miniz.sync; ./amalgamate.sh
	cp $(UPDATEDIR)/miniz.sync/amalgamation/miniz.[ch] src/modules/io/external

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
	cp -r $(UPDATEDIR)/flextgl.sync/templates/vulkan-dynamic tools/flextGL/templates

update-libvxl:
	$(call UPDATE_GIT,libvxl,https://github.com/xtreme8000/libvxl.git)
	cp $(UPDATEDIR)/libvxl.sync/libvxl.c $(UPDATEDIR)/libvxl.sync/libvxl.h src/modules/voxelformat/external

update-ogt_vox:
	$(call UPDATE_GIT,ogl_vox,https://github.com/jpaver/opengametools)
	cp $(UPDATEDIR)/ogl_vox.sync/src/ogt_vox.h src/modules/voxelformat/external
	sed -i 's/[ \t]*$$//' src/modules/voxelformat/external/ogt_vox.h

update-meshoptimizer:
	$(call UPDATE_GIT,meshoptimizer,https://github.com/zeux/meshoptimizer.git)
	rm -f contrib/libs/meshoptimizer/*.[ch]*
	cp $(UPDATEDIR)/meshoptimizer.sync/src/* contrib/libs/meshoptimizer

update-yocto:
	$(call UPDATE_GIT,yocto-gl,https://github.com/xelatihy/yocto-gl.git)
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_bvh.cpp contrib/libs/yocto/yocto_bvh.cpp
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_bvh.h contrib/libs/yocto/yocto_bvh.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_color.h contrib/libs/yocto/yocto_color.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_geometry.h contrib/libs/yocto/yocto_geometry.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_image.cpp contrib/libs/yocto/yocto_image.cpp
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_image.h contrib/libs/yocto/yocto_image.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_math.h contrib/libs/yocto/yocto_math.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_noise.h contrib/libs/yocto/yocto_noise.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_parallel.h contrib/libs/yocto/yocto_parallel.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_sampling.h contrib/libs/yocto/yocto_sampling.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_scene.cpp contrib/libs/yocto/yocto_scene.cpp
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_scene.h contrib/libs/yocto/yocto_scene.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_shading.h contrib/libs/yocto/yocto_shading.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_shape.cpp contrib/libs/yocto/yocto_shape.cpp
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_shape.h contrib/libs/yocto/yocto_shape.h
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_trace.cpp contrib/libs/yocto/yocto_trace.cpp
	cp $(UPDATEDIR)/yocto-gl.sync/libs/yocto/yocto_trace.h contrib/libs/yocto/yocto_trace.h
	sed -i 's/[ \t]*$$//' contrib/libs/yocto/*.[ch]*

update-tinygltf:
	$(call UPDATE_GIT,tinygltf,https://github.com/syoyo/tinygltf.git)
	cp $(UPDATEDIR)/tinygltf.sync/tiny_gltf.h src/modules/voxelformat/external
	cp $(UPDATEDIR)/tinygltf.sync/json.hpp contrib/libs/json

update-tinyobjloader:
	$(call UPDATE_GIT,tinyobjloader,https://github.com/tinyobjloader/tinyobjloader.git)
	cp $(UPDATEDIR)/tinyobjloader.sync/tiny_obj_loader.h src/modules/voxelformat/external

update-ufbx:
	$(call UPDATE_GIT,ufbx,https://github.com/bqqbarbhg/ufbx.git)
	cp $(UPDATEDIR)/ufbx.sync/ufbx.h $(UPDATEDIR)/ufbx.sync/ufbx.c src/modules/voxelformat/external

update-icons:
	$(call UPDATE_GIT,iconfontcppheaders,https://github.com/juliettef/IconFontCppHeaders)
	cp $(UPDATEDIR)/iconfontcppheaders.sync/IconsLucide.h src/modules/ui/
	curl -L https://unpkg.com/lucide-static@latest/font/lucide.ttf -o data/ui/lucide.ttf

update-fonts:
	curl -o $(UPDATEDIR)/arimo.zip https://fonts.google.com/download?family=Arimo
	unzip -jo $(UPDATEDIR)/arimo.zip static/Arimo-Regular.ttf -d data/ui

appimage: voxedit
	$(Q)cd $(BUILDDIR) && cmake --install . --component voxedit --prefix install-voxedit/usr
	$(Q)cd $(BUILDDIR) && curl https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20220822-1/linuxdeploy-x86_64.AppImage --output linuxdeploy-x86_64.AppImage --silent -L -f
	$(Q)chmod +x $(BUILDDIR)/linuxdeploy-x86_64.AppImage
	$(Q)$(BUILDDIR)/linuxdeploy-x86_64.AppImage --output=appimage --appdir $(BUILDDIR)/install-voxedit

emscripten-%: $(BUILDDIR)/CMakeCache.txt
	$(Q)$(CMAKE) --build $(BUILDDIR) --target codegen
	$(Q)mkdir -p build/emscripten
	$(Q)rm -rf build/emscripten/generated
	$(Q)cp -r $(BUILDDIR)/generated build/emscripten
	$(Q)$(EMCMAKE) $(CMAKE) -H$(CURDIR) -Bbuild/emscripten $(CMAKE_OPTIONS) -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_PRECOMPILE_HEADERS=On -DUSE_LINK_TIME_OPTIMIZATION=Off
	$(Q)$(CMAKE) --build build/emscripten --target $(subst emscripten-,,$@)

run-emscripten-%: emscripten-%
	$(Q)$(EMRUN) build/emscripten/$(subst run-emscripten-,,$@)/vengi-$(subst run-emscripten-,,$@).html
