/**
 * @file
 */

#include "VoxConvert.h"
#include "core/Color.h"
#include "core/Enum.h"
#include "core/GameConfig.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "core/Var.h"
#include "command/Command.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Set.h"
#include "image/Image.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "metric/Metric.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/Format.h"
#include "voxelformat/SceneGraph.h"
#include "voxelgenerator/LUAGenerator.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeRescaler.h"
#include "voxelutil/VolumeResizer.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeSplitter.h"

#define MaxHeightmapWidth 1024
#define MaxHeightmapHeight 1024

VoxConvert::VoxConvert(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "voxconvert");
}

app::AppState VoxConvert::onConstruct() {
	const app::AppState state = Super::onConstruct();
	registerArg("--crop").setDescription("Reduce the volumes to their real voxel sizes");
	registerArg("--dump").setDescription("Dump the scene graph of the input file");
	registerArg("--export-layers").setDescription("Export all the layers of a scene into single files");
	registerArg("--export-palette").setDescription("Export the used palette data into an image. Use in combination with --src-palette");
	registerArg("--filter").setDescription("Layer filter. For example '1-4,6'");
	registerArg("--force").setShort("-f").setDescription("Overwrite existing files");
	registerArg("--input").setShort("-i").setDescription("Allow to specify input files");
	registerArg("--merge").setShort("-m").setDescription("Merge layers into one volume");
	registerArg("--mirror").setDescription("Mirror by the given axis (x, y or z)");
	registerArg("--output").setShort("-o").setDescription("Allow to specify the output file");
	registerArg("--pivot").setDescription("Change the pivots of the volume layers");
	registerArg("--rotate").setDescription("Rotate by 90 degree at the given axis (x, y or z)");
	registerArg("--resize").setDescription("Resize the volume by the given x (right), y (up) and z (back) values");
	registerArg("--scale").setShort("-s").setDescription("Scale layer to 50% of its original size");
	registerArg("--script").setDefaultValue("script.lua").setDescription("Apply the given lua script to the output volume");
	registerArg("--split").setDescription("Slices the volumes into pieces of the given size <x:y:z>");
	registerArg("--src-palette").setShort("-p").setDescription("Keep the source palette and don't perform quantization");
	registerArg("--translate").setShort("-t").setDescription("Translate the volumes by x (right), y (up), z (back)");

	_mergeQuads = core::Var::get(cfg::VoxformatMergequads, "true", core::CV_NOPERSIST);
	_mergeQuads->setHelp("Merge similar quads to optimize the mesh");
	_reuseVertices = core::Var::get(cfg::VoxformatReusevertices, "true", core::CV_NOPERSIST);
	_reuseVertices->setHelp("Reuse vertices or always create new ones");
	_ambientOcclusion = core::Var::get(cfg::VoxformatAmbientocclusion, "false", core::CV_NOPERSIST);
	_ambientOcclusion->setHelp("Extra vertices for ambient occlusion");
	_scale = core::Var::get(cfg::VoxformatScale, "1.0", core::CV_NOPERSIST);
	_scale->setHelp("Scale the vertices by the given factor");
	_quads = core::Var::get(cfg::VoxformatQuads, "true", core::CV_NOPERSIST);
	_quads->setHelp("Export as quads. If this false, triangles will be used.");
	_withColor = core::Var::get(cfg::VoxformatWithcolor, "true", core::CV_NOPERSIST);
	_withColor->setHelp("Export with vertex colors");
	_withTexCoords = core::Var::get(cfg::VoxformatWithtexcoords, "true", core::CV_NOPERSIST);
	_withTexCoords->setHelp("Export with uv coordinates of the palette image");
	_palette = core::Var::get("palette", voxel::getDefaultPaletteName());
	_palette->setHelp("This is the NAME part of palette-<NAME>.png or absolute png file to use (1x256)");

	if (!filesystem()->registerPath("scripts/")) {
		Log::warn("Failed to register lua generator script path");
	}

	return state;
}

void VoxConvert::usage() const {
	Super::usage();
	Log::info("Load support:");
	for (const io::FormatDescription *desc = voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD; desc->ext != nullptr; ++desc) {
		Log::info(" * %s (*.%s)", desc->name, desc->ext);
	}
	Log::info("Save support:");
	for (const io::FormatDescription *desc = voxelformat::SUPPORTED_VOXEL_FORMATS_SAVE; desc->ext != nullptr; ++desc) {
		Log::info(" * %s (*.%s)", desc->name, desc->ext);
	}
}

app::AppState VoxConvert::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (_argc < 2) {
		_logLevelVar->setVal(SDL_LOG_PRIORITY_INFO);
		Log::init();
		usage();
		return app::AppState::InitFailure;
	}

	core::String infilesstr;
	core::DynamicArray<core::String> infiles;
	if (hasArg("--input")) {
		int argn = 0;
		for (;;) {
			const core::String &val = getArgVal("--input", "", &argn);
			if (val.empty()) {
				break;
			}
			infiles.push_back(val);
			if (!infilesstr.empty()) {
				infilesstr += ", ";
			}
			infilesstr += val;
		}
	} else {
		Log::error("No input file was specified");
		return app::AppState::InitFailure;
	}
	core::String outfile;
	if (hasArg("--output")) {
		outfile = getArgVal("--output");
	}

	const bool mergeVolumes = hasArg("--merge");
	const bool scaleVolumes = hasArg("--scale");
	const bool mirrorVolumes = hasArg("--mirror");
	const bool rotateVolumes = hasArg("--rotate");
	const bool translateVolumes = hasArg("--translate");
	const bool srcPalette = hasArg("--src-palette");
	const bool exportPalette = hasArg("--export-palette");
	const bool exportLayers = hasArg("--export-layers");
	const bool changePivot = hasArg("--pivot");
	const bool cropVolumes = hasArg("--crop");
	const bool splitVolumes = hasArg("--split");
	const bool dumpSceneGraph = hasArg("--dump");
	const bool resizeVolumes = hasArg("--resize");

	Log::info("Options");
	if (voxelformat::isMeshFormat(outfile)) {
		Log::info("* mergeQuads:                    - %s", _mergeQuads->strVal().c_str());
		Log::info("* reuseVertices:                 - %s", _reuseVertices->strVal().c_str());
		Log::info("* ambientOcclusion:              - %s", _ambientOcclusion->strVal().c_str());
		Log::info("* scale:                         - %s", _scale->strVal().c_str());
		Log::info("* quads:                         - %s", _quads->strVal().c_str());
		Log::info("* withColor:                     - %s", _withColor->strVal().c_str());
		Log::info("* withTexCoords:                 - %s", _withTexCoords->strVal().c_str());
	}
	if (!srcPalette) {
		Log::info("* palette:                       - %s", _palette->strVal().c_str());
	}
	Log::info("* input files:                   - %s", infilesstr.c_str());
	if (!outfile.empty()) {
		Log::info("* output files:                  - %s", outfile.c_str());
	}
	core::String scriptParameters;
	if (hasArg("--script")) {
		scriptParameters = getArgVal("--script");
		Log::info("* script:                        - %s", scriptParameters.c_str());
	}
	Log::info("* dump scene graph:              - %s", (dumpSceneGraph ? "true" : "false"));
	Log::info("* merge volumes:                 - %s", (mergeVolumes ? "true" : "false"));
	Log::info("* scale volumes:                 - %s", (scaleVolumes ? "true" : "false"));
	Log::info("* crop volumes:                  - %s", (cropVolumes ? "true" : "false"));
	Log::info("* split volumes:                 - %s", (splitVolumes ? "true" : "false"));
	Log::info("* change pivot:                  - %s", (changePivot ? "true" : "false"));
	Log::info("* mirror volumes:                - %s", (mirrorVolumes ? "true" : "false"));
	Log::info("* translate volumes:             - %s", (translateVolumes ? "true" : "false"));
	Log::info("* rotate volumes:                - %s", (rotateVolumes ? "true" : "false"));
	Log::info("* use source file palette:       - %s", (srcPalette ? "true" : "false"));
	Log::info("* export used palette as image:  - %s", (exportPalette ? "true" : "false"));
	Log::info("* export layers:                 - %s", (exportLayers ? "true" : "false"));
	Log::info("* resize volumes:                - %s", (resizeVolumes ? "true" : "false"));

	if (!srcPalette) {
		io::FilePtr paletteFile = filesystem()->open(_palette->strVal());
		if (!paletteFile->exists()) {
			paletteFile = filesystem()->open(core::string::format("palette-%s.png", _palette->strVal().c_str()));
		}
		if (paletteFile->exists() && !voxel::initMaterialColors(paletteFile, io::FilePtr())) {
			Log::warn("Failed to init material colors");
		}
	}

	io::FilePtr outputFile;
	if (!outfile.empty()) {
		const bool outfileExists = filesystem()->open(outfile)->exists();
		if (outfileExists) {
			if (!hasArg("--force")) {
				Log::error("Given output file '%s' already exists", outfile.c_str());
				return app::AppState::InitFailure;
			}
		}

		outputFile = filesystem()->open(outfile, io::FileMode::SysWrite);
		if (!outputFile->validHandle()) {
			Log::error("Could not open target file: %s", outfile.c_str());
			return app::AppState::InitFailure;
		}
	} else if (!exportLayers && !exportPalette && !dumpSceneGraph) {
		Log::error("No output specified");
		return app::AppState::InitFailure;
	}

	voxel::SceneGraph sceneGraph;
	for (const core::String& infile : infiles) {
		if (filesystem()->isReadableDir(infile)) {
			core::DynamicArray<io::Filesystem::DirEntry> entities;
			filesystem()->list(infile, entities);
			Log::info("Found %i entries in dir %s", (int)entities.size(), infile.c_str());
			int success = 0;
			for (const io::Filesystem::DirEntry &entry : entities) {
				if (entry.type != io::Filesystem::DirEntry::Type::file) {
					continue;
				}
				const core::String fullpath = core::string::path(infile, entry.name);
				if (!handleInputFile(fullpath, sceneGraph, srcPalette, exportPalette, dumpSceneGraph)) {
					++success;
				}
			}
			if (success == 0) {
				Log::error("Could not find a valid input file in directory %s", infile.c_str());
				return app::AppState::InitFailure;
			}
		} else {
			if (!handleInputFile(infile, sceneGraph, srcPalette, exportPalette, dumpSceneGraph)) {
				return app::AppState::InitFailure;
			}
		}
	}

	const bool applyFilter = hasArg("--filter");
	if (applyFilter) {
		if (infiles.size() == 1u) {
			filterVolumes(sceneGraph);
		} else {
			Log::warn("Don't apply layer filters for multiple input files");
		}
	}

	if (exportLayers) {
		if (infiles.size() > 1) {
			Log::warn("The format and path of the first input file is used for exporting all layers");
		}
		exportLayersIntoSingleObjects(sceneGraph, infiles[0]);
	}

	if (mergeVolumes) {
		Log::info("Merge layers");
		voxel::RawVolume* merged = sceneGraph.merge();
		if (merged == nullptr) {
			Log::error("Failed to merge volumes");
			return app::AppState::InitFailure;
		}
		sceneGraph.clear();
		voxel::SceneGraphNode node;
		node.setVolume(merged, true);
		node.setName(infilesstr);
		sceneGraph.emplace(core::move(node));
	}

	if (scaleVolumes) {
		scale(sceneGraph);
	}

	if (resizeVolumes) {
		resize(getArgIvec3("--resize"), sceneGraph);
	}

	if (mirrorVolumes) {
		mirror(getArgVal("--mirror"), sceneGraph);
	}

	if (rotateVolumes) {
		rotate(getArgVal("--rotate"), sceneGraph);
	}

	if (translateVolumes) {
		translate(getArgIvec3("--translate"), sceneGraph);
	}

	if (!scriptParameters.empty()) {
		script(scriptParameters, sceneGraph);
	}

	if (changePivot) {
		pivot(getArgIvec3("--pivot"), sceneGraph);
	}

	if (cropVolumes) {
		crop(sceneGraph);
	}

	if (splitVolumes) {
		split(getArgIvec3("--split"), sceneGraph);
	}

	if (outputFile) {
		Log::debug("Save %i volumes", (int)sceneGraph.size());
		if (!voxelformat::saveFormat(outputFile, sceneGraph)) {
			Log::error("Failed to write to output file '%s'", outfile.c_str());
			return app::AppState::InitFailure;
		}
		Log::info("Wrote output file %s", outputFile->name().c_str());
	}
	return state;
}

core::String VoxConvert::getFilenameForLayerName(const core::String &inputfile, const core::String &layerName, int id) {
	const core::String &ext = core::string::extractExtension(inputfile);
	core::String name;
	if (layerName.empty()) {
		name = core::string::format("layer-%i.%s", id, ext.c_str());
	} else {
		name = core::string::format("%s.%s", layerName.c_str(), ext.c_str());
	}
	core::String modelPath = core::string::extractPath(inputfile);
	if (!modelPath.empty()) {
		modelPath.append("/");
	}
	modelPath.append(core::string::sanitizeFilename(name));
	return modelPath;
}

bool VoxConvert::handleInputFile(const core::String &infile, voxel::SceneGraph &sceneGraph, bool srcPalette,
								 bool exportPalette, bool dumpSceneGraph) {
	const io::FilePtr inputFile = filesystem()->open(infile, io::FileMode::SysRead);
	if (!inputFile->exists()) {
		Log::error("Given input file '%s' does not exist", infile.c_str());
		_exitCode = 127;
		return false;
	}
	const bool inputIsImage = inputFile->isAnyOf(io::format::images());
	if (!inputIsImage && srcPalette) {
		core::Array<uint32_t, 256> palette;
		io::FileStream palStream(inputFile.get());
		Log::info("Load palette from %s", infile.c_str());
		const size_t numColors = voxelformat::loadPalette(inputFile->name(), palStream, palette);
		if (numColors == 0) {
			Log::error("Failed to load palette");
			return false;
		}
		if (!voxel::initMaterialColors((const uint8_t*)palette.begin(), numColors * 4, "")) {
			Log::error("Failed to initialize material colors from loaded palette");
			return false;
		}
	}

	if (inputIsImage) {
		const image::ImagePtr& image = image::loadImage(inputFile, false);
		if (!image || !image->isLoaded()) {
			Log::error("Couldn't load image %s", infile.c_str());
			return false;
		}
		Log::info("Generate from heightmap (%i:%i)", image->width(), image->height());
		if (image->width() > MaxHeightmapWidth || image->height() >= MaxHeightmapHeight) {
			Log::warn("Skip creating heightmap - image dimensions exceeds the max allowed boundaries");
			return false;
		}
		voxel::Region region(0, 0, 0, image->width(), 255, image->height());
		voxel::RawVolume* volume = new voxel::RawVolume(region);
		voxel::SceneGraphNode node;
		node.setVolume(volume, true);
		node.setName(infile);
		sceneGraph.emplace(core::move(node));
		voxel::RawVolumeWrapper wrapper(volume);
		voxelutil::importHeightmap(wrapper, image);
	} else {
		io::FileStream inputFileStream(inputFile.get());
		voxel::SceneGraph newSceneGraph;
		if (!voxelformat::loadFormat(inputFile->name(), inputFileStream, newSceneGraph)) {
			return false;
		}
		if (dumpSceneGraph) {
			dump(newSceneGraph);
		}
		for (voxel::SceneGraphNode &node : newSceneGraph) {
			sceneGraph.emplace(core::move(node));
		}
	}

	if (exportPalette) {
		const core::String &paletteFile = core::string::stripExtension(infile) + ".png";
		voxel::saveMaterialColorPng(paletteFile);
		if (!srcPalette) {
			Log::info(" .. not using the input file palette");
		}
	}
	return true;
}

void VoxConvert::exportLayersIntoSingleObjects(voxel::SceneGraph& sceneGraph, const core::String &inputfile) {
	Log::info("Export layers into single objects");
	int n = 0;
	for (voxel::SceneGraphNode& node : sceneGraph) {
		voxel::SceneGraph newSceneGraph;
		voxel::SceneGraphNode newNode;
		newNode.setName(node.name());
		newNode.setVolume(node.volume(), false);
		newSceneGraph.emplace(core::move(newNode));
		const core::String& filename = getFilenameForLayerName(inputfile, node.name(), n);
		if (voxelformat::saveFormat(filesystem()->open(filename, io::FileMode::SysWrite), newSceneGraph)) {
			Log::info(" .. %s", filename.c_str());
		} else {
			Log::error(" .. %s", filename.c_str());
		}
	}
}

glm::ivec3 VoxConvert::getArgIvec3(const core::String &name) {
	const core::String &arguments = getArgVal(name);
	glm::ivec3 t(0);
	SDL_sscanf(arguments.c_str(), "%i:%i:%i", &t.x, &t.y, &t.z);
	return t;
}

void VoxConvert::split(const glm::ivec3 &size, voxel::SceneGraph& sceneGraph) {
	Log::info("split volumes at %i:%i:%i", size.x, size.y, size.z);
	voxel::RawVolume *merged = sceneGraph.merge();
	sceneGraph.clear();
	core::DynamicArray<voxel::RawVolume *> rawVolumes;
	voxel::splitVolume(merged, size, rawVolumes);
	delete merged;
	for (voxel::RawVolume *v : rawVolumes) {
		voxel::SceneGraphNode node;
		node.setVolume(v, true);
		sceneGraph.emplace(core::move(node));
	}
}

void VoxConvert::dumpNode_r(const voxel::SceneGraph& sceneGraph, int nodeId, int indent) {
	const voxel::SceneGraphNode& node = sceneGraph.node(nodeId);
	static const char* NodeTypeStr[] {
		"Root",
		"Model",
		"ModelReference",
		"Group",
		"Camera",
		"Unknown"
	};
	static_assert(core::enumVal(voxel::SceneGraphNodeType::Max) == lengthof(NodeTypeStr), "Array sizes don't match Max");
	const voxel::SceneGraphNodeType type = node.type();

	Log::info("%*sNode: %i (parent %i)", indent, " ", nodeId, node.parent());
	Log::info("%*s  |- name: %s", indent, " ", node.name().c_str());
	Log::info("%*s  |- type: %s", indent, " ", NodeTypeStr[core::enumVal(type)]);
	if (type == voxel::SceneGraphNodeType::Model) {
		Log::info("%*s  |- volume: %s", indent, " ", node.volume() != nullptr ? node.volume()->region().toString().c_str() : "no volume");
	}
	for (const auto & entry : node.properties()) {
		Log::info("%*s  |- %s: %s", indent, " ", entry->key.c_str(), entry->value.c_str());
	}
	Log::info("%*s  |- children: %i", indent, " ", (int)node.children().size());
	for (int children : node.children()) {
		dumpNode_r(sceneGraph, children, indent + 2);
	}
}

void VoxConvert::dump(const voxel::SceneGraph& sceneGraph) {
	dumpNode_r(sceneGraph, sceneGraph.root().id(), 0);
}

void VoxConvert::crop(voxel::SceneGraph& sceneGraph) {
	Log::info("Crop volumes");
	for (voxel::SceneGraphNode& node : sceneGraph) {
		node.setVolume(voxel::cropVolume(node.volume()), true);
	}
}

void VoxConvert::pivot(const glm::ivec3& pivot, voxel::SceneGraph& sceneGraph) {
	Log::info("Set pivot to %i:%i:%i", pivot.x, pivot.y, pivot.z);
	for (voxel::SceneGraphNode& node : sceneGraph) {
		node.setPivot(pivot, node.region().getDimensionsInVoxels());
	}
}

void VoxConvert::script(const core::String &scriptParameters, voxel::SceneGraph& sceneGraph) {
	voxelgenerator::LUAGenerator script;
	if (!script.init()) {
		Log::warn("Failed to initialize the script bindings");
	} else {
		core::DynamicArray<core::String> tokens;
		core::string::splitString(scriptParameters, tokens);
		const core::String &luaScript = script.load(tokens[0]);
		if (luaScript.empty()) {
			Log::error("Failed to load %s", tokens[0].c_str());
		} else {
			const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
			core::DynamicArray<voxelgenerator::LUAParameterDescription> argsInfo;
			if (!script.argumentInfo(luaScript, argsInfo)) {
				Log::warn("Failed to get argument details");
			}
			core::DynamicArray<core::String> args(tokens.size() - 1);
			for (size_t i = 1; i < tokens.size(); ++i) {
				args[i - 1] = tokens[i];
			}
			Log::info("Execute script %s", tokens[0].c_str());
			for (voxel::SceneGraphNode& node : sceneGraph) {
				voxel::RawVolumeWrapper wrapper(node.volume());
				script.exec(luaScript, &wrapper, wrapper.region(), voxel, args);
			}
		}
	}

	script.shutdown();
}

void VoxConvert::scale(voxel::SceneGraph& sceneGraph) {
	Log::info("Scale layers");
	for (voxel::SceneGraphNode& node : sceneGraph) {
		const voxel::Region srcRegion = node.region();
		const glm::ivec3& targetDimensionsHalf = (srcRegion.getDimensionsInVoxels() / 2) - 1;
		const voxel::Region destRegion(srcRegion.getLowerCorner(), srcRegion.getLowerCorner() + targetDimensionsHalf);
		if (destRegion.isValid()) {
			voxel::RawVolume* destVolume = new voxel::RawVolume(destRegion);
			rescaleVolume(*node.volume(), *destVolume);
			node.setVolume(destVolume, true);
		}
	}
}

void VoxConvert::resize(const glm::ivec3 &size, voxel::SceneGraph& sceneGraph) {
	Log::info("Resize layers");
	for (voxel::SceneGraphNode& node : sceneGraph) {
		node.setVolume(voxel::resize(node.volume(), size), true);
	}
}

void VoxConvert::filterVolumes(voxel::SceneGraph& sceneGraph) {
	const core::String &filter = getArgVal("--filter");
	if (filter.empty()) {
		Log::warn("No filter specified");
		return;
	}

	core::Set<int> layers;
	core::DynamicArray<core::String> tokens;
	core::string::splitString(filter, tokens, ",");
	for (const core::String& token : tokens) {
		if (token.contains("-")) {
			const int start = token.toInt();
			const size_t index = token.find("-");
			const core::String &endString = token.substr(index + 1);
			const int end = endString.toInt();
			for (int layer = start; layer <= end; ++layer) {
				layers.insert(layer);
			}
		} else {
			const int layer = token.toInt();
			layers.insert(layer);
		}
	}
	for (int i = 0; i < (int)sceneGraph.size(); ++i) {
		if (!layers.has(i)) {
			sceneGraph[i]->release();
			Log::debug("Remove layer %i - not part of the filter expression", i);
		}
	}
	Log::info("Filtered layers: %i", (int)layers.size());
}

void VoxConvert::mirror(const core::String& axisStr, voxel::SceneGraph& sceneGraph) {
	const math::Axis axis = math::toAxis(axisStr);
	if (axis == math::Axis::None) {
		return;
	}
	Log::info("Mirror on axis %c", axisStr[0]);
	for (voxel::SceneGraphNode &node : sceneGraph) {
		node.setVolume(voxel::mirrorAxis(node.volume(), axis), true);
	}
}

void VoxConvert::rotate(const core::String& axisStr, voxel::SceneGraph& sceneGraph) {
	const math::Axis axis = math::toAxis(axisStr);
	if (axis == math::Axis::None) {
		return;
	}
	Log::info("Rotate on axis %c", axisStr[0]);
	for (voxel::SceneGraphNode &node : sceneGraph) {
		node.setVolume(voxel::rotateAxis(node.volume(), axis), true);
	}
}

void VoxConvert::translate(const glm::ivec3& pos, voxel::SceneGraph& sceneGraph) {
	Log::info("Translate by %i:%i:%i", pos.x, pos.y, pos.z);
	for (voxel::SceneGraphNode &node : sceneGraph) {
		node.translate(pos);
	}
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	VoxConvert app(metric, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
