/**
 * @file
 */

#include "VoxConvert.h"
#include "core/ConfigVar.h"
#include "core/Enum.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Tokenizer.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Set.h"
#include "core/collection/StringSet.h"
#include "core/concurrent/Concurrency.h"
#include "engine-git.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"
#include "palette/Palette.h"
#include "palette/PaletteFormatDescription.h"
#include "scenegraph/JsonExporter.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/FormatThumbnail.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelgenerator/LUAApi.h"
#include "voxelutil/Hollow.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeRescaler.h"
#include "voxelutil/VolumeResizer.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeSplitter.h"
#include "voxelutil/VolumeVisitor.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

VoxConvert::VoxConvert(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider, core::cpus()) {
	init(ORGANISATION, "voxconvert");
	_wantCrashLogs = true;
}

app::AppState VoxConvert::onConstruct() {
	const app::AppState state = Super::onConstruct();
	registerArg("--crop").setDescription("Reduce the models to their real voxel sizes");
	registerArg("--json").setDescription(
		"Print the scene graph of the input file. Give full as argument to also get mesh details");
	registerArg("--image").setDescription("Print the scene graph of the input file as image to the console");
	registerArg("--isometric").setDescription("Create an isometric thumbnail of the input file when --image is used");
	registerArg("--export-models").setDescription("Export all the models of a scene into single files");
	registerArg("--export-palette").setDescription("Export the palette data into the given output file format");
	registerArg("--filter").setDescription("Model filter. For example '1-4,6'");
	registerArg("--filter-property").setDescription("Model filter by property. For example 'name:foo'");
	registerArg("--force").setShort("-f").setDescription("Overwrite existing files");
	registerArg("--input").setShort("-i").setDescription("Allow to specify input files").addFlag(ARGUMENT_FLAG_FILE);
	registerArg("--wildcard")
		.setShort("-w")
		.setDescription("Allow to specify input file filter if --input is a directory");
	registerArg("--merge").setShort("-m").setDescription("Merge models into one volume");
	registerArg("--mirror").setDescription("Mirror by the given axis (x, y or z)");
	registerArg("--output")
		.setShort("-o")
		.setDescription("Allow to specify the output file")
		.addFlag(ARGUMENT_FLAG_FILE);
	registerArg("--rotate")
		.setDescription(
			"Rotate by 90 degree at the given axis (x, y or z), specify e.g. x:180 to rotate around x by 180 degree.");
	registerArg("--resize").setDescription("Resize the volume by the given x (right), y (up) and z (back) values");
	registerArg("--scale").setShort("-s").setDescription("Scale model to 50% of its original size");
	registerArg("--script").setDescription("Apply the given lua script to the output volume");
	registerArg("--scriptcolor")
		.setDefaultValue("1")
		.setDescription("Set the palette index that is given to the color script parameters of the main function");
	registerArg("--split").setDescription("Slices the models into pieces of the given size <x:y:z>");
	registerArg("--surface-only")
		.setDescription("Remove any non surface voxel. If you are meshing with this, you get also faces on the inner "
						"side of your mesh.");
	registerArg("--translate").setShort("-t").setDescription("Translate the models by x (right), y (up), z (back)");
	registerArg("--print-formats").setDescription("Print supported formats as json for easier parsing in other tools");
	registerArg("--print-scripts").setDescription("Print found lua scripts as json for easier parsing in other tools");

	voxelformat::FormatConfig::init();

	_mergeQuads = core::Var::getSafe(cfg::VoxformatMergequads);
	_reuseVertices = core::Var::getSafe(cfg::VoxformatReusevertices);
	_ambientOcclusion = core::Var::getSafe(cfg::VoxformatAmbientocclusion);
	_scale = core::Var::getSafe(cfg::VoxformatScale);
	_scaleX = core::Var::getSafe(cfg::VoxformatScaleX);
	_scaleY = core::Var::getSafe(cfg::VoxformatScaleY);
	_scaleZ = core::Var::getSafe(cfg::VoxformatScaleZ);
	_quads = core::Var::getSafe(cfg::VoxformatQuads);
	_withColor = core::Var::getSafe(cfg::VoxformatWithColor);
	_withTexCoords = core::Var::getSafe(cfg::VoxformatWithtexcoords);
	core::Var::get(cfg::VoxConvertDepthFactor2D, 0.0f);

	if (!filesystem()->registerPath("scripts/")) {
		Log::warn("Failed to register lua generator script path");
	}

	return state;
}

void VoxConvert::printUsageHeader() const {
	Super::printUsageHeader();
	Log::info("Git commit " GIT_COMMIT " - " GIT_COMMIT_DATE);
}

void VoxConvert::usage() const {
	Super::usage();
	Log::info("Load support:");
	for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		for (const core::String &ext : desc->exts) {
			Log::info(" * %s (*.%s)", desc->name.c_str(), ext.c_str());
		}
	}
	Log::info("Save support:");
	for (const io::FormatDescription *desc = voxelformat::voxelSave(); desc->valid(); ++desc) {
		for (const core::String &ext : desc->exts) {
			Log::info(" * %s (*.%s)", desc->name.c_str(), ext.c_str());
		}
	}
	Log::info("Supported image formats:");
	for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
		for (const core::String &ext : desc->exts) {
			Log::info(" * %s (*.%s)", desc->name.c_str(), ext.c_str());
		}
	}
	Log::info("Supported palette formats:");
	for (const io::FormatDescription *desc = palette::palettes(); desc->valid(); ++desc) {
		for (const core::String &ext : desc->exts) {
			Log::info(" * %s (*.%s)", desc->name.c_str(), ext.c_str());
		}
	}
	Log::info("Built-in palettes:");
	for (int i = 0; i < lengthof(palette::Palette::builtIn); ++i) {
		Log::info(" * %s", palette::Palette::builtIn[i]);
	}
	Log::info("Scripts:");
	voxelgenerator::LUAApi scriptApi(filesystem());
	core::DynamicArray<voxelgenerator::LUAScript> scripts = scriptApi.listScripts();
	for (voxelgenerator::LUAScript &script : scripts) {
		scriptApi.reloadScriptParameters(script);
	}
	for (const voxelgenerator::LUAScript &script : scripts) {
		Log::info(" * %s%s %s", script.filename.c_str(), script.valid ? "" : " (invalid)", script.desc.c_str());
		if (!script.valid) {
			continue;
		}
		for (const voxelgenerator::LUAParameterDescription &param : script.parameterDescription) {
			Log::info("   * %s: %s (default: '%s')", param.name.c_str(), param.description.c_str(),
					  param.defaultValue.c_str());
		}
	}

	usageFooter();
}

static void printProgress(const char *name, int cur, int max) {
	Log::trace("%s: %i/%i", name, cur, max);
}

app::AppState VoxConvert::onInit() {
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

	if (hasArg("--print-formats")) {
		Log::printf("{\"voxels\":[");
		io::format::printJson(voxelformat::voxelLoad(), {{"thumbnail_embedded", VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED},
														 {"palette_embedded", VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
														 {"mesh", VOX_FORMAT_FLAG_MESH},
														 {"animation", VOX_FORMAT_FLAG_ANIMATION},
														 {"save", FORMAT_FLAG_SAVE}});
		Log::printf("],\"images\":[");
		io::format::printJson(io::format::images(), {{"save", FORMAT_FLAG_SAVE}});
		Log::printf("],\"palettes\":[");
		io::format::printJson(palette::palettes(), {{"save", FORMAT_FLAG_SAVE}});
		Log::printf("]}\n");
		return state;
	}

	if (hasArg("--print-scripts")) {
		Log::printf("{\"scripts\":[");
		voxelgenerator::LUAApi scriptApi(filesystem());
		const core::DynamicArray<voxelgenerator::LUAScript> &scripts = scriptApi.listScripts();
		for (size_t i = 0; i < scripts.size(); ++i) {
			if (i > 0) {
				Log::printf(",");
			}
			Log::printf("{\"name\":\"%s\",\"valid\":%s}", scripts[i].filename.c_str(),
						(scripts[i].valid ? "true" : "false"));
		}
		Log::printf("]}\n");
		return state;
	}

	const bool hasScript = hasArg("--script");

	core::String infilesstr;
	core::DynamicArray<core::String> infiles;
	bool inputIsMesh = false;
	if (hasArg("--input")) {
		int argn = 0;
		for (;;) {
			core::String val = getArgVal("--input", "", &argn);
			if (val.empty()) {
				break;
			}
			io::normalizePath(val);
			infiles.push_back(val);
			if (voxelformat::isMeshFormat(val, false)) {
				inputIsMesh = true;
			}
			if (!infilesstr.empty()) {
				infilesstr += ", ";
			}
			infilesstr += val;
		}
	} else if (!hasScript) {
		Log::error("No input file was specified");
		return app::AppState::InitFailure;
	}

	core::String outfilesstr;
	core::DynamicArray<core::String> outfiles;
	bool outputIsMesh = false;
	if (hasArg("--output")) {
		int argn = 0;
		for (;;) {
			core::String val = getArgVal("--output", "", &argn);
			if (val.empty()) {
				break;
			}
			io::normalizePath(val);
			outfiles.push_back(val);
			if (voxelformat::isMeshFormat(val, false)) {
				outputIsMesh = true;
			}
			if (!outfilesstr.empty()) {
				outfilesstr += ", ";
			}
			outfilesstr += val;
		}
	}

	_mergeModels = hasArg("--merge");
	_scaleModels = hasArg("--scale");
	_mirrorModels = hasArg("--mirror");
	_rotateModels = hasArg("--rotate");
	_translateModels = hasArg("--translate");
	_exportPalette = hasArg("--export-palette");
	_exportModels = hasArg("--export-models");
	_cropModels = hasArg("--crop");
	_surfaceOnly = hasArg("--surface-only");
	_splitModels = hasArg("--split");
	_printSceneGraph = hasArg("--json");
	_printSceneToConsole = hasArg("--image");
	_resizeModels = hasArg("--resize");

	Log::info("Options");
	if (inputIsMesh || outputIsMesh) {
		Log::info("* mergeQuads:        - %s", _mergeQuads->strVal().c_str());
		Log::info("* reuseVertices:     - %s", _reuseVertices->strVal().c_str());
		Log::info("* ambientOcclusion:  - %s", _ambientOcclusion->strVal().c_str());
		Log::info("* scale:             - %s", _scale->strVal().c_str());
		Log::info("* scaleX:            - %s", _scaleX->strVal().c_str());
		Log::info("* scaleY:            - %s", _scaleY->strVal().c_str());
		Log::info("* scaleZ:            - %s", _scaleZ->strVal().c_str());
		Log::info("* quads:             - %s", _quads->strVal().c_str());
		Log::info("* withColor:         - %s", _withColor->strVal().c_str());
		Log::info("* withTexCoords:     - %s", _withTexCoords->strVal().c_str());
	}
	const core::VarPtr &paletteVar = core::Var::getSafe(cfg::VoxelPalette);
	if (!paletteVar->strVal().empty()) {
		Log::info("* palette:           - %s", paletteVar->strVal().c_str());
	}
	Log::info("* input files:       - %s", infilesstr.c_str());
	if (!outfilesstr.empty()) {
		Log::info("* output files:      - %s", outfilesstr.c_str());
	}
	if (hasScript) {
		int argn = 0;
		for (;;) {
			core::String val = getArgVal("--script", "", &argn);
			if (val.empty()) {
				break;
			}
			if (val.empty()) {
				Log::error("* script:            - Missing script parameters");
			} else {
				Log::info("* script:            - %s", val.c_str());
			}
		}
	}

	Log::info("* show scene graph:  - %s", (_printSceneGraph ? "true" : "false"));
	Log::info("* scene graph image: - %s", (_printSceneToConsole ? "true" : "false"));
	Log::info("* merge models:      - %s", (_mergeModels ? "true" : "false"));
	Log::info("* scale models:      - %s", (_scaleModels ? "true" : "false"));
	Log::info("* crop models:       - %s", (_cropModels ? "true" : "false"));
	Log::info("* surface only:      - %s", (_surfaceOnly ? "true" : "false"));
	Log::info("* split models:      - %s", (_splitModels ? "true" : "false"));
	Log::info("* mirror models:     - %s", (_mirrorModels ? "true" : "false"));
	Log::info("* translate models:  - %s", (_translateModels ? "true" : "false"));
	Log::info("* rotate models:     - %s", (_rotateModels ? "true" : "false"));
	Log::info("* export palette:    - %s", (_exportPalette ? "true" : "false"));
	Log::info("* export models:     - %s", (_exportModels ? "true" : "false"));
	Log::info("* resize models:     - %s", (_resizeModels ? "true" : "false"));

	if (core::Var::getSafe(cfg::MetricFlavor)->strVal().empty()) {
		Log::info(
			"Please enable anonymous usage statistics. You can do this by setting the metric_flavor cvar to 'json'");
		Log::info("Example: '%s -set metric_flavor json --input xxx --output yyy'", fullAppname().c_str());
	}

	if (!outfiles.empty()) {
		if (!hasArg("--force")) {
			for (const core::String &outfile : outfiles) {
				const bool outfileExists = filesystem()->open(outfile)->exists();
				if (outfileExists) {
					Log::error("Given output file '%s' already exists", outfile.c_str());
					return app::AppState::InitFailure;
				}
			}
		}
	} else if (!_exportModels && !_printSceneGraph && !_printSceneToConsole) {
		Log::error("No output specified");
		return app::AppState::InitFailure;
	}

	static image::ImagePtr thumbnail;
	if (infiles.size() == 1) {
		voxelformat::LoadContext loadCtx;
		loadCtx.monitor = printProgress;
		thumbnail = voxelformat::loadScreenshot(infiles[0], io::openFilesystemArchive(filesystem()), loadCtx);
	}
	const io::ArchivePtr &fsArchive = io::openFilesystemArchive(filesystem());
	scenegraph::SceneGraph sceneGraph;
	for (const core::String &infile : infiles) {
		if (shouldQuit()) {
			break;
		}
		if (io::Filesystem::sysIsReadableDir(infile)) {
			core::DynamicArray<io::FilesystemEntry> entities;
			filesystem()->list(infile, entities, getArgVal("--wildcard", ""));
			Log::info("Found %i entries in dir %s", (int)entities.size(), infile.c_str());
			int success = 0;
			for (const io::FilesystemEntry &entry : entities) {
				if (shouldQuit()) {
					break;
				}
				if (entry.type != io::FilesystemEntry::Type::file) {
					continue;
				}
				const core::String fullpath = core::string::path(infile, entry.name);
				if (handleInputFile(fullpath, fsArchive, sceneGraph, entities.size() > 1)) {
					++success;
				}
			}
			if (success == 0) {
				Log::error("Could not find a valid input file in directory %s", infile.c_str());
				return app::AppState::InitFailure;
			}
		} else if (!io::isA(infile, voxelformat::voxelLoad()) && io::isZipArchive(infile)) {
			io::FileStream archiveStream(filesystem()->open(infile, io::FileMode::SysRead));
			io::ArchivePtr archive = io::openZipArchive(&archiveStream);
			if (!archive) {
				Log::error("Failed to open archive %s", infile.c_str());
				return app::AppState::InitFailure;
			}

			const core::String filter = getArgVal("--wildcard", "");
			for (const auto &entry : archive->files()) {
				if (shouldQuit()) {
					break;
				}
				if (!entry.isFile()) {
					continue;
				}
				if (!io::isA(entry.name, voxelformat::voxelLoad())) {
					continue;
				}
				if (!filter.empty()) {
					if (!core::string::fileMatchesMultiple(entry.name.c_str(), filter.c_str())) {
						Log::debug("Entity %s doesn't match filter %s", entry.name.c_str(), filter.c_str());
						continue;
					}
				}
				const core::String &fullPath = filesystem()->homeWritePath(entry.fullPath);
				if (!handleInputFile(fullPath, archive, sceneGraph, archive->files().size() > 1)) {
					Log::error("Failed to handle input file %s", fullPath.c_str());
				}
			}
		} else {
			if (!handleInputFile(infile, fsArchive, sceneGraph, infiles.size() > 1)) {
				return app::AppState::InitFailure;
			}
		}
	}
	if (hasArg("--script") && sceneGraph.empty()) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		const voxel::Region region(0, 63);
		node.setVolume(new voxel::RawVolume(region), true);
		node.setName("Script generated");
		sceneGraph.emplace(core::move(node));
	}

	if (sceneGraph.empty()) {
		if (_exportPalette) {
			return state;
		}
		Log::error("No valid input found in the scenegraph to operate on.");
		return app::AppState::InitFailure;
	}

	// STEP 1: apply the filter
	applyFilters(sceneGraph, infiles, outfiles);

	if (_printSceneGraph) {
		scenegraph::sceneGraphJson(sceneGraph, getArgVal("--json", "") == "full");
	}

	if (_printSceneToConsole) {
		scenegraph::SceneGraph::MergeResult merged = sceneGraph.merge();
		if (!merged.hasVolume()) {
			Log::error("No valid volume in the scenegraph to print");
			return app::AppState::InitFailure;
		}
		core::ScopedPtr<voxel::RawVolume> v(merged.volume());
		int width = terminalWidth();
		int height = -1;
		const core::RGBA bgColor(0, 0, 0, 255);
		const core::String &faceStr = getArgVal("--image", voxel::faceNameString(voxel::FaceNames::Front));
		Log::debug("Print image with width %i and height %i for face %s", width, height, faceStr.c_str());
		const voxel::FaceNames frontFace = voxel::toFaceNames(faceStr, voxel::FaceNames::Front);
		const float depthFactor2D = core::Var::getSafe(cfg::VoxConvertDepthFactor2D)->floatVal();
		const image::ImagePtr &image =
			hasArg("--isometric")
				? voxelutil::renderIsometricImage(v, merged.palette, frontFace, bgColor, width, height)
				: voxelutil::renderToImage(v, merged.palette, frontFace, bgColor, width, height, false, depthFactor2D);
		const core::String prt(image::print(image, false));
		Log::printf("%s", prt.c_str());
	}

	if (_exportModels) {
		if (infiles.size() > 1) {
			Log::warn("The format and path of the first input file is used for exporting all models");
		}
		for (const core::String &outfile : outfiles) {
			io::FilePtr outputFile = filesystem()->open(outfile, io::FileMode::SysWrite);
			if (!outputFile->validHandle()) {
				Log::error("Could not open target file: %s", outfile.c_str());
				return app::AppState::InitFailure;
			}
			exportModelsIntoSingleObjects(sceneGraph, infiles[0], outputFile ? outputFile->extension() : "");
		}
		return state;
	}

	// STEP 2: merge all models
	if (_mergeModels) {
		Log::info("Merge models");
		const scenegraph::SceneGraph::MergeResult &merged = sceneGraph.merge();
		if (!merged.hasVolume()) {
			Log::error("Failed to merge models");
			return app::AppState::InitFailure;
		}
		sceneGraph.clear();
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(merged.volume(), true);
		node.setPalette(merged.palette);
		node.setNormalPalette(merged.normalPalette);
		node.setName(infilesstr);
		sceneGraph.emplace(core::move(node));
	}

	// STEP 3: lod 50% downsampling
	if (_scaleModels) {
		scale(sceneGraph);
	}

	// STEP 4: resize to the given size
	if (_resizeModels) {
		resize(getArgIvec3("--resize"), sceneGraph);
	}

	// STEP 5: apply mirror
	if (_mirrorModels) {
		mirror(getArgVal("--mirror"), sceneGraph);
	}

	// STEP 6: apply rotation
	if (_rotateModels) {
		rotate(getArgVal("--rotate"), sceneGraph);
	}

	// STEP 7: apply translation
	if (_translateModels) {
		translate(getArgIvec3("--translate"), sceneGraph);
	}

	// STEP 8: apply scripts
	applyScripts(sceneGraph);

	// STEP 9: crop the models
	if (_cropModels) {
		crop(sceneGraph);
	}

	// STEP 10: remove non surface voxels
	if (_surfaceOnly) {
		removeNonSurfaceVoxels(sceneGraph);
	}

	// STEP 11: split the models
	if (_splitModels) {
		split(getArgIvec3("--split"), sceneGraph);
	}

	for (const core::String &outfile : outfiles) {
		if (_exportPalette || (!io::isA(outfile, voxelformat::voxelSave()) && io::isA(outfile, palette::palettes()))) {
			// if the given format is a palette only format (some voxel formats might have the same
			// extension - so we check that here)
			const palette::Palette &palette = sceneGraph.mergePalettes(false);
			if (!palette.save(outfile.c_str())) {
				Log::error("Failed to save palette to %s", outfile.c_str());
				return app::AppState::InitFailure;
			}
			Log::info("Saved palette with %i colors to %s", palette.colorCount(), outfile.c_str());
		} else {
			Log::debug("Save %i models", (int)sceneGraph.size());
			voxelformat::SaveContext saveCtx;
			if (thumbnail && thumbnail->isLoaded()) {
				auto fn = [](const scenegraph::SceneGraph &, const voxelformat::ThumbnailContext &ctx) {
					thumbnail->resize(ctx.outputSize.x, ctx.outputSize.y);
					return thumbnail;
				};
				saveCtx.thumbnailCreator = fn;
				Log::info("Using thumbnail with size %i x %i", thumbnail->width(), thumbnail->height());
			}
			if (!voxelformat::saveFormat(sceneGraph, outfile, nullptr, fsArchive, saveCtx)) {
				Log::error("Failed to write to output file '%s'", outfile.c_str());
				return app::AppState::InitFailure;
			}
			Log::info("Wrote output file %s", outfile.c_str());
		}
	}
	return state;
}

void VoxConvert::applyFilters(scenegraph::SceneGraph &sceneGraph, const core::DynamicArray<core::String> &infiles,
							  const core::DynamicArray<core::String> &outfiles) {
	const bool applyFilter = hasArg("--filter");
	if (applyFilter) {
		if (infiles.size() == 1u) {
			filterModels(sceneGraph);
		} else {
			Log::warn("Don't apply model filters for multiple input files");
		}
	}

	const bool applyFilterProperty = hasArg("--filter-property");
	if (applyFilterProperty) {
		if (infiles.size() == 1u) {
			const core::String &property = getArgVal("--filter-property");
			core::String key = property;
			core::String value;
			const size_t colonPos = property.find(":");
			if (colonPos != core::String::npos) {
				key = property.substr(0, colonPos);
				value = property.substr(colonPos + 1);
			}
			filterModelsByProperty(sceneGraph, key, value);
		} else {
			Log::warn("Don't apply model property filters for multiple input files");
		}
	}
}

void VoxConvert::applyScripts(scenegraph::SceneGraph &sceneGraph) {
	int argn = 0;
	for (;;) {
		core::String val = getArgVal("--script", "", &argn);
		if (val.empty()) {
			break;
		}
		if (!val.empty()) {
			const core::String &color = getArgVal("--scriptcolor");
			script(val, sceneGraph, color.toInt());
		}
	}
}

core::String VoxConvert::getFilenameForModelName(const core::String &inputfile, const core::String &modelName,
												 const core::String &outExt, int id, bool uniqueNames) {
	const core::String &ext = outExt.empty() ? core::string::extractExtension(inputfile) : outExt;
	core::String name;
	if (modelName.empty()) {
		name = core::String::format("model-%i.%s", id, ext.c_str());
	} else if (uniqueNames) {
		name = core::String::format("%s.%s", modelName.c_str(), ext.c_str());
	} else {
		name = core::String::format("%s-%i.%s", modelName.c_str(), id, ext.c_str());
	}
	return core::string::path(core::string::extractDir(inputfile), core::string::sanitizeFilename(name));
}

bool VoxConvert::handleInputFile(const core::String &infile, const io::ArchivePtr &archive,
								 scenegraph::SceneGraph &sceneGraph, bool multipleInputs) {
	Log::info("-- current input file: %s", infile.c_str());
	if (!archive->exists(infile)) {
		Log::error("Given input file '%s' does not exist", infile.c_str());
		_exitCode = 127;
		return false;
	}
	scenegraph::SceneGraph newSceneGraph;
	voxelformat::LoadContext loadCtx;
	loadCtx.monitor = printProgress;
	io::FileDescription fileDesc;
	fileDesc.set(infile);
	if (!voxelformat::loadFormat(fileDesc, archive, newSceneGraph, loadCtx)) {
		return false;
	}

	int parent = sceneGraph.root().id();
	if (multipleInputs) {
		scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
		groupNode.setName(core::string::extractFilename(infile));
		parent = sceneGraph.emplace(core::move(groupNode), parent);
	}
	scenegraph::addSceneGraphNodes(sceneGraph, newSceneGraph, parent);

	return true;
}

static bool hasUniqueModelNames(const scenegraph::SceneGraph &sceneGraph) {
	core::StringSet names;
	for (const auto &entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->value;
		if (!node.isModelNode()) {
			continue;
		}
		if (!names.insert(node.name())) {
			return false;
		}
	}
	return true;
}

void VoxConvert::exportModelsIntoSingleObjects(scenegraph::SceneGraph &sceneGraph, const core::String &inputfile,
											   const core::String &ext) {
	Log::info("Export models into single objects");
	int id = 0;
	voxelformat::SaveContext saveCtx;
	const auto &nodes = sceneGraph.nodes();
	const bool uniqueNames = hasUniqueModelNames(sceneGraph);
	for (auto entry : nodes) {
		const scenegraph::SceneGraphNode &node = entry->value;
		if (!node.isModelNode()) {
			continue;
		}
		scenegraph::SceneGraph newSceneGraph;
		scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
		scenegraph::copyNode(node, newNode, false);
		newSceneGraph.emplace(core::move(newNode));
		const core::String &filename = getFilenameForModelName(inputfile, node.name(), ext, id, uniqueNames);
		const io::ArchivePtr &archive = io::openFilesystemArchive(filesystem());
		if (voxelformat::saveFormat(newSceneGraph, filename, nullptr, archive, saveCtx)) {
			Log::info(" .. %s", filename.c_str());
		} else {
			Log::error(" .. %s", filename.c_str());
		}
		++id;
	}
}

glm::ivec3 VoxConvert::getArgIvec3(const core::String &name) {
	const core::String &arguments = getArgVal(name);
	glm::ivec3 t(0);
	SDL_sscanf(arguments.c_str(), "%i:%i:%i", &t.x, &t.y, &t.z);
	return t;
}

void VoxConvert::split(const glm::ivec3 &size, scenegraph::SceneGraph &sceneGraph) {
	Log::info("split volumes at %i:%i:%i", size.x, size.y, size.z);
	const scenegraph::SceneGraph::MergeResult &merged = sceneGraph.merge();
	sceneGraph.clear();
	core::ScopedPtr<voxel::RawVolume> volume(merged.volume());
	core::Buffer<voxel::RawVolume *> rawVolumes = voxelutil::splitVolume(volume, size);
	for (voxel::RawVolume *v : rawVolumes) {
		if (v == nullptr) {
			continue;
		}
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(v, true);
		node.setPalette(merged.palette);
		node.setNormalPalette(merged.normalPalette);

		sceneGraph.emplace(core::move(node));
	}
}

void VoxConvert::crop(scenegraph::SceneGraph &sceneGraph) {
	Log::info("Crop volumes");
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		if (voxel::RawVolume *v = voxelutil::cropVolume(node.volume())) {
			node.setVolume(v, true);
		}
	}
}

void VoxConvert::removeNonSurfaceVoxels(scenegraph::SceneGraph &sceneGraph) {
	Log::info("Remove non-surface voxels");
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		voxelutil::hollow(*node.volume());
	}
}

void VoxConvert::script(const core::String &scriptParameters, scenegraph::SceneGraph &sceneGraph, uint8_t color) {
	voxelgenerator::LUAApi script(_filesystem);
	if (!script.init()) {
		Log::warn("Failed to initialize the script bindings");
	} else {
		core::TokenizerConfig cfg;
		core::Tokenizer tokenizer(cfg, scriptParameters);
		const core::Tokens &tokens = tokenizer.tokens();
		for (const core::String &token : tokens) {
			Log::debug("Script token: %s", token.c_str());
		}
		const core::String &luaScript = script.load(tokens[0]);
		if (luaScript.empty()) {
			Log::error("Failed to load %s", tokens[0].c_str());
		} else {
			const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, color);
			core::DynamicArray<core::String> args(tokens.size() - 1);
			for (size_t i = 1; i < tokens.size(); ++i) {
				args[i - 1] = tokens[i];
			}
			Log::info("Execute script %s", tokens[0].c_str());
			core::Buffer<int> nodes;
			for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
				nodes.push_back((*iter).id());
			}
			for (int nodeId : nodes) {
				const scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
				Log::debug("execute for node: %i", nodeId);
				if (!script.exec(luaScript, sceneGraph, node.id(), node.region(), voxel, args)) {
					break;
				}
				while (script.scriptStillRunning()) {
					script.update(0.0);
				}
			}
		}
	}

	script.shutdown();
}

void VoxConvert::scale(scenegraph::SceneGraph &sceneGraph) {
	Log::info("Scale models");
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		const voxel::Region srcRegion = node.region();
		const glm::ivec3 &targetDimensionsHalf = (srcRegion.getDimensionsInVoxels() / 2) - 1;
		const voxel::Region destRegion(srcRegion.getLowerCorner(), srcRegion.getLowerCorner() + targetDimensionsHalf);
		if (destRegion.isValid()) {
			voxel::RawVolume *destVolume = new voxel::RawVolume(destRegion);
			voxelutil::scaleDown(*node.volume(), node.palette(), *destVolume);
			node.setVolume(destVolume, true);
		}
	}
}

void VoxConvert::resize(const glm::ivec3 &size, scenegraph::SceneGraph &sceneGraph) {
	Log::info("Resize models");
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		voxel::RawVolume *v = voxelutil::resize(node.volume(), size);
		if (v == nullptr) {
			Log::warn("Failed to resize volume");
			continue;
		}
		node.setVolume(v, true);
	}
}

void VoxConvert::filterModels(scenegraph::SceneGraph &sceneGraph) {
	const core::String &filter = getArgVal("--filter");
	if (filter.empty()) {
		Log::warn("No filter specified");
		return;
	}

	core::Set<int> models;
	core::DynamicArray<core::String> tokens;
	core::string::splitString(filter, tokens, ",");
	for (const core::String &token : tokens) {
		if (token.contains("-")) {
			const int start = token.toInt();
			const size_t index = token.find("-");
			const core::String &endString = token.substr(index + 1);
			const int end = endString.toInt();
			for (int modelIndex = start; modelIndex <= end; ++modelIndex) {
				models.insert(modelIndex);
			}
		} else {
			const int modelIndex = token.toInt();
			models.insert(modelIndex);
		}
	}
	auto iter = sceneGraph.beginModel();
	core::Set<int> removeNodes;
	for (int i = 0; iter != sceneGraph.end(); ++i, ++iter) {
		if (!models.has(i)) {
			removeNodes.insert((*iter).id());
			Log::debug("Remove model %i - not part of the filter expression", i);
		}
	}
	for (const auto &entry : removeNodes) {
		sceneGraph.removeNode(entry->key, false);
	}
	Log::info("Filtered models: %i", (int)models.size());
}

void VoxConvert::filterModelsByProperty(scenegraph::SceneGraph &sceneGraph, const core::String &property,
										const core::String &value) {
	if (property.empty()) {
		Log::warn("No property specified to filter");
		return;
	}

	core::Set<int> removeNodes;
	for (auto iter : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = iter->value;
		if (!node.isModelNode()) {
			continue;
		}
		const core::String &pkey = node.property(property);
		if (value.empty() && !pkey.empty()) {
			continue;
		}
		if (pkey == value) {
			continue;
		}
		Log::debug("Remove model %i - not part of the filter expression", node.id());
		removeNodes.insert(node.id());
	}
	for (const auto &entry : removeNodes) {
		sceneGraph.removeNode(entry->key, false);
	}
	Log::info("Filtered models: %i", (int)sceneGraph.size(scenegraph::SceneGraphNodeType::Model));
}

void VoxConvert::mirror(const core::String &axisStr, scenegraph::SceneGraph &sceneGraph) {
	const math::Axis axis = math::toAxis(axisStr);
	if (axis == math::Axis::None) {
		return;
	}
	Log::info("Mirror on axis %c", axisStr[0]);
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		node.setVolume(voxelutil::mirrorAxis(node.volume(), axis), true);
	}
}

void VoxConvert::rotate(const core::String &axisStr, scenegraph::SceneGraph &sceneGraph) {
	const math::Axis axis = math::toAxis(axisStr);
	if (axis == math::Axis::None) {
		return;
	}
	float degree = 90.0f;
	if (axisStr.contains(":")) {
		degree = glm::mod(axisStr.substr(2).toFloat(), 360.0f);
	}
	if (degree <= 1.0f) {
		Log::warn("Don't rotate on axis %c by %f degree", axisStr[0], degree);
		return;
	}
	Log::info("Rotate on axis %c by %f degree", axisStr[0], degree);
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		glm::vec3 rotVec{0.0f};
		rotVec[math::getIndexForAxis(axis)] = degree;
		node.setVolume(voxelutil::rotateVolume(node.volume(), rotVec, glm::vec3(0.5f)), true);
	}
}

void VoxConvert::translate(const glm::ivec3 &pos, scenegraph::SceneGraph &sceneGraph) {
	Log::info("Translate by %i:%i:%i", pos.x, pos.y, pos.z);
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		if (voxel::RawVolume *v = node.volume()) {
			v->translate(pos);
		}
	}
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	VoxConvert app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
