/**
 * @file
 */

#include "SkinFormat.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "image/Image.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/Face.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/ImportFace.h"
#include <glm/trigonometric.hpp>

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
		glm::ivec3 size;		  // width, height, depth in voxels
		glm::vec3 translation;	  // World position (offset) of the part
		glm::vec3 rotationDegree; // Rotation in degrees around the pivot point
		glm::vec3 pivot;
		glm::ivec2 tex[(int)voxel::FaceNames::Max]; // left, right, top, bottom, front, back
	};

	const bool applyTransform = core::Var::getSafe(cfg::VoxformatSkinApplyTransform)->boolVal();
	const bool addGroup = core::Var::getSafe(cfg::VoxformatSkinAddGroups)->boolVal();

	// Define the skin boxes and use names that animate.lua can work with
	const SkinBox skinParts[] = {{"head",
								  {8, 8, 8},
								  {0.0f, 24.0f, 0.0f},
								  {0, 0, 0},
								  {0.0f, 0.0f, 0.0f},
								  {{0, 8}, {16, 8}, {16, 0}, {8, 0}, {24, 8}, {8, 8}}},
								 {"body",
								  {8, 12, 4},
								  {4.0f, 12.0f, 4.0f},
								  {0, 0, 0},
								  {0.5f, 0.0f, 0.5f},
								  {{16, 20}, {28, 20}, {28, 16}, {20, 16}, {32, 20}, {20, 20}}},
								 {"shoulder_r",
								  {4, 12, 4},
								  {8.0f, 21.6f, 4.0f},
								  {45, 0, 0},
								  {0.0f, 0.8f, 0.5f},
								  {{40, 20}, {48, 20}, {48, 16}, {44, 16}, {52, 20}, {44, 20}}},
								 {"shoulder_l",
								  {4, 12, 4},
								  {0.0f, 21.6f, 4.0f},
								  {-45, 0, 0},
								  {1.0f, 0.8f, 0.5f},
								  {{32, 52}, {40, 52}, {40, 48}, {36, 48}, {44, 52}, {36, 52}}},
								 {"leg_r",
								  {4, 12, 4},
								  {2.0f, 12.0f, 4.0f},
								  {-45, 0, 0},
								  {0.5f, 1.0f, 0.5f},
								  {{0, 20}, {8, 20}, {8, 16}, {4, 16}, {12, 20}, {4, 20}}},
								 {"leg_l",
								  {4, 12, 4},
								  {6.0f, 12.0f, 4.0f},
								  {45, 0, 0},
								  {0.5f, 1.0f, 0.5f},
								  {{16, 52}, {24, 52}, {24, 48}, {20, 48}, {28, 52}, {20, 52}}}};

	const voxel::FaceNames order[] = {voxel::FaceNames::NegativeX /* left */,  voxel::FaceNames::PositiveX /* right */,
									  voxel::FaceNames::PositiveY /* top */,   voxel::FaceNames::NegativeY /* bottom */,
									  voxel::FaceNames::PositiveZ /* front */, voxel::FaceNames::NegativeZ /* back */};
	static_assert(lengthof(SkinBox::tex) == lengthof(order),
				  "SkinBox::tex and order must have the same number of elements");

	for (const auto &part : skinParts) {
		const glm::ivec3 size = part.size;
		const voxel::Region region(0, 0, 0, size.x - 1, size.y - 1, size.z - 1);

		int parentId = 0;
		if (addGroup) {
			scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
			groupNode.setName(part.name);
			groupNode.setPalette(palette);
			parentId = sceneGraph.emplace(core::move(groupNode));
		}
		// TODO: VOXELFORMAT: back side is somehow flipped - maybe even the uvs are wrong
		for (int i = 0; i < lengthof(order); ++i) {
			const glm::ivec2 &uv = part.tex[i];
			const glm::ivec2 uvMin(uv.x, uv.y);
			const glm::ivec2 uvMax = uvMin +
									 glm::ivec2((i == 2 || i == 3)	 ? size.x
												: (i == 0 || i == 1) ? size.z
																	 : size.x,
												(i == 2 || i == 3) ? size.z : size.y) -
									 glm::ivec2(1);
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setVolume(new voxel::RawVolume(region), true);
			node.setName(part.name);
			node.setPalette(palette);
			voxelutil::importFace(*node.volume(), node.region(), palette, order[i], image, image->uv(uv.x, uvMax.y),
								  image->uv(uvMax.x, uv.y));
			scenegraph::SceneGraphTransform transform;
			if (applyTransform) {
				transform.setLocalTranslation(part.translation);
				transform.setLocalOrientation(glm::quat(glm::radians(part.rotationDegree)));
				node.setPivot(part.pivot);
			} else {
				const glm::vec3 regionSize(region.getDimensionsInVoxels());
				transform.setLocalTranslation(part.translation - part.pivot * regionSize);
			}
			node.setTransform(0, transform);
			sceneGraph.emplace(core::move(node), parentId);
		}
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
