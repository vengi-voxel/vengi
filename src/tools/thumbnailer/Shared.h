/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "io/Archive.h"
#include "voxel/Face.h"
#include "voxelformat/FormatThumbnail.h"

image::ImagePtr volumeThumbnail(const core::String &fileName, const io::ArchivePtr &archive,
								voxelformat::ThumbnailContext &ctx, voxel::FaceNames image2dFace, bool isometric2d);

bool volumeTurntable(const core::String &fileName, const core::String &imageFile,
					 const voxelformat::ThumbnailContext &ctx, int loops);
