/**
 * @file
 */

#include "Thumbnailer.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include "io/FormatDescription.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/ImageGenerator.h"
#include "engine-git.h"
#include "voxelrender/SceneGraphRenderer.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

Thumbnailer::Thumbnailer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider) {
	init(ORGANISATION, "thumbnailer");
	_showWindow = false;
	_wantCrashLogs = true;
	_initialLogLevel = SDL_LOG_PRIORITY_ERROR;
}

void Thumbnailer::printUsageHeader() const {
	Super::printUsageHeader();
	Log::info("Git commit " GIT_COMMIT " - " GIT_COMMIT_DATE);
}

app::AppState Thumbnailer::onConstruct() {
	app::AppState state = Super::onConstruct();

	voxelformat::FormatConfig::init();

	registerArg("--input")
		.setShort("-i")
		.setDescription("The input file to create a thumbnail for")
		.addFlag(ARGUMENT_FLAG_FILE)
		.addFlag(ARGUMENT_FLAG_MANDATORY);
	registerArg("--output").setShort("-o").setDescription("The output image file").addFlag(ARGUMENT_FLAG_FILE);
	registerArg("--size")
		.setShort("-s")
		.setDescription("Size of the thumbnail in pixels")
		.setDefaultValue("128")
		.addFlag(ARGUMENT_FLAG_MANDATORY);
	registerArg("--turntable").setShort("-t").setDescription("Render in different angles");
	registerArg("--fallback").setShort("-f").setDescription("Create a fallback thumbnail if an error occurs");
	registerArg("--use-scene-camera")
		.setShort("-c")
		.setDescription("Use the first scene camera for rendering the thumbnail");
	registerArg("--distance")
		.setShort("-d")
		.setDefaultValue("-1")
		.setDescription("Set the camera distance to the target");
	registerArg("--angles")
		.setShort("-a")
		.setDefaultValue("0:0:0")
		.setDescription("Set the camera angles (pitch:yaw:roll))");
	registerArg("--position").setShort("-p").setDefaultValue("0:0:0").setDescription("Set the camera position");
	Argument& cameraMode = registerArg("--camera-mode").setDefaultValue("free").setDescription("Allow to change the camera positioning for rendering");
	for (int i = 0; i < (int)voxelrender::SceneCameraMode::Max; ++i) {
		cameraMode.addValidValue(voxelrender::SceneCameraModeStr[i]);
	}

	return state;
}

app::AppState Thumbnailer::onInit() {
	const app::AppState state = Super::onInit();

	if (state != app::AppState::Running) {
		const bool fallback = hasArg("--fallback");
		if (fallback) {
			_outfile = getArgVal("--output");
			if (_outfile.empty()) {
				Log::error("No output file given");
				return app::AppState::InitFailure;
			}
			Log::warn("Use fallback (black) image");
			image::ImagePtr image = image::createEmptyImage(_outfile);
			core::RGBA black(0, 0, 0, 255);
			image->loadRGBA((const uint8_t *)&black, 1, 1);
			saveImage(image);
			return app::AppState::Cleanup;
		}
		return state;
	}

	return state;
}

static image::ImagePtr volumeThumbnail(const core::String &fileName, const io::ArchivePtr &archive,
									   voxelformat::ThumbnailContext &ctx) {
	voxelformat::LoadContext loadctx;
	image::ImagePtr image = voxelformat::loadScreenshot(fileName, archive, loadctx);
	if (image && image->isLoaded()) {
		return image;
	}

	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	fileDesc.set(fileName);
	if (!voxelformat::loadFormat(fileDesc, archive, sceneGraph, loadctx)) {
		Log::error("Failed to load given input file: %s", fileName.c_str());
		return image::ImagePtr();
	}

	return voxelrender::volumeThumbnail(sceneGraph, ctx);
}

static bool volumeTurntable(const core::String &fileName, const core::String &imageFile,
							voxelformat::ThumbnailContext ctx, int loops) {
	scenegraph::SceneGraph sceneGraph;
	const io::ArchivePtr &archive = io::openFilesystemArchive(io::filesystem());
	voxelformat::LoadContext loadctx;
	io::FileDescription fileDesc;
	fileDesc.set(fileName);
	if (!voxelformat::loadFormat(fileDesc, archive, sceneGraph, loadctx)) {
		Log::error("Failed to load given input file: %s", fileName.c_str());
		return false;
	}

	Log::info("Render turntable");
	return voxelrender::volumeTurntable(sceneGraph, imageFile, ctx, loops);
}

app::AppState Thumbnailer::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}

	const core::String infile = getArgVal("--input");
	if (infile.empty()) {
		Log::error("No input file given");
		return app::AppState::InitFailure;
	}

	_outfile = getArgVal("--output");
	if (_outfile.empty()) {
		Log::error("No output file given");
		return app::AppState::InitFailure;
	}

	Log::debug("infile: %s", infile.c_str());
	Log::debug("outfile: %s", _outfile.c_str());

	_infile = filesystem()->open(infile, io::FileMode::SysRead);
	if (!_infile->exists()) {
		Log::error("Given input file '%s' does not exist", infile.c_str());
		return app::AppState::InitFailure;
	}

	const int outputSize = core::string::toInt(getArgVal("--size"));

	voxelformat::ThumbnailContext ctx;
	ctx.outputSize = glm::ivec2(outputSize);
	ctx.useSceneCamera = hasArg("--use-scene-camera");
	ctx.distance = core::string::toFloat(getArgVal("--distance", "-1.0"));
	ctx.cameraMode = getArgVal("--camera-mode", "free");
	ctx.useWorldPosition = hasArg("--position");
	if (ctx.useWorldPosition) {
		const core::String &pos = getArgVal("--position");
		core::string::parseVec3(pos, glm::value_ptr(ctx.worldPosition), ":");
		Log::info("Use position %f:%f:%f", ctx.worldPosition.x, ctx.worldPosition.y, ctx.worldPosition.z);
	}
	if (hasArg("--angles")) {
		const core::String &anglesStr = getArgVal("--angles");
		glm::vec3 angles(0.0f);
		core::string::parseVec3(anglesStr, glm::value_ptr(angles), ":");
		ctx.pitch = angles.x;
		ctx.yaw = angles.y;
		ctx.roll = angles.z;
		Log::info("Use euler angles %f:%f:%f", ctx.pitch, ctx.yaw, ctx.roll);
	}

	const bool renderTurntable = hasArg("--turntable");
	if (renderTurntable) {
		volumeTurntable(_infile->name(), _outfile, ctx, 16);
	} else {
		const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);
		if (!archive) {
			Log::error("Failed to open %s for reading", _infile->name().c_str());
			return app::AppState::Cleanup;
		}
		const image::ImagePtr &image = volumeThumbnail(_infile->name(), archive, ctx);
		saveImage(image);
	}

	requestQuit();
	return state;
}

bool Thumbnailer::saveImage(const image::ImagePtr &image) {
	if (image) {
		const io::FilePtr &outfile = io::filesystem()->open(_outfile, io::FileMode::SysWrite);
		io::FileStream outStream(outfile);
		if (!outStream.valid()) {
			Log::error("Failed to open %s for writing", _outfile.c_str());
			return false;
		}
		if (!image::Image::writePng(outStream, image->data(), image->width(), image->height(), image->depth())) {
			Log::error("Failed to write image %s", _outfile.c_str());
		} else {
			Log::info("Write image %s", _outfile.c_str());
		}
		return true;
	}
	Log::error("Failed to create thumbnail for %s", _infile->name().c_str());
	return false;
}

app::AppState Thumbnailer::onCleanup() {
	return Super::onCleanup();
}

#ifndef WINDOWS_THUMBNAILER_DLL
int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	Thumbnailer app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
#endif
