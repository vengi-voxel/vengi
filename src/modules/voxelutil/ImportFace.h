/**
 * @file
 */

#pragma once

#include "app/Async.h"
#include "image/Image.h"
#include "palette/Palette.h"
#include "voxel/Face.h"
#include "voxel/Region.h"

namespace voxelutil {

template<class VOLUME>
bool importFace(VOLUME &volume, const voxel::Region &region, const palette::Palette &palette, voxel::FaceNames faceName,
				const image::ImagePtr &image, const glm::vec2 &uv0, const glm::vec2 &uv1, uint8_t replacementPalIdx = 0) {
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();
	const math::Axis axis = faceToAxis(faceName);
	const int axisIdx0 = math::getIndexForAxis(axis);
	const int axisIdx1 = axis == math::Axis::Y ? (axisIdx0 + 2) % 3 : (axisIdx0 + 1) % 3;
	const int axisIdx2 = axis == math::Axis::Y ? (axisIdx0 + 1) % 3 : (axisIdx0 + 2) % 3;
	const glm::vec3 size = region.getDimensionsInVoxels();
	const bool negativeFace = voxel::isNegativeFace(faceName);

	const int axisFixed = negativeFace ? mins[axisIdx0] : maxs[axisIdx0];
	const int axisMins1 = mins[axisIdx1];
	const int axisMins2 = mins[axisIdx2];
	const int axisMaxs1 = maxs[axisIdx1];
	const int axisMaxs2 = maxs[axisIdx2];
	const int axisIdxUV1 = (axisIdx1 + 0) % 2;
	const int axisIdxUV2 = (axisIdx1 + 1) % 2;

	app::for_parallel(
		axisMins1, axisMaxs1 + 1,
		[axisFixed, axisIdx0, axisIdx1, axisIdx2, axisMaxs2, axisMins2, axisIdxUV1, axisIdxUV2, replacementPalIdx,
		 &image, &palette, &uv0, &uv1, &size, &volume, axisMins1](int start, int end) {
			const bool flipU = false;
			const bool flipV = false;
			const image::TextureWrap wrapS = flipU ? image::TextureWrap::MirroredRepeat : image::Repeat;
			const image::TextureWrap wrapT = flipV ? image::TextureWrap::MirroredRepeat : image::Repeat;
			for (int axis1 = start; axis1 < end; ++axis1) {
				const float axis1Factor = ((float)(axis1 - axisMins1) + 0.5f) / (float)size[axisIdx1];
				for (int axis2 = axisMins2; axis2 <= axisMaxs2; ++axis2) {
					int palIdx = replacementPalIdx;
					if (image) {
						const float axis2Factor = ((float)(axis2 - axisMins2) + 0.5f) / (float)size[axisIdx2];
						glm::vec2 uv;
						uv[axisIdxUV1] = glm::mix(flipU ? -uv0[axisIdxUV1] : uv0[axisIdxUV1],
												  flipV ? -uv1[axisIdxUV1] : uv1[axisIdxUV1], axis1Factor);
						uv[axisIdxUV2] = glm::mix(flipU ? -uv0[axisIdxUV2] : uv0[axisIdxUV2],
												  flipV ? -uv1[axisIdxUV2] : uv1[axisIdxUV2], axis2Factor);
						const core::RGBA color = image->colorAt(uv, wrapS, wrapT);
						if (color.a == 0) {
							continue;
						}
						palIdx = palette.getClosestMatch(color);
						if (palIdx == palette::PaletteColorNotFound) {
							palIdx = replacementPalIdx;
						}
					}
					glm::ivec3 pos;
					pos[axisIdx0] = axisFixed;
					pos[axisIdx1] = axis1;
					pos[axisIdx2] = axis2;
					volume.setVoxel(pos, voxel::createVoxel(palette, palIdx));
				}
			}
		});
	return true;
}

} // namespace voxelutil
