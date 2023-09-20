/**
 * @file
 */

#include "StampBrush.h"
#include "app/App.h"
#include "command/Command.h"
#include "core/Log.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/FormatDescription.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeResizer.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

void StampBrush::construct() {
	command::Command::registerCommand("togglestampbrushcenter", [this](const command::CmdArgs &args) {
		_center ^= true;
	}).setHelp("Toggle center at cursor");

	command::Command::registerCommand("togglestampbrushcontinuous", [this](const command::CmdArgs &args) {
		_continuous ^= true;
	}).setHelp("Toggle continuously placing the stamp voxels");
}

voxel::Region StampBrush::calcRegion(const BrushContext &context) const {
	glm::ivec3 mins = context.cursorPosition;
	const glm::ivec3 &dim = _volume->region().getDimensionsInVoxels();
	if (_center) {
		mins -= dim / 2;
	} else {
		switch (context.cursorFace) {
		case voxel::FaceNames::NegativeX:
			mins.x -= (dim.x - 1);
			break;
		case voxel::FaceNames::NegativeY:
			mins.y -= (dim.y - 1);
			break;
		case voxel::FaceNames::NegativeZ:
			mins.z -= (dim.z - 1);
			break;
		default:
			break;
		}
	}
	glm::ivec3 maxs = mins + _volume->region().getDimensionsInCells();
	return voxel::Region(mins, maxs);
}

bool StampBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						 const BrushContext &context) {
	if (!_volume) {
		return false;
	}

	const voxel::Region &region = calcRegion(context);
	const glm::ivec3 &offset = region.getLowerCorner();
	voxelutil::visitVolume(*_volume, [&](int x, int y, int z, const voxel::Voxel &v) {
		wrapper.setVoxel(offset.x + x, offset.y + y, offset.z + z, v);
	});

	return true;
}

void StampBrush::reset() {
	Super::reset();
	_center = true;
	_continuous = false;
	_volume = nullptr;
}

void StampBrush::setSize(const glm::ivec3 &size) {
	if (glm::any(glm::greaterThan(size, glm::ivec3(MaxSize)))) {
		return;
	}
	if (_volume) {
		_volume = voxelutil::resize(_volume, voxel::Region(glm::ivec3(0), size - 1));
		_volume->translate(-_volume->region().getLowerCorner());
		markDirty();
	}
}

void StampBrush::setVolume(const voxel::RawVolume &volume, const voxel::Palette &palette) {
	_volume = voxelutil::cropVolume(&volume);
	if (_volume) {
		if (glm::any(glm::greaterThan(_volume->region().getDimensionsInVoxels(), glm::ivec3(MaxSize)))) {
			Log::warn("Stamp size exceeds the max allowed size of 32x32x32");
			_volume = voxelutil::resize(_volume, voxel::Region(0, MaxSize));
		}
		_volume->translate(-_volume->region().getLowerCorner());
	}
	_palette = palette;
	markDirty();
}

void StampBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);
	if (ctx.cursorPosition != _lastCursorPosition) {
		_lastCursorPosition = ctx.cursorPosition;
		markDirty();
	}
}

void StampBrush::setVoxel(const voxel::Voxel &voxel, const voxel::Palette &palette) {
	if (_volume) {
		voxelutil::visitVolume(*_volume,
							   [&](int x, int y, int z, const voxel::Voxel &v) { _volume->setVoxel(x, y, z, voxel); });
	} else {
		_volume = new voxel::RawVolume(voxel::Region(glm::ivec3(0), glm::ivec3(0)));
		_volume->setVoxel(0, 0, 0, voxel);
		_palette = palette;
	}
	markDirty();
}

void StampBrush::convertToPalette(const voxel::Palette &palette) {
	const voxel::Region &dirtyRegion = voxelutil::remapToPalette(_volume, _palette, palette);
	if (dirtyRegion.isValid()) {
		_palette = palette;
		markDirty();
	}
}

bool StampBrush::load(const core::String &filename) {
	const io::FilePtr &filePtr = io::filesystem()->open(filename);
	if (!filePtr->validHandle()) {
		Log::error("Failed to open model file %s", filename.c_str());
		return false;
	}
	scenegraph::SceneGraph newSceneGraph;
	io::FileStream stream(filePtr);
	voxelformat::LoadContext loadCtx;
	io::FileDescription fileDesc;
	fileDesc.set(filePtr->name());
	if (!voxelformat::loadFormat(fileDesc, stream, newSceneGraph, loadCtx)) {
		Log::error("Failed to load %s", filename.c_str());
		return false;
	}
	scenegraph::SceneGraphNode *node = newSceneGraph.firstModelNode();
	if (!node) {
		Log::error("No model node found in %s", filename.c_str());
		return false;
	}
	voxel::RawVolume *v = node->volume();
	if (!v) {
		Log::error("No volume found in %s", filename.c_str());
		return false;
	}
	setVolume(*v, node->palette());
	return true;
}

} // namespace voxedit
