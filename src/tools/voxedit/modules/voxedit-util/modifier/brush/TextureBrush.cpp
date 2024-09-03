/**
 * @file
 */

#include "TextureBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "image/Image.h"
#include "palette/Palette.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Face.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

// we have to flip some positions to project the texture correctly
static glm::ivec3 getUVPosForFace(int x, int y, int z, const voxel::Region &region, voxel::FaceNames face) {
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	if (face != voxel::FaceNames::Up && face != voxel::FaceNames::Down) {
		y = mins.y + dim.y - (y - mins.y);
	}
	switch (face) {
	case voxel::FaceNames::Down:
	case voxel::FaceNames::Front:
		return glm::ivec3(mins.x + dim.x - (x - mins.x), y, z);
	case voxel::FaceNames::Right:
		return glm::ivec3(x, y, mins.z + dim.z - (z - mins.z));
	case voxel::FaceNames::Up:
		return glm::ivec3(mins.x + dim.x - (x - mins.x), y, mins.z + dim.z - (z - mins.z));
	default:
		break;
	}
	return glm::ivec3(x, y, z);
}

bool TextureBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						   const BrushContext &context) {
	if (!_image || !_image->isLoaded()) {
		setErrorReason(_("No image loaded for texture brush"));
		return false;
	}
	return Super::execute(sceneGraph, wrapper, context);
}

bool TextureBrush::needsAdditionalAction(const BrushContext &context) const {
	if (!_projectOntoSurface) {
		return false;
	}
	return Super::needsAdditionalAction(context);
}

void TextureBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
							const BrushContext &context, const voxel::Region &region) {
	if (!_image || !_image->isLoaded()) {
		Log::error("Can't perform action: No image loaded for texture brush");
		return;
	}

	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::vec3 &size = region.getDimensionsInVoxels();
	const math::Axis axis = faceToAxis(_aabbFace);
	const int axisIdx0 = math::getIndexForAxis(axis);
	const int axisIdx1 = axis == math::Axis::Y ? (axisIdx0 + 2) % 3 : (axisIdx0 + 1) % 3;
	const int axisIdx2 = axis == math::Axis::Y ? (axisIdx0 + 1) % 3 : (axisIdx0 + 2) % 3;
	const int axisIdxUV1 = (axisIdx1 + 0) % 2;
	const int axisIdxUV2 = (axisIdx1 + 1) % 2;
	const palette::Palette &palette = wrapper.node().palette();

	auto visitor = [&](int x, int y, int z, const voxel::Voxel &voxel) {
		const glm::ivec3 uvPos = getUVPosForFace(x, y, z, region, _aabbFace);
		const float axis1Factor = ((float)(uvPos[axisIdx1] - mins[axisIdx1]) + 0.5f) / size[axisIdx1];
		const float axis2Factor = ((float)(uvPos[axisIdx2] - mins[axisIdx2]) + 0.5f) / size[axisIdx2];
		glm::vec2 uv;
		uv[axisIdxUV1] = glm::mix(_uv0[axisIdxUV1], _uv1[axisIdxUV1], axis1Factor);
		uv[axisIdxUV2] = glm::mix(_uv0[axisIdxUV2], _uv1[axisIdxUV2], axis2Factor);
		const core::RGBA color = _image->colorAt(uv, image::TextureWrap::Repeat, image::TextureWrap::Repeat, true);
		if (color.a == 0) {
			return;
		}
		int palIdx = palette.getClosestMatch(color);
		if (palIdx == palette::PaletteColorNotFound) {
			palIdx = 0;
		}
		const glm::ivec3 pos(x, y, z);
		wrapper.setVoxel(pos.x, pos.y, pos.z, voxel::createVoxel(palette, palIdx));
	};

	const int cnt = voxelutil::visitFace(wrapper, region, _aabbFace, visitor, _projectOntoSurface);
	Log::debug("Visited %i voxels for face %s", cnt, voxel::faceNameString(_aabbFace));
}

void TextureBrush::setImage(const image::ImagePtr &texture) {
	_image = texture;
	markDirty();
}

const image::ImagePtr &TextureBrush::image() const {
	return _image;
}

void TextureBrush::setUV0(const glm::vec2 &uv0) {
	_uv0 = uv0;
	markDirty();
}

const glm::vec2 &TextureBrush::uv0() const {
	return _uv0;
}

void TextureBrush::setUV1(const glm::vec2 &uv1) {
	_uv1 = uv1;
	markDirty();
}

const glm::vec2 &TextureBrush::uv1() const {
	return _uv1;
}

void TextureBrush::setProjectOntoSurface(bool projectOntoSurface) {
	_projectOntoSurface = projectOntoSurface;
	markDirty();
}

bool TextureBrush::projectOntoSurface() const {
	return _projectOntoSurface;
}

void TextureBrush::construct() {
	Super::construct();

	command::Command::registerCommand("texturebrushmirroru", [&](const command::CmdArgs &args) {
		core::exchange(_uv0.x, _uv1.x);
	}).setHelp(_("Mirror the uv coordinates along the u axis"));

	command::Command::registerCommand("texturebrushmirrorv", [&](const command::CmdArgs &args) {
		core::exchange(_uv0.y, _uv1.y);
	}).setHelp(_("Mirror the uv coordinates along the v axis"));

	command::Command::registerCommand("texturebrushresetuv", [&](const command::CmdArgs &args) {
		_uv0 = glm::vec2(0.0f);
		_uv1 = glm::vec2(1.0f);
	}).setHelp(_("Reset the uv coordinates"));
}

void TextureBrush::shutdown() {
	Super::shutdown();
	command::Command::unregisterCommand("texturebrushmirroru");
	command::Command::unregisterCommand("texturebrushmirrorv");
	command::Command::unregisterCommand("texturebrushresetuv");
}

} // namespace voxedit
