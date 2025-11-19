/**
 * @file
 */

#include "Thumbnailer.h"
#include "Shared.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include "voxel/Face.h"
#include "voxelformat/FormatConfig.h"
#include "engine-git.h"
#include "voxelrender/SceneGraphRenderer.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

Thumbnailer::Thumbnailer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider) {
	init(ORGANISATION, "thumbnailer");
	_showWindow = false;
	_wantCrashLogs = true;
	_initialLogLevel = Log::Level::Error;
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
	registerArg("--output")
		.setShort("-o")
		.setDescription("The output image file")
		.addFlag(ARGUMENT_FLAG_FILE | ARGUMENT_FLAG_MANDATORY);
	registerArg("--size").setShort("-s").setDescription("Size of the thumbnail in pixels").setDefaultValue("128");
	registerArg("--turntable").setShort("-t").setDescription("Render in different angles (16 by default)");
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
	registerArg("--sunelevation")
		.setDefaultValue("45")
		.setDescription("Set the sun elevation");
	registerArg("--sunazimuth")
		.setDefaultValue("135")
		.setDescription("Set the sun azimuth");
	registerArg("--position").setShort("-p").setDefaultValue("0:0:0").setDescription("Set the camera position");
	registerArg("--image").setDescription("Create a 2d image of the scene");
	registerArg("--isometric").setDescription("Create an isometric thumbnail of the input file when --image is used");
	Argument &cameraMode =
		registerArg("--camera-mode")
			.setDefaultValue(voxelrender::SceneCameraModeStr[(int)voxelrender::SceneCameraMode::Free])
			.setDescription("Allow to change the camera positioning for rendering");
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

	if (!io::Filesystem::sysExists(infile)) {
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
		Log::debug("Use position %f:%f:%f", ctx.worldPosition.x, ctx.worldPosition.y, ctx.worldPosition.z);
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
	if (hasArg("--sunelevation")) {
		ctx.sunElevation = core::string::toFloat(getArgVal("--sunelevation"));
	}
	if (hasArg("--sunazimuth")) {
		ctx.sunAzimuth = core::string::toFloat(getArgVal("--sunazimuth"));
	}

	const int renderTurntableLoops = hasArg("--turntable") ? getArgVal("--turntable", "16").toInt() : 0;
	if (renderTurntableLoops > 0) {
		volumeTurntable(infile, _outfile, ctx, renderTurntableLoops);
	} else {
		const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);
		if (!archive) {
			Log::error("Failed to open %s for reading", infile.c_str());
			return app::AppState::Cleanup;
		}
		voxel::FaceNames frontFace = voxel::FaceNames::Max;
		bool isometric2d = false;
		if (hasArg("--image")) {
			const core::String &faceStr = getArgVal("--image", voxel::faceNameString(voxel::FaceNames::Front));
			frontFace = voxel::toFaceNames(faceStr, voxel::FaceNames::Front);
			isometric2d = hasArg("--isometric");
		}
		const image::ImagePtr &image = volumeThumbnail(infile, archive, ctx, frontFace, isometric2d);
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
		if (!image::Image::writePNG(outStream, image->data(), image->width(), image->height(), image->components())) {
			Log::error("Failed to write image %s", _outfile.c_str());
		} else {
			Log::info("Write image %s", _outfile.c_str());
		}
		return true;
	}
	Log::error("Failed to create thumbnail");
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
