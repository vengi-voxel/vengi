/**
 * @file
 */

#include "StampBrush.h"
#include "app/App.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "core/Log.h"
#include "io/FilesystemArchive.h"
#include "io/FormatDescription.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/Clipboard.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/SelectionManager.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeResizer.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#include <glm/vector_relational.hpp>

namespace voxedit {

void StampBrush::construct() {
	Super::construct();
	command::Command::registerCommand("togglestampbrushcenter", [this](const command::CmdArgs &args) {
		_center ^= true;
	}).setHelp(_("Toggle center at cursor"));

	command::Command::registerCommand("togglestampbrushcontinuous", [this](const command::CmdArgs &args) {
		_continuous ^= true;
	}).setHelp(_("Toggle continuously placing the stamp voxels"));

	command::Command::registerCommand("stampbrushrotate", [this](const command::CmdArgs &args) {
		if (args.size() < 1) {
			Log::info("Usage: stampbrushrotate <x|y|z>");
			return;
		}
		const math::Axis axis = math::toAxis(args[0]);
		_volume = voxelutil::rotateAxis(_volume, axis);
		_volume->translate(-_volume->region().getLowerCorner());
		markDirty();
	}).setHelp(_("Rotate stamp volume around the given axis by 90 degrees"));

	command::Command::registerCommand("stampbrushuseselection", [this](const command::CmdArgs &) {
		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		if (!node) {
			return;
		}
		if (!node->hasSelection()) {
			Log::warn("There's no selection to use as stamp");
			return;
		}
		Modifier &modifier = _sceneMgr->modifier();
		const SelectionManagerPtr &selectionMgr = modifier.selectionMgr();
		voxel::RawVolume stampVolume(node->volume(), selectionMgr->calculateRegion(*node));
		setVolume(stampVolume, node->palette());
		// we unselect here as it's not obvious for the user that the stamp also only operates in the selection
		// this can sometimes lead to confusion if you e.g. created a stamp from a fully filled selected area
		command::executeCommands("select none");
	}).setHelp(_("Use the current selection as new stamp"));

	command::Command::registerCommand("stampbrushusenode", [this](const command::CmdArgs &) {
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNode);
		if (node == nullptr) {
			Log::warn("No active model node to use as stamp");
			return;
		}
		setVolume(*node->volume(), node->palette());
	}).setHelp(_("Use the current selected node volume as new stamp"));

	command::Command::registerCommand("stampbrushpaste", [this](const command::CmdArgs &) {
		const voxel::ClipboardData &clipBoard = _sceneMgr->clipboardData();
		if (clipBoard) {
			setVolume(*clipBoard.volume, *clipBoard.palette);
		}
	}).setHelp(_("Paste the current clipboard content as stamp"));

	_maxVolumeSize = core::Var::getSafe(cfg::VoxEditMaxSuggestedVolumeSizePreview);
}

voxel::Region StampBrush::calcRegion(const BrushContext &ctx) const {
	if (_volume == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	glm::ivec3 mins = ctx.cursorPosition + _offset;
	const glm::ivec3 &dim = _volume->region().getDimensionsInVoxels();
	if (_center) {
		mins -= dim / 2;
	} else {
		switch (ctx.cursorFace) {
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

void StampBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region) {
	const glm::ivec3 &offset = region.isValid() ? region.getLowerCorner() : ctx.cursorPosition + _offset;
	if (!_volume) {
		wrapper.setVoxel(offset.x, offset.y, offset.z, ctx.cursorVoxel);
		return;
	}

	// TODO: BRUSH: context.lockedAxis support
	auto func = [&](int x, int y, int z, const voxel::Voxel &v) {
		wrapper.setVoxel(offset.x + x, offset.y + y, offset.z + z, v);
	};
	voxelutil::visitVolumeParallel(*_volume, func);
}

void StampBrush::reset() {
	Super::reset();
	_center = true;
	_continuous = false;
	_volume = nullptr;
	_offset = glm::ivec3(0);
}

void StampBrush::setSize(const glm::ivec3 &size) {
	if (glm::any(glm::greaterThan(size, glm::ivec3(_maxVolumeSize->intVal())))) {
		return;
	}
	if (_volume) {
		_volume = voxelutil::resize(_volume, voxel::Region(glm::ivec3(0), size - 1));
		_volume->translate(-_volume->region().getLowerCorner());
		markDirty();
	}
}

void StampBrush::setVolume(const voxel::RawVolume &volume, const palette::Palette &palette) {
	_volume = voxelutil::cropVolume(&volume);
	if (!_volume) {
		_volume = new voxel::RawVolume(volume);
	}
	if (_volume) {
		const int maxSize = _maxVolumeSize->intVal();
		const voxel::Region region = _volume->region();
		if (region.voxels() > maxSize * maxSize * maxSize) {
			Log::warn("Stamp size exceeds the max allowed size of %ix%ix%i (check cvar %s)", maxSize, maxSize, maxSize,
					  cfg::VoxEditMaxSuggestedVolumeSizePreview);
			const glm::ivec3 &mins = region.getLowerCorner();
			const voxel::Region resize(mins, mins + maxSize - 1);
			_volume = new voxel::RawVolume(*_volume, resize);
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
	if (!_volume) {
		setErrorReason(_("No stamp volume set"));
	}
}

void StampBrush::setVoxel(const voxel::Voxel &voxel, const palette::Palette &palette) {
	if (_volume) {
		auto func = [&](int x, int y, int z, const voxel::Voxel &v) { _volume->setVoxel(x, y, z, voxel); };
		voxelutil::visitVolumeParallel(*_volume, func);
	} else {
		_volume = new voxel::RawVolume(voxel::Region(glm::ivec3(0), glm::ivec3(0)));
		_volume->setVoxel(0, 0, 0, voxel);
		_palette = palette;
	}
	markDirty();
}

void StampBrush::convertToPalette(const palette::Palette &palette) {
	const voxel::Region &dirtyRegion = voxelutil::remapToPalette(_volume, _palette, palette);
	if (dirtyRegion.isValid()) {
		_palette = palette;
		markDirty();
	}
}

bool StampBrush::load(const core::String &filename) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(io::filesystem());
	scenegraph::SceneGraph newSceneGraph;
	voxelformat::LoadContext loadCtx;
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	if (!voxelformat::loadFormat(fileDesc, archive, newSceneGraph, loadCtx)) {
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
