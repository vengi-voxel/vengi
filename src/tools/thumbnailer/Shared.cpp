/**
 * @file
 */

#include "voxelformat/FormatThumbnail.h"
#include "Shared.h"
#include "app/App.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "voxel/RawVolume.h"
#include "io/FilesystemArchive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "video/Camera.h"
#include "voxelformat/Format.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelpathtracer/PathTracer.h"
#include "voxelpathtracer/PathTracerState.h"
#include "VoxelpathtracerShaderConstants.h"
#include "voxelrender/ImageGenerator.h"
#include "voxelrender/RenderUtil.h"
#include "voxelutil/ImageUtils.h"
#include <glm/gtc/constants.hpp>

namespace {

void configurePathTracerParams(voxelpathtracer::PathTracerParams &params, const voxelformat::ThumbnailContext &ctx,
							   voxelpathtracer::PathTracerQuality quality, int samples) {
	params = voxelpathtracer::PathTracerParams();
	applyPathTracerQuality(params, quality);
	// Thumbnails should not add an artificial ground plane by default.
	params.floorEnabled = false;
	params.vignette = 0.0f;
	params.resolution = core_max(1, ctx.outputSize.x);
	params.renderScale = 1.0f;
	params.sunElevation = ctx.sunElevation;
	params.sunAzimuth = ctx.sunAzimuth;
	if (samples > 0) {
		params.samples = samples;
	}
	// Offline thumbnails can submit larger batches than the interactive UI.
	params.samplesThisPass = core_min(params.samples, shader::VoxelpathtracerShaderConstants::getMaxSamplesThisPass());
}

video::Camera thumbnailCamera(const scenegraph::SceneGraph &sceneGraph, const voxelformat::ThumbnailContext &ctx) {
	video::Camera camera;
	if (ctx.useSceneCamera && sceneGraph.size(scenegraph::SceneGraphNodeType::Camera) > 0) {
		const scenegraph::SceneGraphNodeCamera &cameraNode =
			scenegraph::toCameraNode(*sceneGraph.begin(scenegraph::SceneGraphNodeType::Camera));
		camera = voxelrender::toCamera(ctx.outputSize, cameraNode);
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
		voxelrender::configureCamera(camera, sceneGraph.sceneRegion(), cameraMode, ctx.farPlane,
									 {ctx.pitch, ctx.yaw, ctx.roll});
		if (ctx.useWorldPosition) {
			camera.setRotationType(video::CameraRotationType::Eye);
			camera.setWorldPosition(ctx.worldPosition);
		}
	}
	camera.update(ctx.deltaFrameSeconds);
	return camera;
}

image::ImagePtr pathTracerThumbnail(const scenegraph::SceneGraph &sceneGraph, const voxelformat::ThumbnailContext &ctx,
									voxelpathtracer::PathTracerQuality quality, int samples) {
	voxelpathtracer::PathTracer pathTracer;
	if (!pathTracer.init()) {
		Log::error("Failed to initialize pathtracer for thumbnail");
		return image::ImagePtr();
	}
	configurePathTracerParams(pathTracer.state().params, ctx, quality, samples);
	const video::Camera camera = thumbnailCamera(sceneGraph, ctx);
	if (!pathTracer.start(sceneGraph, &camera)) {
		Log::error("Failed to start pathtracer for thumbnail");
		pathTracer.shutdown();
		return image::ImagePtr();
	}
	while (!pathTracer.update()) {
	}
	const image::ImagePtr image = pathTracer.image();
	pathTracer.shutdown();
	return image;
}

bool pathTracerTurntable(const scenegraph::SceneGraph &sceneGraph, const core::String &imageFile,
						 voxelformat::ThumbnailContext ctx, int loops, voxelpathtracer::PathTracerQuality quality,
						 int samples) {
	voxelpathtracer::PathTracer pathTracer;
	if (!pathTracer.init()) {
		Log::error("Failed to initialize pathtracer for turntable");
		return false;
	}
	configurePathTracerParams(pathTracer.state().params, ctx, quality, samples);

	const core::String ext = core::string::extractExtension(imageFile);
	const core::String baseFilePath = core::string::stripExtension(imageFile);
	for (int i = 0; i < loops; ++i) {
		const core::String &filepath = core::String::format("%s_%i.%s", baseFilePath.c_str(), i, ext.c_str());
		const io::FilePtr &outfile = io::filesystem()->open(filepath, io::FileMode::SysWrite);
		io::FileStream outStream(outfile);
		const video::Camera camera = thumbnailCamera(sceneGraph, ctx);
		if (i == 0) {
			if (!pathTracer.start(sceneGraph, &camera)) {
				Log::error("Failed to start pathtracer for turntable");
				pathTracer.shutdown();
				return false;
			}
		} else {
			pathTracer.applyCamera(&camera);
		}
		while (!pathTracer.update()) {
		}
		const image::ImagePtr &image = pathTracer.image();
		if (!image) {
			Log::error("Failed to create pathtracer thumbnail for %s", imageFile.c_str());
			pathTracer.shutdown();
			return false;
		}
		if (!image::Image::writePNG(outStream, image->data(), image->width(), image->height(), image->components())) {
			Log::error("Failed to write image %s", filepath.c_str());
			pathTracer.shutdown();
			return false;
		}
		Log::info("Write image %s", filepath.c_str());
		ctx.omega = glm::vec3(0.0f, glm::two_pi<float>() / (float)loops, 0.0f);
		ctx.deltaFrameSeconds += 1000.0 / (double)loops;
	}
	pathTracer.shutdown();
	return true;
}

} // namespace

image::ImagePtr volumeThumbnail(const core::String &fileName, const io::ArchivePtr &archive,
								voxelformat::ThumbnailContext &ctx, voxel::FaceNames image2dFace, bool isometric2d,
								bool pathTracer, voxelpathtracer::PathTracerQuality quality, int samples) {
	if (!pathTracer) {
		voxelformat::LoadContext loadctx;
		image::ImagePtr image = voxelformat::loadScreenshot(fileName, archive, loadctx);
		if (image && image->isLoaded()) {
			return image;
		}
	}

	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	fileDesc.set(fileName);
	voxelformat::LoadContext loadctx;
	if (!voxelformat::loadFormat(fileDesc, archive, sceneGraph, loadctx)) {
		Log::error("Failed to load given input file: %s", fileName.c_str());
		return image::ImagePtr();
	}

	if (image2dFace != voxel::FaceNames::Max) {
		if (pathTracer) {
			Log::warn("--pathtracer is ignored when --image is used");
		}
		scenegraph::SceneGraph::MergeResult merged = sceneGraph.merge();
		if (!merged.hasVolume()) {
			Log::error("No valid volume in the scenegraph to print");
			return image::ImagePtr();
		}
		core::ScopedPtr<voxel::RawVolume> v(merged.volume());
		const color::RGBA bgColor(0, 0, 0, 255);
		const float depthFactor2D = 0.0f;
		return isometric2d ? voxelutil::renderIsometricImage(v, merged.palette, image2dFace, bgColor, ctx.outputSize.x,
															 ctx.outputSize.y)
						   : voxelutil::renderToImage(v, merged.palette, image2dFace, bgColor, ctx.outputSize.x,
													  ctx.outputSize.y, false, depthFactor2D);
	}

	if (pathTracer) {
		return pathTracerThumbnail(sceneGraph, ctx, quality, samples);
	}
	return voxelrender::volumeThumbnail(sceneGraph, ctx);
}

bool volumeTurntable(const core::String &fileName, const core::String &imageFile, const voxelformat::ThumbnailContext &ctx,
					 int loops, bool pathTracer, voxelpathtracer::PathTracerQuality quality, int samples) {
	scenegraph::SceneGraph sceneGraph;
	const io::ArchivePtr &archive = io::openFilesystemArchive(io::filesystem());
	voxelformat::LoadContext loadctx;
	io::FileDescription fileDesc;
	fileDesc.set(fileName);
	if (!voxelformat::loadFormat(fileDesc, archive, sceneGraph, loadctx)) {
		Log::error("Failed to load given input file: %s", fileName.c_str());
		return false;
	}

	if (pathTracer) {
		Log::info("Render pathtracer turntable");
		return pathTracerTurntable(sceneGraph, imageFile, ctx, loops, quality, samples);
	}
	Log::info("Render turntable");
	return voxelrender::volumeTurntable(sceneGraph, imageFile, ctx, loops);
}
