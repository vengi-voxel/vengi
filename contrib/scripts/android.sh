#!/bin/bash

set -euo pipefail

DIR=$(dirname "$(readlink -f "$0")")
ROOT=$(readlink -f "$DIR/../..")

NDK_VERSION="${ANDROID_NDK_VERSION:-30.0.16248370}"
CMDLINE_VERSION="${ANDROID_CMDLINE_TOOLS:-15859902}"
COMPILE_SDK="${ANDROID_COMPILE_SDK:-37}"
# Stable platform package. API 37 is published as android-37.x, not android-37.
PLATFORM_PKG="${ANDROID_PLATFORM_PKG:-platforms;android-37.2}"
BUILD_TOOLS_VER="${ANDROID_BUILD_TOOLS:-37.0.0}"
NATIVE_PLATFORM="${ANDROID_PLATFORM:-android-24}"
SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$ROOT/build/android_sdk}}"
BUILD_DIR="${ANDROID_BUILD_DIR:-$ROOT/build/android}"
HOST_DIR="${ANDROID_HOST_DIR:-$ROOT/build/android-host}"
TESTS_DIR="${ANDROID_TESTS_DIR:-$ROOT/build/android-tests}"
APP_ID="io.github.vengi_voxel.voxedit"
ACTIVITY="${APP_ID}/.VoxEdit"

usage() {
	cat <<EOF
Usage: $(basename "$0") [command]

Commands:
  setup          Install SDK, NDK, emulator, system image, AVD (skips existing)
  build          Host codegen + native libmain/SDL3 + debug APK (default; CI)
  emulator       Start the AVD if no Android device is connected
  wait           Wait until the emulator/device has finished booting
  install        adb install the VoxEdit APK; starts the emulator if needed
  start          Launch VoxEdit
  logs           Follow logcat for VoxEdit
  run            setup + build (host ABI) + emulator + install + start
  tests          Build host-ABI gtest binaries, boot the emulator, run them
  stop           Shut down a running emulator
  help           Show this help

Environment:
  ANDROID_SDK_ROOT / ANDROID_HOME   SDK path (default: $ROOT/build/android_sdk)
  ANDROID_NDK_HOME / ANDROID_NDK    NDK path (default: SDK ndk/$NDK_VERSION)
  ANDROID_ABI                       build default: arm64-v8a; run default: host ABI
  ANDROID_PLATFORM                  native API (default: android-24)
  BUILDTYPE                         Debug or Release native libs (default: Release)
  ANDROID_AVD                       AVD name (default: vengi-<abi>)
  ANDROID_EMULATOR_HEADLESS=1       no emulator window
  ANDROID_LOGS_DUMP=1               logs: print the buffer and exit
EOF
}

host_abi() {
	case "$(uname -m)" in
		x86_64) echo x86_64 ;;
		aarch64|arm64) echo arm64-v8a ;;
		i386|i686) echo x86 ;;
		*) echo x86_64 ;;
	esac
}

ndk_usable() {
	local ndk="${1:-}"
	[ -n "$ndk" ] && [ -f "$ndk/build/cmake/android.toolchain.cmake" ]
}

resolve_ndk() {
	if ndk_usable "${ANDROID_NDK_HOME:-}"; then
		return 0
	fi
	if ndk_usable "${ANDROID_NDK:-}"; then
		export ANDROID_NDK_HOME="$ANDROID_NDK"
		return 0
	fi
	if ndk_usable "$SDK_ROOT/ndk/$NDK_VERSION"; then
		export ANDROID_NDK_HOME="$SDK_ROOT/ndk/$NDK_VERSION"
		return 0
	fi
	local d
	for d in "$SDK_ROOT"/ndk/*; do
		if ndk_usable "$d"; then
			export ANDROID_NDK_HOME="$d"
			return 0
		fi
	done
	return 1
}

export_android_env() {
	export ANDROID_SDK_ROOT="$SDK_ROOT"
	export ANDROID_HOME="$SDK_ROOT"
	resolve_ndk || true
	export PATH="$SDK_ROOT/cmdline-tools/latest/bin:$SDK_ROOT/platform-tools:$SDK_ROOT/emulator:${ANDROID_NDK_HOME:-}:$PATH"
}

write_env_file() {
	local env_file="$SDK_ROOT/android-env.sh"
	mkdir -p "$SDK_ROOT"
	{
		echo "export ANDROID_SDK_ROOT=$SDK_ROOT"
		echo "export ANDROID_HOME=$SDK_ROOT"
		echo "export ANDROID_NDK_HOME=${ANDROID_NDK_HOME:-$SDK_ROOT/ndk/$NDK_VERSION}"
		echo "export PATH=\"\$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:\$ANDROID_SDK_ROOT/platform-tools:\$ANDROID_SDK_ROOT/emulator:\$ANDROID_NDK_HOME:\$PATH\""
	} > "$env_file"
}

sdk_pkg_path() {
	local pkg="$1"
	case "$pkg" in
		platform-tools) printf '%s\n' "$SDK_ROOT/platform-tools/adb" ;;
		emulator) printf '%s\n' "$SDK_ROOT/emulator/emulator" ;;
		ndk\;*) printf '%s\n' "$SDK_ROOT/ndk/${pkg#ndk;}" ;;
		platforms\;*) printf '%s\n' "$SDK_ROOT/platforms/${pkg#platforms;}" ;;
		build-tools\;*) printf '%s\n' "$SDK_ROOT/build-tools/${pkg#build-tools;}" ;;
		system-images\;*)
			local rest="${pkg#system-images;}"
			printf '%s\n' "$SDK_ROOT/system-images/${rest//;//}"
			;;
		*) printf '%s\n' "$SDK_ROOT/${pkg//;//}" ;;
	esac
}

sdk_pkg_present() {
	local path
	path="$(sdk_pkg_path "$1")"
	[ -e "$path" ]
}

need_cmd() {
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "Missing command: $1" >&2
		exit 1
	fi
}

java_major() {
	java -version 2>&1 | awk -F '"' '/version/ {
		split($2, v, ".");
		if (v[1] == "1") print v[2]; else print v[1];
		exit
	}'
}

check_java() {
	if ! command -v java >/dev/null 2>&1; then
		echo "JDK 17+ is required (CI uses Temurin 21). Install e.g. openjdk-21-jdk." >&2
		exit 1
	fi
	local major
	major="$(java_major)"
	if [ -z "$major" ] || [ "$major" -lt 17 ]; then
		echo "JDK 17+ is required, found: $(java -version 2>&1 | head -1)" >&2
		exit 1
	fi
}

ensure_cmdline_tools() {
	local sdkmanager="$SDK_ROOT/cmdline-tools/latest/bin/sdkmanager"
	if [ -x "$sdkmanager" ]; then
		return 0
	fi
	need_cmd unzip
	local sdk_file tmp
	sdk_file="commandlinetools-linux-${CMDLINE_VERSION}_latest.zip"
	tmp=$(mktemp -d)
	echo "Downloading Android cmdline-tools ${CMDLINE_VERSION}"
	if command -v curl >/dev/null 2>&1; then
		curl -fL --retry 3 -o "$tmp/$sdk_file" "https://dl.google.com/android/repository/${sdk_file}"
	else
		need_cmd wget
		wget --no-verbose -O "$tmp/$sdk_file" "https://dl.google.com/android/repository/${sdk_file}"
	fi
	mkdir -p "$SDK_ROOT/cmdline-tools"
	rm -rf "$SDK_ROOT/cmdline-tools/latest"
	unzip -q "$tmp/$sdk_file" -d "$tmp"
	mv "$tmp/cmdline-tools" "$SDK_ROOT/cmdline-tools/latest"
	rm -rf "$tmp"
}

accept_licenses() {
	local sdkmanager="$SDK_ROOT/cmdline-tools/latest/bin/sdkmanager"
	set +e
	yes | "$sdkmanager" --sdk_root="$SDK_ROOT" --licenses >/dev/null 2>&1
	set -e
}

install_sdk_packages() {
	local sdkmanager="$SDK_ROOT/cmdline-tools/latest/bin/sdkmanager"
	local missing=()
	local pkg
	for pkg in "$@"; do
		if sdk_pkg_present "$pkg"; then
			echo "SDK package already present: $pkg"
		else
			missing+=("$pkg")
		fi
	done
	if [ ${#missing[@]} -eq 0 ]; then
		return 0
	fi
	echo "Installing SDK packages: ${missing[*]}"
	accept_licenses
	"$sdkmanager" --sdk_root="$SDK_ROOT" "${missing[@]}"
}

setup_sdk_ndk() {
	mkdir -p "$SDK_ROOT"
	check_java
	ensure_cmdline_tools
	export_android_env
	local ndk_pkg="ndk;$NDK_VERSION"
	local platform_pkg="$PLATFORM_PKG"
	if resolve_ndk; then
		echo "Using existing NDK: $ANDROID_NDK_HOME"
		install_sdk_packages \
			"$platform_pkg" \
			"build-tools;${BUILD_TOOLS_VER}" \
			platform-tools
	else
		install_sdk_packages \
			"$platform_pkg" \
			"build-tools;${BUILD_TOOLS_VER}" \
			platform-tools \
			"$ndk_pkg"
		resolve_ndk
	fi
	if ! resolve_ndk; then
		echo "ANDROID_NDK_HOME does not look like an NDK: ${ANDROID_NDK_HOME:-unset}" >&2
		exit 1
	fi
	write_env_file
	export_android_env
}

resolve_sysimage() {
	local abi="$1"
	local sdkmanager="$SDK_ROOT/cmdline-tools/latest/bin/sdkmanager"
	local list candidate
	list="$("$sdkmanager" --sdk_root="$SDK_ROOT" --list 2>/dev/null || true)"
	for candidate in \
		"system-images;android-${COMPILE_SDK};google_apis;${abi}" \
		"system-images;android-${COMPILE_SDK}.0;google_apis;${abi}" \
		"system-images;android-${COMPILE_SDK};google_apis_playstore;${abi}" \
		"system-images;android-${COMPILE_SDK}.0;google_apis_playstore;${abi}" \
		"system-images;android-${COMPILE_SDK}.0;google_apis_ps16k;${abi}" \
		"system-images;android-${COMPILE_SDK};default;${abi}" \
		"system-images;android-${COMPILE_SDK}.0;default;${abi}"
	do
		if printf '%s\n' "$list" | grep -F "$candidate" >/dev/null; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done
	echo "No Android ${COMPILE_SDK} system image for ABI ${abi}" >&2
	echo "sdkmanager --list output did not contain a google_apis or default image." >&2
	return 1
}

avd_name() {
	printf '%s\n' "${ANDROID_AVD:-vengi-$(host_abi)}"
}

emulator_unit() {
	local name
	name="$(printf '%s' "$(avd_name)" | tr -c 'A-Za-z0-9' '-')"
	printf 'vengi-emulator-%s.service\n' "$name"
}

setup_emulator() {
	local image avd avdmanager
	image="$(resolve_sysimage "$(host_abi)")"
	avd="$(avd_name)"
	install_sdk_packages emulator "$image"
	export_android_env
	avdmanager="$SDK_ROOT/cmdline-tools/latest/bin/avdmanager"
	if [ -f "$HOME/.android/avd/${avd}.ini" ]; then
		echo "AVD already exists: $avd"
		return 0
	fi
	echo "Creating AVD $avd ($image)"
	if ! printf 'no\n' | "$avdmanager" create avd --force \
		--name "$avd" \
		--package "$image" \
		--device pixel_6; then
		printf 'no\n' | "$avdmanager" create avd --force \
			--name "$avd" \
			--package "$image"
	fi
	local config="$HOME/.android/avd/${avd}.avd/config.ini"
	if [ -f "$config" ]; then
		grep -q '^hw.keyboard=' "$config" || echo 'hw.keyboard=yes' >> "$config"
	fi
}

device_connected() {
	local adb="$SDK_ROOT/platform-tools/adb"
	[ -x "$adb" ] || return 1
	"$adb" start-server >/dev/null 2>&1 || true
	"$adb" devices 2>/dev/null | awk 'NR>1 && $2=="device" {found=1} END {exit found?0:1}'
}

cmd_wait() {
	local adb="$SDK_ROOT/platform-tools/adb"
	local timeout="${ANDROID_BOOT_TIMEOUT:-300}"
	local boot
	echo "Waiting for Android device/emulator..."
	"$adb" wait-for-device
	while [ "$timeout" -gt 0 ]; do
		boot="$("$adb" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')"
		if [ "$boot" = "1" ]; then
			echo "Device booted"
			return 0
		fi
		sleep 2
		timeout=$((timeout - 2))
	done
	echo "Timed out waiting for the emulator to boot. See $BUILD_DIR/emulator.log" >&2
	exit 1
}

cmd_emulator() {
	setup_sdk_ndk
	setup_emulator
	if device_connected; then
		echo "Android device already connected"
		return 0
	fi
	local emu="$SDK_ROOT/emulator/emulator"
	local avd args=()
	avd="$(avd_name)"
	if [ ! -x "$emu" ]; then
		echo "Emulator binary not found: $emu" >&2
		exit 1
	fi
	args=(-avd "$avd" -netdelay none -netspeed full)
	if [ -z "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] || [ "${ANDROID_EMULATOR_HEADLESS:-}" = "1" ]; then
		args+=(-no-window -no-audio -gpu swiftshader_indirect)
	fi
	if [ ! -e /dev/kvm ]; then
		echo "Warning: /dev/kvm is missing - the emulator will be slow" >&2
		args+=(-accel off -gpu swiftshader_indirect)
	elif [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] && [ "${ANDROID_EMULATOR_HEADLESS:-}" != "1" ]; then
		# gfxstream (-gpu host / auto) segfaults in glestranslator on glTexImage2D
		# (emulator 37, NVIDIA). Software GLES stays up when an image is uploaded.
		args+=(-gpu "${ANDROID_EMULATOR_GPU:-swiftshader_indirect}")
	fi
	mkdir -p "$BUILD_DIR"
	echo "Starting emulator AVD $avd"
	# A transient user service is not a child of this shell. Otherwise the
	# emulator exits when the caller (make, or a short-lived shell) does.
	local launch unit
	launch="$BUILD_DIR/emulator-launch.sh"
	{
		echo '#!/bin/bash'
		printf 'exec '
		printf '%q ' "$emu" "${args[@]}"
		printf '> %q 2>&1\n' "$BUILD_DIR/emulator.log"
	} >"$launch"
	chmod +x "$launch"
	if command -v systemd-run >/dev/null 2>&1 && [ -n "${XDG_RUNTIME_DIR:-}" ]; then
		unit="$(emulator_unit)"
		systemctl --user stop "$unit" >/dev/null 2>&1 || true
		systemd-run --user --collect --unit="${unit%.service}" \
			-p MemoryMax=6G -p CPUQuota=300% -- "$launch"
		systemctl --user show -p MainPID --value "$unit" >"$BUILD_DIR/emulator.pid" || true
	else
		setsid "$launch" </dev/null &
		echo $! >"$BUILD_DIR/emulator.pid"
	fi
	cmd_wait
}

cmd_stop() {
	local adb="$SDK_ROOT/platform-tools/adb"
	if command -v systemctl >/dev/null 2>&1 && [ -n "${XDG_RUNTIME_DIR:-}" ]; then
		systemctl --user stop "$(emulator_unit)" >/dev/null 2>&1 || true
	fi
	if [ -x "$adb" ]; then
		"$adb" start-server >/dev/null 2>&1 || true
		"$adb" emu kill >/dev/null 2>&1 || true
	fi
	if [ -f "$BUILD_DIR/emulator.pid" ]; then
		kill "$(cat "$BUILD_DIR/emulator.pid")" >/dev/null 2>&1 || true
		rm -f "$BUILD_DIR/emulator.pid"
	fi
	pkill -f "qemu-system.*$(avd_name)" >/dev/null 2>&1 || true
	local n=0
	while [ "$n" -lt 20 ]; do
		if ! device_connected; then
			echo "Emulator stopped"
			return 0
		fi
		sleep 1
		n=$((n + 1))
	done
	echo "Emulator may still be shutting down" >&2
}

native_build_type() {
	if [ -n "${BUILDTYPE:-}" ]; then
		printf '%s\n' "$BUILDTYPE"
	else
		printf 'Release\n'
	fi
}

cache_abi() {
	local cache="$1/CMakeCache.txt"
	if [ ! -f "$cache" ]; then
		return 0
	fi
	# The NDK toolchain stores the ABI as ANDROID_ABI (UNINITIALIZED). SDL's
	# check_c_source_compiles results are cached, so an in-place ABI switch
	# keeps arm64-only sources such as SVE2. Read whichever key is present.
	sed -n -e 's/^CMAKE_ANDROID_ARCH_ABI:[^=]*=//p' -e 's/^ANDROID_ABI:[^=]*=//p' "$cache" | head -1
}

ensure_host_codegen() {
	need_cmd cmake
	need_cmd ninja
	if [ ! -f "$HOST_DIR/CMakeCache.txt" ]; then
		echo "Configuring host codegen in $HOST_DIR"
		cmake -S "$ROOT" -B "$HOST_DIR" -G Ninja \
			-DCMAKE_BUILD_TYPE=Release \
			-DUSE_SANITIZERS=OFF \
			-DUSE_LINK_TIME_OPTIMIZATION=OFF \
			-DUSE_BENCHMARKS=OFF \
			-DUNITTESTS=OFF \
			-DVISUALTESTS=OFF \
			-DVOXCONVERT=OFF \
			-DPALCONVERT=OFF \
			-DTHUMBNAILER=OFF \
			-DUSE_LIBS_FORCE_LOCAL=ON \
			-DUSE_GLSLANG_VALIDATOR=OFF \
			-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON \
			-DSDL_UNIX_CONSOLE_BUILD=ON
	fi
	echo "Generating shader and font sources"
	cmake --build "$HOST_DIR" --target engine_codegen
}

configure_android_tree() {
	local dir="$1"
	local abi="$2"
	shift 2
	local existing
	existing="$(cache_abi "$dir" || true)"
	if [ -n "$existing" ] && [ "$existing" != "$abi" ]; then
		echo "ABI changed ($existing -> $abi), reconfiguring $dir"
		rm -rf "$dir"
	fi
	need_cmd cmake
	need_cmd ninja
	mkdir -p "$dir"
	echo "Using ANDROID_NDK_HOME=$ANDROID_NDK_HOME"
	echo "Using ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT"
	echo "Using ANDROID_ABI=$abi"
	cmake -S "$ROOT" -B "$dir" -G Ninja \
		-DCMAKE_BUILD_TYPE="$(native_build_type)" \
		-DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/toolchains/android-toolchain.cmake" \
		-DANDROID_ABI="$abi" \
		-DANDROID_PLATFORM="$NATIVE_PLATFORM" \
		-DANDROID_STL=c++_static \
		-DGENERATE_DIR="$HOST_DIR/generated" \
		-DUSE_SANITIZERS=OFF \
		-DUSE_LINK_TIME_OPTIMIZATION=OFF \
		-DUSE_BENCHMARKS=OFF \
		-DVISUALTESTS=OFF \
		-DUSE_LIBS_FORCE_LOCAL=ON \
		-DUSE_OPENGLES=ON \
		-DUSE_GLSLANG_VALIDATOR=OFF \
		-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON \
		-DCMAKE_UNITY_BUILD=ON \
		"$@"
}

configure_native() {
	local abi="${ANDROID_ABI:-arm64-v8a}"
	configure_android_tree "$BUILD_DIR" "$abi" \
		-DUNITTESTS=OFF \
		-DVOXCONVERT=OFF \
		-DPALCONVERT=OFF \
		-DTHUMBNAILER=OFF \
		-DVOXEDIT=ON \
		-DUSE_IMGUITESTENGINE=ON
}

configure_tests() {
	# The emulator AVD matches the host CPU. Test binaries have to match it.
	local abi
	abi="$(host_abi)"
	export ANDROID_ABI="$abi"
	configure_android_tree "$TESTS_DIR" "$abi" \
		-DUNITTESTS=ON \
		-DVOXCONVERT=OFF \
		-DPALCONVERT=OFF \
		-DTHUMBNAILER=OFF \
		-DVOXEDIT=ON \
		-DUSE_IMGUITESTENGINE=OFF
}

write_local_properties() {
	printf 'sdk.dir=%s\n' "$ANDROID_SDK_ROOT" > "$ROOT/android-project/local.properties"
}

apk_path() {
	printf '%s\n' "$ROOT/android-project/app/build/outputs/apk/debug/app-debug.apk"
}

stage_native_libs() {
	local abi dest mainso
	abi="${ANDROID_ABI:-arm64-v8a}"
	dest="$BUILD_DIR/android/jniLibs/$abi"
	rm -rf "$BUILD_DIR/android/jniLibs"
	mkdir -p "$dest"
	mainso="$(find "$BUILD_DIR" -name libmain.so -type f ! -path '*/CMakeFiles/*' -print -quit)"
	if [ -z "$mainso" ]; then
		echo "libmain.so was not produced" >&2
		exit 1
	fi
	cp -a "$mainso" "$dest/"
	local so base
	while IFS= read -r so; do
		base="$(basename "$so")"
		case "$base" in
			libmain.so) continue ;;
		esac
		if [ ! -f "$dest/$base" ]; then
			cp -a "$so" "$dest/"
		fi
	done < <(find "$BUILD_DIR" -name 'lib*.so' -type f ! -path '*/CMakeFiles/*')
	echo "Staged native libraries in $dest"
	ls -l "$dest"
}

stage_assets() {
	local src="$BUILD_DIR/voxedit"
	local dest="$BUILD_DIR/android/assets"
	rm -rf "$dest"
	mkdir -p "$dest"
	if [ ! -d "$src" ]; then
		echo "Runtime data directory missing: $src" >&2
		exit 1
	fi
	# cmake configure copies lua, translations and packaged data next to the app.
	cp -a "$src"/. "$dest/"
	find "$dest" -name 'lib*.so' -delete
	find "$dest" -name '*.a' -delete
	echo "Staged assets in $dest"
}

cmd_build() {
	check_java
	setup_sdk_ndk
	ensure_host_codegen
	configure_native
	cmake --build "$BUILD_DIR" --target voxedit
	stage_native_libs
	stage_assets
	write_local_properties
	local abi="${ANDROID_ABI:-arm64-v8a}"
	(
		cd "$ROOT/android-project"
		./gradlew --no-daemon assembleDebug \
			-PnativeLibDir="$BUILD_DIR/android/jniLibs" \
			-PassetsDir="$BUILD_DIR/android/assets"
	)
	echo "APK: $(apk_path)"
}

cmd_install() {
	local adb apk
	setup_sdk_ndk
	if ! device_connected; then
		cmd_emulator
	else
		cmd_wait
	fi
	adb="$SDK_ROOT/platform-tools/adb"
	apk="$(apk_path)"
	if [ ! -f "$apk" ]; then
		echo "APK not found: $apk (build it with: $0 build)" >&2
		exit 1
	fi
	echo "Installing $apk"
	"$adb" install -r "$apk"
}

cmd_start() {
	local adb="$SDK_ROOT/platform-tools/adb"
	setup_sdk_ndk
	if ! device_connected; then
		echo "No Android device connected" >&2
		exit 1
	fi
	"$adb" shell am start -n "$ACTIVITY"
}

cmd_logs() {
	setup_sdk_ndk
	if ! device_connected; then
		echo "No Android device connected. Start one with: make android-emulator" >&2
		exit 1
	fi
	local adb="$SDK_ROOT/platform-tools/adb"
	local filters=(
		SDL:V
		VoxEdit:V
		DEBUG:I
		AndroidRuntime:V
		libc:V
		ActivityManager:I
		ActivityTaskManager:I
	)
	if [ "${ANDROID_LOGS_DUMP:-}" = "1" ]; then
		"$adb" logcat -d -v threadtime -t "${ANDROID_LOGS_LINES:-500}" "${filters[@]}" '*:S'
		return 0
	fi
	echo "Following logcat for ${APP_ID} (Ctrl-C to stop)"
	"$adb" logcat -d -v threadtime -t 400 | grep -E 'F/DEBUG|E/AndroidRuntime|FATAL EXCEPTION|UnsatisfiedLinkError|F/libc' || true
	echo "---- live ----"
	"$adb" logcat -v threadtime "${filters[@]}" '*:S'
}

cmd_run() {
	ANDROID_ABI="${ANDROID_ABI:-$(host_abi)}"
	export ANDROID_ABI
	setup_sdk_ndk
	setup_emulator
	cmd_build
	cmd_emulator
	cmd_install
	cmd_start
}

list_test_targets() {
	# ninja lists each suite twice: the cmake target (phony tests-core) and the
	# linked output (tests-core/vengi-tests-core). The phony helpers
	# (tests-core-run, -debug, -memcheck, ...) are not gtest binaries.
	ninja -C "$TESTS_DIR" -t targets all | awk -F: '
		$2 ~ /phony/ && $1 ~ /^tests-/ &&
		$1 !~ /-(run|debug|memcheck|perf|heaptrack|cachegrind|callgrind|dhat|drd|helgrind|massif)$/ {
			print $1
		}'
}

stage_test_libs() {
	local dest="$1"
	local bin needed found pass
	mkdir -p "$dest"
	if ! command -v readelf >/dev/null 2>&1; then
		find "$TESTS_DIR" -name 'lib*.so' -type f ! -path '*/CMakeFiles/*' -exec cp -a {} "$dest/" \;
		return 0
	fi
	for pass in 1 2 3 4; do
		while IFS= read -r bin; do
			[ -f "$bin" ] || continue
			while IFS= read -r needed; do
				[ -n "$needed" ] || continue
				if [ -f "$dest/$needed" ]; then
					continue
				fi
				found="$(find "$TESTS_DIR" "$dest" -name "$needed" -type f ! -path '*/CMakeFiles/*' -print -quit 2>/dev/null || true)"
				if [ -n "$found" ] && [ "$found" != "$dest/$needed" ]; then
					cp -a "$found" "$dest/$needed"
				fi
			done < <(readelf -d "$bin" 2>/dev/null | sed -n 's/.*NEEDED.*\[\(.*\)\].*/\1/p')
		done < <(find "$TESTS_DIR" "$dest" -type f ! -path '*/CMakeFiles/*' \( -name 'vengi-tests-*' -o -name 'lib*.so' \))
	done
}

cmd_tests() {
	local adb remote_root libdir timeout log failed=0 ran=0
	local -a targets=()
	check_java
	setup_sdk_ndk
	ensure_host_codegen
	configure_tests
	mapfile -t targets < <(list_test_targets)
	if [ ${#targets[@]} -eq 0 ]; then
		echo "No unit test targets were generated in $TESTS_DIR" >&2
		exit 1
	fi
	echo "Building ${#targets[@]} Android unit test binaries"
	cmake --build "$TESTS_DIR" --target "${targets[@]}"
	setup_emulator
	cmd_emulator
	adb="$SDK_ROOT/platform-tools/adb"
	# shell_data_file cannot create fifo_file. The userdebug emulator allows
	# adb root, and the su domain can. Production devices stay unrooted.
	if "$adb" root 2>&1 | grep -q "cannot run as root"; then
		echo "adb root is unavailable; named-pipe tests may fail" >&2
	else
		"$adb" wait-for-device
	fi
	remote_root="/data/local/tmp/vengi-tests"
	libdir="$TESTS_DIR/android-test-libs"
	rm -rf "$libdir"
	stage_test_libs "$libdir"
	timeout="${ANDROID_TEST_TIMEOUT:-900}"
	log="$TESTS_DIR/android-test-results.txt"
	: >"$log"
	echo "Pushing test libraries"
	"$adb" shell "rm -rf $remote_root && mkdir -p $remote_root/lib $remote_root/tmp"
	local -a solibs=()
	shopt -s nullglob
	solibs=("$libdir"/*.so)
	shopt -u nullglob
	if [ ${#solibs[@]} -gt 0 ]; then
		"$adb" push "$libdir/." "$remote_root/lib/"
	fi
	local name bin base remote rc gtest_filter
	gtest_filter=""
	if [ -n "${GTEST_FILTER:-}" ]; then
		gtest_filter=" --gtest_filter='$GTEST_FILTER'"
	fi
	for name in "${targets[@]}"; do
		bin="$TESTS_DIR/$name/vengi-$name"
		if [ ! -f "$bin" ]; then
			echo "MISSING $name" | tee -a "$log"
			failed=$((failed + 1))
			continue
		fi
		base="$(basename "$bin")"
		remote="$remote_root/$name"
		echo "Running $name"
		"$adb" shell "mkdir -p $remote"
		"$adb" push "$TESTS_DIR/$name/." "$remote/" >/dev/null
		"$adb" shell "chmod 755 $remote/$base"
		set +e
		timeout --foreground "$timeout" "$adb" shell \
			"cd $remote && LD_LIBRARY_PATH=$remote_root/lib HOME=$remote TMPDIR=$remote_root/tmp SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy ./$base --gtest_brief=1 --gtest_print_time=1$gtest_filter"
		rc=$?
		set -e
		ran=$((ran + 1))
		if [ "$rc" -eq 0 ]; then
			echo "PASS $name" | tee -a "$log"
		elif [ "$rc" -eq 124 ]; then
			echo "TIMEOUT $name" | tee -a "$log"
			failed=$((failed + 1))
		else
			echo "FAIL $name (exit $rc)" | tee -a "$log"
			failed=$((failed + 1))
		fi
	done
	echo "Android unit tests: $ran run, $failed failed. Log: $log"
	if [ "$failed" -ne 0 ]; then
		exit 1
	fi
}

cmd_setup() {
	check_java
	setup_sdk_ndk
	setup_emulator
	write_env_file
	echo "Android setup complete."
	echo "  ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT"
	echo "  ANDROID_NDK_HOME=$ANDROID_NDK_HOME"
	echo "  AVD=$(avd_name)"
	echo "Source $SDK_ROOT/android-env.sh or use: make android / make android-run"
}

cmd="${1:-build}"
if [ $# -gt 0 ]; then
	shift
fi

case "$cmd" in
	setup) cmd_setup ;;
	build) cmd_build ;;
	emulator) cmd_emulator ;;
	wait) export_android_env; cmd_wait ;;
	install) cmd_install ;;
	start) cmd_start ;;
	logs) cmd_logs ;;
	run) cmd_run ;;
	tests) cmd_tests ;;
	stop) cmd_stop ;;
	help|-h|--help) usage ;;
	*)
		echo "Unknown command: $cmd" >&2
		usage >&2
		exit 1
		;;
esac
