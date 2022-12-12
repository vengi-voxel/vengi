/**
 * @file
 */

#include "ImageGenerator.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Stream.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "video/Renderer.h"
#include "video/Texture.h"
#include "voxelformat/Format.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/SceneGraphRenderer.h"

namespace voxelrender {

static image::ImagePtr volumeThumbnail(RenderContext &renderContext, voxelrender::SceneGraphRenderer &volumeRenderer, const voxelformat::SceneGraph &sceneGraph, const voxelformat::ThumbnailContext &ctx) {
	video::FrameBuffer frameBuffer;
	video::clearColor(ctx.clearColor);
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);
	video::enable(video::State::Blend);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);

	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	textureCfg.format(video::TextureFormat::RGBA);
	video::FrameBufferConfig cfg;
	cfg.dimension(glm::ivec2(ctx.outputSize)).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24);
	cfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	frameBuffer.init(cfg);

	core_trace_scoped(EditorSceneRenderFramebuffer);
	frameBuffer.bind(true);
	volumeRenderer.prepare(const_cast<voxelformat::SceneGraph&>(sceneGraph));

	{
		const voxel::Region &region = sceneGraph.region();
		const glm::vec3 center(region.getCenter());
		const glm::vec3 dim(region.getDimensionsInVoxels());
		const int height = region.getHeightInCells();
		const float distance = ctx.distance <= 0.01f ? glm::length(dim) : ctx.distance;
		video::Camera camera;
		camera.setSize(ctx.outputSize);
		camera.setMode(video::CameraMode::Perspective);
		camera.setType(video::CameraType::Free);
		camera.setRotationType(video::CameraRotationType::Target);
		camera.setAngles(ctx.pitch, ctx.yaw, ctx.roll);
		camera.setFarPlane(5000.0f);
		camera.setTarget(center);
		camera.setTargetDistance(distance * 2.0f);
		camera.setWorldPosition(glm::vec3(-distance, (float)height + distance, -distance));
		camera.setOmega(ctx.omega);
		camera.update(ctx.deltaFrameSeconds);
		volumeRenderer.render(renderContext, camera, true, true);
	}
	frameBuffer.unbind();

	image::ImagePtr image = frameBuffer.image("thumbnail", video::FrameBufferAttachment::Color0);
	frameBuffer.shutdown();

	return image;
}

image::ImagePtr volumeThumbnail(const core::String &fileName, io::SeekableReadStream &stream, const voxelformat::ThumbnailContext &ctx) {
	image::ImagePtr image = voxelformat::loadScreenshot(fileName, stream);
	if (image && image->isLoaded()) {
		return image;
	}

	voxelformat::SceneGraph sceneGraph;
	stream.seek(0);
	if (!voxelformat::loadFormat(fileName, stream, sceneGraph)) {
		Log::error("Failed to load given input file: %s", fileName.c_str());
		return image::ImagePtr();
	}

	voxelrender::SceneGraphRenderer volumeRenderer;
	RenderContext renderContext;
	renderContext.init(ctx.outputSize);
	volumeRenderer.construct();
	if (!volumeRenderer.init()) {
		Log::error("Failed to initialize the renderer");
		return image::ImagePtr();
	}

	volumeRenderer.setSceneMode(true);
	image = volumeThumbnail(renderContext, volumeRenderer, sceneGraph, ctx);
	volumeRenderer.shutdown();
	renderContext.shutdown();
	return image;
}

image::ImagePtr volumeThumbnail(const voxelformat::SceneGraph &sceneGraph, const voxelformat::ThumbnailContext &ctx) {
	voxelrender::SceneGraphRenderer volumeRenderer;
	volumeRenderer.construct();
	RenderContext renderContext;
	renderContext.init(ctx.outputSize);
	if (!volumeRenderer.init()) {
		Log::error("Failed to initialize the renderer");
		return image::ImagePtr();
	}

	volumeRenderer.setSceneMode(true);
	const image::ImagePtr &image = volumeThumbnail(renderContext, volumeRenderer, sceneGraph, ctx);
	volumeRenderer.shutdown();
	renderContext.shutdown();
	return image;
}

bool volumeTurntable(const core::String &modelFile, const core::String &imageFile, voxelformat::ThumbnailContext ctx, int loops) {
	const core::String ext = core::string::extractExtension(imageFile);
	const core::String baseFilePath = core::string::stripExtension(imageFile);

	voxelformat::SceneGraph sceneGraph;
	io::FileStream stream(io::filesystem()->open(modelFile, io::FileMode::SysRead));
	stream.seek(0);
	if (!voxelformat::loadFormat(modelFile, stream, sceneGraph)) {
		Log::error("Failed to load given input file: %s", modelFile.c_str());
		return false;
	}

	voxelrender::SceneGraphRenderer volumeRenderer;
	RenderContext renderContext;
	renderContext.init(ctx.outputSize);
	volumeRenderer.construct();
	if (!volumeRenderer.init()) {
		Log::error("Failed to initialize the renderer");
		return image::ImagePtr();
	}

	volumeRenderer.setSceneMode(true);

	for (int i = 0; i < loops; ++i) {
		const core::String &filepath = core::string::format("%s_%i.%s", baseFilePath.c_str(), i, ext.c_str());
		const io::FilePtr &outfile = io::filesystem()->open(filepath, io::FileMode::SysWrite);
		io::FileStream outStream(outfile);
		const image::ImagePtr &image = volumeThumbnail(renderContext, volumeRenderer, sceneGraph, ctx);
		if (image) {
			if (!image::Image::writePng(outStream, image->data(), image->width(), image->height(), image->depth())) {
				Log::error("Failed to write image %s", filepath.c_str());
				volumeRenderer.shutdown();
				renderContext.shutdown();
				return false;
			} else {
				Log::info("Write image %s", filepath.c_str());
			}
		} else {
			Log::error("Failed to create thumbnail for %s", modelFile.c_str());
			volumeRenderer.shutdown();
			renderContext.shutdown();
			return false;
		}
		ctx.omega = glm::vec3(0.0f, glm::two_pi<float>() / (float)loops, 0.0f);
		ctx.deltaFrameSeconds += 1000.0 / (double)loops;
	}
	volumeRenderer.shutdown();
	renderContext.shutdown();
	return true;
}

} // namespace voxelrender
