Q              ?= @
UPDATEDIR      := /tmp
BUILDTYPE      ?= Debug
BUILDDIR       ?= ./build
LIBS_LOCAL     ?= 0
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
EMSDK_DIR      ?= $(BUILDDIR)/emsdk
EMSDK_UPSTREAM ?= $(EMSDK_DIR)/upstream/emscripten/
EMCMAKE        ?= $(EMSDK_UPSTREAM)/emcmake
EMRUN          ?= $(EMSDK_UPSTREAM)/emrun
CMAKE_INTERNAL_OPTIONS ?= -DUSE_GLSLANG_VALIDATOR=ON -DUSE_LINK_TIME_OPTIMIZATION=OFF -DUSE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=$(BUILDTYPE) -G"$(GENERATOR)" --graphviz=$(BUILDDIR)/deps.dot -DUSE_LIBS_FORCE_LOCAL=$(LIBS_LOCAL)
CMAKE_OPTIONS          ?=
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
	$(CMAKE) -H$(CURDIR) -B$(BUILDDIR) $(CMAKE_INTERNAL_OPTIONS) $(CMAKE_OPTIONS)

release:
	$(Q)$(MAKE) BUILDTYPE=RelWithDebInfo

tests-voxedit:
	$(Q)$(CMAKE) -H$(CURDIR) -B$(BUILDDIR)-$@ $(CMAKE_INTERNAL_OPTIONS) $(CMAKE_OPTIONS) -DUSE_IMGUITESTENGINE=On
	$(Q)$(CMAKE) --build $(BUILDDIR)-$@ --target engine_codegen
	$(Q)$(CMAKE) --build $(BUILDDIR)-$@ --target $@
	$(Q)cd $(BUILDDIR)-$@ && ctest -V -R '^$@$$'

clean:
	$(Q)git clean -fdx $(BUILDDIR)

distclean:
	$(Q)git clean -fdx

analysebuild:
	$(Q)ccache -cC
	$(Q)rm -rf $(BUILDDIR)/analyse
	$(Q)mkdir -p $(BUILDDIR)/analyse
	$(Q)CC=clang CXX=clang++ $(CMAKE) -H$(CURDIR) -B$(BUILDDIR)/analyse $(CMAKE_INTERNAL_OPTIONS) $(CMAKE_OPTIONS) -DUSE_SANITIZERS=OFF
	$(Q)ClangBuildAnalyzer --start $(BUILDDIR)/analyse
	$(Q)$(CMAKE) --build $(BUILDDIR)/analyse --target $(ALLTARGET)
	$(Q)ClangBuildAnalyzer --stop $(BUILDDIR)/analyse $(BUILDDIR)/analyse/capture_file
	$(Q)ClangBuildAnalyzer --analyze $(BUILDDIR)/analyse/capture_file

%.png: data/voxedit/%.vengi
	$(Q)$(call EXEC_PATH,thumbnailer) -s 128 --use-scene-camera --input $< --output data/voxedit/$@
	$(Q)pngquant -f --ext .png data/voxedit/$@

thumbnails: thumbnailer $(patsubst data/voxedit/%.vengi,%.png,$(wildcard data/voxedit/*.vengi))

pot:
	$(Q)git grep -l -w "[NC]*_(" src/ > $(BUILDDIR)/POTFILES
	$(Q)xgettext --directory=. --output=data/vengi.pot --omit-header --package-name=vengi \
		--no-location --sort-by-file \
		--keyword=_ --keyword=N_ --keyword="C_:1c,2" --keyword="NC_:1c,2" \
		-C --files-from=$(BUILDDIR)/POTFILES
	$(Q)msgmerge --update data/shared/de_DE.po data/vengi.pot

doc-images:
	$(Q)pngquant -f --ext .png docs/img/*.png

deb-changelog:
	$(Q)contrib/installer/linux/changelog.py docs/CHANGELOG.md > debian/changelog

.PHONY: debian/vengi-palconvert.bash-completion
debian/vengi-palconvert.bash-completion: palconvert
	$(Q)$(call EXEC_PATH,palconvert) --completion bash > $@

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
deb-bash-completion: debian/vengi-palconvert.bash-completion debian/vengi-voxconvert.bash-completion debian/vengi-voxedit.bash-completion debian/vengi-thumbnailer.bash-completion

deb: deb-changelog deb-bash-completion
	$(Q)debuild -b -ui -uc -us

tests:
	$(Q)ctest --test-dir $(BUILDDIR) $(CTEST_FLAGS)

package: voxedit thumbnailer voxconvert palconvert $(BUILDDIR)/CMakeCache.txt
ifeq ($(OS),Windows_NT)
	$(Q)cd $(BUILDDIR) & cpack
else
	$(Q)cd $(BUILDDIR); cpack
endif
ifeq ($(OS),Darwin)
	$(MAKE) mac-sign-dmg
endif

ifeq ($(OS),Darwin)
xcode-local:
	$(Q)$(MAKE) LIBS_LOCAL=1 GENERATOR=Xcode

# create an app password for notarization
# https://appleid.apple.com/ - needed for the mac-notarize target
mac-app-password:
	$(Q)xcrun notarytool store-credentials "KC_PROFILE" --apple-id martin.gerhardy@gmail.com --team-id "Apple Distribution"

mac-verify-signatures-app:
	$(Q)codesign --verify --verbose=2 $(call APP_PATH,voxedit)
	$(Q)codesign --verify --verbose=2 $(call APP_PATH,thumbnailer)
	$(Q)codesign --verify --verbose=2 $(call APP_PATH,voxconvert) # doesn't work
	$(Q)codesign --verify --verbose=2 $(call APP_PATH,palconvert) # doesn't work

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
	$(Q)$(CMAKE) -H$(CURDIR) -B$(BUILDDIR) $(CMAKE_INTERNAL_OPTIONS) $(CMAKE_OPTIONS)

.PHONY: ccmake
ccmake:
	$(Q)ccmake -B$(BUILDDIR) -S.

release-%:
	$(Q)$(MAKE) BUILDTYPE=Release $(subst release-,,$@)

shelltests: all
	$(Q)cd $(BUILDDIR) && ctest -V -C $(BUILDTYPE) -R shelltests-

formatprinter thumbnailer voxedit voxconvert palconvert update-videobindings engine_codegen: $(BUILDDIR)/CMakeCache.txt
	$(Q)$(CMAKE) --build $(BUILDDIR) --target $@
	$(Q)$(CMAKE) --install $(BUILDDIR) --component $@ --prefix $(INSTALL_DIR)/install-$@/usr
ifneq ($(OS),Windows_NT)
	$(Q)$(CMAKE) -E create_symlink $(BUILDDIR)/compile_commands.json compile_commands.json
endif

%-run %-memcheckxml %-memcheck %-debug %-apitrace %-perf %-heaptrack tests-%: $(BUILDDIR)/CMakeCache.txt
	$(Q)$(CMAKE) --build $(BUILDDIR) --target $@

docs/Formats.md: formatprinter
	$(Q)$(call EXEC_PATH,formatprinter) --markdown > $@

contrib/installer/windows/wixpatch.xml: formatprinter
	$(Q)$(call EXEC_PATH,formatprinter) --wix > $@

tools/html/data.js: formatprinter
	$(Q)printf "const jsonData = " > $@
	$(Q)$(call EXEC_PATH,formatprinter) --palette --image --voxel | jq >> $@

contrib/installer/linux/x-voxel.xml: formatprinter
	$(Q)$(call EXEC_PATH,formatprinter) --mimeinfo > $@
	$(Q)contrib/installer/linux/mimetypes.py

contrib/installer/osx/%.plist.in: formatprinter
	$(Q)$(call EXEC_PATH,formatprinter) --plist > $@

contrib/installer/linux/%.man.in: formatprinter
	$(Q)$(call EXEC_PATH,formatprinter) --manpage $* > $@

manpages: contrib/installer/linux/voxconvert.man.in contrib/installer/linux/palconvert.man.in contrib/installer/linux/thumbnailer.man.in contrib/installer/linux/application.man.in
plists: contrib/installer/osx/application.plist.in contrib/installer/osx/voxedit.plist.in
formats: manpages plists tools/html/data.js contrib/installer/linux/x-voxel.xml docs/Formats.md contrib/installer/windows/wixpatch.xml
metainfo:
	$(Q)contrib/installer/linux/metainfo.py contrib/installer/linux/io.github.vengi_voxel.vengi.voxedit.metainfo.xml docs/CHANGELOG.md
prepare-release: formats metainfo find-undocumented-cvars pot

dependency-%:
	$(Q)$(CMAKE) -H$(CURDIR) -B$(BUILDDIR) $(CMAKE_INTERNAL_OPTIONS) $(CMAKE_OPTIONS)
	$(Q)dot -Tsvg $(BUILDDIR)/deps.dot.$(subst dependency-,,$@) -o $(BUILDDIR)/deps.dot.$(subst dependency-,,$@).svg;
	$(Q)xdg-open $(BUILDDIR)/deps.dot.$(subst dependency-,,$@).svg;

tracy:
	$(Q)git submodule update --init --recursive
	$(Q)cmake -Hsrc/modules/core/tracy/profiler -Bsrc/modules/core/tracy/profiler/build -DLEGACY=ON -G$(GENERATOR) -DCMAKE_BUILD_TYPE=Release
	$(Q)cmake --build src/modules/core/tracy/profiler/build --config Release --parallel
	$(Q)src/modules/core/tracy/profiler/build/tracy-profiler

update-libs:
	$(Q)python3 tools/update-dependencies.py

update-fonts:
	curl -o $(UPDATEDIR)/arimo.zip https://fonts.google.com/download?family=Arimo
	unzip -jo $(UPDATEDIR)/arimo.zip static/Arimo-Regular.ttf -d data/ui

update-icons:
	$(Q)rm -rf $(BUILDDIR)/IconFontCppHeaders
	$(Q)git clone https://github.com/juliettef/IconFontCppHeaders.git $(BUILDDIR)/IconFontCppHeaders
	$(Q)cd $(BUILDDIR)/IconFontCppHeaders && python3 GenerateIconFontCppHeaders.py
	$(Q)cp $(BUILDDIR)/IconFontCppHeaders/IconsLucide.h src/modules/ui/IconsLucide.h
	$(Q)curl -L https://unpkg.com/lucide-static@latest/font/lucide.ttf -o data/ui/lucide.ttf

update-tracy:
	git submodule update --remote --merge src/modules/core/tracy

appimage: voxedit
	$(Q)cd $(BUILDDIR) && cmake --install . --component voxedit --prefix install-voxedit/usr
	$(Q)cd $(BUILDDIR) && curl https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20220822-1/linuxdeploy-x86_64.AppImage --output linuxdeploy-x86_64.AppImage --silent -L -f
	$(Q)chmod +x $(BUILDDIR)/linuxdeploy-x86_64.AppImage
	$(Q)$(BUILDDIR)/linuxdeploy-x86_64.AppImage --output=appimage --appdir $(BUILDDIR)/install-voxedit

build/emsdk/emsdk_env.sh:
	git clone https://github.com/emscripten-core/emsdk.git $(BUILDDIR)/emsdk
	$(BUILDDIR)/emsdk/emsdk install latest
	$(BUILDDIR)/emsdk/emsdk activate latest

emscripten-%: $(BUILDDIR)/CMakeCache.txt build/emsdk/emsdk_env.sh
	$(Q)$(CMAKE) --build $(BUILDDIR) --target engine_codegen
	$(Q)mkdir -p build/emscripten
	$(Q)rm -rf build/emscripten/generated
	$(Q)cp -r $(BUILDDIR)/generated build/emscripten
	$(EMCMAKE) $(CMAKE) -H$(CURDIR) -Bbuild/emscripten $(CMAKE_INTERNAL_OPTIONS) $(CMAKE_OPTIONS) -DCMAKE_BUILD_TYPE=$(BUILDTYPE) -DCMAKE_DISABLE_PRECOMPILE_HEADERS=On -DUSE_LINK_TIME_OPTIMIZATION=Off
	$(Q)$(CMAKE) --build build/emscripten --target $(subst emscripten-,,$@)

run-emscripten-%: emscripten-%
	$(Q)$(EMRUN) build/emscripten/$(subst run-emscripten-,,$@)/vengi-$(subst run-emscripten-,,$@).html

server-emscripten-%: emscripten-%
	$(Q)contrib/installer/emscripten/server.py -d build/emscripten/$(subst server-emscripten-,,$@)

find-undocumented-cvars:
	$(Q)for i in $$(cat ./src/modules/core/ConfigVar.h | grep Voxformat | awk -F '"' '{ print $$2 }'); do \
		grep -q $$i docs/Configuration.md; \
		if [ $$? -ne 0 ]; then echo $$i; fi; \
	done
	$(Q)for i in $$(cat ./src/modules/core/ConfigVar.h | grep Palformat | awk -F '"' '{ print $$2 }'); do \
		grep -q $$i docs/Configuration.md; \
		if [ $$? -ne 0 ]; then echo $$i; fi; \
	done
