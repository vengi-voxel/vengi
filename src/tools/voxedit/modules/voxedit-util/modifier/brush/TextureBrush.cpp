/**
 * @file
 */

#include "TextureBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "image/Image.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/ImportFace.h"

namespace voxedit {

bool TextureBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						   const BrushContext &ctx) {
	if (!_image || !_image->isLoaded()) {
		setErrorReason(_("No image loaded for texture brush"));
		return false;
	}
	return Super::execute(sceneGraph, wrapper, ctx);
}

bool TextureBrush::needsAdditionalAction(const BrushContext &ctx) const {
	if (!_projectOntoSurface) {
		return false;
	}
	return Super::needsAdditionalAction(ctx);
}

void TextureBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
							const BrushContext &ctx, const voxel::Region &region) {
	if (!_image || !_image->isLoaded()) {
		Log::error("Can't perform action: No image loaded for texture brush");
		return;
	}

	voxelutil::applyTextureToFace(wrapper, region, wrapper.node().palette(), _aabbFace, _image, _uv0, _uv1, _projectOntoSurface);
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
