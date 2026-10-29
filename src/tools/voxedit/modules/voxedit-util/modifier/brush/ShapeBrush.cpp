/**
 * @file
 */

#include "ShapeBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "core/Log.h"
#include "core/Var.h"
#include "palette/Palette.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Face.h"

namespace voxedit {

void ShapeBrush::construct() {
	Super::construct();
	const core::VarDef voxEditShapeFaceCursor(cfg::VoxEditShapeFaceCursor, true, N_("Face cursor for single voxel"),
											  N_("Use the red face cursor instead of a full cube preview when the shape brush radius is 0"));
	core::Var::registerVar(voxEditShapeFaceCursor);
	for (int type = ShapeType::Min; type < ShapeType::Max; ++type) {
		const core::String &cmd = core::String::format("shape%s", ShapeTypeCmdStr[type]);
		const core::String &help = core::String::format(_("Change the shape type to %s"), _(ShapeTypeStr[type]));
		command::Command::registerCommand(cmd.c_str())
			.setHandler([&, type](const command::CommandArgs &args) {
				setShapeType((ShapeType)type);
			}).setHelp(help);
	}
}

ShapeType ShapeBrush::shapeType() const {
	return anyStrokeMode() && radius() == 0 ? ShapeType::Box : _shapeType;
}

void ShapeBrush::setShapeType(ShapeType type) {
	_shapeType = type;
	markDirty();
}

bool ShapeBrush::preferFaceCursorPreview() const {
	if (!anyStrokeMode() || radius() != 0) {
		return false;
	}
	if (const core::VarPtr &var = core::findVar(cfg::VoxEditShapeFaceCursor)) {
		return var->boolVal();
	}
	return true;
}

void ShapeBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);
	const bool faceCursorPreview = preferFaceCursorPreview();
	if (faceCursorPreview != _lastFaceCursorPreview) {
		_lastFaceCursorPreview = faceCursorPreview;
		markDirty();
	}
	if (_lastCursorFace != ctx.cursorFace) {
		_lastCursorFace = ctx.cursorFace;
		if (anyStrokeMode() && radius() > 0) {
			markDirty();
		}
	}
}

ShapeSettings ShapeBrush::settings() const {
	return {shapeType(), _thickness};
}

void ShapeBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region) {
	ShapeParameters parameters;
	parameters.region = region;
	parameters.face = anyStrokeMode() ? ctx.cursorFace : _boxGesture.face();
	parameters.voxel = ctx.cursorVoxel;
	evaluateShape(settings(), parameters, wrapper);
}

int ShapeBrush::thickness() const {
	return _thickness;
}

void ShapeBrush::setThickness(int thickness) {
	_thickness = core_max(1, thickness);
	markDirty();
}

void ShapeBrush::reset() {
	Super::reset();
	_shapeType = ShapeType::Box;
	_thickness = 1;
}

} // namespace voxedit
