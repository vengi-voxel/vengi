/**
 * @file
 */

#include "Shared.h"
#include "core/ScopedPtr.h"
#include "io/FilesystemArchive.h"
#include "voxelformat/Format.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/ImageGenerator.h"
#include "voxelutil/ImageUtils.h"

image::ImagePtr volumeThumbnail(const core::String &fileName, const io::ArchivePtr &archive,
								voxelformat::ThumbnailContext &ctx, voxel::FaceNames image2dFace, bool isometric2d) {
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

	if (image2dFace != voxel::FaceNames::Max) {
		scenegraph::SceneGraph::MergeResult merged = sceneGraph.merge();
		if (!merged.hasVolume()) {
			Log::error("No valid volume in the scenegraph to print");
			return image::ImagePtr();
		}
		core::ScopedPtr<voxel::RawVolume> v(merged.volume());
		const core::RGBA bgColor(0, 0, 0, 255);
		const float depthFactor2D = 0.0f;
		return isometric2d ? voxelutil::renderIsometricImage(v, merged.palette, image2dFace, bgColor, ctx.outputSize.x,
															 ctx.outputSize.y)
						   : voxelutil::renderToImage(v, merged.palette, image2dFace, bgColor, ctx.outputSize.x,
													  ctx.outputSize.y, false, depthFactor2D);
	}

	return voxelrender::volumeThumbnail(sceneGraph, ctx);
}

bool volumeTurntable(const core::String &fileName, const core::String &imageFile,
					 const voxelformat::ThumbnailContext &ctx, int loops) {
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
