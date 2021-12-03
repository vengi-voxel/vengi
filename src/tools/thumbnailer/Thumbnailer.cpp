/**
 * @file
 */

#include "Thumbnailer.h"
#include "core/Color.h"
#include "command/Command.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "metric/Metric.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"
#include "video/Renderer.h"
#include "voxel/MaterialColor.h"
#include "video/Camera.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/Format.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "video/FrameBuffer.h"
#include "video/Texture.h"

Thumbnailer::Thumbnailer(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
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
		Log::error("Failed to init application");
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

bool Thumbnailer::renderVolume() {
	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to init default material colors");
		return false;
	}

	video::FrameBuffer frameBuffer;
	voxelrender::RawVolumeRenderer volumeRenderer;

	volumeRenderer.construct();

	if (!volumeRenderer.init()) {
		Log::error("Failed to initialize the renderer");
		return false;
	}

	video::clearColor(::core::Color::Black);
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);
	video::enable(video::State::Blend);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);
	voxel::VoxelVolumes volumes;
	io::FileStream stream(_infile.get());
	if (!voxelformat::loadVolumeFormat(_infile->fileName(), stream, volumes)) {
		Log::error("Failed to load given input file");
		return false;
	}

	_outputSize = core::string::toInt(getArgVal("--size"));

	const int volumesSize = (int)volumes.size();
	for (int i = 0; i < volumesSize; ++i) {
		volumeRenderer.setVolume(i, volumes[i].volume);
		volumeRenderer.extractRegion(i, volumes[i].volume->region());
	}

	video::Camera camera;
	camera.setSize(glm::ivec2(_outputSize));
	camera.setRotationType(video::CameraRotationType::Target);
	camera.setMode(video::CameraMode::Perspective);
	camera.setAngles(0.0f, 0.0f, 0.0f);
	const voxel::Region& region = volumeRenderer.region();
	const glm::ivec3& center = region.getCenter();
	camera.setTarget(center);
	const glm::vec3 dim(region.getDimensionsInVoxels());
	const float distance = glm::length(dim);
	camera.setTargetDistance(distance * 2.0f);
	const int height = region.getHeightInCells();
	camera.setWorldPosition(glm::vec3(-distance, height + distance, -distance));
	camera.lookAt(center);
	camera.setFarPlane(5000.0f);
	camera.update(0.001);

	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	textureCfg.format(video::TextureFormat::RGBA);
	video::FrameBufferConfig cfg;
	cfg.dimension(glm::ivec2(_outputSize)).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24);
	cfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	frameBuffer.init(cfg);

	volumeRenderer.waitForPendingExtractions();
	volumeRenderer.update();
	core_trace_scoped(EditorSceneRenderFramebuffer);
	frameBuffer.bind(true);
	volumeRenderer.render(camera);
	frameBuffer.unbind();

	bool success = true;
	const video::TexturePtr& fboTexture = frameBuffer.texture(video::FrameBufferAttachment::Color0);
	uint8_t *pixels = nullptr;
	if (video::readTexture(video::TextureUnit::Upload,
			textureCfg.type(), textureCfg.format(), fboTexture->handle(),
			fboTexture->width(), fboTexture->height(), &pixels)) {
		image::Image::flipVerticalRGBA(pixels, fboTexture->width(), fboTexture->height());
		const io::FilePtr& outfile = filesystem()->open(_outfile, io::FileMode::SysWrite);
		if (!image::Image::writePng(outfile->name().c_str(), pixels, fboTexture->width(), fboTexture->height(), 4)) {
			Log::error("Failed to write image %s", outfile->name().c_str());
			success = false;
		} else {
			Log::info("Created thumbnail at %s", outfile->name().c_str());
		}
	} else {
		Log::error("Failed to read framebuffer");
		success = false;
	}
	SDL_free(pixels);

	const core::DynamicArray<voxel::RawVolume*>& old = volumeRenderer.shutdown();
	for (auto* v : old) {
		delete v;
	}

	frameBuffer.shutdown();

	return success;
}

bool Thumbnailer::saveEmbeddedScreenshot() {
	io::FileStream stream(_infile.get());
	const image::ImagePtr &image = voxelformat::loadVolumeScreenshot(_infile->fileName(), stream);
	if (!image) {
		Log::error("Failed to load screenshot from input file");
		return false;
	}

	bool success = true;
	const io::FilePtr& outfile = filesystem()->open(_outfile, io::FileMode::SysWrite);
	if (!image::Image::writePng(outfile->name().c_str(), image->data(), image->width(), image->height(), image->depth())) {
		Log::error("Failed to write image %s", outfile->name().c_str());
		success = false;
	} else {
		Log::info("Created thumbnail at %s", outfile->name().c_str());
	}

	return success;
}

app::AppState Thumbnailer::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!saveEmbeddedScreenshot()) {
		if (!renderVolume()) {
			_exitCode = 1;
		}
	}

	requestQuit();
	return state;
}

app::AppState Thumbnailer::onCleanup() {
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	Thumbnailer app(metric, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
