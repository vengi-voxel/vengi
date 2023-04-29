/**
 * @file
 */

#include "ImageGenerator.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "video/Renderer.h"
#include "video/Texture.h"
#include "voxel/RawVolume.h"
#include "voxelformat/Format.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/SceneGraphRenderer.h"

namespace voxelrender {

static image::ImagePtr volumeThumbnail(RenderContext &renderContext, voxelrender::SceneGraphRenderer &volumeRenderer, const scenegraph::SceneGraph &sceneGraph, const voxelformat::ThumbnailContext &ctx) {
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

	core_trace_scoped(EditorSceneRenderFramebuffer);
	volumeRenderer.prepare(const_cast<scenegraph::SceneGraph&>(sceneGraph));

	{
		const voxel::Region &region = sceneGraph.region();
		const glm::vec3 &center = sceneGraph.center();
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
		renderContext.frameBuffer.bind(true);
		volumeRenderer.render(renderContext, camera, true, true);
		renderContext.frameBuffer.unbind();
	}

	return renderContext.frameBuffer.image("thumbnail", video::FrameBufferAttachment::Color0);
}

image::ImagePtr volumeThumbnail(const scenegraph::SceneGraph &sceneGraph, const voxelformat::ThumbnailContext &ctx) {
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

bool volumeTurntable(const scenegraph::SceneGraph &sceneGraph, const core::String &imageFile, voxelformat::ThumbnailContext ctx, int loops) {
	voxelrender::SceneGraphRenderer volumeRenderer;
	RenderContext renderContext;
	renderContext.init(ctx.outputSize);
	volumeRenderer.construct();
	if (!volumeRenderer.init()) {
		Log::error("Failed to initialize the renderer");
		return image::ImagePtr();
	}

	volumeRenderer.setSceneMode(true);

	const core::String ext = core::string::extractExtension(imageFile);
	const core::String baseFilePath = core::string::stripExtension(imageFile);
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
			Log::error("Failed to create thumbnail for %s", imageFile.c_str());
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
