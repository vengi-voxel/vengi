/**
 * @file
 */

#include "ImageGenerator.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "video/Renderer.h"
#include "video/Texture.h"
#include "voxel/MeshState.h"
#include "voxelformat/Format.h"
#include "voxelrender/RenderContext.h"
#include "voxelrender/RenderUtil.h"
#include "voxelrender/SceneGraphRenderer.h"

namespace voxelrender {

static image::ImagePtr volumeThumbnail(const voxel::MeshStatePtr &meshState, RenderContext &renderContext, voxelrender::SceneGraphRenderer &volumeRenderer, const voxelformat::ThumbnailContext &ctx) {
	if (!renderContext.sceneGraph) {
		Log::error("No scene graph set");
		return image::ImagePtr();
	}
	const scenegraph::SceneGraph &sceneGraph = *renderContext.sceneGraph;
	// TODO: RENDERER: use scoped state changes like video::ScopedBlendMode
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

	video::Camera camera;

	if (ctx.useSceneCamera && sceneGraph.size(scenegraph::SceneGraphNodeType::Camera) > 0) {
		const scenegraph::SceneGraphNodeCamera &cameraNode =
			scenegraph::toCameraNode(*sceneGraph.begin(scenegraph::SceneGraphNodeType::Camera));
		camera = toCamera(ctx.outputSize, cameraNode);
		if (ctx.distance > 0.01f) {
			camera.setTargetDistance(ctx.distance);
		}
		if (ctx.useWorldPosition) {
			camera.setWorldPosition(ctx.worldPosition);
		}
	} else {
		if (ctx.useSceneCamera) {
			Log::warn("Could not find any camera in the scene");
		}

		camera.setOmega(ctx.omega);
		camera.setSize(ctx.outputSize);
		camera.setMode(video::CameraMode::Perspective);
		camera.setType(video::CameraType::Free);
		voxelrender::SceneCameraMode cameraMode = voxelrender::SceneCameraMode::Free;
		for (int i = 0; i < lengthof(voxelrender::SceneCameraModeStr); ++i) {
			if (core::string::iequals(ctx.cameraMode, voxelrender::SceneCameraModeStr[i])) {
				cameraMode = (voxelrender::SceneCameraMode)i;
				break;
			}
		}
		configureCamera(camera, sceneGraph.sceneRegion(), cameraMode, ctx.farPlane, {ctx.pitch, ctx.yaw, ctx.roll});
		if (ctx.useWorldPosition) {
			camera.setRotationType(video::CameraRotationType::Eye);
			camera.setWorldPosition(ctx.worldPosition);
		}
	}
	camera.update(ctx.deltaFrameSeconds);

	renderContext.frameBuffer.bind(true);
	volumeRenderer.render(meshState, renderContext, camera, true, true);
	renderContext.frameBuffer.unbind();

	return renderContext.frameBuffer.image("thumbnail", video::FrameBufferAttachment::Color0);
}

image::ImagePtr volumeThumbnail(const scenegraph::SceneGraph &sceneGraph, const voxelformat::ThumbnailContext &ctx) {
	voxelrender::SceneGraphRenderer sceneGraphRenderer;
	sceneGraphRenderer.construct();
	RenderContext renderContext;
	renderContext.init(ctx.outputSize);
	renderContext.renderMode = RenderMode::Scene;
	renderContext.sceneGraph = &sceneGraph;
	renderContext.onlyModels = true;
	sceneGraphRenderer.setSunAngle(glm::vec3(ctx.sunElevation, ctx.sunAzimuth, 0.0f));
	const voxel::MeshStatePtr meshState = core::make_shared<voxel::MeshState>();
	meshState->construct();
	meshState->init();
	if (!sceneGraphRenderer.init(meshState->hasNormals())) {
		Log::error("Failed to initialize the renderer");
		return image::ImagePtr();
	}

	const image::ImagePtr &image = volumeThumbnail(meshState, renderContext, sceneGraphRenderer, ctx);
	sceneGraphRenderer.shutdown();
	renderContext.shutdown();
	// don't free the volumes here, they belong to the scene graph
	(void)meshState->shutdown();
	return image;
}

bool volumeTurntable(const scenegraph::SceneGraph &sceneGraph, const core::String &imageFile, voxelformat::ThumbnailContext ctx, int loops) {
	voxelrender::SceneGraphRenderer sceneGraphRenderer;
	RenderContext renderContext;
	renderContext.init(ctx.outputSize);
	renderContext.renderMode = RenderMode::Scene;
	renderContext.sceneGraph = &sceneGraph;
	renderContext.onlyModels = true;
	const voxel::MeshStatePtr meshState = core::make_shared<voxel::MeshState>();
	meshState->construct();
	meshState->init();

	sceneGraphRenderer.construct();
	if (!sceneGraphRenderer.init(meshState->hasNormals())) {
		Log::error("Failed to initialize the renderer");
		return image::ImagePtr();
	}

	const core::String ext = core::string::extractExtension(imageFile);
	const core::String baseFilePath = core::string::stripExtension(imageFile);
	for (int i = 0; i < loops; ++i) {
		const core::String &filepath = core::String::format("%s_%i.%s", baseFilePath.c_str(), i, ext.c_str());
		const io::FilePtr &outfile = io::filesystem()->open(filepath, io::FileMode::SysWrite);
		io::FileStream outStream(outfile);
		const image::ImagePtr &image = volumeThumbnail(meshState, renderContext, sceneGraphRenderer, ctx);
		if (image) {
			if (!image::Image::writePNG(outStream, image->data(), image->width(), image->height(), image->components())) {
				Log::error("Failed to write image %s", filepath.c_str());
				sceneGraphRenderer.shutdown();
				renderContext.shutdown();
				// don't free the volumes here, they belong to the scene graph
				(void)meshState->shutdown();
				return false;
			} else {
				Log::info("Write image %s", filepath.c_str());
			}
		} else {
			Log::error("Failed to create thumbnail for %s", imageFile.c_str());
			sceneGraphRenderer.shutdown();
			renderContext.shutdown();
			// don't free the volumes here, they belong to the scene graph
			(void)meshState->shutdown();
			return false;
		}
		ctx.omega = glm::vec3(0.0f, glm::two_pi<float>() / (float)loops, 0.0f);
		ctx.deltaFrameSeconds += 1000.0 / (double)loops;
	}
	sceneGraphRenderer.shutdown();
	renderContext.shutdown();
	// don't free the volumes here, they belong to the scene graph
	(void)meshState->shutdown();
	return true;
}

} // namespace voxelrender
