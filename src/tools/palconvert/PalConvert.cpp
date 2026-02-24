/**
 * @file
 */

#include "PalConvert.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "core/concurrent/Concurrency.h"
#include "engine-git.h"
#include "io/Archive.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "palette/FormatConfig.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "palette/PaletteFormatDescription.h"
#include "palette/private/PaletteFormat.h"

PalConvert::PalConvert(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider, core::cpus()) {
	init(ORGANISATION, "palconvert");
	_wantCrashLogs = true;
}

void PalConvert::printUsageHeader() const {
	Super::printUsageHeader();
	Log::info("Git commit " GIT_COMMIT " - " GIT_COMMIT_DATE);
}

void PalConvert::usage() const {
	Super::usage();
	Log::info("Supported palette formats:");
	int maxNameLen = 0;
	int maxExtLen = 0;
	for (const io::FormatDescription *desc = palette::palettes(); desc->valid(); ++desc) {
		maxNameLen = core_max(maxNameLen, (int)desc->name.size());
		for (const core::String &ext : desc->exts) {
			maxExtLen = core_max(maxExtLen, (int)ext.size());
		}
	}
	for (const io::FormatDescription *desc = palette::palettes(); desc->valid(); ++desc) {
		const char *save = desc->flags & FORMAT_FLAG_SAVE ? "yes" : "no";
		for (const core::String &ext : desc->exts) {
			Log::info(" * %-*s (*.%-*s) (save: %s)", maxNameLen, desc->name.c_str(), maxExtLen, ext.c_str(), save);
		}
	}
	Log::info("Built-in palettes:");
	for (int i = 0; i < lengthof(palette::Palette::builtIn); ++i) {
		Log::info(" * %s", palette::Palette::builtIn[i]);
	}

	usageFooter();
}

app::AppState PalConvert::onConstruct() {
	const app::AppState state = Super::onConstruct();
	registerArg("--input")
		.setShort("-i")
		.setDescription("Allow to specify input files")
		.addFlag(ARGUMENT_FLAG_FILE | ARGUMENT_FLAG_MANDATORY);
	registerArg("--type").setShort("-t").setDescription("Specify the output type (ansi, json, hex)");
	registerArg("--force").setShort("-f").setDescription("Overwrite existing files");
	registerArg("--output")
		.setShort("-o")
		.setDescription("Allow to specify the output file")
		.addFlag(ARGUMENT_FLAG_FILE);
	registerArg("--quantize").setShort("-q").setDescription("Quantize the input palette to 256 colors");
	registerArg("--optimize").setDescription("Optimize the palette by removing duplicated or full transparent colors");

	palette::FormatConfig::init();

	return state;
}

static void printJsonPalette(const palette::ColorPalette &palette) {
	Log::printf("{");
	Log::printf("\"name\":\"%s\",", palette.name().c_str());
	Log::printf("\"colors\":[");
	for (int i = 0; i < (int)palette.size(); ++i) {
		const color::RGBA color = palette.color(i);
		Log::printf("{");
		Log::printf("\"r\":%u,\"g\":%u,\"b\":%u,\"a\":%u", color.r, color.g, color.b, color.a);
		if (!palette.colorName(i).empty()) {
			Log::printf(",\"name\":\"%s\"", palette.colorName(i).c_str());
		}
		const palette::Material &mat = palette.material(i);
		int n = palette::MaterialProperty::MaterialMetal;
		const int maxN = palette::MaterialProperty::MaterialMax;
		Log::printf(",\"material\":{");
		int matPrinted = 0;
		for (; n < maxN; ++n) {
			const palette::MaterialProperty propEnum = (palette::MaterialProperty)n;
			if (!mat.has(propEnum)) {
				continue;
			}
			if (matPrinted > 0) {
				Log::printf(",");
			}
			Log::printf("\"%s\":%f", palette::MaterialPropertyName(propEnum), mat.value(propEnum));
			matPrinted++;
		}
		Log::printf("}"); // material
		Log::printf("}"); // color

		if (i != (int)palette.size() - 1) {
			Log::printf(",");
		}
	}
	Log::printf("]");
	Log::printf("}\n");
}

static void printHexPalette(const palette::ColorPalette &palette) {
	for (int i = 0; i < (int)palette.size(); ++i) {
		const color::RGBA color = palette.color(i);
		Log::printf("0x%02x%02x%02x%02x", color.r, color.g, color.b, color.a);
		if (i != (int)palette.size() - 1) {
			Log::printf(", ");
		}
	}
	Log::printf("\n");
}

bool PalConvert::handleInputFile(const core::String &infile, const core::String &outfile) {
	Log::info("-- current input file: %s", infile.c_str());
	palette::ColorPalette palette;
	if (palette::Palette::isBuiltIn(infile) || palette::Palette::isLospec(infile)) {
		palette::Palette pal;
		if (!pal.load(infile.c_str())) {
			Log::error("Failed to load palette from '%s'", infile.c_str());
			return false;
		}
		palette = palette::toColorPalette(pal);
	} else {
		const io::FilePtr &file = _filesystem->open(infile);
		io::FileStream stream(file);
		if (!stream.valid()) {
			Log::error("Failed to open input file '%s'", infile.c_str());
			return false;
		}
		if (hasArg("--quantize")) {
			palette::Palette pal;
			if (!palette::loadPalette(infile, stream, pal)) {
				Log::error("Failed to load palette from '%s'", infile.c_str());
				return false;
			}
			palette = palette::toColorPalette(pal);
			Log::info("Quantized palette to %i colors", palette.colorCount());
		} else if (!palette::loadPalette(infile, stream, palette)) {
			Log::error("Failed to load palette from '%s'", infile.c_str());
			return false;
		}
	}

	Log::info("Palette with %i colors loaded from '%s' with name '%s'\n", (int)palette.size(), infile.c_str(),
			  palette.name().c_str());

	if (hasArg("--optimize")) {
		palette.optimize();
		Log::info("Optimized palette to %i colors", palette.colorCount());
	}

	if (outfile.empty()) {
		const core::String type = getArgVal("--type", "ansi");
		if (type == "json") {
			printJsonPalette(palette);
		} else if (type == "hex") {
			printHexPalette(palette);
		} else {
			const core::String &paletteName = palette.name();
			if (!paletteName.empty()) {
				Log::printf("Palette name: %s\n", paletteName.c_str());
			}
			const core::String palStr = palette::ColorPalette::print(palette);
			Log::printf("%s", palStr.c_str());
			Log::printf("\n");

			for (int i = 0; i < palette.colorCount(); ++i) {
				const core::String &name = palette.colorName(i);
				if (name.empty()) {
					continue;
				}
				Log::printf("%03i: %s\n", i, name.c_str());
			}
		}
	} else {
		const io::FilePtr &file = _filesystem->open(outfile, io::FileMode::SysWrite);
		io::FileStream stream(file);
		if (!palette::savePalette(palette, outfile, stream)) {
			Log::error("Failed to save palette to '%s'", outfile.c_str());
			return false;
		}
		Log::info("Palette saved to '%s'", outfile.c_str());
	}

	return true;
}

app::AppState PalConvert::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (_argc < 2) {
		_logLevelVar->setVal((int)Log::Level::Info);
		Log::init();
		usage();
		return app::AppState::InitFailure;
	}

	core::String infile;
	if (hasArg("--input")) {
		infile = getArgVal("--input");
		io::normalizePath(infile);
	}

	core::String outfile;
	if (hasArg("--output")) {
		outfile = getArgVal("--output");
		io::normalizePath(outfile);
	}

	Log::info("* input file:        - %s", infile.c_str());
	if (!outfile.empty()) {
		Log::info("* output file:       - %s", outfile.c_str());
	}

	if (core::Var::getVar(cfg::MetricFlavor)->strVal().empty()) {
		Log::info(
			"Please enable anonymous usage statistics. You can do this by setting the metric_flavor cvar to 'json'");
		Log::info("Example: '%s -set metric_flavor json --input xxx --output yyy'", fullAppname().c_str());
	}

	if (!outfile.empty()) {
		if (!hasArg("--force")) {
			const bool outfileExists = filesystem()->open(outfile)->exists();
			if (outfileExists) {
				Log::error("Given output file '%s' already exists", outfile.c_str());
				return app::AppState::InitFailure;
			}
		}
	}

	if (!handleInputFile(infile, outfile)) {
		return app::AppState::InitFailure;
	}

	return state;
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	PalConvert app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
