/**
 * @file
 */

#include "SkinFormat.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "image/Image.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/ImportFace.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/trigonometric.hpp>

namespace voxelformat {

// Defines a single 3D part from its 6 faces (x, y, z sizes and face coordinates)
struct SkinBox {
	const char *name;
	glm::ivec3 size;		  // width, height, depth in voxels
	glm::vec3 translation;	  // World position (offset) of the part
	glm::vec3 rotationDegree; // Rotation in degrees around the pivot point
	glm::vec3 pivot;
	glm::ivec2 tex[(int)voxel::FaceNames::Max]; // left, right, top, bottom, front, back
};

// Define the skin boxes and use names that animate.lua can work with
static const SkinBox skinParts[] = {{"head",
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

static const voxel::FaceNames order[] = {
	voxel::FaceNames::NegativeX /* left */,	 voxel::FaceNames::PositiveX /* right */,
	voxel::FaceNames::NegativeY /* bottom */,voxel::FaceNames::PositiveY /* top */,
	voxel::FaceNames::PositiveZ /* front */, voxel::FaceNames::NegativeZ /* back */};

static void addNode(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, int parentId,
					bool applyTransform, voxel::FaceNames faceNameOffset, const SkinBox &part) {
	scenegraph::SceneGraphTransform transform;
	glm::vec3 translation = part.translation;
	if (faceNameOffset != voxel::FaceNames::Max) {
		// offset the translation by the face name offset to prevent z-fighting
		const bool isX = voxel::isX(faceNameOffset);
		const bool isY = voxel::isY(faceNameOffset);
		const bool isZ = voxel::isZ(faceNameOffset);
		const bool isNegative = voxel::isNegativeFace(faceNameOffset);
		const float offset = applyTransform ? 0.1f : 0.02f;
		const float offsetSign = isNegative ? -offset : offset;
		if (isX) {
			translation.x += offsetSign;
		} else if (isY) {
			translation.y += offsetSign;
		} else if (isZ) {
			translation.z += offsetSign;
		}
	}
	if (applyTransform) {
		transform.setLocalTranslation(translation);
		const glm::quat orientation(glm::radians(part.rotationDegree));
		transform.setLocalOrientation(orientation);
		node.setPivot(part.pivot);
	} else {
		const glm::vec3 regionSize(node.region().getDimensionsInVoxels());
		transform.setLocalTranslation(translation - part.pivot * regionSize);
	}
	node.setTransform(0, transform);
	sceneGraph.emplace(core::move(node), parentId);
}

static int getFaceIndex(voxel::FaceNames faceName) {
	for (int i = 0; i < lengthof(order); ++i) {
		if (order[i] == faceName) {
			return i;
		}
	}
	return 0;
}

// we have special needs for the visitor order here - to be independent from other use-cases for the
// face visitor, we define our own order here
static voxelutil::VisitorOrder visitorOrderForFace(voxel::FaceNames face) {
	voxelutil::VisitorOrder visitorOrder;
	switch (face) {
	case voxel::FaceNames::Front:
		visitorOrder = voxelutil::VisitorOrder::mYmXZ;
		break;
	case voxel::FaceNames::Back:
		visitorOrder = voxelutil::VisitorOrder::mYXmZ;
		break;
	case voxel::FaceNames::Right:
		visitorOrder = voxelutil::VisitorOrder::mYmZmX;
		break;
	case voxel::FaceNames::Left:
		visitorOrder = voxelutil::VisitorOrder::mYZX;
		break;
	case voxel::FaceNames::Up:
		visitorOrder = voxelutil::VisitorOrder::mZmXmY;
		break;
	case voxel::FaceNames::Down:
		visitorOrder = voxelutil::VisitorOrder::ZmXY;
		break;
	default:
		return voxelutil::VisitorOrder::Max;
	}
	return visitorOrder;
}

static void importPart(const image::ImagePtr &image, const SkinBox &part, voxel::FaceNames faceName,
					   scenegraph::SceneGraphNode &node) {
	int idx = getFaceIndex(faceName);
	// TODO: VOXELFORMAT: back side is somehow flipped - maybe even the uvs are wrong
	const glm::ivec2 &uv = part.tex[idx];
	const glm::ivec2 uvMin(uv.x, uv.y);
	const bool isY = voxel::isY(faceName);
	const bool isX = voxel::isX(faceName);
	const glm::ivec3 &size = part.size;
	const int uvMaxX = isY ? size.x : (isX ? size.z : size.x);
	const int uvMaxY = isY ? size.z : size.y;
	const glm::ivec2 uvMax = uvMin + glm::ivec2(uvMaxX, uvMaxY) - glm::ivec2(1);
	const glm::vec2 uv0(image->uv(uv.x, uvMax.y));
	const glm::vec2 uv1(image->uv(uvMax.x, uv.y));
	const palette::Palette &palette = node.palette();
	voxelutil::importFace(*node.volume(), node.region(), palette, faceName, image, uv0, uv1);
}

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

	const bool applyTransform = core::Var::getSafe(cfg::VoxformatSkinApplyTransform)->boolVal();
	const bool addGroup = core::Var::getSafe(cfg::VoxformatSkinAddGroups)->boolVal();
	const bool mergeFaces = core::Var::getSafe(cfg::VoxformatSkinMergeFaces)->boolVal();

	static_assert(lengthof(SkinBox::tex) == lengthof(order),
				  "SkinBox::tex and order must have the same number of elements");

	for (const auto &part : skinParts) {
		const glm::ivec3 size = part.size;
		const voxel::Region region(0, 0, 0, size.x - 1, size.y - 1, size.z - 1);

		int parentId = 0;
		if (addGroup) {
			scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
			groupNode.setName(core::String::format("Group %s", part.name));
			groupNode.setPalette(palette);
			parentId = sceneGraph.emplace(core::move(groupNode));
		}
		if (mergeFaces) {
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setVolume(new voxel::RawVolume(region), true);
			node.setName(part.name);
			node.setPalette(palette);
			for (int i = 0; i < lengthof(order); ++i) {
				importPart(image, part, order[i], node);
			}
			addNode(sceneGraph, node, parentId, applyTransform, voxel::FaceNames::Max, part);
		} else {
			for (int i = 0; i < lengthof(order); ++i) {
				scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
				node.setVolume(new voxel::RawVolume(region), true);
				node.setName(core::String::format("%s_%s", part.name, voxel::faceNameString(order[i])));
				node.setPalette(palette);

				const voxel::FaceNames faceName = order[i];
				importPart(image, part, faceName, node);
				addNode(sceneGraph, node, parentId, applyTransform, faceName, part);
			}
		}
	}
	return true;
}

bool SkinFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}

	image::ImagePtr image = image::createEmptyImage("Minecraft Skin");
	image->resize(64, 64);
	for (int y = 0; y < image->height(); ++y) {
		for (int x = 0; x < image->width(); ++x) {
			image->setColor(core::RGBA(0, 0, 0, 0), x, y);
		}
	}

	const bool mergedFaces = sceneGraph.findNodeByName(skinParts[0].name) != nullptr;
	for (const auto &part : skinParts) {
		for (int i = 0; i < lengthof(order); ++i) {
			const voxel::FaceNames faceName = order[i];
			const core::String name =
				mergedFaces ? part.name : core::String::format("%s_%s", part.name, voxel::faceNameString(faceName));
			const scenegraph::SceneGraphNode *node = sceneGraph.findNodeByName(name);
			if (!node) {
				Log::error("Node %s not found in scene graph", name.c_str());
				continue;
			}
			if (!node->isAnyModelNode()) {
				Log::error("Node %s is not a model node", name.c_str());
				continue;
			}
			const palette::Palette &palette = node->palette();
			const math::Axis axis = voxel::faceToAxis(faceName);
			const int axisIdx = math::getIndexForAxis(axis);
			const glm::ivec3 &mins = node->region().getLowerCorner();
			const glm::ivec3 &dim = node->region().getDimensionsInCells();
			const int idx1 = (axisIdx + 1) % 3;
			const int idx2 = (axisIdx + 2) % 3;
			if (isZ(faceName)) {
				Log::error("foo");
			}
			const voxel::RawVolume *v = sceneGraph.resolveVolume(*node);
			auto fn = [&image, &part, palette, i, mins, dim, idx1, idx2](int x, int y, int z,
																		 const voxel::Voxel &voxel) {
				const core::RGBA &color = palette.color(voxel.getColor());
				if (color.a == 0) {
					return;
				}
				const glm::ivec3 pos(x, y, z);
				const int px = part.tex[i].x + (pos[idx1] - mins[idx1]);
				const int py = part.tex[i].y + dim[idx2] - (pos[idx2] - mins[idx2]);
				if (px < 0 || px >= image->width() || py < 0 || py >= image->height()) {
					Log::error("Pixel (%i, %i) is out of bounds for image size %ix%i", px, py, image->width(),
							   image->height());
					return;
				}
				image->setColor(color, px, py);
			};
			const voxelutil::VisitorOrder visitorOrder = visitorOrderForFace(faceName);
			voxelutil::visitFace(*v, node->region(), faceName, fn, visitorOrder, false);
		}
	}
	return image->writePNG(*stream);
}

} // namespace voxelformat
