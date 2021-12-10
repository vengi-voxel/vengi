/**
 * @file
 */

#include "ImageGenerator.h"
#include "image/Image.h"
#include "io/Stream.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "video/Renderer.h"
#include "video/Texture.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/Format.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "core/Color.h"
#include "io/FileStream.h"

namespace thumbnailer {

image::ImagePtr volumeThumbnail(const core::String &fileName, io::SeekableReadStream &stream, int outputSize) {
	image::ImagePtr image = voxelformat::loadVolumeScreenshot(fileName, stream);
	if (image) {
		return image;
	}
	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the default materials");
		return image::ImagePtr();
	}

	stream.seek(0);
	core::Array<uint32_t, 256> palette;
	const size_t paletteCount = voxelformat::loadVolumePalette(fileName, stream, palette);
	if (paletteCount > 0) {
		voxel::overrideMaterialColors((const uint8_t*)palette.begin(), paletteCount * 4, "");
	}

	voxel::VoxelVolumes volumes;
	stream.seek(0);
	if (!voxelformat::loadVolumeFormat(fileName, stream, volumes)) {
		Log::error("Failed to load given input file");
		return image::ImagePtr();
	}

	video::FrameBuffer frameBuffer;
	voxelrender::RawVolumeRenderer volumeRenderer;
	volumeRenderer.construct();
	if (!volumeRenderer.init()) {
		Log::error("Failed to initialize the renderer");
		return image::ImagePtr();
	}

	video::clearColor(::core::Color::Black);
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);
	video::enable(video::State::Blend);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);

	const int volumesSize = (int)volumes.size();
	for (int i = 0; i < volumesSize; ++i) {
		volumeRenderer.setVolume(i, volumes[i].volume);
		volumeRenderer.extractRegion(i, volumes[i].volume->region());
	}

	video::Camera camera;
	camera.setSize(glm::ivec2(outputSize));
	camera.setRotationType(video::CameraRotationType::Target);
	camera.setMode(video::CameraMode::Perspective);
	camera.setAngles(0.0f, 0.0f, 0.0f);
	const voxel::Region &region = volumeRenderer.region();
	const glm::ivec3 &center = region.getCenter();
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
	cfg.dimension(glm::ivec2(outputSize)).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24);
	cfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	frameBuffer.init(cfg);

	volumeRenderer.waitForPendingExtractions();
	volumeRenderer.update();
	core_trace_scoped(EditorSceneRenderFramebuffer);
	frameBuffer.bind(true);
	volumeRenderer.render(camera);
	frameBuffer.unbind();

	const video::TexturePtr &fboTexture = frameBuffer.texture(video::FrameBufferAttachment::Color0);
	uint8_t *pixels = nullptr;
	if (video::readTexture(video::TextureUnit::Upload, textureCfg.type(), textureCfg.format(), fboTexture->handle(),
						   fboTexture->width(), fboTexture->height(), &pixels)) {
		image::Image::flipVerticalRGBA(pixels, fboTexture->width(), fboTexture->height());
		image = image::createEmptyImage("thumbnail");
		image->loadRGBA(pixels, fboTexture->width() * fboTexture->height() * 4, fboTexture->width(), fboTexture->height());
	} else {
		Log::error("Failed to read framebuffer");
	}
	SDL_free(pixels);

	const core::DynamicArray<voxel::RawVolume *> &old = volumeRenderer.shutdown();
	for (auto *v : old) {
		delete v;
	}

	frameBuffer.shutdown();

	return image;
}

} // namespace thumbnailer
