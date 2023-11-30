/**
 * @file
 */

#include "FormatPrinter.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include "palette/Palette.h"
#include "voxelformat/VolumeFormat.h"

FormatPrinter::FormatPrinter(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider) {
	init(ORGANISATION, "formatprinter");
}

app::AppState FormatPrinter::onConstruct() {
	registerArg("--palette").setDescription("Print the supported palettes");
	registerArg("--image").setDescription("Print the supported image");
	registerArg("--voxel").setDescription("Print the supported voxel formats");
	return Super::onConstruct();
}

template<class T>
static void printStringArray(const T &array) {
	int i = 0;
	for (const core::String &e : array) {
		if (i != 0) {
			printf(",");
		}
		printf("\"%s\"", e.c_str());
		++i;
	}
}

static bool voxelSaveSupported(const io::FormatDescription &desc) {
	for (const io::FormatDescription *d = voxelformat::voxelSave(); d->valid(); ++d) {
		int foundExtensionMatch = 0;
		for (const core::String &ext : d->exts) {
			if (!desc.matchesExtension(ext)) {
				continue;
			}
			if (desc.name == d->name) {
				Log::debug("Found save format %s", d->name.c_str());
				return true;
			}
			++foundExtensionMatch;
			Log::debug("Found save format by extension %s but it does not match by name %s vs %s", ext.c_str(),
						desc.name.c_str(), d->name.c_str());
			break;
		}
		Log::debug("Found matches for %s with %i extensions", desc.name.c_str(), foundExtensionMatch);
		if (foundExtensionMatch == 1) {
			return true;
		}
	}
	return false;
}

app::AppState FormatPrinter::onRunning() {
	const bool palette = hasArg("--palette");
	const bool image = hasArg("--image");
	const bool voxel = hasArg("--voxel");
	if (!palette && !image && !voxel) {
		usage();
		return app::AppState::Cleanup;
	}
	printf("{");
	if (palette) {
		printf("\"palettes\": [");
		int i = 0;
		for (const io::FormatDescription *desc = io::format::palettes(); desc->valid(); ++desc) {
			if (i != 0) {
				printf(",");
			}
			printf("{");
			printf("\"name\": \"%s\",", desc->name.c_str());
			printf("\"extensions\": [");
			printStringArray(desc->exts);
			printf("]");
			printf("}");
			++i;
		}
		printf("]");
	}
	if (image) {
		if (palette) {
			printf(",");
		}
		printf("\"images\": [");
		int i = 0;
		for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
			if (i != 0) {
				printf(",");
			}
			printf("{");
			printf("\"name\": \"%s\",", desc->name.c_str());
			printf("\"extensions\": [");
			printStringArray(desc->exts);
			printf("]");
			printf("}");
			++i;
		}
		printf("]");
	}
	if (voxel) {
		if (palette || image) {
			printf(",");
		}
		printf("\"voxels\": [");
		int i = 0;
		for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
			if (i != 0) {
				printf(",");
			}
			printf("{");
			printf("\"name\": \"%s\",", desc->name.c_str());
			printf("\"extensions\": [");
			printStringArray(desc->exts);
			printf("],");
			// TODO: maybe add a mimetype to the format description
			core::String name = desc->name.toLower();
			if (name.contains(" ")) {
				core::string::replaceAllChars(name, ' ', '-');
			}
			printf("\"mimetype\": \"application/x-%s\",", name.c_str());
			if (voxelformat::isMeshFormat(*desc)) {
				printf("\"mesh\": true,");
			}
			if (voxelformat::isAnimationSupported(*desc)) {
				printf("\"animation\": true,");
			} else {
				printf("\"animation\": false,");
			}
			if (voxelSaveSupported(*desc)) {
				printf("\"save\": true");
			} else {
				printf("\"save\": false");
			}
			printf("}");
			++i;
		}
		printf("]");
	}
	printf("}\n");
	return app::AppState::Cleanup;
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	FormatPrinter app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
