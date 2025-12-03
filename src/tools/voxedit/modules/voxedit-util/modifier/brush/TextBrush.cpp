/**
 * @file
 */

#include "TextBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "core/Log.h"
#include "core/Unicode.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include <glm/vector_relational.hpp>

namespace voxedit {

void TextBrush::construct() {
	Super::construct();
	command::Command::registerCommand("textbrushaxis", [this](const command::CmdArgs &args) {
		if (args.size() < 1) {
			Log::info("Usage: textbrushaxis <x|y|z>");
			return;
		}
		_axis = math::toAxis(args[0]);
		markDirty();
	}).setHelp(_("Change the text brush axis"));
}

voxel::Region TextBrush::calcRegion(const BrushContext &ctx) const {
	if (!_voxelFont.init(_font)) {
		Log::error("Failed to initialize voxel font with %s", _font.c_str());
		return voxel::Region::InvalidRegion;
	}
	const int l = core::unicode::length(_input.c_str());
	int dimX = 0;
	int dimY = 0;
	_voxelFont.dimensions(_input.c_str(), _size, dimX, dimY);
	const int w = core_max(0, (l - 1)) * _spacing + dimX;
	const int h = dimY;
	const int d = _thickness;
	const glm::ivec3 &mins = ctx.cursorPosition;
	glm::ivec3 maxs = mins;
	const int widthIndex = math::getIndexForAxis(_axis);
	maxs[(widthIndex + 0) % 3] += (w - 1);
	maxs[(widthIndex + 1) % 3] += (h - 1);
	maxs[(widthIndex + 2) % 3] += (d - 1);
	return voxel::Region(mins, maxs);
}

void TextBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						 const voxel::Region &region) {
	if (!_voxelFont.init(_font)) {
		Log::error("Failed to initialize voxel font with %s", _font.c_str());
		return;
	}
	const char *t = _input.c_str();
	const char **s = &t;
	glm::ivec3 pos = region.getLowerCorner();
	const int widthIndex = math::getIndexForAxis(_axis);

	for (int c = core::unicode::next(s); c != -1; c = core::unicode::next(s)) {
		pos[widthIndex] += _voxelFont.renderCharacter(c, _size, _thickness, pos, wrapper, ctx.cursorVoxel, _axis);
		pos[widthIndex] += _spacing;
	}
}

void TextBrush::setSize(int size) {
	_size = glm::clamp(size, 6, 255);
	markDirty();
}

void TextBrush::setThickness(int thickness) {
	_thickness = glm::clamp(thickness, 1, 255);
	markDirty();
}

void TextBrush::reset() {
	Super::reset();
	_font = "font.ttf";
	_input = "text";
	_size = 16;
	_spacing = 1;
	_thickness = 1;
}

void TextBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);
	if (ctx.cursorPosition != _lastCursorPosition) {
		_lastCursorPosition = ctx.cursorPosition;
		markDirty();
	}
}

void TextBrush::shutdown() {
	_voxelFont.shutdown();
}

} // namespace voxedit
