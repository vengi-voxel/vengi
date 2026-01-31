/**
 * @file
 */

#include "FormatPrinter.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicStringMap.h"
#include "core/collection/Set.h"
#include "io/FormatDescription.h"
#include "io/MemoryReadStream.h"
#include "io/BufferedReadWriteStream.h"
#include "palette/FormatConfig.h"
#include "palette/PaletteFormatDescription.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelgenerator/LUAApi.h"
#include "json/JSON.h"
#include "engine-config.h"
#include <ctype.h>

FormatPrinter::FormatPrinter(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider) {
	init(ORGANISATION, "formatprinter");
	_saveConfiguration = false;
}

app::AppState FormatPrinter::onConstruct() {
	registerArg("--palette").setDescription("Print the supported palettes");
	registerArg("--image").setDescription("Print the supported image");
	registerArg("--manpage").setDescription("Print the manpage entries for loading and saving");
	registerArg("--voxel").setDescription("Print the supported voxel formats");
	registerArg("--mimeinfo").setDescription("Generate the mimeinfo file for voxel formats");
	registerArg("--markdown").setDescription("Generate the markdown tables for voxel, image and palette formats");
	registerArg("--plist").setDescription("Generate the plist file for voxel formats");
	registerArg("--wix").setDescription("Generate the wix file for msi installers");
	registerArg("--magic").setDescription("Generate the magic file");
	registerArg("--lua-api").setDescription("Generate the lua api markdown documentation files");
	app::AppState state = Super::onConstruct();

	core::Var::visit([this](const core::VarPtr &var) {
		_varsAtStartup.insert(var->name());
	});

	return state;
}

template<class T>
static void printJsonStringArray(const T &array) {
	int i = 0;
	for (const core::String &e : array) {
		if (i != 0) {
			Log::printf(",");
		}
		Log::printf("\"%s\"", e.c_str());
		++i;
	}
}

template<class T>
static void printJsonMagicArray(const T &array) {
	int i = 0;
	for (const io::Magic &e : array) {
		if (i != 0) {
			Log::printf(",");
		}
		Log::printf("{");
		Log::printf("\"type\": \"bytes\", \"value\": \"");
		Log::printf("0x");
		for (int j = 0; j < e.size(); ++j) {
			Log::printf("%02X", (uint8_t)e.data.u8[j]);
		}
		Log::printf("\"");
		Log::printf("}");
		++i;
	}
}

static bool voxelSaveSupported(const io::FormatDescription &desc) {
	if ((desc.flags & FORMAT_FLAG_SAVE) == 0) {
		Log::debug("Format %s does not support saving", desc.name.c_str());
		return false;
	}
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
	} else if (hasArg("--markdown")) {
		printMarkdownTables();
	} else if (hasArg("--magic")) {
		printMagic();
	} else if (hasArg("--manpage")) {
		const core::String app = getArgVal("--manpage", "");
		printManPage(app);
	} else if (hasArg("--plist")) {
		printApplicationPlist();
	} else if (hasArg("--wix")) {
		printInstallerWix();
	} else if (hasArg("--lua-api")) {
		printLuaApiMarkdown();
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
	core::String name = desc.name.toLower();
	core::String mt = desc.mimeType();
	return mt;
}

void FormatPrinter::addManPageOption(const core::String &option, const core::String &description) {
	Log::printf(".TP\n");
	if (core::string::startsWith(option, "-")) {
		Log::printf("\\fB\\%s\\fR\n", option.c_str());
	} else {
		Log::printf("\\fB%s\\fR\n", option.c_str());
	}
	Log::printf("%s\n\n", description.c_str());
}

void FormatPrinter::printManPageVars() {
	core::DynamicArray<core::String> formatVars;
	core::Var::visit([this, &formatVars](const core::VarPtr &var) {
		if (!_varsAtStartup.hasKey(var->name())) {
			formatVars.push_back(var->name());
		}
	});
	formatVars.sort(core::Greater<core::String>());

	Log::printf(".SH CONFIG VARS\n");
	Log::printf("\n");
	for (const core::String &var : formatVars) {
		Log::printf(".PP\n");
		Log::printf("\\fB%s\\fP: %s\n", var.c_str(), core::Var::getSafe(var)->help());
	}
	Log::printf("\n");
}

void FormatPrinter::printManPage(const core::String &app) {
	const bool save = app == "voxconvert";
	const bool commandLineApp = app == "thumbnailer" || app == "voxconvert" || app == "palconvert";
	Log::printf(".\\\" This man page was written by Martin Gerhardy in @COPYRIGHT_MONTH@ @COPYRIGHT_YEAR@. It is provided\n");
	Log::printf(".\\\" under the GNU General Public License 3 or (at your option) any later version.\n");
	Log::printf(".TH @COMMANDLINE@ \"1\" \"@COPYRIGHT_MONTH@ @COPYRIGHT_YEAR@\" \"@COMMANDLINE@\"\n");
	Log::printf(".SH NAME\n");
	Log::printf("@COMMANDLINE@ \\- @DESCRIPTION@\n");
	Log::printf("\n");

	Log::printf(".SH SYNOPSIS\n");
	Log::printf(".PP\n");
	if (app == "thumbnailer") {
		Log::printf("\\fB@NAME@\\fR [\\fIoption\\fR] -s <size> --input infile --output outfile\n");
		Log::printf(".SH DESCRIPTION\n");
		Log::printf("\\fB@COMMANDLINE@\\fP is a command line application that can create thumbnails from\n");
		Log::printf("voxel models.\n");
	} else if (app == "voxconvert") {
		Log::printf("\\fB@NAME@\\fR [\\fIoption\\fR] --input infile --output outfile\n");
		Log::printf(".SH DESCRIPTION\n");
		Log::printf("\\fB@COMMANDLINE@\\fP is a command line application that can convert several voxel\n");
		Log::printf("volume formats into others. Supported formats are e.g. cub (CubeWorld), qb/qbt\n");
		Log::printf("(Qubicle), vox (MagicaVoxel), vmx (VoxEdit Sandbox), kvx (Build engine), kv6 (SLAB6),\n");
		Log::printf("binvox and others. It can also export to mesh formats like obj, gltf, stl and ply with\n");
		Log::printf("a number of options.\n");
	} else if (app == "palconvert") {
		Log::printf("\\fB@NAME@\\fR [\\fIoption\\fR] --input infile --output outfile\n");
		Log::printf(".SH DESCRIPTION\n");
		Log::printf("\\fB@COMMANDLINE@\\fP is a command line application that can convert several palette\n");
		Log::printf("formats into others.\n");
	}
	Log::printf("\n");

	Log::printf(".SH GENERAL OPTIONS\n");
	addManPageOption("--completion bash|zsh", "Generate a bash or zsh-completion script");
	addManPageOption("--help|-h", "Print usage information with a a full list of cvars");
	addManPageOption("--loglevel|-l", "Set the log level to trace, debug, info, warn, error or fatal");
	if (commandLineApp) {
		addManPageOption("--trace|--debug|--info|--warn|--error", "Enable error, warn, trace, debug or info logging");
	}
	addManPageOption("--version|-v", "Print the version of the application.");

	if (app == "thumbnailer") {
		Log::printf(".SH OPTIONS\n");
		addManPageOption("--camera-mode", "Allow to specify the camera mode to render the scene with. Valid values are top, left, right, back, bottom and free");
		addManPageOption("--distance distance", "Set the camera distance to the target");
		addManPageOption("--angles|-a x:y:z", "Set the camera angles (pitch:yaw:roll))");
		addManPageOption("--input infile", "Specify the input file to read from.");
		addManPageOption("--output outfile", "Specify the output file to write to.");
		addManPageOption("--position|-p x:y:z", "Set the camera position");
		addManPageOption("--size|-s size", "Specify the size (same width and height) of the thumbnail.");
		addManPageOption("--turntable|-t", "Render in different angles (16 by default)");
		addManPageOption("--fallback|-f", "Create a fallback thumbnail if an error occurs");
		addManPageOption("--use-scene-camera|-c", "Use a camera that is available in the scene. Not all formats are supporting this feature.");
		palette::FormatConfig::init();
		voxelformat::FormatConfig::init();
		printManPageVars();
	} else if (app == "voxconvert") {
		Log::printf(".SH OPTIONS\n");
		addManPageOption("--crop", "Crop the models to the smallest possible size.");
		addManPageOption("--export-models", "Export all the models of a scene into single files");
		addManPageOption("--export-palette", "Export the palette data into the given output file format");
		addManPageOption("--filter 1-4,6", "Model filter. For example '1-4,6'.");
		addManPageOption("--filter-property name:foo", "Model filter by property. For example 'name:foo'.");
		addManPageOption("--force|-f", "Overwrite existing files.");
		addManPageOption("--input|-i infile", "Specify the input file to read from.");
		addManPageOption("--image", "Print the voxel scene to the console");
		addManPageOption("--json", "Dump the scene graph of the input file. Use \\fBfull\\fR as parameter to also print mesh details");
		addManPageOption("--merge|-m", "Merge models into one volume.");
		addManPageOption("--mirror axis", "Mirror by the given axis (x, y or z)");
		addManPageOption("--output|-o outfile", "Specify the output file to write to.");
		addManPageOption("--print-formats", "Print supported formats as json for easier parsing in other tools.");
		addManPageOption("--print-scripts", "Print found lua scripts as json for easier parsing in other tools.");
		addManPageOption("--rotate axis", "Rotate by 90 degree at the given axis (x, y or z), specify e.g. x:180 to rotate around x by 180 degree.");
		addManPageOption("--resize x:y:z", "Resize the volume by the given x (right), y (up) and z (back) values.");
		addManPageOption("--scale|-s", "Scale model to 50% of its original size.");
		addManPageOption("--script script.lua scriptparameter1 scriptparameter2", "Apply the given lua script to the output volume.");
		addManPageOption("--scriptcolor 1", "Set the palette index that is given to the color script parameters of the main function.");
		addManPageOption("--split x:y:z", "Slices the models into pieces of the given size.");
		addManPageOption("--surface-only", "Remove any non surface voxel. If you are meshing with this, you get also faces on the inner side of your mesh.");
		addManPageOption("--translate|-t x:y:z", "Translate the models by x (right), y (up), z (back).");
		addManPageOption("--wildcard|-w", "Allow to specify input file filter if --input is a directory.");

		palette::FormatConfig::init();
		voxelformat::FormatConfig::init();
		printManPageVars();

		Log::printf(".SH ORDER OF EXECUTION\n");
		Log::printf("\n");
		Log::printf(".TP\n");
		Log::printf("filter\n");
		Log::printf(".TP\n");
		Log::printf("merge\n");
		Log::printf(".TP\n");
		Log::printf("lod 50%% downsampling\n");
		Log::printf(".TP\n");
		Log::printf("resize\n");
		Log::printf(".TP\n");
		Log::printf("mirror\n");
		Log::printf(".TP\n");
		Log::printf("rotate\n");
		Log::printf(".TP\n");
		Log::printf("translate\n");
		Log::printf(".TP\n");
		Log::printf("script\n");
		Log::printf(".TP\n");
		Log::printf("crop\n");
		Log::printf(".TP\n");
		Log::printf("surface-only\n");
		Log::printf(".TP\n");
		Log::printf("split\n");
		Log::printf("\n");
		Log::printf("\n");

		Log::printf(".SH MODELS\n");
		Log::printf("\n");
		Log::printf("Some formats also have multiple model support. Our models are maybe not the models you know from your favorite editor. Each\n");
		Log::printf("model can currently only have one object or volume in it. To get the proper model ids (starting from 0) for your voxel\n");
		Log::printf("file, you should load it once in voxedit and check the model panel or use \\fB--json\\fR to get a list.\n");
		Log::printf("\n");
		Log::printf("Especially magicavoxel supports more objects in one model. This might be confusing to get the right numbers for\n");
		Log::printf("voxconvert.\n");
		Log::printf("\n");

		Log::printf(".SH EXAMPLES\n");
		Log::printf("\n");
		Log::printf(".SS Level of detail (LOD)\n");
		Log::printf("Generate a lod scaled by 50%% from the input model:\n");
		Log::printf("\n");
		Log::printf("\\fB@NAME@ -s --input infile.vengi output.vengi\\fP\n");
		Log::printf("\n");
		Log::printf(".SS Merge several models\n");
		Log::printf("Merge several models into one:\n");
		Log::printf("\n");
		Log::printf("\\fB@NAME@ --input one.vengi --input two.vengi --output onetwo.vengi\\fP\n");
		Log::printf("\n");
		Log::printf(".SS Generate from heightmap\n");
		Log::printf("Just specify the heightmap as input file like this:\n");
		Log::printf("\n");
		Log::printf("\\fB@NAME@ --input heightmap.png --output outfile.vengi -set voxformat_imageimporttype 1\\fP\n");
		Log::printf("\n");
		Log::printf(".SS Translate the voxels\n");
		Log::printf("You can translate the voxels in the world like this:\n");
		Log::printf("\n");
		Log::printf("\\fB@NAME@ --translate 0:10:0 --input heightmap.png --output outfile.vengi\\fP\n");
		Log::printf("\n");
		Log::printf("This would move the voxels 10 units upwards. But keep in mind that not every format supports a translation offset.\n");
		Log::printf("\n");
		Log::printf(".SS Execute lua script\n");
		Log::printf("Use the \\fB--script\\fP parameter:\n");
		Log::printf("\n");
		Log::printf("\\fB@NAME@ --script \"cover 2\" --input infile.vengi --output outfile.vengi\\fP\n");
		Log::printf("\n");
		Log::printf("This is executing the script in \\fB./scripts/cover.lua\\fP with a parameter of \\fB2\\fP.\n");
		Log::printf("\n");
		Log::printf("\\fB@NAME@ --script \"./scripts/cover.lua 2\" --input infile.vengi --output outfile.vengi\\fP\n");
		Log::printf("\n");
		Log::printf("This is doing exactly the same as above - just with a full path.\n");
		Log::printf("\n");
		Log::printf("The scripting docs are available at https://vengi-voxel.github.io/vengi/LUAScript/.\n");
		Log::printf("\n");
		Log::printf(".SS Extract palette png\n");
		Log::printf("Saves the png in the same dir as the vox file:\n");
		Log::printf("\n");
		Log::printf("\\fB@NAME@ --export-palette --input infile.vengi --output outfile-palette.png\\fP\n");
		Log::printf("\n");
		Log::printf("There will be an \\fBinfile.png\\fP now.\n");
		Log::printf("\n");
		Log::printf(".SS Extract single models\n");
		Log::printf("Extract just a few models from the input file.\n");
		Log::printf("\n");
		Log::printf("\\fB@NAME@ --filter 1-2,4 --input infile.vengi --output outfile.vengi\\fP\n");
		Log::printf("\n");
		Log::printf("This will export models 1, 2 and 4.\n");
		Log::printf("\n");
		Log::printf(".SS Convert to mesh\n");
		Log::printf("You can export your volume model into a obj or ply.\n");
		Log::printf("\n");
		Log::printf("\\fB@NAME@ --input infile.vengi --output outfile.obj\\fP\n");
		Log::printf("\n");
		core::DynamicArray<core::String> meshExts;
		for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
			if (voxelformat::isMeshFormat(*desc)) {
				const core::String &ext = desc->exts[0];
				if (core::find(meshExts.begin(), meshExts.end(), ext) != meshExts.end()) {
					continue;
				}
				meshExts.push_back(desc->exts[0]);
			}
		}
		meshExts.sort(core::Greater<core::String>());
		core::String meshExtsStr;
		for (const core::String &e : meshExts) {
			if (!meshExtsStr.empty()) {
				meshExtsStr += ", ";
			}
			meshExtsStr += e;
		}
		Log::printf("\n");
	} else if (app == "palconvert") {
		Log::printf(".SH OPTIONS\n");
		addManPageOption("--force|-f", "Overwrite existing files.");
		addManPageOption("--input|-i infile", "Specify the input file to read from.");
		addManPageOption("--output|-o outfile", "Specify the output file to write to.");
		addManPageOption("--type|-t type", "Specify the output type (ansi, json, hex)");
		palette::FormatConfig::init();
		printManPageVars();
	}

	printManPageFormats(app, save);

	Log::printf(".SH HOMEPAGE\n");
	Log::printf("@CMAKE_PROJECT_HOMEPAGE_URL@\n");
	Log::printf("\n");
	Log::printf(".SH COPYRIGHT\n");
	Log::printf("Copyright \\[co] 2015\\-@COPYRIGHT_YEAR@ by Martin Gerhardy.\n");
	Log::printf("\n");
	Log::printf(".SH BUGS\n");
	Log::printf("If you find a bug, please report it at https://github.com/vengi-voxel/vengi/issues\n");
}

void FormatPrinter::printManPageFormats(const core::String& app, bool save) {
	core::DynamicArray<io::FormatDescription> loadDescriptions;
	core::DynamicArray<io::FormatDescription> saveDescriptions;
	if (app == "palconvert") {
		for (const io::FormatDescription *desc = palette::palettes(); desc->valid(); ++desc) {
			if ((desc->flags & FORMAT_FLAG_NO_LOAD) != 0) {
				continue;
			}
			loadDescriptions.push_back(*desc);
		}
		for (const io::FormatDescription *desc = palette::palettes(); desc->valid(); ++desc) {
			if ((desc->flags & FORMAT_FLAG_SAVE) == 0) {
				continue;
			}
			saveDescriptions.push_back(*desc);
		}
	} else {
		for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
			loadDescriptions.push_back(*desc);
		}
		for (const io::FormatDescription *desc = voxelformat::voxelSave(); desc->valid(); ++desc) {
			saveDescriptions.push_back(*desc);
		}
	}
	saveDescriptions.sort(core::Greater<io::FormatDescription>());
	loadDescriptions.sort(core::Greater<io::FormatDescription>());

	Log::printf(".SH LOAD\n");
	for (const io::FormatDescription &desc : loadDescriptions) {
		Log::printf(".TP\n");
		Log::printf("%s (", desc.name.c_str());
		int ext = 0;
		for (const core::String &e : desc.exts) {
			if (ext > 0) {
				Log::printf(", ");
			}
			Log::printf("*.%s", e.c_str());
			++ext;
		}
		Log::printf(")\n");
	}
	Log::printf("\n");

	if (!save) {
		return;
	}

	Log::printf(".SH SAVE\n");
	for (const io::FormatDescription &desc : saveDescriptions) {
		if (!voxelSaveSupported(desc)) {
			continue;
		}
		Log::printf(".TP\n");
		Log::printf("%s (", desc.name.c_str());
		int ext = 0;
		for (const core::String &e : desc.exts) {
			if (ext > 0) {
				Log::printf(", ");
			}
			Log::printf("*.%s", e.c_str());
			++ext;
		}
		Log::printf(")\n");
	}
	Log::printf("\n");
}

void FormatPrinter::printMagic() {
	core::DynamicArray<io::FormatDescription> formatDescriptions;
	for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		formatDescriptions.push_back(*desc);
	}
	formatDescriptions.sort(core::Greater<io::FormatDescription>());

	for (const io::FormatDescription &desc : formatDescriptions) {
		if (desc.magics.empty()) {
			continue;
		}
		Log::printf("# %s\n", desc.name.c_str());
		const core::String &m = uniqueMimetype(desc);
		for (const io::Magic &magic : desc.magics) {
			Log::printf("50 byte 0 ");
			for (int i = 0; i < magic.size(); ++i) {
				Log::printf("%02X", (uint8_t)magic.data.u8[i]);
			}
			Log::printf(" %s\n", m.c_str());
		}
	}
}

void FormatPrinter::printMarkdownTables() {
	Log::printf("# Formats\n");
	Log::printf("\n");
	Log::printf("## Voxel formats\n");
	Log::printf("\n");
	Log::printf("> The [vengi](FormatSpec.md) format is the best supported format. Saving into any other format might lose several details "
		   "from your scene. This depends on the capabilities of the format and the completeness of the implementation "
		   "for supporting that particular format.\n");
	Log::printf("\n");
	Log::printf("| Name                       | Extension   | Loading | Saving | Thumbnails | Palette | Animations |\n");
	Log::printf("| :------------------------- | ----------- | ------- | ------ | ---------- | ------- | ---------- |\n");
	core::DynamicArray<io::FormatDescription> formatDescriptions;
	for (const io::FormatDescription *desc = voxelformat::voxelFormats(); desc->valid(); ++desc) {
		formatDescriptions.push_back(*desc);
	}
	formatDescriptions.sort(core::Greater<io::FormatDescription>());
	for (const io::FormatDescription &desc : formatDescriptions) {
		if (voxelformat::isMeshFormat(desc)) {
			continue;
		}
		const bool screenshot = desc.flags & VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED;
		const bool palette = desc.flags & VOX_FORMAT_FLAG_PALETTE_EMBEDDED;
		const bool animation = desc.flags & VOX_FORMAT_FLAG_ANIMATION;
		const bool save = voxelSaveSupported(desc);
		const bool load = !(desc.flags & FORMAT_FLAG_NO_LOAD);
		Log::printf("| %-26s | %-11s | %-7s | %-6s | %-10s | %-7s | %-10s |\n", desc.name.c_str(),
					desc.mainExtension().c_str(), load ? "X" : " ", save ? "X" : " ", screenshot ? "X" : " ",
					palette ? "X" : " ", animation ? "X" : " ");
	}
	Log::printf("\n");
	Log::printf("## Mesh formats\n");
	Log::printf("\n");
	Log::printf("| Name                       | Extension | Loading | Saving    | Animations |\n");
	Log::printf("| :------------------------- | --------- | ------- | --------- | ---------- |\n");
	for (const io::FormatDescription &desc : formatDescriptions) {
		if (!voxelformat::isMeshFormat(desc)) {
			continue;
		}
		core::String spec;
		const bool animation = desc.flags & VOX_FORMAT_FLAG_ANIMATION;
		const bool save = voxelSaveSupported(desc);
		const bool load = !(desc.flags & FORMAT_FLAG_NO_LOAD);
		Log::printf("| %-26s | %-9s | %-7s | %-9s | %-10s |\n", desc.name.c_str(), desc.mainExtension().c_str(),
					load ? "X" : " ", save ? "X" : " ", animation ? "X" : " ");
	}
	Log::printf("\nPoint cloud support for `ply` and `gtlf` is implemented, too.\n");
	Log::printf("\n");
	Log::printf("## Palettes\n");
	Log::printf("\n");
	Log::printf("| Name                            | Extension | Loading | Saving |\n");
	Log::printf("| :------------------------------ | --------- | ------- | ------ |\n");

	formatDescriptions.clear();
	for (const io::FormatDescription *desc = palette::palettes(); desc->valid(); ++desc) {
		formatDescriptions.push_back(*desc);
	}
	formatDescriptions.sort(core::Greater<io::FormatDescription>());
	for (const io::FormatDescription &desc : formatDescriptions) {
		const bool save = desc.flags & FORMAT_FLAG_SAVE;
		Log::printf("| %-31s | %-9s | X       | %c      |\n", desc.name.c_str(), desc.mainExtension().c_str(),
			   save ? 'X' : ' ');
	}

	Log::printf("\n> The `gpl` format also supports the [Aseprite extension](https://github.com/aseprite/aseprite/blob/main/docs/gpl-palette-extension.md) for alpha values\n");

	Log::printf("\n");
	Log::printf("## Images/textures\n");
	Log::printf("\n");
	Log::printf("| Name                        | Extension |\n");
	Log::printf("| :-------------------------- | --------- |\n");
	formatDescriptions.clear();
	for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
		formatDescriptions.push_back(*desc);
	}
	formatDescriptions.sort(core::Greater<io::FormatDescription>());
	for (const io::FormatDescription &desc : formatDescriptions) {
		Log::printf("| %-27s | %-9s |\n", desc.name.c_str(), desc.exts[0].c_str());
	}
}

#define REG_PATH_THUMBEXT "ShellEx\\{E357FCCD-A995-4576-B01F-234630154E96}"
#define VOXTHUMB_CLSID "{CD1F0EA0-283C-4D90-A41D-DEBD9207D91F}"
void FormatPrinter::printInstallerWix() {
	Log::printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	Log::printf("<!--\n");
	Log::printf("	using the formatprinter and capturing the stdout to write it into this file can add BOMs under windows\n");
	Log::printf("	if you are getting weird errors about unclosed tokens, remove the BOM\n");
	Log::printf("-->\n");
	Log::printf("<CPackWiXPatch>\n");
	Log::printf("	<!--  Fragment ID is from: <your build dir>/_CPack_Packages/win64/WIX/files.wxs -->\n");
	Log::printf("	<CPackWiXFragment Id=\"CM_CP_voxelthumb.vengi_voxelthumb.dll\">\n");
	Log::printf("		<RegistryKey Root=\"HKCR\" Key=\"CLSID\\" VOXTHUMB_CLSID "\" ForceDeleteOnUninstall=\"yes\">\n");
	Log::printf("			<RegistryValue Type=\"string\" Value=\"Vengi thumbnailer\" />\n");
	Log::printf("			<RegistryKey Key=\"InprocServer32\">\n");
	Log::printf("				<RegistryValue Type=\"string\" Value=\"[#CM_FP_voxelthumb.vengi_voxelthumb.dll]\" />\n");
	Log::printf("				<RegistryValue Type=\"string\" Name=\"ThreadingModel\" Value=\"Both\" />\n");
	Log::printf("			</RegistryKey>\n");
	Log::printf("		</RegistryKey>\n");
	core::StringSet uniqueExtensions;
	for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		for (const core::String &e : desc->exts) {
			if (!uniqueExtensions.insert(e)) {
				continue;
			}
			Log::printf("		<RegistryKey Root=\"HKCR\" Key=\".%s\">\n", e.c_str());
			Log::printf("			<RegistryValue Key=\"" REG_PATH_THUMBEXT"\" Type=\"string\" Value=\"" VOXTHUMB_CLSID "\" />\n");
			Log::printf("		</RegistryKey>\n");
		}
	}
	uniqueExtensions.clear();
	Log::printf("	</CPackWiXFragment>\n");
	Log::printf("	<CPackWiXFragment Id=\"CM_CP_voxedit.voxedit.vengi_voxedit.exe\">\n");
	Log::printf("		<Environment Id=\"PATH\" Name=\"PATH\" Value=\"[INSTALL_ROOT]\" Permanent=\"yes\" Part=\"last\" Action=\"set\" System=\"yes\" />\n");
	Log::printf("\n");
	Log::printf("		<!-- Open With -->\n");
	Log::printf("		<RegistryValue Root=\"HKCR\" Key=\"Applications\\vengi_voxedit.exe\" Type=\"string\" Name=\"FriendlyAppName\" Value=\"Vengi Voxel Editor\"/>\n");
	Log::printf("		<RegistryValue Root=\"HKCR\" Key=\"Applications\\vengi_voxedit.exe\\shell\\open\\command\" Type=\"string\" Value=\"[#CM_FP_voxedit.voxedit.vengi_voxedit.exe] &quot;%%1&quot;\"/>\n");
	Log::printf("\n");
	Log::printf("		<!-- App Paths -->\n");
	Log::printf("		<RegistryKey Root=\"HKLM\" Key=\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\vengi_voxedit.exe\">\n");
	Log::printf("			<RegistryValue Type=\"string\" Value=\"[#CM_FP_voxedit.voxedit.vengi_voxedit.exe]\" />\n");
	Log::printf("			<RegistryValue Type=\"string\" Name=\"Path\" Value=\"[INSTALL_ROOT]\" />\n");
	Log::printf("		</RegistryKey>\n");
	Log::printf("\n");
	Log::printf("		<!-- Default Programs Capabilities -->\n");
	Log::printf("		<RegistryKey Root=\"HKLM\" Key=\"SOFTWARE\\vengi-voxedit\\Capabilities\">\n");
	Log::printf("			<RegistryValue Type=\"string\" Name=\"ApplicationName\" Value=\"vengi-voxedit\" />\n");
	Log::printf("			<RegistryValue Type=\"string\" Name=\"ApplicationDescription\" Value=\"Vengi Voxel Editor\" />\n");
	Log::printf("		</RegistryKey>\n");
	Log::printf("\n");
	Log::printf("		<!-- Registered file extensions -->\n");
	for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		for (const core::String &e : desc->exts) {
			if (!uniqueExtensions.insert(e)) {
				continue;
			}
			const core::String &m = uniqueMimetype(*desc);
			Log::printf("		<RegistryValue Root=\"HKLM\" Key=\"Software\\vengi-voxedit\\Capabilities\\FileAssociations\" Name=\".%s\" Value=\"vengi-voxedit.%s\" Type=\"string\" />\n", e.c_str(), e.c_str());
			Log::printf("		<RegistryValue Root=\"HKCR\" Key=\"Applications\\vengi_voxedit.exe\\SupportedTypes\" Name=\".%s\" Value=\"\" Type=\"string\" />\n", e.c_str());
			Log::printf("		<RegistryValue Root=\"HKCR\" Key=\".%s\\OpenWithProgids\" Name=\"vengi-voxedit.%s\" Value=\"\" Type=\"string\" />\n", e.c_str(), e.c_str());
			Log::printf("		<ProgId Id=\"vengi-voxedit.%s\" Description=\"%s\" Icon=\"CM_FP_voxedit.voxedit.vengi_voxedit.exe\">\n", e.c_str(), desc->name.c_str());
			Log::printf("			<Extension Id=\"%s\" ContentType=\"%s\">\n", e.c_str(), m.c_str());
			Log::printf("				<Verb Id=\"open\" TargetFile=\"CM_FP_voxedit.voxedit.vengi_voxedit.exe\" Argument=\"&quot;%%1&quot;\" />\n");
			Log::printf("			</Extension>\n");
			Log::printf("		</ProgId>\n");
			Log::printf("\n");
		}
	}
	Log::printf("\n");
	Log::printf("		<!-- Default Programs entry -->\n");
	Log::printf("		<RegistryValue Root=\"HKLM\" Key=\"SOFTWARE\\RegisteredApplications\" Name=\"vengi-voxedit\" Value=\"SOFTWARE\\vengi-voxedit\\Capabilities\" Type=\"string\" />\n");
	Log::printf("	</CPackWiXFragment>\n");
	Log::printf("</CPackWiXPatch>\n");
}

void FormatPrinter::printApplicationPlist() {
	Log::printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	Log::printf("<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" "
		   "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n");
	Log::printf("<plist version=\"1.0\">\n");
	Log::printf("  <dict>\n");
	Log::printf("    <key>CFBundleDevelopmentRegion</key>\n");
	Log::printf("    <string>en-US</string>\n");
	Log::printf("    <key>CFBundleExecutable</key>\n");
	Log::printf("    <string>${MACOSX_BUNDLE_EXECUTABLE_NAME}</string>\n");
	Log::printf("    <key>CFBundleShortVersionString</key>\n");
	Log::printf("    <string>${MACOSX_BUNDLE_SHORT_VERSION_STRING}</string>\n");
	Log::printf("    <key>CFBundleLongVersionString</key>\n");
	Log::printf("    <string>${MACOSX_BUNDLE_LONG_VERSION_STRING}</string>\n");
	Log::printf("    <key>CFBundleIdentifier</key>\n");
	Log::printf("    <string>${MACOSX_BUNDLE_GUI_IDENTIFIER}</string>\n");
	Log::printf("    <key>CFBundleInfoDictionaryVersion</key>\n");
	Log::printf("    <string>6.0</string>\n");
	Log::printf("    <key>CFBundleIconFile</key>\n");
	Log::printf("    <string>${MACOSX_BUNDLE_ICON_FILE}</string>\n");
	Log::printf("    <key>NSHumanReadableCopyright</key>\n");
	Log::printf("    <string>${MACOSX_BUNDLE_COPYRIGHT}</string>\n");
	Log::printf("    <key>CFBundleName</key>\n");
	Log::printf("    <string>${MACOSX_BUNDLE_BUNDLE_NAME}</string>\n");
	Log::printf("    <key>CFBundlePackageType</key>\n");
	Log::printf("    <string>APPL</string>\n");
	Log::printf("    <key>NSHighResolutionCapable</key>\n");
	Log::printf("    <true/>\n");
	Log::printf("    <key>NSRequiresAquaSystemAppearance</key>\n");
	Log::printf("    <false/>\n");
	Log::printf("    <key>CFBundleDocumentTypes</key>\n");
	Log::printf("    <array>\n");
	for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		const core::String &m = uniqueMimetype(*desc);
		Log::printf("      <dict>\n");
		Log::printf("        <key>CFBundleTypeName</key>\n");
		Log::printf("        <string>%s</string>\n", desc->name.c_str());
		Log::printf("        <key>CFBundleTypeIconFile</key>\n");
		Log::printf("        <string>icon.icns</string>\n");
		Log::printf("        <key>CFBundleTypeMIMETypes</key>\n");
		Log::printf("        <array>\n");
		Log::printf("          <string>%s</string>\n", m.c_str());
		Log::printf("        </array>\n");
		Log::printf("        <key>CFBundleTypeExtensions</key>\n");
		Log::printf("        <array>\n");
		for (const core::String &e : desc->exts) {
			Log::printf("          <string>%s</string>\n", e.c_str());
		}
		Log::printf("        </array>\n");
		Log::printf("        <key>CFBundleTypeRole</key>\n");
		Log::printf("        <string>Editor</string>\n");
		Log::printf("        <key>NSDocumentClass</key>\n");
		Log::printf("        <string>AppDocument</string>\n");
		Log::printf("      </dict>\n");
	}
	Log::printf("    </array>\n");
	Log::printf("  </dict>\n");
	Log::printf("</plist>\n");
}

void FormatPrinter::printMimeInfo() {
	_uniqueMimetypes.clear();
	Log::printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	Log::printf("<mime-info xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\">\n");
	for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		const core::String &m = uniqueMimetype(*desc);
		Log::printf("\t<mime-type type=\"%s\">\n", m.c_str());
		Log::printf("\t\t<comment>%s</comment>\n", desc->name.c_str());
		for (const core::String &e : desc->exts) {
			Log::printf("\t\t<glob pattern=\"*.%s\"/>\n", e.c_str());
		}
		for (const io::Magic &e : desc->magics) {
			Log::printf("\t\t<magic priority=\"50\">\n");
			bool useString = true;
			for (int i = 0; i < e.size(); ++i) {
				if (e.data.u8[i] < 32 || e.data.u8[i] > 126) {
					useString = false;
					break;
				}
			}
			if (useString) {
				Log::printf("\t\t\t<match type=\"string\" offset=\"0\" value=\"");
				for (int i = 0; i < e.size(); ++i) {
					Log::printf("%c", (char)e.data.u8[i]);
				}
				Log::printf("\"/>\n");
			} else {
				for (int i = 0; i < e.size(); ++i) {
					Log::printf("\t\t\t<match type=\"byte\" offset=\"%i\" value=\"0x%x\"/>\n", (int)i, (uint8_t)e.data.u8[i]);
				}
			}
			Log::printf("\t\t</magic>\n");
		}
		Log::printf("\t</mime-type>\n");
	}
	Log::printf("</mime-info>\n");
}

void FormatPrinter::printJson(bool palette, bool image, bool voxel) {
	Log::printf("{");
	if (palette) {
		Log::printf("\"palettes\": [");
		int i = 0;
		for (const io::FormatDescription *desc = palette::palettes(); desc->valid(); ++desc) {
			if (i != 0) {
				Log::printf(",");
			}
			Log::printf("{");
			Log::printf("\"name\": \"%s\",", desc->name.c_str());
			Log::printf("\"extensions\": [");
			printJsonStringArray(desc->exts);
			Log::printf("],\"magics\": [");
			printJsonMagicArray(desc->magics);
			Log::printf("],");
			if (desc->flags & FORMAT_FLAG_NO_LOAD) {
				Log::printf("\"load\": false,");
			} else {
				Log::printf("\"load\": true,");
			}
			if (desc->flags & FORMAT_FLAG_SAVE) {
				Log::printf("\"save\": true");
			} else {
				Log::printf("\"save\": false");
			}
			Log::printf("}");
			++i;
		}
		Log::printf("]");
	}
	if (image) {
		if (palette) {
			Log::printf(",");
		}
		Log::printf("\"images\": [");
		int i = 0;
		for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
			if (i != 0) {
				Log::printf(",");
			}
			Log::printf("{");
			Log::printf("\"name\": \"%s\",", desc->name.c_str());
			Log::printf("\"extensions\": [");
			printJsonStringArray(desc->exts);
			Log::printf("],\"magics\": [");
			printJsonMagicArray(desc->magics);
			Log::printf("]");
			Log::printf("}");
			++i;
		}
		Log::printf("]");
	}
	if (voxel) {
		if (palette || image) {
			Log::printf(",");
		}
		Log::printf("\"voxels\": [");
		int i = 0;
		for (const io::FormatDescription *desc = voxelformat::voxelFormats(); desc->valid(); ++desc) {
			if (i != 0) {
				Log::printf(",");
			}
			Log::printf("{");
			Log::printf("\"name\": \"%s\",", desc->name.c_str());
			Log::printf("\"extensions\": [");
			printJsonStringArray(desc->exts);
			Log::printf("],\"magics\": [");
			printJsonMagicArray(desc->magics);
			Log::printf("],");
			const core::String &m = uniqueMimetype(*desc);
			Log::printf("\"mimetype\": \"%s\",", m.c_str());
			if (voxelformat::isMeshFormat(*desc)) {
				Log::printf("\"mesh\": true,");
			}
			if (voxelformat::isAnimationSupported(*desc)) {
				Log::printf("\"animation\": true,");
			} else {
				Log::printf("\"animation\": false,");
			}
			if (desc->flags & FORMAT_FLAG_NO_LOAD) {
				Log::printf("\"load\": false,");
			} else {
				Log::printf("\"load\": true,");
			}
			if (voxelSaveSupported(*desc)) {
				Log::printf("\"save\": true");
			} else {
				Log::printf("\"save\": false");
			}
			Log::printf("}");
			++i;
		}
		Log::printf("]");
	}
	Log::printf("}\n");
}

// Map from Lua global/meta names to documentation page names
static core::String getDocPageName(const core::String &name) {
	if (name == "g_scenegraph") {
		return "scenegraph";
	} else if (name == "g_region") {
		return "region";
	} else if (name == "g_palette") {
		return "palette";
	} else if (name == "g_noise") {
		return "noise";
	} else if (name == "g_shape") {
		return "shape";
	} else if (name == "g_import") {
		return "import";
	} else if (name == "g_algorithm") {
		return "algorithm";
	} else if (name == "g_http") {
		return "http";
	} else if (name == "g_io") {
		return "io";
	} else if (name == "g_cmd") {
		return "cmd";
	} else if (name == "g_var") {
		return "var";
	} else if (name == "g_log") {
		return "log";
	} else if (name == "g_sys") {
		return "sys";
	} else if (name == "g_vec2" || name == "g_vec3" || name == "g_vec4" ||
			   name == "g_ivec2" || name == "g_ivec3" || name == "g_ivec4") {
		return "vector";
	} else if (name == "g_quat") {
		return "quat";
	} else if (name == "scenegraphnode") {
		return "scenegraphnode";
	} else if (name == "keyframe") {
		return "keyframe";
	} else if (name == "volume") {
		return "volume";
	} else if (name == "region" || name == "region_gc") {
		return "region";
	} else if (name == "palette" || name == "palette_gc") {
		return "palette";
	} else if (name == "stream") {
		return "stream";
	} else if (name == "image") {
		return "image";
	}
	return name;
}

static core::String getDocTitle(const core::String &pageName) {
	if (pageName == "scenegraph") {
		return "SceneGraph";
	} else if (pageName == "scenegraphnode") {
		return "SceneGraphNode";
	} else if (pageName == "region") {
		return "Region";
	} else if (pageName == "palette") {
		return "Palette";
	} else if (pageName == "noise") {
		return "Noise";
	} else if (pageName == "shape") {
		return "Shape";
	} else if (pageName == "import") {
		return "Import";
	} else if (pageName == "algorithm") {
		return "Algorithm";
	} else if (pageName == "http") {
		return "HTTP";
	} else if (pageName == "io") {
		return "IO";
	} else if (pageName == "cmd") {
		return "Command";
	} else if (pageName == "var") {
		return "Cvar";
	} else if (pageName == "log") {
		return "Logging";
	} else if (pageName == "sys") {
		return "System";
	} else if (pageName == "vector") {
		return "Vectors";
	} else if (pageName == "quat") {
		return "Quaternion";
	} else if (pageName == "volume") {
		return "Volume";
	} else if (pageName == "keyframe") {
		return "Keyframe";
	} else if (pageName == "stream") {
		return "Stream";
	} else if (pageName == "image") {
		return "Image";
	}
	return pageName;
}

static core::String getGlobalName(const core::String &pageName) {
	if (pageName == "scenegraph") {
		return "g_scenegraph";
	} else if (pageName == "region") {
		return "g_region";
	} else if (pageName == "palette") {
		return "g_palette";
	} else if (pageName == "noise") {
		return "g_noise";
	} else if (pageName == "shape") {
		return "g_shape";
	} else if (pageName == "import") {
		return "g_import";
	} else if (pageName == "algorithm") {
		return "g_algorithm";
	} else if (pageName == "http") {
		return "g_http";
	} else if (pageName == "io") {
		return "g_io";
	} else if (pageName == "cmd") {
		return "g_cmd";
	} else if (pageName == "var") {
		return "g_var";
	} else if (pageName == "log") {
		return "g_log";
	} else if (pageName == "sys") {
		return "g_sys";
	} else if (pageName == "vector") {
		return "g_vec3, g_ivec3, ...";
	} else if (pageName == "quat") {
		return "g_quat";
	}
	return "";
}

void FormatPrinter::printLuaApiMarkdown() {
	voxelgenerator::LUAApi scriptApi(_filesystem);
	if (!scriptApi.init()) {
		Log::error("Failed to initialize LUA API");
		return;
	}

	// Get JSON from LUAApi
	io::BufferedReadWriteStream stream;
	if (!scriptApi.apiJsonToStream(stream)) {
		Log::error("Failed to generate Lua API JSON");
		scriptApi.shutdown();
		return;
	}
	scriptApi.shutdown();

	stream.seek(0);
	core::String jsonStr;
	stream.readString((int)stream.size(), jsonStr);

	nlohmann::json apiJson = nlohmann::json::parse(jsonStr.c_str(), nullptr, false, true);
	if (apiJson.is_discarded()) {
		Log::error("Failed to parse Lua API JSON");
		return;
	}

	// Group by documentation page
	core::DynamicStringMap<nlohmann::json> pageContent;

	for (auto it = apiJson.begin(); it != apiJson.end(); ++it) {
		const core::String name = it.key().c_str();
		const core::String pageName = getDocPageName(name);

		auto found = pageContent.find(pageName);
		if (found == pageContent.end()) {
			pageContent.put(pageName, nlohmann::json::array());
		}

		nlohmann::json entry;
		entry["name"] = name.c_str();
		entry["type"] = it.value().value("type", "");
		entry["methods"] = it.value().value("methods", nlohmann::json::array());
		auto iter = pageContent.find(pageName);
		if (iter != pageContent.end()) {
			iter->value.push_back(entry);
		}
	}

	// Generate markdown for each page
	for (auto it = pageContent.begin(); it != pageContent.end(); ++it) {
		const core::String &pageName = it->key;
		const nlohmann::json &entries = it->value;

		Log::printf("--- BEGIN FILE: lua/%s.md ---\n", pageName.c_str());
		Log::printf("# %s\n\n", getDocTitle(pageName).c_str());

		const core::String globalName = getGlobalName(pageName);
		if (!globalName.empty()) {
			Log::printf("Global: `%s`\n\n", globalName.c_str());
		}

		// Collect all methods across all entries (for vector types there are multiple globals)
		core::DynamicStringMap<nlohmann::json> allMethods;
		core::DynamicArray<core::String> sortedMethodNames;
		bool isMetatable = false;

		for (const auto &entry : entries) {
			const std::string entryType = entry.value("type", "");
			const auto &methods = entry.value("methods", nlohmann::json::array());

			if (entryType == "metatable") {
				isMetatable = true;
			}

			for (const auto &method : methods) {
				const std::string methodName = method.value("name", "");
				if (methodName.empty() || methodName[0] == '_') {
					continue; // Skip metamethods
				}

				// Store for documentation (avoid duplicates for vector types)
				if (allMethods.find(methodName.c_str()) == allMethods.end()) {
					allMethods.put(methodName.c_str(), method);
					sortedMethodNames.push_back(methodName.c_str());
				}
			}
		}

		// Sort method names alphabetically
		sortedMethodNames.sort(core::Greater<core::String>());

		// Print function/method table
		if (isMetatable) {
			Log::printf("## Methods\n\n");
			Log::printf("| Method | Description |\n");
			Log::printf("| ------ | ----------- |\n");
		} else {
			Log::printf("## Functions\n\n");
			Log::printf("| Function | Description |\n");
			Log::printf("| -------- | ----------- |\n");
		}

		for (const core::String &methodName : sortedMethodNames) {
			auto mit = allMethods.find(methodName);
			if (mit == allMethods.end()) {
				continue;
			}
			const nlohmann::json &method = mit->value;
			const std::string summary = method.value("summary", "");

			// Build parameter signature
			core::String params;
			if (method.contains("parameters")) {
				bool first = true;
				for (const auto &param : method["parameters"]) {
					if (!first) {
						params += ", ";
					}
					params += param.value("name", "").c_str();
					first = false;
				}
			}
			Log::printf("| `%s(%s)` | %s |\n", methodName.c_str(), params.c_str(), summary.c_str());
		}

		Log::printf("\n## Detailed Documentation\n\n");

		// Print detailed documentation for each method (sorted)
		for (const core::String &methodName : sortedMethodNames) {
			auto mit = allMethods.find(methodName);
			if (mit == allMethods.end()) {
				continue;
			}
			const nlohmann::json &method = mit->value;

			const std::string summary = method.value("summary", "");

			Log::printf("### %s\n\n", methodName.c_str());

			if (!summary.empty()) {
				Log::printf("%s\n\n", summary.c_str());
			}

			if (method.contains("parameters") && !method["parameters"].empty()) {
				Log::printf("**Parameters:**\n\n");
				Log::printf("| Name | Type | Description |\n");
				Log::printf("| ---- | ---- | ----------- |\n");
				for (const auto &param : method["parameters"]) {
					const std::string pname = param.value("name", "");
					const std::string ptype = param.value("type", "");
					const std::string pdesc = param.value("description", "");
					Log::printf("| `%s` | `%s` | %s |\n", pname.c_str(), ptype.c_str(), pdesc.c_str());
				}
				Log::printf("\n");
			}

			if (method.contains("returns") && !method["returns"].empty()) {
				Log::printf("**Returns:**\n\n");
				Log::printf("| Type | Description |\n");
				Log::printf("| ---- | ----------- |\n");
				for (const auto &ret : method["returns"]) {
					const std::string rtype = ret.value("type", "");
					const std::string rdesc = ret.value("description", "");
					Log::printf("| `%s` | %s |\n", rtype.c_str(), rdesc.c_str());
				}
				Log::printf("\n");
			}
		}

		Log::printf("--- END FILE ---\n\n");
	}
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	FormatPrinter app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
