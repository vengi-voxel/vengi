/**
 * @file
 */

#include "SkinFormat.h"
#include "core/ScopedPtr.h"
#include "image/Image.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Face.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelutil/ImageUtils.h"

namespace voxelformat {

bool SkinFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
								scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}

	const image::ImagePtr &image = image::loadImage(filename, *stream);
	if (!image || !image->isLoaded()) {
		Log::error("Failed to load image %s", filename.c_str());
		return false;
	}

	if (image->width() != 64 || image->height() != 64) {
		Log::error("Invalid skin image size %ix%i, expected 64x64", image->width(), image->height());
		return false;
	}

	// Defines a single 3D part from its 6 faces (x, y, z sizes and face coordinates)
	struct SkinBox {
		const char *name;
		glm::ivec3 size;   // width, height, depth in voxels
		glm::ivec3 pivot;  // World position (offset) of the part
		glm::ivec2 tex[6]; // left, right, top, bottom, front, back
	};

	// Define the skin boxes and use names that animate.lua can work with
	const SkinBox skinParts[] = {
		{"head", {8, 8, 8}, {0, 24, 0}, {{0, 8}, {16, 8}, {16, 0}, {8, 0}, {24, 8}, {8, 8}}},
		{"body", {8, 12, 4}, {0, 12, 2}, {{16, 20}, {28, 20}, {28, 16}, {20, 16}, {32, 20}, {20, 20}}},
		{"shoulder_r", {4, 12, 4}, {-4, 12, 2}, {{40, 20}, {48, 20}, {48, 16}, {44, 16}, {52, 20}, {44, 20}}},
		{"shoulder_l", {4, 12, 4}, {8, 12, 2}, {{32, 52}, {40, 52}, {40, 48}, {36, 48}, {44, 52}, {36, 52}}},
		{"leg_r", {4, 12, 4}, {0, 0, 2}, {{0, 20}, {8, 20}, {8, 16}, {4, 16}, {12, 20}, {4, 20}}},
		{"leg_l", {4, 12, 4}, {4, 0, 2}, {{16, 52}, {24, 52}, {24, 48}, {20, 48}, {28, 52}, {20, 52}}}};

	for (const auto &part : skinParts) {
		const glm::ivec3 size = part.size;
		voxel::Region region(0, 0, 0, size.x - 1, size.y - 1, size.z - 1);
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(region), true);
		node.setName(part.name);
		node.setPalette(palette);

		voxel::RawVolumeWrapper wrapper(node.volume());

		const voxel::FaceNames order[] = {voxel::FaceNames::NegativeX, voxel::FaceNames::PositiveX,
										  voxel::FaceNames::NegativeY, voxel::FaceNames::PositiveY,
										  voxel::FaceNames::NegativeZ, voxel::FaceNames::PositiveZ};

		// TODO: VOXELFORMAT: each face should be imported into a separate volume (and a group for each body component)
		// and should also be placed by the scenegraph
		// TODO: VOXELFORMAT: pivot to prepare for animation
		for (int i = 0; i < 6; ++i) {
			const glm::ivec2 &uv = part.tex[i];
			const glm::ivec2 uvMin(uv.x, uv.y);
			const glm::ivec2 uvMax = uvMin +
									 glm::ivec2((i == 2 || i == 3)	 ? size.x
												: (i == 0 || i == 1) ? size.z
																	 : size.x,
												(i == 2 || i == 3) ? size.z : size.y) -
									 glm::ivec2(1);
			voxelutil::importFace(wrapper, wrapper.region(), palette, order[i], image, image->uv(uv.x, uvMax.y),
								  image->uv(uvMax.x, uv.y));
		}
		node.volume()->region().shift(part.pivot);
		sceneGraph.emplace(core::move(node));
	}
	return true;
}

// TODO: VOXELFORMAT: implement saving
bool SkinFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							const io::ArchivePtr &archive, const SaveContext &ctx) {
	Log::error("Saving Minecraft skin format is not supported");
	return false;
}

} // namespace voxelformat
