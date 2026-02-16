/**
 * @file
 */

#include "TextureBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "image/Image.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/ImportFace.h"

namespace voxedit {

bool TextureBrush::createImageFromFace(const voxel::RawVolume *volume, const palette::Palette &palette,
									   const voxel::Region &region, voxel::FaceNames face) {
	if (volume == nullptr) {
		Log::error("No volume given for createImageFromFace");
		return false;
	}
	if (face == voxel::FaceNames::Max) {
		Log::error("No valid face given for createImageFromFace");
		return false;
	}
	const image::ImagePtr &image = voxelutil::renderFaceToImage(volume, palette, region, face);
	if (!image || !image->isLoaded()) {
		Log::error("Failed to create image from face");
		return false;
	}
	setImage(image);
	_uv0 = glm::vec2(0.0f);
	_uv1 = glm::vec2(1.0f);
	Log::info("Created texture from face with dimensions %ix%i", image->width(), image->height());
	return true;
}

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

void TextureBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
							const voxel::Region &region) {
	if (!_image || !_image->isLoaded()) {
		Log::error("Can't perform action: No image loaded for texture brush");
		return;
	}

	voxelutil::applyTextureToFace(wrapper, region, wrapper.node().palette(), _aabbFace, _image, _uv0, _uv1,
								  _projectOntoSurface);
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

	command::Command::registerCommand("texturebrushmirroru")
		.setHandler([&](const command::CommandArgs &args) {
			core::exchange(_uv0.x, _uv1.x);
		}).setHelp(_("Mirror the uv coordinates along the u axis"));

	command::Command::registerCommand("texturebrushmirrorv")
		.setHandler([&](const command::CommandArgs &args) {
			core::exchange(_uv0.y, _uv1.y);
		}).setHelp(_("Mirror the uv coordinates along the v axis"));

	command::Command::registerCommand("texturebrushresetuv")
		.setHandler([&](const command::CommandArgs &args) {
			_uv0 = glm::vec2(0.0f);
			_uv1 = glm::vec2(1.0f);
		}).setHelp(_("Reset the uv coordinates"));

	command::Command::registerCommand("texturebrushfromface")
		.setHandler([this](const command::CommandArgs &args) {
			const int activeNode = _sceneMgr->sceneGraph().activeNode();
			const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNode);
			if (node == nullptr) {
				Log::warn("No active model node available");
				return;
			}
			const voxel::FaceNames face = _sceneMgr->modifier().brushContext().cursorFace;
			if (face == voxel::FaceNames::Max) {
				Log::warn("No valid face selected for texture capture");
				return;
			}
			const voxel::RawVolume *volume = node->volume();
			const palette::Palette &palette = node->palette();
			const voxel::Region &region = volume->region();
			createImageFromFace(volume, palette, region, face);
		}).setHelp(_("Create a texture from the active model's face colors"));
}

void TextureBrush::shutdown() {
	Super::shutdown();
	command::Command::unregisterCommand("texturebrushmirroru");
	command::Command::unregisterCommand("texturebrushmirrorv");
	command::Command::unregisterCommand("texturebrushresetuv");
	command::Command::unregisterCommand("texturebrushfromface");
}

} // namespace voxedit
