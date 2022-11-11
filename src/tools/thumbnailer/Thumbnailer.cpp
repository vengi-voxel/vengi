/**
 * @file
 */

#include "Thumbnailer.h"
#include "core/Color.h"
#include "command/Command.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/ImageGenerator.h"
#include "core/Log.h"

Thumbnailer::Thumbnailer(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	init(ORGANISATION, "thumbnailer");
	_showWindow = false;
	_initialLogLevel = SDL_LOG_PRIORITY_ERROR;
	_additionalUsage = "<infile> <outfile>";
}

app::AppState Thumbnailer::onConstruct() {
	app::AppState state = Super::onConstruct();

	registerArg("--size").setShort("-s").setDescription("Size of the thumbnail in pixels").setDefaultValue("128").setMandatory();

	return state;
}

app::AppState Thumbnailer::onInit() {
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
	_outfile = _argv[_argc - 1];

	Log::debug("infile: %s", infile.c_str());
	Log::debug("outfile: %s", _outfile.c_str());

	_infile = filesystem()->open(infile, io::FileMode::SysRead);
	if (!_infile->exists()) {
		Log::error("Given input file '%s' does not exist", infile.c_str());
		usage();
		return app::AppState::InitFailure;
	}

	return state;
}

app::AppState Thumbnailer::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}

	const int outputSize = core::string::toInt(getArgVal("--size"));
	const io::FilePtr& outfile = filesystem()->open(_outfile, io::FileMode::SysWrite);
	io::FileStream outStream(outfile);
	io::FileStream stream(_infile);

	const image::ImagePtr &image = voxelrender::volumeThumbnail(_infile->name(), stream, glm::ivec2(outputSize));
	if (image) {
		if (!image::Image::writePng(outStream, image->data(), image->width(), image->height(), image->depth())) {
			Log::error("Failed to write image");
		} else {
			Log::info("Write image %s", _outfile.c_str());
		}
	} else {
		Log::error("Failed to create thumbnail for %s", _infile->name().c_str());
	}

	requestQuit();
	return state;
}

app::AppState Thumbnailer::onCleanup() {
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr& filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	Thumbnailer app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
