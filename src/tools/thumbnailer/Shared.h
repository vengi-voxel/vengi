/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "io/Archive.h"
#include "voxel/Face.h"
#include "voxelformat/FormatThumbnail.h"
#include "voxelpathtracer/PathTracerState.h"

/**
 * @param pathTracer When true, render with the GPU pathtracer instead of the forward renderer.
 * @param quality Pathtracer quality preset. Ignored when pathTracer is false.
 * @param samples Override pathtracer sample count when > 0. Ignored when pathTracer is false.
 */
image::ImagePtr volumeThumbnail(const core::String &fileName, const io::ArchivePtr &archive,
								voxelformat::ThumbnailContext &ctx, voxel::FaceNames image2dFace, bool isometric2d,
								bool pathTracer = false,
								voxelpathtracer::PathTracerQuality quality = voxelpathtracer::PathTracerQuality::Default,
								int samples = -1);

bool volumeTurntable(const core::String &fileName, const core::String &imageFile, const voxelformat::ThumbnailContext &ctx,
					 int loops, bool pathTracer = false,
					 voxelpathtracer::PathTracerQuality quality = voxelpathtracer::PathTracerQuality::Default,
					 int samples = -1);
