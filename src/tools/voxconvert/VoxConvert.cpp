/**
 * @file
 */

#include "VoxConvert.h"
#include "core/Color.h"
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
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/Format.h"
#include "voxelgenerator/LUAGenerator.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/VolumeRescaler.h"

VoxConvert::VoxConvert(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "voxconvert");
	_initialLogLevel = SDL_LOG_PRIORITY_ERROR;
}

app::AppState VoxConvert::onConstruct() {
	const app::AppState state = Super::onConstruct();
	registerArg("--export-palette").setDescription("Export the used palette data into an image. Use in combination with --src-palette");
	registerArg("--filter").setDescription("Layer filter. For example '1-4,6'");
	registerArg("--force").setShort("-f").setDescription("Overwrite existing files");
	registerArg("--merge").setShort("-m").setDescription("Merge layers into one volume");
	registerArg("--scale").setShort("-s").setDescription("Scale layer to 50% of its original size");
	registerArg("--script").setDefaultValue("script.lua").setDescription("Apply the given lua script to the output volume");
	registerArg("--src-palette").setShort("-p").setDescription("Keep the source palette and don't perform quantization");

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

	const core::String infile = _argv[_argc - 2];
	const core::String outfile = _argv[_argc - 1];

	const bool mergeVolumes = hasArg("--merge") || hasArg("-m");
	const bool scaleVolumes = hasArg("--scale") || hasArg("-s");
	const bool srcPalette = hasArg("--src-palette") || hasArg("-p");
	const bool exportPalette = hasArg("--export-palette");

	Log::info("Options");
	if (voxelformat::isMeshFormat(outfile)) {
		Log::info("* palette:          - %s", _palette->strVal().c_str());
		Log::info("* mergeQuads:       - %s", _mergeQuads->strVal().c_str());
		Log::info("* reuseVertices:    - %s", _reuseVertices->strVal().c_str());
		Log::info("* ambientOcclusion: - %s", _ambientOcclusion->strVal().c_str());
		Log::info("* scale:            - %s", _scale->strVal().c_str());
		Log::info("* quads:            - %s", _quads->strVal().c_str());
		Log::info("* withColor:        - %s", _withColor->strVal().c_str());
		Log::info("* withTexCoords:    - %s", _withTexCoords->strVal().c_str());
	}
	Log::info("* infile:                        - %s", infile.c_str());
	Log::info("* outfile:                       - %s", outfile.c_str());
	core::String scriptParameters;
	if (hasArg("--script")) {
		scriptParameters = getArgVal("--script");
		Log::info("* script:                        - %s", scriptParameters.c_str());
	}
	Log::info("* merge volumes:                 - %s", (mergeVolumes ? "true" : "false"));
	Log::info("* scale volumes:                 - %s", (scaleVolumes ? "true" : "false"));
	Log::info("* use source file palette:       - %s", (srcPalette ? "true" : "false"));
	Log::info("* export used palette as image:  - %s", (exportPalette ? "true" : "false"));

	const io::FilePtr inputFile = filesystem()->open(infile, io::FileMode::SysRead);
	if (!inputFile->exists()) {
		Log::error("Given input file '%s' does not exist", infile.c_str());
		_exitCode = 127;
		return app::AppState::InitFailure;
	}
	const bool inputIsImage = inputFile->isAnyOf(io::format::images());
	Log::info("* generate from heightmap:       - %s", (inputIsImage ? "true" : "false"));

	io::FilePtr paletteFile = filesystem()->open(core::string::format("palette-%s.png", _palette->strVal().c_str()));
	if (!paletteFile->exists()) {
		paletteFile = filesystem()->open(_palette->strVal());
	}
	if (!voxel::initMaterialColors(paletteFile, io::FilePtr())) {
		Log::error("Failed to init default material colors");
		return app::AppState::InitFailure;
	}

	if (!inputIsImage && srcPalette) {
		core::Array<uint32_t, 256> palette;
		io::FileStream palStream(inputFile.get());
		const size_t numColors = voxelformat::loadPalette(inputFile->name(), palStream, palette);
		if (numColors == 0) {
			Log::error("Failed to load palette from input file");
			return app::AppState::InitFailure;
		}
		if (!voxel::initMaterialColors((const uint8_t*)palette.begin(), numColors, "")) {
			Log::error("Failed to initialize material colors from input file");
			return app::AppState::InitFailure;
		}

		if (exportPalette) {
			const core::String &paletteFile = core::string::stripExtension(infile) + ".png";
			image::Image img(paletteFile);
			img.loadRGBA((const uint8_t*)palette.begin(), (int)numColors * 4, (int)numColors, 1);
			if (!img.writePng()) {
				Log::warn("Failed to write the palette file");
			}
		}
	}

	const io::FilePtr outputFile = filesystem()->open(outfile, io::FileMode::SysWrite);
	if (!outputFile->validHandle()) {
		Log::error("Could not open target file: %s", outfile.c_str());
		return app::AppState::InitFailure;
	}
	if (outputFile->exists()) {
		if (!hasArg("--force") && !hasArg("-f")) {
			Log::error("Given output file '%s' already exists", outfile.c_str());
			return app::AppState::InitFailure;
		}
	}

	voxel::VoxelVolumes volumes;
	if (inputIsImage) {
		const image::ImagePtr& image = image::loadImage(inputFile, false);
		if (!image || !image->isLoaded()) {
			Log::error("Couldn't load image %s", infile.c_str());
			return app::AppState::InitFailure;
		}
		voxel::Region region(0, 0, 0, image->width(), 255, image->height());
		voxel::RawVolume* volume = new voxel::RawVolume(region);
		volumes.push_back(voxel::VoxelVolume(volume, infile, true, glm::ivec3(0)));
		voxel::RawVolumeWrapper wrapper(volume);
		voxelutil::importHeightmap(wrapper, image);
	} else {
		io::FileStream inputFileStream(inputFile.get());
		if (!voxelformat::loadFormat(inputFile->name(), inputFileStream, volumes)) {
			Log::error("Failed to load given input file");
			return app::AppState::InitFailure;
		}
	}

	filterVolumes(volumes);

	if (mergeVolumes) {
		Log::info("Merge layers");
		voxel::RawVolume* merged = volumes.merge();
		if (merged == nullptr) {
			Log::error("Failed to merge volumes");
			return app::AppState::InitFailure;
		}
		voxelformat::clearVolumes(volumes);
		volumes.push_back(voxel::VoxelVolume(merged));
	}

	if (scaleVolumes) {
		Log::info("Scale layers");
		for (auto& v : volumes) {
			const voxel::Region srcRegion = v.volume->region();
			const glm::ivec3& targetDimensionsHalf = (srcRegion.getDimensionsInVoxels() / 2) - 1;
			const voxel::Region destRegion(srcRegion.getLowerCorner(), srcRegion.getLowerCorner() + targetDimensionsHalf);
			if (destRegion.isValid()) {
				voxel::RawVolume* destVolume = new voxel::RawVolume(destRegion);
				rescaleVolume(*v.volume, *destVolume);
				delete v.volume;
				v.volume = destVolume;
			}
		}
	}

	if (!scriptParameters.empty()) {
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
				for (auto& v : volumes) {
					voxel::RawVolumeWrapper wrapper(v.volume);
					script.exec(luaScript, &wrapper, wrapper.region(), voxel, args);
				}
			}
		}

		script.shutdown();
	}

	Log::debug("Save");
	if (!voxelformat::saveFormat(outputFile, volumes)) {
		voxelformat::clearVolumes(volumes);
		Log::error("Failed to write to output file '%s'", outfile.c_str());
		return app::AppState::InitFailure;
	}
	Log::info("Wrote output file %s", outputFile->name().c_str());

	voxelformat::clearVolumes(volumes);

	return state;
}

void VoxConvert::filterVolumes(voxel::VoxelVolumes& volumes) {
	const bool applyFilter = hasArg("--filter");
	if (!applyFilter) {
		return;
	}

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
	for (int i = 0; i < (int)volumes.size(); ++i) {
		if (!layers.has(i)) {
			delete volumes.volumes[i].volume;
			volumes.volumes[i].volume = nullptr;
		}
	}
	Log::info("Filtered layers: %i", (int)layers.size());
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	VoxConvert app(metric, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
