/**
 * @file
 */

#include "VoxConvert.h"
#include "core/Color.h"
#include "core/Var.h"
#include "command/Command.h"
#include "io/Filesystem.h"
#include "metric/Metric.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/VoxFileFormat.h"
#include "voxelutil/VolumeRescaler.h"

VoxConvert::VoxConvert(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "voxconvert");
	_initialLogLevel = SDL_LOG_PRIORITY_ERROR;
}

app::AppState VoxConvert::onConstruct() {
	const app::AppState state = Super::onConstruct();
	registerArg("--merge").setShort("-m").setDescription("Merge layers into one volume");
	registerArg("--scale").setShort("-s").setDescription("Scale layer to 50% of its original size");
	registerArg("--force").setShort("-f").setDescription("Overwrite existing files");

	core::Var::get("voxformat_mergequads", "true", core::CV_NOPERSIST)->setHelp("Merge similar quads to optimize the mesh");
	core::Var::get("voxformat_reusevertices", "true", core::CV_NOPERSIST)->setHelp("Reuse vertices or always create new ones");
	core::Var::get("voxformat_ambientocclusion", "false", core::CV_NOPERSIST)->setHelp("Extra vertices for ambient occlusion");

	_palette = core::Var::get("palette", voxel::getDefaultPaletteName());
	_palette->setHelp("This is the NAME part of palette-<NAME>.pngor absolute png file to use (1x256)");

	return state;
}

app::AppState VoxConvert::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		Log::error("Failed to init application");
		return state;
	}

	if (_argc < 2) {
		_logLevelVar->setVal(SDL_LOG_PRIORITY_INFO);
		Log::init();
		usage();
		return app::AppState::InitFailure;
	}

	io::FilePtr paletteFile = filesystem()->open(core::string::format("palette-%s.png", _palette->strVal().c_str()));
	if (!paletteFile->exists()) {
		paletteFile = filesystem()->open(_palette->strVal());
	}
	if (!voxel::initMaterialColors(paletteFile, io::FilePtr())) {
		Log::error("Failed to init default material colors");
		return app::AppState::InitFailure;
	}

	const core::String infile = _argv[_argc - 2];
	const core::String outfile = _argv[_argc - 1];

	Log::debug("infile: %s", infile.c_str());
	Log::debug("outfile: %s", outfile.c_str());

	const io::FilePtr inputFile = filesystem()->open(infile, io::FileMode::SysRead);
	if (!inputFile->exists()) {
		Log::error("Given input file '%s' does not exist", infile.c_str());
		_exitCode = 127;
		return app::AppState::InitFailure;
	}

	const io::FilePtr outputFile = filesystem()->open(outfile, io::FileMode::SysWrite);
	if (!outputFile->validHandle()) {
		Log::error("Could not open target file: %s", outfile.c_str());
		return app::AppState::InitFailure;
	}
	if (outputFile->length() > 0) {
		if (!hasArg("--force") && !hasArg("-f")) {
			Log::error("Given output file '%s' already exists", outfile.c_str());
			return app::AppState::InitFailure;
		}
	}

	voxel::VoxelVolumes volumes;
	if (!voxelformat::loadVolumeFormat(inputFile, volumes)) {
		Log::error("Failed to load given input file");
		return app::AppState::InitFailure;
	}

	if (hasArg("--merge") || hasArg("-m")) {
		voxel::RawVolume* merged = volumes.merge();
		if (merged == nullptr) {
			Log::error("Failed to merge volumes");
			return app::AppState::InitFailure;
		}
		voxelformat::clearVolumes(volumes);
		volumes.push_back(voxel::VoxelVolume(merged));
	}

	if (hasArg("--scale") || hasArg("-s")) {
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

	if (!voxelformat::saveFormat(outputFile, volumes)) {
		voxelformat::clearVolumes(volumes);
		Log::error("Failed to write to output file '%s'", outfile.c_str());
		return app::AppState::InitFailure;
	}
	Log::info("Wrote output file %s", outputFile->name().c_str());

	voxelformat::clearVolumes(volumes);

	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	VoxConvert app(metric, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
