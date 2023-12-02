/**
 * @file
 */

#include "FormatPrinter.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include "voxelformat/VolumeFormat.h"
#include <SDL_stdinc.h>

FormatPrinter::FormatPrinter(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider) {
	init(ORGANISATION, "formatprinter");
}

app::AppState FormatPrinter::onConstruct() {
	registerArg("--palette").setDescription("Print the supported palettes");
	registerArg("--image").setDescription("Print the supported image");
	registerArg("--voxel").setDescription("Print the supported voxel formats");
	registerArg("--mimeinfo").setDescription("Generate the mimeinfo file for voxel formats");
	registerArg("--plist").setDescription("Generate the plist file for voxel formats");
	return Super::onConstruct();
}

template<class T>
static void printJsonStringArray(const T &array) {
	int i = 0;
	for (const core::String &e : array) {
		if (i != 0) {
			printf(",");
		}
		printf("\"%s\"", e.c_str());
		++i;
	}
}

template<class T>
static void printJsonMagicArray(const T &array) {
	int i = 0;
	for (const core::String &e : array) {
		if (i != 0) {
			printf(",");
		}
		printf("{");
		if (SDL_isprint(e.first())) {
			printf("\"type\": \"string\", \"value\": \"%s\"", e.c_str());
		} else {
			printf("\"type\": \"bytes\", \"value\": \"");
			printf("0x");
			for (size_t i = 0; i < e.size(); ++i) {
				printf("%02X", (uint8_t)e[i]);
			}
			printf("\"");
		}
		printf("}");
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
	if (hasArg("--mimeinfo")) {
		// this is only for voxels
		printMimeInfo();
	} else if (hasArg("--plist")) {
		printApplicationPlist();
	} else {
		const bool palette = hasArg("--palette");
		const bool image = hasArg("--image");
		const bool voxel = hasArg("--voxel");
		if (!palette && !image && !voxel) {
			usage();
			return app::AppState::Cleanup;
		}
		printJson(palette, image, voxel);
	}
	return app::AppState::Cleanup;
}

core::String FormatPrinter::uniqueMimetype(const io::FormatDescription &desc) {
	// TODO: maybe add a mimetype to the format description
	core::String name = desc.name.toLower();
	if (name.contains(" ")) {
		core::string::replaceAllChars(name, ' ', '-');
	}
	core::String mt = core::string::format("application/x-%s", name.c_str());
	if (_uniqueMimetypes.has(mt)) {
		mt += "-" + desc.mainExtension();
	}
	_uniqueMimetypes.insert(mt);
	return mt;
}

void FormatPrinter::printApplicationPlist() {
	printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	printf("<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n");
	printf("<plist version=\"1.0\">\n");
	printf("  <dict>\n");
	printf("    <key>CFBundleDevelopmentRegion</key>\n");
	printf("    <string>en-US</string>\n");
	printf("    <key>CFBundleExecutable</key>\n");
	printf("    <string>${MACOSX_BUNDLE_EXECUTABLE_NAME}</string>\n");
	printf("    <key>CFBundleShortVersionString</key>\n");
	printf("    <string>${MACOSX_BUNDLE_SHORT_VERSION_STRING}</string>\n");
	printf("    <key>CFBundleLongVersionString</key>\n");
	printf("    <string>${MACOSX_BUNDLE_LONG_VERSION_STRING}</string>\n");
	printf("    <key>CFBundleIdentifier</key>\n");
	printf("    <string>${MACOSX_BUNDLE_GUI_IDENTIFIER}</string>\n");
	printf("    <key>CFBundleInfoDictionaryVersion</key>\n");
	printf("    <string>6.0</string>\n");
	printf("    <key>CFBundleIconFile</key>\n");
	printf("    <string>${MACOSX_BUNDLE_ICON_FILE}</string>\n");
	printf("    <key>NSHumanReadableCopyright</key>\n");
	printf("    <string>${MACOSX_BUNDLE_COPYRIGHT}</string>\n");
	printf("    <key>CFBundleName</key>\n");
	printf("    <string>${MACOSX_BUNDLE_BUNDLE_NAME}</string>\n");
	printf("    <key>CFBundlePackageType</key>\n");
	printf("    <string>APPL</string>\n");
	printf("    <key>NSHighResolutionCapable</key>\n");
	printf("    <true/>\n");
	printf("    <key>NSRequiresAquaSystemAppearance</key>\n");
	printf("    <false/>\n");
	printf("    <key>CFBundleDocumentTypes</key>\n");
	printf("    <array>\n");
	for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		const core::String &m = uniqueMimetype(*desc);
		printf("      <dict>\n");
		printf("        <key>CFBundleTypeName</key>\n");
		printf("        <string>%s</string>\n", desc->name.c_str());
		printf("        <key>CFBundleTypeIconFile</key>\n");
		printf("        <string>icon.icns</string>\n");
		printf("        <key>CFBundleTypeMIMETypes</key>\n");
		printf("        <array>\n");
		printf("          <string>%s</string>\n", m.c_str());
		printf("        </array>\n");
		printf("        <key>CFBundleTypeExtensions</key>\n");
		printf("        <array>\n");
		for (const core::String &e : desc->exts) {
			printf("          <string>%s</string>\n", e.c_str());
		}
		printf("        </array>\n");
		printf("        <key>CFBundleTypeRole</key>\n");
		printf("        <string>Editor</string>\n");
		printf("        <key>NSDocumentClass</key>\n");
		printf("        <string>AppDocument</string>\n");
		printf("      </dict>\n");
	}
	printf("    </array>\n");
	printf("  </dict>\n");
	printf("</plist>\n");
}

void FormatPrinter::printMimeInfo() {
	_uniqueMimetypes.clear();
	printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	printf("<mime-info xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\">\n");
	for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		const core::String &m = uniqueMimetype(*desc);
		printf("\t<mime-type type=\"%s\">\n", m.c_str());
		printf("\t\t<comment>%s</comment>\n", desc->name.c_str());
		for (const core::String &e : desc->exts) {
			printf("\t\t<glob pattern=\"*.%s\"/>\n", e.c_str());
		}
		for (const core::String &e : desc->magics) {
			printf("\t\t<magic priority=\"50\">\n");
			if (SDL_isprint(e.first())) {
				printf("\t\t\t<match type=\"string\" offset=\"0\" value=\"%s\"/>\n", e.c_str());
			} else {
				for (size_t i = 0; i < e.size(); ++i) {
					printf("\t\t\t<match type=\"byte\" offset=\"%i\" value=\"%i\"/>\n", (int)i, (int)e[i]);
				}
			}
			printf("\t\t</magic>\n");
		}
		printf("\t</mime-type>\n");
	}
	printf("</mime-info>\n");
}

void FormatPrinter::printJson(bool palette, bool image, bool voxel) {
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
			printJsonStringArray(desc->exts);
			printf("],\"magics\": [");
			printJsonMagicArray(desc->magics);
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
			printJsonStringArray(desc->exts);
			printf("],\"magics\": [");
			printJsonMagicArray(desc->magics);
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
			printJsonStringArray(desc->exts);
			printf("],\"magics\": [");
			printJsonMagicArray(desc->magics);
			printf("],");
			const core::String &m = uniqueMimetype(*desc);
			printf("\"mimetype\": \"%s\",", m.c_str());
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
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	FormatPrinter app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
