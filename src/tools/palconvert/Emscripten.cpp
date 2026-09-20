/**
 * @file
 */

#include "PalConvert.h"
#include "core/String.h"
#include "core/TimeProvider.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "palette/PaletteFormatDescription.h"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE
#endif

extern "C" EMSCRIPTEN_KEEPALIVE void convert_file(char *input, char *output) {
	if (input == nullptr || output == nullptr) {
		return;
	}
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	PalConvert app(filesystem, timeProvider);
	char appnameArg[] = "palconvert";
	char inputArg[] = "--input";
	char outputArg[] = "--output";
	char forceArg[] = "--force";
	char *argv[] = {appnameArg, inputArg, input, outputArg, output, forceArg};
	app.startMainLoop(lengthof(argv), argv);
}

extern "C" EMSCRIPTEN_KEEPALIVE const char *get_config_json() {
	static core::String formats;
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	PalConvert app(filesystem, timeProvider);
	char appnameArg[] = "palconvert";
	char versionArg[] = "--version";
	char *argv[] = {appnameArg, versionArg};
	app.setArgs(lengthof(argv), argv);
	app.onConstruct();
	io::BufferedReadWriteStream stream;
	app.writeConfigJson(stream);
	app.onCleanup();
	app.onDestroy();
	formats = core::String((const char *)stream.getBuffer(), stream.size());
	return formats.c_str();
}

extern "C" EMSCRIPTEN_KEEPALIVE const char *get_supported_formats_json() {
	static core::String formats;
	io::BufferedReadWriteStream stream;
	stream.writeString("{\"palettes\":[", false);
	io::format::writeJson(stream, palette::palettes(), {{"save", FORMAT_FLAG_SAVE}});
	stream.writeString("]}", false);
	formats = core::String((const char *)stream.getBuffer(), stream.size());
	return formats.c_str();
}
