/**
 * @file
 */

#include "SkinFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "image/Image.h"
#include "math/Rect.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/trigonometric.hpp>

namespace voxelformat {

using UV = math::Rect<int>;
struct Part {
	UV rects[6]; // top, bottom, right, front, left, back

	constexpr const UV &operator[](size_t idx) const {
		return rects[idx];
	}
};

// Defines a single 3D part from its 6 faces (x, y, z sizes and face coordinates)
struct SkinBox {
	const char *name;
	glm::ivec3 size;		  // width, height, depth in voxels
	glm::vec3 translation;	  // World position (offset) of the part
	glm::vec3 rotationDegree; // Rotation in degrees around the pivot point
	glm::vec3 pivot;
	Part part;
	bool extension;
};

// |         |  Top    |  Bottom |         |
// |  Right  |  Front  |  Left   | Back    |

// |  HEAD           |  HAT            |
// | LEG_R   | BODY        | ARM_R     |
// | LEG_R_E | BODY_E      | ARM_R_E   |
// | LEG_L_E | LEG_L | ARM_L | ARM_L_E |

static constexpr Part shiftPart(const Part &part, int offsetX, int offsetY) {
	Part shiftedPart;
	for (int i = 0; i < lengthof(Part::rects); ++i) {
		shiftedPart.rects[i] = part[i].offset(offsetX, offsetY);
	}
	return shiftedPart;
}

// slim
// |    | 3 | 3 |    |
// | 4  | 3 | 4  | 3 |

// standard
// |    |  4  |  4  |    |
// | 4  |  4  |  4  | 4  |

static constexpr Part slimPart(const Part &part) {
	Part p = part;
	p.rects[0] = UV(part[0].getMinX() - 0, part[0].getMinZ(), part[0].getMaxX() - 1, part[0].getMaxZ());
	p.rects[1] = UV(part[1].getMinX() - 1, part[1].getMinZ(), part[1].getMaxX() - 2, part[1].getMaxZ());
	// 2 is unchanged
	p.rects[3] = UV(part[3].getMinX() - 0, part[3].getMinZ(), part[3].getMaxX() - 1, part[3].getMaxZ());
	p.rects[4] = UV(part[4].getMinX() - 1, part[4].getMinZ(), part[4].getMaxX() - 1, part[4].getMaxZ());
	p.rects[5] = UV(part[5].getMinX() - 1, part[5].getMinZ(), part[5].getMaxX() - 2, part[5].getMaxZ());
	return p;
}

static constexpr Part head = {{
	{8, 0, 16, 8},	 // top
	{16, 0, 24, 8},	 // bottom
	{0, 8, 8, 16},	 // right
	{8, 8, 16, 16},	 // front
	{16, 8, 24, 16}, // left
	{24, 8, 32, 16}	 // back
}};

static constexpr Part hat(shiftPart(head, 32, 0));
static constexpr Part leg_right = {{
	{4, 16, 8, 20},	 // top
	{8, 16, 12, 20}, // bottom
	{0, 20, 4, 32},	 // right
	{4, 20, 8, 32},	 // front
	{8, 20, 12, 32}, // left
	{12, 20, 16, 32} // back
}};

static constexpr Part body = {{
	{20, 16, 28, 20}, // top
	{28, 16, 36, 20}, // bottom
	{16, 20, 20, 32}, // right
	{20, 20, 28, 32}, // front
	{28, 20, 32, 32}, // left
	{32, 20, 40, 32}  // back
}};

static constexpr Part arm_right(shiftPart(leg_right, 40, 0));
static constexpr Part arm_left(shiftPart(leg_right, 32, 32));
static constexpr Part leg_left(shiftPart(leg_right, 16, 32));

static constexpr Part body_ex(shiftPart(body, 0, 16));
static constexpr Part arm_right_ex(shiftPart(arm_right, 0, 16));
static constexpr Part leg_right_ex(shiftPart(leg_right, 0, 16));
static constexpr Part arm_left_ex(shiftPart(arm_left, 16, 0));
static constexpr Part leg_left_ex(shiftPart(leg_left, -16, 0));

static constexpr Part arm_slim_right(slimPart(arm_right));
static constexpr Part arm_slim_left(slimPart(arm_left));
static constexpr Part arm_slim_right_ex(slimPart(arm_right_ex));
static constexpr Part arm_slim_left_ex(slimPart(arm_left_ex));

// Define the skin boxes and use names that animate.lua can work with
static constexpr SkinBox skinBoxes[] = {
	{"head", {8, 8, 8}, {0.0f, 24.0f, 0.0f}, {0, 0, 0}, {0.0f, 0.0f, 0.0f}, head, false},
	{"hat", {8, 8, 8}, {0.0f, 24.0f, 0.0f}, {0, 0, 0}, {0.0f, 0.0f, 0.0f}, hat, true},
	{"body", {8, 12, 4}, {4.0f, 12.0f, 4.0f}, {0, 0, 0}, {0.5f, 0.0f, 0.5f}, body, false},
	{"shoulder_r", {4, 12, 4}, {8.0f, 21.6f, 4.0f}, {45, 0, 0}, {0.0f, 0.8f, 0.5f}, arm_right, false},
	{"shoulder_l", {4, 12, 4}, {0.0f, 21.6f, 4.0f}, {-45, 0, 0}, {1.0f, 0.8f, 0.5f}, arm_left, false},
	{"leg_r", {4, 12, 4}, {6.0f, 12.0f, 4.0f}, {-45, 0, 0}, {0.5f, 1.0f, 0.5f}, leg_right, false},
	{"leg_l", {4, 12, 4}, {2.0f, 12.0f, 4.0f}, {45, 0, 0}, {0.5f, 1.0f, 0.5f}, leg_left, false},
	{"body_ex", {8, 12, 4}, {4.0f, 12.0f, 4.0f}, {0, 0, 0}, {0.5f, 0.0f, 0.5f}, body_ex, true},
	{"shoulder_r_ex", {4, 12, 4}, {8.0f, 21.6f, 4.0f}, {45, 0, 0}, {0.0f, 0.8f, 0.5f}, arm_right_ex, true},
	{"shoulder_l_ex", {4, 12, 4}, {0.0f, 21.6f, 4.0f}, {-45, 0, 0}, {1.0f, 0.8f, 0.5f}, arm_left_ex, true},
	{"leg_r_ex", {4, 12, 4}, {6.0f, 12.0f, 4.0f}, {-45, 0, 0}, {0.5f, 1.0f, 0.5f}, leg_right_ex, true},
	{"leg_l_ex", {4, 12, 4}, {2.0f, 12.0f, 4.0f}, {45, 0, 0}, {0.5f, 1.0f, 0.5f}, leg_left_ex, true}
};

static constexpr SkinBox skinBoxesSlim[] = {
	skinBoxes[0],
	skinBoxes[1],
	skinBoxes[2],
	{"shoulder_r", {3, 12, 4}, {8.0f, 21.6f, 4.0f}, {45, 0, 0}, {0.0f, 0.8f, 0.5f}, arm_slim_right, false},
	{"shoulder_l", {3, 12, 4}, {0.0f, 21.6f, 4.0f}, {-45, 0, 0}, {1.0f, 0.8f, 0.5f}, arm_slim_left, false},
	skinBoxes[5],
	skinBoxes[6],
	skinBoxes[7],
	{"shoulder_r_ex", {3, 12, 4}, {8.0f, 21.6f, 4.0f}, {45, 0, 0}, {0.0f, 0.8f, 0.5f}, arm_slim_right_ex, true},
	{"shoulder_l_ex", {3, 12, 4}, {0.0f, 21.6f, 4.0f}, {-45, 0, 0}, {1.0f, 0.8f, 0.5f}, arm_slim_left_ex, true},
	skinBoxes[10],
	skinBoxes[11]
};

static const voxel::FaceNames order[] = {voxel::FaceNames::Top,	  voxel::FaceNames::Bottom, voxel::FaceNames::Right,
										 voxel::FaceNames::Front, voxel::FaceNames::Left,	voxel::FaceNames::Back};

static void addNode(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, int parentId,
					bool applyTransform, voxel::FaceNames faceNameOffset, const SkinBox &skinBox) {
	scenegraph::SceneGraphTransform transform;
	glm::vec3 translation = skinBox.translation;
	if (faceNameOffset != voxel::FaceNames::Max) {
		// offset the translation by the face name offset to prevent z-fighting
		const bool isX = voxel::isX(faceNameOffset);
		const bool isY = voxel::isY(faceNameOffset);
		const bool isZ = voxel::isZ(faceNameOffset);
		const bool isNegative = voxel::isNegativeFace(faceNameOffset);
		float offset = applyTransform ? 0.1f : 0.02f;
		if (skinBox.extension) {
			offset = 0.5f;
		}
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
		const glm::quat orientation(glm::radians(skinBox.rotationDegree));
		transform.setLocalOrientation(orientation);
		node.setPivot(skinBox.pivot);
	} else {
		const glm::vec3 regionSize(node.region().getDimensionsInVoxels());
		transform.setLocalTranslation(translation - skinBox.pivot * regionSize);
	}
	node.setTransform(0, transform);
	sceneGraph.emplace(core::move(node), parentId);
}

// check the image for importing the skin
static bool isSlim(const image::ImagePtr &image) {
	color::RGBA pixel = image->colorAt(54, 20);
	return pixel.a == 0;
}

// check the volume region for exporting the skin
static bool isSlim(const SkinBox &skinBox, const voxel::RawVolume *v) {
	const core::String name = skinBox.name;
	if (name == "shoulder_r" || name == "shoulder_l") {
		if (v->region().getWidthInVoxels() == 3) {
			return true; // slim shoulder
		}
	}
	return false;
}

// we have special needs for the visitor order here - to be independent from other use-cases for the
// face visitor, we define our own order here
static voxelutil::VisitorOrder visitorOrderForFace(voxel::FaceNames face) {
	voxelutil::VisitorOrder visitorOrder;
	switch (face) {
	case voxel::FaceNames::Top:
		visitorOrder = voxelutil::VisitorOrder::mZmXmY;
		break;
	case voxel::FaceNames::Bottom:
		visitorOrder = voxelutil::VisitorOrder::mZmXY;
		break;
	case voxel::FaceNames::Right:
		visitorOrder = voxelutil::VisitorOrder::mYmZmX;
		break;
	case voxel::FaceNames::Front:
		visitorOrder = voxelutil::VisitorOrder::mYmXZ;
		break;
	case voxel::FaceNames::Left:
		visitorOrder = voxelutil::VisitorOrder::mYZX;
		break;
	case voxel::FaceNames::Back:
		visitorOrder = voxelutil::VisitorOrder::mYXmZ;
		break;
	default:
		return voxelutil::VisitorOrder::Max;
	}
	return visitorOrder;
}

template<class FUNC>
static void visitSkinFace(const voxel::RawVolume *v, const image::ImagePtr &image, const SkinBox &box, int faceIndex,
						  voxel::FaceNames faceName, FUNC &&func) {
	const voxelutil::VisitorOrder visitorOrder = visitorOrderForFace(faceName);
	const UV &rect = box.part[faceIndex];
	int pixelIndex = 0;
	auto visitor = [&](int x, int y, int z, const voxel::Voxel &voxel) {
		const int px = rect.getMinX() + (pixelIndex % rect.width());
		const int py = rect.getMinZ() + (pixelIndex / rect.width());
		++pixelIndex;
		if (px < 0 || px >= image->width() || py < 0 || py >= image->height()) {
			Log::error("Pixel (%i, %i) is out of bounds for image size %ix%i (%s:%i at %i:%i:%i) %s", px, py,
					   image->width(), image->height(), box.name, faceIndex, x, y, z, v->region().toString().c_str());
			return;
		}
		func(x, y, z, voxel, image, px, py);
	};
	voxelutil::visitFace(*v, faceName, visitor, visitorOrder, false);
	if (pixelIndex != rect.width() * rect.height()) {
		Log::error("Pixel index %i does not match expected size %i for face %s in box %s (%s)", pixelIndex,
				   rect.width() * rect.height(), voxel::faceNameString(faceName), box.name,
				   voxelutil::VisitorOrderStr[(int)visitorOrder]);
	}
};

static void importPart(const image::ImagePtr &image, const SkinBox &box, int faceIndex, voxel::FaceNames faceName,
					   scenegraph::SceneGraphNode &node) {
	const palette::Palette &palette = node.palette();
	auto readFromImage = [palette, v = node.volume()](int x, int y, int z, const voxel::Voxel &voxel,
													  const image::ImagePtr &img, int px, int py) {
		const color::RGBA color = img->colorAt(px, py);
		if (color.a == 0) {
			return;
		}
		int palIdx = palette.getClosestMatch(color);
		v->setVoxel(x, y, z, voxel::createVoxel(palette, palIdx));
	};

	visitSkinFace(node.volume(), image, box, faceIndex, faceName, readFromImage);
}

size_t SkinFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return 0;
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

	for (int y = 0; y < image->height(); ++y) {
		for (int x = 0; x < image->width(); ++x) {
			const color::RGBA rgba = image->colorAt(x, y);
			if (rgba.a == 0) {
				continue; // skip transparent pixels
			}
			palette.tryAdd(rgba);
		}
	}
	return palette.size();
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

	const bool applyTransform = core::getVar(cfg::VoxformatSkinApplyTransform)->boolVal();
	const bool addGroup = core::getVar(cfg::VoxformatSkinAddGroups)->boolVal();
	const bool mergeFaces = core::getVar(cfg::VoxformatSkinMergeFaces)->boolVal();

	const SkinBox *boxes = skinBoxes;
	int nBoxes = lengthof(skinBoxes);
	if (isSlim(image)) {
		Log::debug("Detected slim skin format");
		boxes = skinBoxesSlim;
		nBoxes = lengthof(skinBoxesSlim);
	} else {
		Log::debug("Detected classic skin format");
	}

	for (int i = 0; i < nBoxes; ++i) {
		const SkinBox &skinBox = boxes[i];
		const glm::ivec3 size = skinBox.size;
		const voxel::Region region(0, 0, 0, size.x - 1, size.y - 1, size.z - 1);

		int parentId = 0;
		if (addGroup) {
			scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
			groupNode.setName(core::String::format("Group %s", skinBox.name));
			groupNode.setPalette(palette);
			parentId = sceneGraph.emplace(core::move(groupNode));
		}
		if (mergeFaces) {
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setVolume(new voxel::RawVolume(region), true);
			node.setName(skinBox.name);
			node.setPalette(palette);
			for (int faceIndex = 0; faceIndex < lengthof(order); ++faceIndex) {
				importPart(image, skinBox, faceIndex, order[faceIndex], node);
			}
			addNode(sceneGraph, node, parentId, applyTransform, voxel::FaceNames::Max, skinBox);
		} else {
			// TODO: VOXELFORMAT: it would improve the editing experience if we only make the volume region 1 voxel thick
			for (int faceIndex = 0; faceIndex < lengthof(order); ++faceIndex) {
				const voxel::FaceNames faceName = order[faceIndex];
				scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
				node.setVolume(new voxel::RawVolume(region), true);
				node.setName(core::String::format("%s_%s", skinBox.name, voxel::faceNameString(faceName)));
				node.setPalette(palette);

				importPart(image, skinBox, faceIndex, faceName, node);
				addNode(sceneGraph, node, parentId, applyTransform, faceName, skinBox);
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
			image->setColor(color::RGBA(0, 0, 0, 0), x, y);
		}
	}

	const bool mergedFaces = sceneGraph.findNodeByName(skinBoxes[0].name) != nullptr;
	for (int n = 0; n < lengthof(skinBoxes); ++n) {
		const SkinBox &skinBox = skinBoxes[n];
		for (int faceIndex = 0; faceIndex < lengthof(order); ++faceIndex) {
			const voxel::FaceNames faceName = order[faceIndex];
			const core::String name =
				mergedFaces ? skinBox.name
							: core::String::format("%s_%s", skinBox.name, voxel::faceNameString(faceName));
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
			const voxel::RawVolume *v = sceneGraph.resolveVolume(*node);

			auto writeToImage = [&palette](int x, int y, int z, const voxel::Voxel &voxel, const image::ImagePtr &img,
										   int px, int py) {
				if (voxel::isAir(voxel.getMaterial())) {
					return;
				}
				const color::RGBA &color = palette.color(voxel.getColor());
				if (color.a == 0) {
					return;
				}
				img->setColor(color, px, py);
			};

			const SkinBox *skinBoxPtr = &skinBox;
			if (isSlim(skinBox, v)) {
				// use the slim part for the skin box
				skinBoxPtr = &skinBoxesSlim[n];
			}
			visitSkinFace(v, image, *skinBoxPtr, faceIndex, faceName, writeToImage);
		}
	}
	return image->writePNG(*stream);
}

} // namespace voxelformat
