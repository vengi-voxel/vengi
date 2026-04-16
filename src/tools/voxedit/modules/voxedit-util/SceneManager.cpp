/**
 * @file
 */

#include "SceneManager.h"

#include "app/Async.h"
#include "core/ConfigVar.h"
#include "app/I18N.h"
#include "color/ColorUtil.h"
#include "color/Quantize.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/UUID.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/File.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include "io/FormatDescription.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include "math/Axis.h"
#include "math/Ray.h"
#include "memento/MementoHandler.h"
#include "metric/MetricFacade.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"
#include "palette/PaletteCompleter.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "scenegraph/SceneGraphTransform.h"
#include "scenegraph/SceneGraphUtil.h"
#include "scenegraph/SceneUtil.h"
#include "video/Camera.h"
#include "voxedit-util/ModelNodeSettings.h"
#include "voxedit-util/modifier/SceneModifiedFlags.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"
#include "voxedit-util/network/ServerNetwork.h"
#include "voxel/ClipboardData.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelNormalUtil.h"
#include "voxelfont/VoxelFont.h"
#include "voxelformat/Format.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/vengi/VENGIFormat.h"
#include "voxelgenerator/LSystem.h"
#include "voxelgenerator/LUAApi.h"
#include "voxelrender/ImageGenerator.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/RenderUtil.h"
#include "voxelutil/FillHollow.h"
#include "voxelutil/Hollow.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/Picking.h"
#include "voxelutil/Raycast.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeRescaler.h"
#include "voxelutil/VolumeResizer.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeSplitter.h"
#include "voxelutil/VolumeSelect.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VoxelUtil.h"

#include "Config.h"
#include "CommandCompleter.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/transform.hpp>

namespace voxedit {

SceneManager::SceneManager(const core::TimeProviderPtr &timeProvider, const io::FilesystemPtr &filesystem,
						   const SceneRendererPtr &sceneRenderer, const ModifierRendererPtr &modifierRenderer)
	: _timeProvider(timeProvider), _sceneRenderer(sceneRenderer),
	  _modifierFacade(this, modifierRenderer), _luaApi(filesystem),
	  _luaApiListener(this, _mementoHandler, _sceneGraph), _filesystem(filesystem),
	  _server(&_luaApi, this), _client(this), _recorder(this), _player(this) {
	server().setState(&_sceneGraph);
}

SceneManager::~SceneManager() {
	core_assert_msg(_initialized == 0, "SceneManager was not properly shut down");
}

bool SceneManager::loadPalette(const core::String& paletteName, bool searchBestColors, bool save) {
	palette::Palette palette;

	const bool isNodePalette = core::string::startsWith(paletteName, NODE_PALETTE_PREFIX);
	if (isNodePalette) {
		const size_t nodeDetails = paletteName.rfind("##");
		if (nodeDetails != core::String::npos) {
			const int nodeId = core::string::toInt(paletteName.substr(nodeDetails + 2, paletteName.size()));
			if (_sceneGraph.hasNode(nodeId)) {
				palette = _sceneGraph.node(nodeId).palette();
			} else {
				Log::warn("Couldn't find palette for node %i", nodeId);
			}
		}
	}

	if (palette.colorCount() == 0 && !palette.load(paletteName.c_str())) {
		return false;
	}
	if (!setActivePalette(palette, searchBestColors)) {
		return false;
	}
	core::getVar(cfg::VoxEditLastPalette)->setVal(paletteName);

	if (save && !isNodePalette && !palette.isBuiltIn()) {
		const core::String filename = core::string::extractFilename(palette.name());
		const core::String &paletteFilename = core::String::format("palette-%s.png", filename.c_str());
		const core::String &pngFile = _filesystem->homeWritePath(paletteFilename);
		palette.save(pngFile.c_str());
	}

	return true;
}

bool SceneManager::importPalette(const core::String& file, bool setActive, bool searchBestColors) {
	palette::Palette palette;
	if (!voxelformat::importPalette(file, palette)) {
		Log::warn("Failed to import a palette from file '%s'", file.c_str());
		return false;
	}

	const core::String paletteName(core::string::extractFilename(file));
	const core::String &paletteFilename = core::String::format("palette-%s.png", paletteName.c_str());
	const core::String &pngFile = _filesystem->homeWritePath(paletteFilename);
	if (palette.save(pngFile.c_str())) {
		core::getVar(cfg::VoxEditLastPalette)->setVal(paletteName);
	}

	if (!setActive) {
		return true;
	}
	return setActivePalette(palette, searchBestColors);
}

void SceneManager::nodeGroupCalulateNormals(voxel::Connectivity connectivity, bool recalcAll, bool fillAndHollow) {
	nodeForeachGroup([&] (int groupNodeId) {
		nodeCalculateNormals(groupNodeId, connectivity, recalcAll, fillAndHollow);
	});
}

bool SceneManager::nodeCalculateNormals(int nodeId, voxel::Connectivity connectivity, bool recalcAll, bool fillAndHollow) {
	if (scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId)) {
		if (!node->hasNormalPalette()) {
			Log::warn("Node %i has no normal palette", nodeId);
			return false;
		}
		voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(node->volume());
		if (fillAndHollow) {
			voxelutil::fillHollow(wrapper, _modifierFacade.cursorVoxel());
		}
		const palette::NormalPalette &normalPalette = node->normalPalette();
		voxelutil::visitSurfaceVolumeParallel(*node->volume(), [&] (int x, int y, int z, const voxel::Voxel &voxel) {
			if (!recalcAll && voxel.getNormal() != NO_NORMAL) {
				return;
			}
			voxel::RawVolume::Sampler sampler(wrapper.volume());
			sampler.setPosition(x, y, z);
			const glm::vec3 &normal = voxel::calculateNormal(sampler, connectivity);
			int normalPaletteIndex = normalPalette.getClosestMatch(normal);
			if (normalPaletteIndex == palette::PaletteNormalNotFound) {
				normalPaletteIndex = NO_NORMAL;
			} else {
				normalPaletteIndex += NORMAL_PALETTE_OFFSET;
			}
			const voxel::Voxel newVoxel = voxel::createVoxel(voxel.getMaterial(), voxel.getColor(), normalPaletteIndex, voxel.getFlags());
			wrapper.setVoxel(x, y, z, newVoxel);
		});
		if (fillAndHollow) {
			voxelutil::hollow(wrapper);
		}
		modified(nodeId, wrapper.dirtyRegion(), SceneModifiedFlags::NoResetTrace);
		return true;
	}

	return false;
}

void SceneManager::autosave() {
	if (!_needAutoSave) {
		return;
	}
	const int delay = _autoSaveSecondsDelay->intVal();
	if (delay <= 0 || _lastAutoSave + (double)delay > _timeProvider->tickSeconds()) {
		return;
	}
	// autosaves go into the write path directory (which is usually the home directory of the user)
	io::FileDescription autoSaveFilename;
	if (_lastFilename.empty()) {
		autoSaveFilename.set(_filesystem->homeWritePath("autosave-noname." + voxelformat::VENGIFormat::format().mainExtension()));
	} else {
		const core::String &filename = core::string::extractFilename(_lastFilename.name);
		const core::String &prefix = core::string::startsWith(filename, "autosave-") ? "" : "autosave-";
		const core::String &ext = core::string::extractExtension(_lastFilename.name);
		const core::String &autosaveFilename =
			core::String::format("%s%s.%s", prefix.c_str(), filename.c_str(), ext.c_str());
		autoSaveFilename.set(_filesystem->homeWritePath(autosaveFilename), &_lastFilename.desc);
	}
	if (save(autoSaveFilename, true)) {
		Log::info("Autosave file %s", autoSaveFilename.c_str());
	} else {
		Log::warn("Failed to autosave");
	}
	_lastAutoSave = _timeProvider->tickSeconds();
}

bool SceneManager::nodeSave(int nodeId, const core::String& file) {
	const scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		Log::warn("Node with id %i wasn't found", nodeId);
		return true;
	}
	if (!node->isAnyModelNode()) {
		Log::warn("Given node is no model node");
		return false;
	}
	scenegraph::SceneGraph newSceneGraph;
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	scenegraph::copyNode(*node, newNode, false);
	if (node->isReferenceNode()) {
		newNode.setUnownedVolume(_sceneGraph.resolveVolume(*node));
	}
	newSceneGraph.emplace(core::move(newNode));
	voxelformat::SaveContext saveCtx;
	saveCtx.thumbnailCreator = voxelrender::volumeThumbnail;
	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);
	if (voxelformat::saveFormat(newSceneGraph, file, &_lastFilename.desc, archive, saveCtx)) {
		Log::info("Saved node %i to %s", nodeId, file.c_str());
		return true;
	}
	Log::warn("Failed to save node %i to %s", nodeId, file.c_str());
	return false;
}

void SceneManager::nodeGroupFillHollow() {
	nodeForeachGroup([&] (int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
		voxelutil::fillHollow(wrapper, _modifierFacade.cursorVoxel());
		modified(groupNodeId, wrapper.dirtyRegion());
	});
}

void SceneManager::nodeGroupFill() {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
		voxelutil::fill(wrapper, _modifierFacade.cursorVoxel(), _modifierFacade.isMode(ModifierType::Override));
		modified(groupNodeId, wrapper.dirtyRegion());
	});
}

void SceneManager::nodeGroupClear() {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
		voxelutil::clear(wrapper);
		modified(groupNodeId, wrapper.dirtyRegion());
	});
}

void SceneManager::nodeGroupDeleteSelected() {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		const voxel::Region selRegion = selectionCalculateRegion(*node);
		if (!selRegion.isValid()) {
			return;
		}
		voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
		voxelutil::visitVolume(*v, selRegion, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			wrapper.setVoxel(x, y, z, voxel::Voxel());
		}, voxelutil::VisitSolidOutline());
		modified(groupNodeId, wrapper.dirtyRegion());
	});
}

void SceneManager::nodeGroupColorSelected(uint8_t colorIndex) {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		const voxel::Region selRegion = selectionCalculateRegion(*node);
		if (!selRegion.isValid()) {
			return;
		}
		voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
		voxelutil::recolorSelected(wrapper, *v, selRegion, colorIndex);
		modified(groupNodeId, wrapper.dirtyRegion());
	});
}

void SceneManager::nodeGroupFilterSelection(uint8_t colorIndex, bool deselectMatching) {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		const voxel::Region selRegion = selectionCalculateRegion(*node);
		if (!selRegion.isValid()) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		voxelutil::visitVolume(*v, selRegion, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			const bool matches = voxel.getColor() == colorIndex;
			if (matches == deselectMatching) {
				voxel::Voxel updated = voxel;
				updated.setFlags(voxel.getFlags() & ~voxel::FlagOutline);
				v->setVoxel(x, y, z, updated);
			}
		}, voxelutil::VisitSolidOutline());
		modified(groupNodeId, selRegion, SceneModifiedFlags::NoUndo);
	});
}

void SceneManager::nodeGroupDeselectColor(uint8_t colorIndex) {
	nodeGroupFilterSelection(colorIndex, true);
}

void SceneManager::nodeGroupSelectOnlyColor(uint8_t colorIndex) {
	nodeGroupFilterSelection(colorIndex, false);
}

void SceneManager::nodeGroupSelectByAirAxes(int minAxes) {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		const voxel::Region selRegion = selectionCalculateRegion(*node);
		if (!selRegion.isValid()) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		const voxel::Region &volRegion = v->region();
		voxelutil::visitVolume(*v, selRegion, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			int axesWithAir = 0;
			for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
				const glm::ivec3 neighbor(x + offset.x, y + offset.y, z + offset.z);
				if (!volRegion.containsPoint(neighbor) || !voxel::isBlocked(v->voxel(neighbor).getMaterial())) {
					if (offset.x != 0) {
						axesWithAir |= 1;
					} else if (offset.y != 0) {
						axesWithAir |= 2;
					} else {
						axesWithAir |= 4;
					}
				}
			}
			const int axisCount = (axesWithAir & 1) + ((axesWithAir >> 1) & 1) + ((axesWithAir >> 2) & 1);
			if (axisCount < minAxes) {
				voxel::Voxel updated = voxel;
				updated.setFlags(voxel.getFlags() & ~voxel::FlagOutline);
				v->setVoxel(x, y, z, updated);
			}
		}, voxelutil::VisitSolidOutline());
		modified(groupNodeId, selRegion, SceneModifiedFlags::NoUndo);
	});
}

void SceneManager::nodeGroupSelectOnlyEdges() {
	nodeGroupSelectByAirAxes(2);
}

void SceneManager::nodeGroupSelectOnlyCorners() {
	nodeGroupSelectByAirAxes(3);
}

void SceneManager::nodeGroupSelectOnlyWallEdges() {
	// Ring neighbors in each perpendicular plane (8 neighbors per axis)
	static constexpr glm::ivec3 ringY[] = {
		{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1},
		{1, 0, 1}, {1, 0, -1}, {-1, 0, 1}, {-1, 0, -1}
	};
	static constexpr glm::ivec3 ringX[] = {
		{0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
		{0, 1, 1}, {0, 1, -1}, {0, -1, 1}, {0, -1, -1}
	};
	static constexpr glm::ivec3 ringZ[] = {
		{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0},
		{1, 1, 0}, {1, -1, 0}, {-1, 1, 0}, {-1, -1, 0}
	};

	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		const voxel::Region selRegion = selectionCalculateRegion(*node);
		if (!selRegion.isValid()) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		const voxel::Region &volRegion = v->region();
		voxelutil::visitVolume(*v, selRegion, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			const glm::ivec3 pos(x, y, z);
			bool isWallEdge = false;
			static constexpr int ringSize = lengthof(ringX);
			const glm::ivec3 *rings[] = {ringX, ringY, ringZ};
			static constexpr int maxSolidNeighbors = 1;
			for (int axis = 0; axis < lengthof(rings); ++axis) {
				int solidCount = 0;
				for (int i = 0; i < ringSize; ++i) {
					const glm::ivec3 neighbor = pos + rings[axis][i];
					if (volRegion.containsPoint(neighbor) && voxel::isBlocked(v->voxel(neighbor).getMaterial())) {
						++solidCount;
					}
				}
				if (solidCount <= maxSolidNeighbors) {
					isWallEdge = true;
					break;
				}
			}
			if (!isWallEdge) {
				voxel::Voxel updated = voxel;
				updated.setFlags(voxel.getFlags() & ~voxel::FlagOutline);
				v->setVoxel(x, y, z, updated);
			}
		}, voxelutil::VisitSolidOutline());
		modified(groupNodeId, selRegion, SceneModifiedFlags::NoUndo);
	});
}

void SceneManager::nodeGroupSelectionGrow() {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		const voxel::Region selRegion = selectionCalculateRegion(*node);
		if (!selRegion.isValid()) {
			return;
		}
		const voxel::Region &volRegion = v->region();

		// expand selection region by 1 voxel in each direction, clamped to volume
		const glm::ivec3 expandedMins = glm::max(selRegion.getLowerCorner() - 1, volRegion.getLowerCorner());
		const glm::ivec3 expandedMaxs = glm::min(selRegion.getUpperCorner() + 1, volRegion.getUpperCorner());
		const voxel::Region expandedRegion(expandedMins, expandedMaxs);

		// iterate the expanded shell: for each unselected solid voxel, check if
		// any of its 26 neighbors (within original selRegion) is selected.
		// This avoids collecting source positions and produces no duplicates.
		core::DynamicArray<glm::ivec3> toSelect;
		voxelutil::visitVolume(*v, expandedRegion, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			if ((voxel.getFlags() & voxel::FlagOutline) != 0) {
				return;
			}
			auto hasSelectedNeighbor = [&](const glm::ivec3 *offsets, int count) {
				for (int idx = 0; idx < count; ++idx) {
					const glm::ivec3 neighbor(x + offsets[idx].x, y + offsets[idx].y, z + offsets[idx].z);
					if (!selRegion.containsPoint(neighbor)) {
						continue;
					}
					const voxel::Voxel &nv = v->voxel(neighbor);
					if ((nv.getFlags() & voxel::FlagOutline) != 0) {
						return true;
					}
				}
				return false;
			};
			if (hasSelectedNeighbor(voxel::arrayPathfinderFaces, 6) ||
				hasSelectedNeighbor(voxel::arrayPathfinderEdges, 12) ||
				hasSelectedNeighbor(voxel::arrayPathfinderCorners, 8)) {
				toSelect.push_back(glm::ivec3(x, y, z));
			}
		}, voxelutil::VisitSolid());

		if (toSelect.empty()) {
			return;
		}

		// apply selection flags from the destination list
		voxel::Region dirtyRegion = selRegion;
		for (const glm::ivec3 &pos : toSelect) {
			voxel::Voxel updated = v->voxel(pos);
			updated.setFlags(updated.getFlags() | voxel::FlagOutline);
			v->setVoxel(pos, updated);
			dirtyRegion.accumulate(pos);
		}

		modified(groupNodeId, dirtyRegion);
	});
}

void SceneManager::nodeGroupHollow() {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
		voxelutil::hollow(wrapper);
		modified(groupNodeId, wrapper.dirtyRegion());
	});
}

void SceneManager::fillPlane(const image::ImagePtr &image) {
	const int nodeId = activeNode();
	if (nodeId == InvalidNodeId) {
		Log::error("No active node for fill plane operation");
		return;
	}
	voxel::RawVolume *v = volume(nodeId);
	if (v == nullptr) {
		Log::error("No volume for active node %i", nodeId);
		return;
	}
	voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
	const glm::ivec3 &pos = _modifierFacade.cursorPosition();
	const voxel::FaceNames face = _modifierFacade.cursorFace();
	const voxel::Voxel hitVoxel/* = hitCursorVoxel()*/; // TODO: should be an option
	voxelutil::fillPlane(wrapper, image, hitVoxel, pos, face);
	modified(nodeId, wrapper.dirtyRegion());
}

void SceneManager::nodeUpdateVoxelType(int nodeId, uint8_t palIdx, voxel::VoxelType newType) {
	voxel::RawVolume *v = volume(nodeId);
	if (v == nullptr) {
		return;
	}
	voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
	auto func = [&wrapper, palIdx, newType](int x, int y, int z, const voxel::Voxel &) {
		wrapper.setVoxel(x, y, z, voxel::createVoxel(newType, palIdx));
	};
	voxelutil::visitVolumeParallel(wrapper, func, voxelutil::VisitVoxelColor(palIdx));
	modified(nodeId, wrapper.dirtyRegion());
}

bool SceneManager::saveModels(const core::String& dir) {
	if (dir.empty()) {
		Log::warn("No directory given for model saving");
		return false;
	}
	bool state = false;
	const core::String &suggestedFilename = getSuggestedFilename();
	core::String extension = core::string::extractExtension(suggestedFilename);
	if (extension.empty()) {
		extension = voxelformat::VENGIFormat::format().mainExtension();
	}
	for (auto iter = _sceneGraph.beginAllModels(); iter != _sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const core::String filename = core::string::path(dir, node.name() + "." + extension);
		state |= nodeSave(node.id(), filename);
	}
	return state;
}

bool SceneManager::save(const io::FileDescription &file, bool autosave) {
	if (_sceneGraph.empty()) {
		Log::debug("No volumes for saving found");
		return false;
	}

	if (file.empty()) {
		Log::debug("No filename given for saving");
		return false;
	}
	voxelformat::SaveContext saveCtx;
	saveCtx.thumbnailCreator = voxelrender::volumeThumbnail;
	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);
	if (voxelformat::saveFormat(_sceneGraph, file.name, &file.desc, archive, saveCtx)) {
		if (!autosave) {
			_dirty = false;
			_lastFilename = file;
			const core::String &ext = core::string::extractExtension(file.name);
			metric::count("save", 1, {{"type", ext.toLower()}});
			core::getVar(cfg::UIFileDialogLastFile)->setVal(file.name);
		}
		_needAutoSave = false;
		return true;
	}
	Log::warn("Failed to save to desired format (%s)", file.c_str());
	return false;
}

bool SceneManager::import(const core::String& file) {
	if (file.empty()) {
		Log::error("Can't import model: No file given");
		return false;
	}
	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);
	scenegraph::SceneGraph newSceneGraph;
	voxelformat::LoadContext loadCtx;
	io::FileDescription fileDesc;
	fileDesc.set(file);
	if (!voxelformat::loadFormat(fileDesc, archive, newSceneGraph, loadCtx)) {
		Log::error("Failed to load %s", file.c_str());
		return false;
	}
	if (core::getVar(cfg::VoxEditImportSingleNode)->boolVal()) {
		const scenegraph::SceneGraph::MergeResult &merged = newSceneGraph.merge();
		if (merged.hasVolume()) {
			newSceneGraph.clear();
			scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
			newNode.setVolume(merged.volume());
			newNode.setPalette(merged.palette);
			newNode.setNormalPalette(merged.normalPalette);
			newSceneGraph.emplace(core::move(newNode));
		}
	}

	scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
	groupNode.setName(core::string::extractFilename(file));
	const glm::ivec3 &centerOffset = newSceneGraph.sceneRegion().getDimensionsInVoxels() / 2;
	groupNode.transform(0).setLocalTranslation(referencePosition() - centerOffset);
	int importGroupNodeId = _sceneGraph.emplace(core::move(groupNode), activeNode());
	_sceneGraph.updateTransforms();
	return scenegraph::addSceneGraphNodes(_sceneGraph, newSceneGraph, importGroupNodeId, [this] (int nodeId) {
		onNewNodeAdded(nodeId, false);
	}) > 0;
}

bool SceneManager::importDirectory(const core::String& directory, const io::FormatDescription *format, int depth) {
	if (directory.empty()) {
		return false;
	}
	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem, directory);
	const core::DynamicArray<io::FilesystemEntry> &entities = archive->files();
	if (entities.empty()) {
		Log::info("Could not find any model in %s", directory.c_str());
		return false;
	}

	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "importdirectory");
	bool state = false;
	scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
	groupNode.setName(core::string::extractFilename(directory));
	int importGroupNodeId = _sceneGraph.emplace(core::move(groupNode), activeNode());

	for (const auto &e : entities) {
		if (format == nullptr && !voxelformat::isModelFormat(e.name)) {
			continue;
		}
		scenegraph::SceneGraph newSceneGraph;
		voxelformat::LoadContext loadCtx;
		io::FileDescription fileDesc;
		fileDesc.set(e.fullPath, format);
		if (!voxelformat::loadFormat(fileDesc, archive, newSceneGraph, loadCtx)) {
			Log::error("Failed to load %s", e.fullPath.c_str());
		} else {
			state |= scenegraph::addSceneGraphNodes(_sceneGraph, newSceneGraph, importGroupNodeId, [this] (int nodeId) {
				onNewNodeAdded(nodeId, false);
			}) > 0;
		}
	}
	return state;
}

bool SceneManager::load(const io::FileDescription& file) {
	if (file.empty()) {
		return false;
	}
	if (isLoading()) {
		Log::error("Failed to load '%s' - still loading another model", file.c_str());
		return false;
	}
	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);
	_loadingFuture = app::async([archive, file] () {
		scenegraph::SceneGraph newSceneGraph;
		voxelformat::LoadContext loadCtx;
		voxelformat::loadFormat(file, archive, newSceneGraph, loadCtx);
		return core::move(newSceneGraph);
	});
	_lastFilename.set(file.name, &file.desc);
	core::getVar(cfg::UIFileDialogLastFile)->setVal(file.name);
	return true;
}

bool SceneManager::load(const io::FileDescription& file, const uint8_t *data, size_t size) {
	scenegraph::SceneGraph newSceneGraph;
	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add(file.name, data, size);
	voxelformat::LoadContext loadCtx;
	voxelformat::loadFormat(file, archive, newSceneGraph, loadCtx);
	if (loadSceneGraph(core::move(newSceneGraph))) {
		_needAutoSave = false;
		_dirty = false;
		_lastFilename.clear();
	}
	return true;
}

void SceneManager::setMousePos(int x, int y) {
	if (_mouseLookActive) {
		// In mouselook mode, delta comes from relative mouse input via setMouseDelta().
		// Still update the cursor position so ray casting / brush placement works.
		_mouseCursor.x = x;
		_mouseCursor.y = y;
		_traceViaMouse = true;
		return;
	}
	if (_mouseCursor.x == x && _mouseCursor.y == y) {
		_mouseCursorDelta.x = 0;
		_mouseCursorDelta.y = 0;
		return;
	}
	_mouseCursorDelta.x = x - _mouseCursor.x;
	_mouseCursorDelta.y = y - _mouseCursor.y;
	_mouseCursor.x = x;
	_mouseCursor.y = y;
	// moving the mouse would trigger mouse tracing again
	// TODO: maybe only do this if a mouse button was pressed?
	_traceViaMouse = true;
}

void SceneManager::setMouseLook(bool active) {
	_mouseLookActive = active;
	if (active) {
		if (_camera != nullptr) {
			_preMouselookRotationType = _camera->rotationType();
			_camera->setRotationType(video::CameraRotationType::Eye);
		}
	} else {
		if (_camera != nullptr) {
			if (_preMouselookRotationType == video::CameraRotationType::Target) {
				const glm::vec3 newTarget = _camera->worldPosition() + _camera->forward() * _camera->targetDistance();
				_camera->setTarget(newTarget);
			}
			_camera->setRotationType(_preMouselookRotationType);
		}
		_mouseCursorDelta = {0, 0};
	}
}

void SceneManager::setMouseDelta(int dx, int dy) {
	_mouseCursorDelta.x = dx;
	_mouseCursorDelta.y = dy;
}

glm::mat4 SceneManager::worldMatrix(scenegraph::FrameIndex frameIdx, bool applyTransforms) const {
	const int nodeId = _sceneGraph.activeNode();
	const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	return _sceneGraph.worldMatrix(node, frameIdx, applyTransforms);
}

void SceneManager::modified(int nodeId, const voxel::Region& modifiedRegion, SceneModifiedFlags flags, uint64_t renderRegionMillis) {
	const bool markUndo = (flags & SceneModifiedFlags::MarkUndo) == SceneModifiedFlags::MarkUndo;
	Log::debug("Modified node %i, record undo state: %s", nodeId, markUndo ? "true" : "false");
	voxel::logRegion("Modified", modifiedRegion);
	if (markUndo) {
		const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		_mementoHandler.markModification(_sceneGraph, node, modifiedRegion);
	}
	const bool updateRegion = (flags & SceneModifiedFlags::UpdateRendererRegion) == SceneModifiedFlags::UpdateRendererRegion;
	if (updateRegion && modifiedRegion.isValid()) {
		Log::debug("Modify region for nodeid %i", nodeId);
		_sceneRenderer->updateNodeRegion(nodeId, modifiedRegion, renderRegionMillis);
	}
	const bool invalidateNodeCache = (flags & SceneModifiedFlags::InvalidateNodeCache) == SceneModifiedFlags::InvalidateNodeCache;
	if (invalidateNodeCache) {
		// TODO: SELECTION: TODO: PERF this invalidates the cache too often - if nothing regarding selection was changed, we should keep the cache.
		_selectionRegionCache.invalidate(nodeId);
	}
	markDirty();
	const bool resetTrace = (flags & SceneModifiedFlags::ResetTrace) == SceneModifiedFlags::ResetTrace;
	if (resetTrace) {
		resetLastTrace();
	}
}

int SceneManager::nodeColorToNewNode(const voxel::Voxel voxelColor) {
	const int nodeId = _sceneGraph.activeNode();
	return nodeColorToNewNode(nodeId, voxelColor);
}

int SceneManager::nodeColorToNewNode(int nodeId, const voxel::Voxel voxelColor) {
	scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	voxel::RawVolume *v = _sceneGraph.resolveVolume(node);
	if (v == nullptr) {
		return InvalidNodeId;
	}
	const voxel::Region &region = v->region();
	voxel::RawVolume* newVolume = new voxel::RawVolume(region);
	voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
	auto func = [&] (int32_t x, int32_t y, int32_t z, const voxel::Voxel& voxel) {
		newVolume->setVoxel(x, y, z, voxel);
		wrapper.setVoxel(x, y, z, voxel::Voxel());
	};
	voxelutil::visitVolumeParallel(wrapper, func, voxelutil::VisitVoxelColor(voxelColor.getColor()));
	modified(nodeId, wrapper.dirtyRegion());
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	copyNode(node, newNode, false, true);
	newNode.setVolume(newVolume);
	newNode.setName(core::String::format("color: %i", (int)voxelColor.getColor()));
	return moveNodeToSceneGraph(newNode, node.parent());
}

void SceneManager::nodeScaleUp(int nodeId) {
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		return;
	}
	voxel::RawVolume *destVolume = voxelutil::scaleUp(*v);
	if (destVolume == nullptr) {
		return;
	}
	if (!setNewVolume(nodeId, destVolume, true)) {
		delete destVolume;
		return;
	}
	modified(nodeId, destVolume->region());
}

void SceneManager::nodeScaleDown(int nodeId) {
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		return;
	}
	const voxel::Region srcRegion = v->region();
	const glm::ivec3& targetDimensionsHalf = (srcRegion.getDimensionsInVoxels() / 2) - 1;
	if (targetDimensionsHalf.x < 0 || targetDimensionsHalf.y < 0 || targetDimensionsHalf.z < 0) {
		Log::debug("Can't scale anymore");
		return;
	}
	const voxel::Region destRegion(srcRegion.getLowerCorner(), srcRegion.getLowerCorner() + targetDimensionsHalf);
	voxel::RawVolume* destVolume = new voxel::RawVolume(destRegion);
	voxelutil::scaleDown(*v, _sceneGraph.node(nodeId).palette(), *destVolume);
	if (!setNewVolume(nodeId, destVolume, true)) {
		delete destVolume;
		return;
	}
	modified(nodeId, srcRegion);
}

void SceneManager::nodeSplitObjects(int nodeId) {
	scenegraph::SceneGraphNode* node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return;
	}
	core::Buffer<voxel::RawVolume *> volumes = voxelutil::splitObjects(node->volume());
	if (volumes.empty()) {
		return;
	}

	for (voxel::RawVolume *newVolume : volumes) {
		scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
		newNode.setVolume(newVolume);
		newNode.setName(node->name());
		newNode.setPalette(node->palette());
		moveNodeToSceneGraph(newNode, nodeId);
	}
}

void SceneManager::nodeCrop(int nodeId) {
	scenegraph::SceneGraphNode* node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return;
	}
	voxel::RawVolume* newVolume = voxelutil::cropVolume(node->volume());
	if (newVolume == nullptr) {
		return;
	}
	if (!setNewVolume(nodeId, newVolume, true)) {
		delete newVolume;
		return;
	}
	modified(nodeId, newVolume->region());
}

// TODO: not yet working for reference nodes
void SceneManager::nodeBakeTransform(int nodeId) {
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return;
	}
	const scenegraph::FrameTransform &transform = _sceneGraph.transformForFrame(*node, _currentFrameIdx);
	voxel::RawVolume *newVolume =
		voxelutil::applyTransformToVolume(_sceneGraph.resolveVolume(*node), transform.worldMatrix(), node->pivot());
	if (newVolume == nullptr) {
		return;
	}
	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "applytransform");
	if (!setNewVolume(nodeId, newVolume, true)) {
		delete newVolume;
		return;
	}
	scenegraph::KeyFrameIndex keyFrameIdx = node->keyFrameForFrame(_currentFrameIdx);
	scenegraph::SceneGraphTransform &nodeTransform = node->transform(keyFrameIdx);
	nodeTransform.setWorldMatrix(glm::mat4(1.0f));
	node->setPivot(glm::vec3(0.0f));
	if (nodeTransform.dirty()) {
		nodeTransform.update(_sceneGraph, *node, _currentFrameIdx, false);
		nodeKeyFramesChanged(*node);
	}
	modified(nodeId, newVolume->region());
}

void SceneManager::nodeResize(int nodeId, const glm::ivec3& size) {
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		return;
	}
	voxel::Region region = v->region();
	region.shiftUpperCorner(size);
	nodeResize(nodeId, region);
}

void SceneManager::nodeResize(int nodeId, const voxel::Region &region) {
	if (!region.isValid()) {
		return;
	}
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		Log::error("Failed to lookup volume for node %i", nodeId);
		return;
	}
	const voxel::Region oldRegion = v->region();
	Log::debug("Resize volume from %s to %s", oldRegion.toString().c_str(), region.toString().c_str());
	voxel::RawVolume* newVolume = voxelutil::resize(v, region);
	if (newVolume == nullptr) {
		return;
	}
	if (!setNewVolume(nodeId, newVolume, false)) {
		delete newVolume;
		return;
	}
	// Use the enclosing region of old and new to ensure the memento captures all changes
	// and the undo/redo can properly restore the volume in both directions
	const voxel::Region modifiedRegion(
		glm::min(oldRegion.getLowerCorner(), region.getLowerCorner()),
		glm::max(oldRegion.getUpperCorner(), region.getUpperCorner())
	);
	const glm::ivec3 oldMins = oldRegion.getLowerCorner();
	const glm::ivec3 oldMaxs = oldRegion.getUpperCorner();
	const glm::ivec3 mins = region.getLowerCorner();
	const glm::ivec3 maxs = region.getUpperCorner();
	if (glm::all(glm::greaterThanEqual(maxs, oldMaxs)) && glm::all(glm::lessThanEqual(mins, oldMins))) {
		// No re-extraction needed if only new empty voxels were added, but still
		// record the memento state with a valid region for undo/redo and network
		modified(nodeId, modifiedRegion, SceneModifiedFlags::NoRegionUpdate);
	} else {
		modified(nodeId, modifiedRegion);
	}

	if (activeNode() == nodeId) {
		const glm::ivec3 &refPos = referencePosition();
		if (!region.containsPoint(refPos)) {
			setReferencePosition(region.getCenter());
		}
	}
}

void SceneManager::nodeRescaleContent(int nodeId, const glm::ivec3 &targetSize) {
	voxel::RawVolume *vol = volume(nodeId);
	if (vol == nullptr) {
		Log::error("Failed to lookup volume for node %i", nodeId);
		return;
	}
	const glm::ivec3 currentSize = vol->region().getDimensionsInVoxels();
	if (targetSize.x <= 0 || targetSize.y <= 0 || targetSize.z <= 0) {
		Log::warn("Invalid target size for rescale");
		return;
	}
	const glm::vec3 scale = glm::vec3(targetSize) / glm::vec3(currentSize);
	voxel::RawVolume *scaledVolume = voxelutil::scaleVolume(vol, scale);
	if (scaledVolume == nullptr) {
		Log::error("Failed to rescale volume for node %i", nodeId);
		return;
	}
	if (!setNewVolume(nodeId, scaledVolume, false)) {
		delete scaledVolume;
		return;
	}
	modified(nodeId, scaledVolume->region());
}

void SceneManager::nodeGroupResize(const glm::ivec3& size) {
	nodeForeachGroup([&] (int groupNodeId) {
		nodeResize(groupNodeId, size);
	});
}

voxel::RawVolume* SceneManager::volume(int nodeId) {
	if (nodeId == InvalidNodeId) {
		return nullptr;
	}
	if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
		return _sceneGraph.resolveVolume(*node);
	}
	return nullptr;
}

const voxel::RawVolume* SceneManager::volume(int nodeId) const {
	const scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId);
	core_assert_msg(node != nullptr, "Node with id %i wasn't found in the scene graph", nodeId);
	if (node == nullptr) {
		return nullptr;
	}
	return _sceneGraph.resolveVolume(*node);
}

int SceneManager::activeNode() const {
	return _sceneGraph.activeNode();
}

scenegraph::SceneGraphNodeCamera *SceneManager::activeCameraNode() {
	return _sceneGraph.activeCameraNode();
}

palette::Palette &SceneManager::activePalette() const {
	const int nodeId = activeNode();
	if (!_sceneGraph.hasNode(nodeId)) {
		return _sceneGraph.firstPalette();
	}
	return _sceneGraph.node(nodeId).palette();
}

bool SceneManager::setActivePalette(const palette::Palette &palette, bool searchBestColors) {
	const int nodeId = activeNode();
	if (!_sceneGraph.hasNode(nodeId)) {
		Log::warn("Failed to set the active palette - node with id %i not found", nodeId);
		return false;
	}
	scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	if (node.type() != scenegraph::SceneGraphNodeType::Model) {
		Log::warn("Failed to set the active palette - node with id %i is no model node", nodeId);
		return false;
	}
	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "palette");
	if (searchBestColors) {
		const voxel::Region dirtyRegion = node.remapToPalette(palette);
		if (!dirtyRegion.isValid()) {
			Log::warn("Remapping palette indices failed");
			return false;
		}
		modified(nodeId, dirtyRegion);
	}
	node.setPalette(palette);
	_mementoHandler.markPaletteChange(_sceneGraph, node);
	return true;
}

voxel::RawVolume* SceneManager::activeVolume() {
	const int nodeId = activeNode();
	if (nodeId == InvalidNodeId) {
		Log::error("No active node in scene graph");
		return nullptr;
	}
	return volume(nodeId);
}

bool SceneManager::mementoRename(const memento::MementoState& s) {
	const core::String &uuidStr = s.nodeUUID.str();
	Log::debug("Memento: rename of node %s (%s)", uuidStr.c_str(), s.name.c_str());
	if (scenegraph::SceneGraphNode *node = sceneGraphNodeByUUID(s.nodeUUID)) {
		return nodeRename(*node, s.name);
	}
	Log::warn("Failed to handle memento state - node id %s not found (%s)", uuidStr.c_str(), s.name.c_str());
	return false;
}

bool SceneManager::mementoProperties(const memento::MementoState& s) {
	const core::String &uuidStr = s.nodeUUID.str();
	Log::debug("Memento: properties of node %s (%s)", uuidStr.c_str(), s.name.c_str());
	if (scenegraph::SceneGraphNode *node = sceneGraphNodeByUUID(s.nodeUUID)) {
		node->properties().clear();
		node->addProperties(s.properties);
		_mementoHandler.markNodePropertyChange(_sceneGraph, *node);
		return true;
	}
	return false;
}

bool SceneManager::mementoIKConstraint(const memento::MementoState& s) {
	const core::String &uuidStr = s.nodeUUID.str();
	Log::debug("Memento: IK constraint of node %s (%s)", uuidStr.c_str(), s.name.c_str());
	if (scenegraph::SceneGraphNode *node = sceneGraphNodeByUUID(s.nodeUUID)) {
		if (s.ikConstraint.hasValue()) {
			node->setIkConstraint(*s.ikConstraint.value());
		} else {
			node->removeIkConstraint();
		}
		_mementoHandler.markIKConstraintChange(_sceneGraph, *node);
		return true;
	}
	return false;
}

bool SceneManager::mementoAnimations(const memento::MementoState &s) {
	Log::debug("Memento: animations for scene");
	core_assert(s.stringList.hasValue());
	const core::DynamicArray<core::String> *animations = s.stringList.value();
	if (animations == nullptr) {
		return false;
	}
	if (!_sceneGraph.setAnimations(*animations)) {
		return false;
	}
	_mementoHandler.markAnimationAdded(_sceneGraph, "");
	return true;
}

bool SceneManager::mementoKeyFrames(const memento::MementoState& s) {
	const core::String &uuidStr = s.nodeUUID.str();
	Log::debug("Memento: keyframes of node %s (%s)", uuidStr.c_str(), s.name.c_str());
	if (scenegraph::SceneGraphNode *node = sceneGraphNodeByUUID(s.nodeUUID)) {
		_sceneGraph.setAllKeyFramesForNode(*node, s.keyFrames);
		node->setPivot(s.pivot);
		_sceneGraph.updateTransforms();
		_mementoHandler.markKeyFramesChange(_sceneGraph, *node);
		return true;
	}
	return false;
}

bool SceneManager::mementoPaletteChange(const memento::MementoState &s) {
	const core::String &uuidStr = s.nodeUUID.str();
	Log::debug("Memento: palette change of node %s to %s", uuidStr.c_str(), s.name.c_str());
	if (scenegraph::SceneGraphNode* node = sceneGraphNodeByUUID(s.nodeUUID)) {
		node->setPalette(s.palette);
		_mementoHandler.markPaletteChange(_sceneGraph, *node);
		return true;
	}
	return false;
}

bool SceneManager::mementoNormalPaletteChange(const memento::MementoState &s) {
	const core::String &uuidStr = s.nodeUUID.str();
	Log::debug("Memento: normal palette change of node %s to %s", uuidStr.c_str(), s.name.c_str());
	if (scenegraph::SceneGraphNode* node = sceneGraphNodeByUUID(s.nodeUUID)) {
		node->setNormalPalette(s.normalPalette);
		_mementoHandler.markNormalPaletteChange(_sceneGraph, *node);
		return true;
	}
	return false;
}

bool SceneManager::mementoModification(const memento::MementoState& s) {
	const core::String &uuidStr = s.nodeUUID.str();
	Log::debug("Memento: modification in volume of node %s (%s)", uuidStr.c_str(), s.name.c_str());
	if (scenegraph::SceneGraphNode *node = sceneGraphNodeByUUID(s.nodeUUID)) {
		if (node->type() == scenegraph::SceneGraphNodeType::Model && s.nodeType == scenegraph::SceneGraphNodeType::ModelReference) {
			if (scenegraph::SceneGraphNode* referenceNode = sceneGraphNodeByUUID(s.referenceUUID)) {
				node->setReference(referenceNode->id(), true);
			} else {
				const core::String &refUUIDStr = s.referenceUUID.str();
				Log::warn("Failed to handle memento state - reference node id %s not found", refUUIDStr.c_str());
			}
		} else {
			if (node->type() == scenegraph::SceneGraphNodeType::ModelReference && s.nodeType == scenegraph::SceneGraphNodeType::Model) {
				node->unreferenceModelNode(_sceneGraph.node(node->reference()));
			}
			if (node->region() != s.volumeRegion()) {
				voxel::RawVolume *v = new voxel::RawVolume(s.volumeRegion());
				if (!setSceneGraphNodeVolume(*node, v)) {
					delete v;
				}
			}
			if (s.hasVolumeData()) {
				_mementoHandler.extractVolumeRegion(node->volume(), s);
			}
		}
		node->setName(s.name);
		node->setPalette(s.palette);
		modified(node->id(), s.data.modifiedRegion());
		return true;
	}
	Log::warn("Failed to handle memento state - node id %s not found (%s)", uuidStr.c_str(), s.name.c_str());
	return false;
}

bool SceneManager::mementoStateToNode(const memento::MementoState &s) {
	core_assert(s.nodeType != scenegraph::SceneGraphNodeType::Max);
	scenegraph::SceneGraphNode newNode(s.nodeType, s.nodeUUID);
	if (newNode.isModelNode()) {
		newNode.createVolume(s.volumeRegion());
		if (s.hasVolumeData()) {
			memento::MementoData::toVolume(newNode.volume(), s.data, s.data.dataRegion());
		}
	}
	newNode.setPalette(s.palette);
	if (newNode.isReferenceNode()) {
		if (scenegraph::SceneGraphNode* referenceNode = sceneGraphNodeByUUID(s.referenceUUID)) {
			newNode.setReference(referenceNode->id());
		} else {
			const core::String &refUUIDStr = s.referenceUUID.str();
			Log::warn("Failed to handle memento state - reference node id %s not found", refUUIDStr.c_str());
		}
	}
	_sceneGraph.setAllKeyFramesForNode(newNode, s.keyFrames);
	newNode.properties().clear();
	newNode.addProperties(s.properties);
	newNode.setPivot(s.pivot);
	newNode.setName(s.name);
	int parentNodeId = 0;
	if (scenegraph::SceneGraphNode* parentNode = sceneGraphNodeByUUID(s.parentUUID)) {
		parentNodeId = parentNode->id();
	}
	const int newNodeId = moveNodeToSceneGraph(newNode, parentNodeId);
	_sceneGraph.updateTransforms();
	return newNodeId != InvalidNodeId;
}

bool SceneManager::mementoStateExecute(const memento::MementoState &s, bool isRedo) {
	core_assert(s.valid());
	memento::ScopedMementoHandlerLock lock(_mementoHandler);
	if (s.type == memento::MementoType::SceneNodeRenamed) {
		return mementoRename(s);
	}
	if (s.type == memento::MementoType::SceneNodeKeyFrames) {
		return mementoKeyFrames(s);
	}
	if (s.type == memento::MementoType::SceneNodeProperties) {
		return mementoProperties(s);
	}
	if (s.type == memento::MementoType::SceneNodeIKConstraint) {
		return mementoIKConstraint(s);
	}
	if (s.type == memento::MementoType::SceneGraphAnimation) {
		return mementoAnimations(s);
	}
	if (s.type == memento::MementoType::SceneNodePaletteChanged) {
		return mementoPaletteChange(s);
	}
	if (s.type == memento::MementoType::SceneNodeNormalPaletteChanged) {
		return mementoNormalPaletteChange(s);
	}
	if (s.type == memento::MementoType::SceneNodeMove) {
		const core::String &uuidStr = s.nodeUUID.str();
		const core::String &parentUUIDStr = s.parentUUID.str();
		Log::debug("Memento: move of node %s (%s) (new parent %s)", uuidStr.c_str(), s.name.c_str(), parentUUIDStr.c_str());
		scenegraph::SceneGraphNode *node = sceneGraphNodeByUUID(s.nodeUUID);
		scenegraph::SceneGraphNode *nodeParent = sceneGraphNodeByUUID(s.parentUUID);
		if (node && nodeParent) {
			if (nodeMove(node->id(), nodeParent->id(), scenegraph::NodeMoveFlag::None)) {
				const core::String animation = sceneGraph().activeAnimation();
				node->setAllKeyFrames(s.keyFrames, animation);
				return true;
			}
			return false;
		}
		Log::warn("Failed to handle memento move state - node id %s not found (%s)", uuidStr.c_str(), s.name.c_str());
		return false;
	}
	if (s.type == memento::MementoType::Modification) {
		return mementoModification(s);
	}
	if (isRedo) {
		if (s.type == memento::MementoType::SceneNodeRemoved) {
			const core::String &uuidStr = s.nodeUUID.str();
			const core::String &parentUUIDStr = s.parentUUID.str();
			Log::debug("Memento: remove of node %s (%s) from parent %s", uuidStr.c_str(), s.name.c_str(), parentUUIDStr.c_str());
			if (scenegraph::SceneGraphNode *node = sceneGraphNodeByUUID(s.nodeUUID)) {
				return nodeRemove(*node, false);
			}
			Log::warn("Failed to handle redo memento remove state - node id %s not found (%s)", uuidStr.c_str(), s.name.c_str());
			return false;
		}
		if (s.type == memento::MementoType::SceneNodeAdded) {
			const core::String &parentUUIDStr = s.parentUUID.str();
			Log::debug("Memento: add node (%s) to parent %s", s.name.c_str(), parentUUIDStr.c_str());
			return mementoStateToNode(s);
		}
	} else {
		if (s.type == memento::MementoType::SceneNodeRemoved) {
			const core::String &parentUUIDStr = s.parentUUID.str();
			Log::debug("Memento: remove of node (%s) from parent %s", s.name.c_str(), parentUUIDStr.c_str());
			return mementoStateToNode(s);
		}
		if (s.type == memento::MementoType::SceneNodeAdded) {
			const core::String &parentUUIDStr = s.parentUUID.str();
			Log::debug("Memento: add node (%s) to parent %s", s.name.c_str(), parentUUIDStr.c_str());
			if (scenegraph::SceneGraphNode *node = sceneGraphNodeByUUID(s.nodeUUID)) {
				return nodeRemove(*node, false);
			}
			const core::String &uuidStr = s.nodeUUID.str();
			Log::warn("Failed to handle undo memento add state - node id %s not found (%s)", uuidStr.c_str(), s.name.c_str());
		}
	}
	return true;
}

bool SceneManager::undo(int n) {
	Log::debug("undo %i steps", n);
	for (int i = 0; i < n; ++i) {
		if (!doUndo()) {
			return false;
		}
	}
	_modifierFacade.onSceneChange();
	return true;
}

bool SceneManager::redo(int n) {
	Log::debug("redo %i steps", n);
	for (int i = 0; i < n; ++i) {
		if (!doRedo()) {
			return false;
		}
	}
	_modifierFacade.onSceneChange();
	return true;
}

bool SceneManager::doUndo() {
	if (!mementoHandler().canUndo()) {
		Log::debug("Nothing to undo");
		return false;
	}

	const memento::MementoStateGroup& group = _mementoHandler.undo();
	if (group.states.empty()) {
		Log::debug("Nothing to undo");
		return false;
	}
	const int n = (int)group.states.size() - 1;

	core::DynamicArray<const memento::MementoState*> parentFixupStates;
	parentFixupStates.reserve(n);

	for (int i = n; i >= 0; --i) {
		const memento::MementoState& s = group.states[i];
		if (s.type == memento::MementoType::SceneNodeRemoved) {
			parentFixupStates.push_back(&s);
		}
		if (!mementoStateExecute(s, false)) {
			Log::error("Failed to undo memento state %i", (int)s.type);
			return false;
		}
	}
	// When nodes are removed, scene graph re-parents their children, corrupting parent relationships.
	// During undo, nodes may be restored before their parents, defaulting to root.
	// Fix parent connections after all nodes are restored.
	for (const memento::MementoState* state : parentFixupStates) {
		scenegraph::SceneGraphNode *node = sceneGraphNodeByUUID(state->nodeUUID);
		scenegraph::SceneGraphNode *parentNode = sceneGraphNodeByUUID(state->parentUUID);
		if (node && parentNode && node->parent() != parentNode->id()) {
			_sceneGraph.changeParent(node->id(), parentNode->id(), scenegraph::NodeMoveFlag::None);
		}
	}
	return true;
}

bool SceneManager::doRedo() {
	if (!mementoHandler().canRedo()) {
		Log::debug("Nothing to redo");
		return false;
	}

	const memento::MementoStateGroup& group = _mementoHandler.redo();
	for (const memento::MementoState& s : group.states) {
		if (!mementoStateExecute(s, true)) {
			Log::error("Failed to redo memento state %i", (int)s.type);
			return false;
		}
	}
	return true;
}

voxel::ClipboardData SceneManager::nodeClipboardCopy(scenegraph::SceneGraphNode &node) {
	voxel::Region selectionRegion = selectionCalculateRegion(node);
	if (!selectionRegion.isValid()) {
		Log::debug("No selection to copy for node %i", node.id());
		return voxel::ClipboardData();
	}

	// Create a new volume with the selected voxels
	voxel::RawVolume *v = new voxel::RawVolume(selectionRegion);
	const glm::ivec3 &selMins = selectionRegion.getLowerCorner();
	const glm::ivec3 &selMaxs = selectionRegion.getUpperCorner();

	// TODO: PERF: use a sampler
	for (int32_t z = selMins.z; z <= selMaxs.z; ++z) {
		for (int32_t y = selMins.y; y <= selMaxs.y; ++y) {
			for (int32_t x = selMins.x; x <= selMaxs.x; ++x) {
				const voxel::Voxel &voxel = node.volume()->voxel(x, y, z);
				if ((voxel.getFlags() & voxel::FlagOutline) != 0) {
					// Copy voxel without the outline flag
					voxel::Voxel copiedVoxel = voxel::createVoxel(voxel.getMaterial(), voxel.getColor(),
																  voxel.getNormal(), 0, voxel.getBoneIdx());
					v->setVoxel(x, y, z, copiedVoxel);
				}
			}
		}
	}
	return voxel::ClipboardData(v, node.palette(), true);
}

bool SceneManager::saveSelection(const io::FileDescription& file) {
	const int nodeId = activeNode();
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		Log::warn("Node with id %i wasn't found", nodeId);
		return true;
	}

	voxelformat::SaveContext saveCtx;
	saveCtx.thumbnailCreator = voxelrender::volumeThumbnail;

	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);

	voxel::ClipboardData clipboardData = nodeClipboardCopy(*node);
	if (!clipboardData) {
		Log::warn("Failed to copy selection for node %i", nodeId);
		return false;
	}

	scenegraph::SceneGraph newSceneGraph;
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	scenegraph::copyNode(*node, newNode, false);
	newNode.setVolume(new voxel::RawVolume(*clipboardData.volume));
	newNode.setPalette(*clipboardData.palette);
	newSceneGraph.emplace(core::move(newNode));
	if (!voxelformat::saveFormat(newSceneGraph, file.name, &file.desc, archive, saveCtx)) {
		Log::warn("Failed to save node %i to %s", nodeId, file.name.c_str());
		return false;
	}
	Log::info("Saved node %i to %s", nodeId, file.name.c_str());
	return true;
}

bool SceneManager::nodeCopy(int nodeId) {
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return false;
	}
	const voxel::ClipboardData &copyClipboardData = nodeClipboardCopy(*node);
	if (!copyClipboardData) {
		return false;
	}
	_copy = copyClipboardData;
	return _copy;
}

bool SceneManager::nodePasteAsNewNode(int nodeId) {
	if (!_copy) {
		Log::debug("Nothing copied yet - failed to paste");
		return false;
	}
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return false;
	}
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	scenegraph::copyNode(*node, newNode, false);
	newNode.setVolume(new voxel::RawVolume(*_copy.volume));
	newNode.setPalette(*_copy.palette);
	return moveNodeToSceneGraph(newNode, node->parent()) != InvalidNodeId;
}

void SceneManager::autoSelectSolidVoxels(scenegraph::SceneGraphNode *node, const voxel::Region &region) {
	if (node == nullptr) {
		return;
	}
	if (!core::getVar(cfg::VoxEditAutoSelect)->boolVal()) {
		return;
	}
	node->clearSelection();
	voxel::RawVolume *volume = node->volume();
	voxelutil::visitVolumeParallel(*volume, region,
		[volume](int x, int y, int z, const voxel::Voxel &srcVoxel) {
			voxel::Voxel selected = srcVoxel;
			selected.setFlags(selected.getFlags() | voxel::FlagOutline);
			volume->setVoxel(x, y, z, selected);
		},
		voxelutil::VisitSolid());
}

bool SceneManager::loadGlobalClipboard(voxel::ClipboardData &clipData) {
	const core::String clipboardFile = _filesystem->homeWritePath("globalclipboard.vengi");
	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);
	scenegraph::SceneGraph loadedSceneGraph;
	voxelformat::LoadContext loadCtx;
	io::FileDescription fileDesc;
	fileDesc.set(clipboardFile);
	if (!voxelformat::loadFormat(fileDesc, archive, loadedSceneGraph, loadCtx)) {
		Log::warn("Failed to read global clipboard from %s", clipboardFile.c_str());
		return false;
	}
	for (auto iter = loadedSceneGraph.beginModel(); iter != loadedSceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &srcNode = *iter;
		if (srcNode.volume() == nullptr) {
			continue;
		}
		clipData = voxel::ClipboardData(new voxel::RawVolume(*srcNode.volume()), srcNode.palette(), true);
		return true;
	}
	Log::warn("Global clipboard file contained no model data");
	return false;
}

bool SceneManager::paste(const glm::ivec3& pos) {
	const int nodeId = activeNode();
	return nodePaste(nodeId, pos);
}

bool SceneManager::nodePaste(int nodeId, const glm::ivec3& pos) {
	if (!_copy) {
		Log::debug("Nothing copied yet - failed to paste");
		return false;
	}
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		Log::warn("paste: no active model node");
		return false;
	}
	const voxel::Region &destRegion = node->region();
	Log::debug("paste: clipboard region %s, cursor pos %i:%i:%i, dest volume region %s",
			  _copy.volume->region().toString().c_str(), pos.x, pos.y, pos.z,
			  destRegion.toString().c_str());
	voxel::Region modifiedRegion;
	voxel::Region region = _copy.volume->region();
	region.shift(-region.getLowerCorner());
	region.shift(pos);
	modifiedRegion = region;
	voxelutil::mergeVolumes(node->volume(), node->palette(), _copy.volume, *_copy.palette, region, _copy.volume->region());
	Log::debug("Pasted %s", modifiedRegion.toString().c_str());
	if (!modifiedRegion.isValid()) {
		Log::warn("paste: modifiedRegion is invalid after paste");
		return false;
	}
	if (!voxel::intersects(modifiedRegion, destRegion)) {
		Log::warn("paste: modified region %s does not intersect destination volume %s - voxels out of bounds",
				  modifiedRegion.toString().c_str(), destRegion.toString().c_str());
	}
	autoSelectSolidVoxels(node, modifiedRegion);
	const int64_t dismissMillis = core::getVar(cfg::VoxEditModificationDismissMillis)->intVal();
	modified(nodeId, modifiedRegion, SceneModifiedFlags::All, dismissMillis);
	return true;
}

bool SceneManager::nodeCut(int nodeId) {
	voxel::Region selectionRegion = selectionCalculateRegion(nodeId);
	if (!selectionRegion.isValid()) {
		Log::debug("Cut failed: no selected voxels found");
		return false;
	}
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	voxel::RawVolume *volume = node->volume();
	if (volume == nullptr) {
		Log::debug("Cut failed: no voxel data");
		return false;
	}

	voxel::RawVolume *v = new voxel::RawVolume(selectionRegion);
	const glm::ivec3 &selMins = selectionRegion.getLowerCorner();
	const glm::ivec3 &selMaxs = selectionRegion.getUpperCorner();

	// TODO: PERF: use a sampler
	for (int32_t z = selMins.z; z <= selMaxs.z; ++z) {
		for (int32_t y = selMins.y; y <= selMaxs.y; ++y) {
			for (int32_t x = selMins.x; x <= selMaxs.x; ++x) {
				voxel::Voxel voxel = volume->voxel(x, y, z);
				if ((voxel.getFlags() & voxel::FlagOutline) != 0) {
					voxel::Voxel copiedVoxel = voxel::createVoxel(voxel.getMaterial(), voxel.getColor(),
																  voxel.getNormal(), 0, voxel.getBoneIdx());
					v->setVoxel(x, y, z, copiedVoxel);
					volume->setVoxel(x, y, z, voxel::Voxel());
				}
			}
		}
	}

	voxel::Region modifiedRegion;
	if (modifiedRegion.isValid()) {
		modifiedRegion.accumulate(v->region());
	} else {
		modifiedRegion = v->region();
	}
	_copy = {v, node->palette(), true};
	if (!_copy) {
		Log::debug("Failed to cut");
		return false;
	}
	if (!modifiedRegion.isValid()) {
		Log::debug("Failed to cut");
		_copy = {};
		return false;
	}
	const int64_t dismissMillis = core::getVar(cfg::VoxEditModificationDismissMillis)->intVal();
	modified(nodeId, modifiedRegion, SceneModifiedFlags::All, dismissMillis);
	return true;
}

bool SceneManager::globalCopy() {
	// Always try to capture the current selection first.
	// copy() only updates _copy when a selection exists; if there is no
	// selection it leaves _copy unchanged, so a previously copied region
	// is still used as a fallback.
	const int nodeId = activeNode();
	return nodeGlobalCopy(nodeId);
}

bool SceneManager::nodeGlobalCopy(int nodeId) {
	nodeCopy(nodeId);
	if (!_copy) {
		Log::warn("globalcopy: nothing to copy - make a selection first");
		return false;
	}
	scenegraph::SceneGraph newSceneGraph;
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	newNode.setVolume(new voxel::RawVolume(*_copy.volume));
	newNode.setPalette(*_copy.palette);
	newSceneGraph.emplace(core::move(newNode));
	const core::String clipboardFile = _filesystem->homeWritePath("globalclipboard.vengi");
	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);
	voxelformat::SaveContext saveCtx;
	if (!voxelformat::saveFormat(newSceneGraph, clipboardFile, nullptr, archive, saveCtx)) {
		Log::warn("Failed to write global clipboard to %s", clipboardFile.c_str());
		return false;
	}
	Log::debug("Global clipboard written to %s", clipboardFile.c_str());
	return true;
}

bool SceneManager::globalCopyVisible() {
	const scenegraph::SceneGraph::MergeResult &merged = _sceneGraph.merge(true);
	if (!merged.hasVolume()) {
		Log::warn("globalcopyvisible: no visible model nodes to copy");
		return false;
	}
	scenegraph::SceneGraph newSceneGraph;
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	newNode.setVolume(merged.volume());
	newNode.setPalette(merged.palette);
	newSceneGraph.emplace(core::move(newNode));
	const core::String clipboardFile = _filesystem->homeWritePath("globalclipboard.vengi");
	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);
	voxelformat::SaveContext saveCtx;
	if (!voxelformat::saveFormat(newSceneGraph, clipboardFile, nullptr, archive, saveCtx)) {
		Log::warn("Failed to write global clipboard to %s", clipboardFile.c_str());
		return false;
	}
	Log::debug("Global clipboard (visible nodes) written to %s", clipboardFile.c_str());
	return true;
}

bool SceneManager::globalPaste(const glm::ivec3 &pos) {
	const int nodeId = activeNode();
	return nodeGlobalPaste(nodeId, pos);
}

bool SceneManager::nodeGlobalPaste(int nodeId, const glm::ivec3 &pos) {
	if (!loadGlobalClipboard(_copy)) {
		return false;
	}
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node != nullptr) {
		const glm::ivec3 dims = _copy.volume->region().getDimensionsInVoxels();
		const voxel::Region pasteRegion(pos, pos + dims - 1);
		if (node->region().containsRegion(pasteRegion)) {
			Log::debug("globalPaste: merging into active node at %i:%i:%i", pos.x, pos.y, pos.z);
			return paste(pos);
		}
	}

	// Active node can't fit the clipboard at pos - create a new node translated to cursor
	voxel::RawVolume *vol = new voxel::RawVolume(*_copy.volume);
	vol->translate(pos - vol->region().getLowerCorner());
	Log::debug("globalPaste: creating new node at %i:%i:%i (region %s)", pos.x, pos.y, pos.z,
			  vol->region().toString().c_str());
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	newNode.setName("clipboard");
	newNode.setVolume(vol);
	newNode.setPalette(*_copy.palette);
	const int newNodeId = moveNodeToSceneGraph(newNode);
	if (newNodeId == InvalidNodeId) {
		Log::warn("globalPaste: failed to create node from clipboard");
		return false;
	}
	scenegraph::SceneGraphNode *addedNode = sceneGraphModelNode(newNodeId);
	autoSelectSolidVoxels(addedNode, vol->region());
	return true;
}

bool SceneManager::globalPasteNode(const glm::ivec3 &pos) {
	voxel::ClipboardData clipData;
	if (!loadGlobalClipboard(clipData)) {
		return false;
	}

	voxel::RawVolume *vol = new voxel::RawVolume(*clipData.volume);
	vol->translate(pos - vol->region().getLowerCorner());
	Log::debug("globalPasteNode: creating new node at %i:%i:%i (region %s)", pos.x, pos.y, pos.z,
			  vol->region().toString().c_str());
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	newNode.setName("clipboard");
	newNode.setVolume(vol);
	newNode.setPalette(*clipData.palette);
	const int newNodeId = moveNodeToSceneGraph(newNode);
	if (newNodeId == InvalidNodeId) {
		Log::warn("globalPasteNode: failed to create node from clipboard");
		return false;
	}
	autoSelectSolidVoxels(sceneGraphModelNode(newNodeId), vol->region());
	return true;
}

bool SceneManager::splatMerge(int sourceNodeId) {
	scenegraph::SceneGraphNode *sourceNode = sceneGraphModelNode(sourceNodeId);
	if (sourceNode == nullptr) {
		Log::warn("splatmerge: no valid source node");
		return false;
	}
	const voxel::RawVolume *sourceVolume = _sceneGraph.resolveVolume(*sourceNode);
	if (sourceVolume == nullptr) {
		Log::warn("splatmerge: source node has no volume");
		return false;
	}

	// Bake source into world space (apply scene graph transform if any)
	const scenegraph::FrameTransform &srcTransform =
		_sceneGraph.transformForFrame(*sourceNode, _currentFrameIdx);
	core::ScopedPtr<voxel::RawVolume> bakedSource;
	const voxel::RawVolume *worldSource = sourceVolume;
	if (!srcTransform.isIdentity()) {
		bakedSource = voxelutil::applyTransformToVolume(
			*sourceVolume, srcTransform.worldMatrix(), sourceNode->pivot());
		worldSource = &(*bakedSource);
	}
	const voxel::Region &sourceWorldRegion = worldSource->region();
	const palette::Palette &sourcePalette = sourceNode->palette();

	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "splatmerge");

	int mergedCount = 0;
	for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &targetNode = *iter;
		if (targetNode.id() == sourceNodeId) {
			continue;
		}
		if (!targetNode.isModelNode()) {
			continue;
		}
		voxel::RawVolume *targetVolume = targetNode.volume();
		if (targetVolume == nullptr) {
			continue;
		}

		// Get the target's world-space region (accounts for scene graph transforms)
		const voxel::Region targetWorldRegion =
			_sceneGraph.sceneRegion(targetNode, _currentFrameIdx);
		if (!targetWorldRegion.isValid()) {
			continue;
		}
		if (!voxel::intersects(sourceWorldRegion, targetWorldRegion)) {
			continue;
		}

		// Compute the overlapping region in world coordinates
		voxel::Region worldOverlap = sourceWorldRegion;
		worldOverlap.cropTo(targetWorldRegion);

		// Map world overlap back to target local coordinates
		const glm::ivec3 worldToLocal =
			targetVolume->region().getLowerCorner() - targetWorldRegion.getLowerCorner();
		const voxel::Region targetLocalOverlap(
			worldOverlap.getLowerCorner() + worldToLocal,
			worldOverlap.getUpperCorner() + worldToLocal);

		const int count = voxelutil::mergeVolumes(targetVolume, targetNode.palette(),
			worldSource, sourcePalette, targetLocalOverlap, worldOverlap);
		if (count > 0) {
			modified(targetNode.id(), targetLocalOverlap, SceneModifiedFlags::All);
			mergedCount += count;
		}
	}

	if (mergedCount == 0) {
		Log::warn("splatmerge: no overlapping nodes found for source node %i", sourceNodeId);
		return false;
	}

	Log::info("splatmerge: merged %i voxels into overlapping nodes", mergedCount);
	_mementoHandler.markNodeRemove(_sceneGraph, *sourceNode);
	if (_sceneGraph.removeNode(sourceNodeId, false)) {
		_sceneRenderer->removeNode(sourceNodeId);
	}
	return true;
}

bool SceneManager::mergeActiveToBackground() {
	const int sourceNodeId = activeNode();
	scenegraph::SceneGraphNode *sourceNode = sceneGraphModelNode(sourceNodeId);
	if (sourceNode == nullptr) {
		Log::warn("mergeactivetobackground: no valid active node");
		return false;
	}
	const voxel::RawVolume *sourceVolume = _sceneGraph.resolveVolume(*sourceNode);
	if (sourceVolume == nullptr) {
		Log::warn("mergeactivetobackground: active node has no volume");
		return false;
	}

	const scenegraph::FrameTransform &srcTransform =
		_sceneGraph.transformForFrame(*sourceNode, _currentFrameIdx);
	core::ScopedPtr<voxel::RawVolume> bakedSource;
	const voxel::RawVolume *worldSource = sourceVolume;
	if (!srcTransform.isIdentity()) {
		bakedSource = voxelutil::applyTransformToVolume(
			*sourceVolume, srcTransform.worldMatrix(), sourceNode->pivot());
		worldSource = &(*bakedSource);
	}
	const voxel::Region &sourceWorldRegion = worldSource->region();
	const palette::Palette &sourcePalette = sourceNode->palette();

	// Detect grid parameters from existing background nodes
	const int chunkSize = _maxSuggestedVolumeSize->intVal();
	glm::ivec3 gridOffset(0);
	int backgroundParentId = _sceneGraph.root().id();
	for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (node.id() == sourceNodeId || !node.isModelNode()) {
			continue;
		}
		const voxel::Region wr = _sceneGraph.sceneRegion(node, _currentFrameIdx);
		if (!wr.isValid()) {
			continue;
		}
		// Grid offset: lower corner modulo chunkSize (handle negative coords)
		const glm::ivec3 &lc = wr.getLowerCorner();
		gridOffset = ((lc % chunkSize) + glm::ivec3(chunkSize)) % glm::ivec3(chunkSize);
		backgroundParentId = node.parent();
		break;
	}

	// Floor division helper for grid cell origin (handles negative coordinates)
	auto gridCellOrigin = [&](const glm::ivec3 &pos) -> glm::ivec3 {
		const glm::ivec3 shifted = pos - gridOffset;
		glm::ivec3 cell;
		for (int i = 0; i < 3; ++i) {
			cell[i] = (shifted[i] >= 0)
				? (shifted[i] / chunkSize)
				: ((shifted[i] - chunkSize + 1) / chunkSize);
		}
		return cell * chunkSize + gridOffset;
	};

	// Enumerate all grid cells the source overlaps
	struct GridCell {
		glm::ivec3 origin;
		voxel::Region cellRegion;
		int existingNodeId;
	};
	const glm::ivec3 firstCell = gridCellOrigin(sourceWorldRegion.getLowerCorner());
	const glm::ivec3 lastCell = gridCellOrigin(sourceWorldRegion.getUpperCorner());
	core::DynamicArray<GridCell> neededCells;
	for (int y = firstCell.y; y <= lastCell.y; y += chunkSize) {
		for (int z = firstCell.z; z <= lastCell.z; z += chunkSize) {
			for (int x = firstCell.x; x <= lastCell.x; x += chunkSize) {
				const glm::ivec3 origin(x, y, z);
				const voxel::Region cellRegion(origin, origin + glm::ivec3(chunkSize - 1));
				if (!voxel::intersects(sourceWorldRegion, cellRegion)) {
					continue;
				}
				neededCells.push_back({origin, cellRegion, InvalidNodeId});
			}
		}
	}

	// Match existing background nodes to grid cells
	for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (node.id() == sourceNodeId || !node.isModelNode()) {
			continue;
		}
		const voxel::Region wr = _sceneGraph.sceneRegion(node, _currentFrameIdx);
		if (!wr.isValid()) {
			continue;
		}
		const glm::ivec3 nodeCell = gridCellOrigin(wr.getLowerCorner());
		for (GridCell &cell : neededCells) {
			if (cell.origin == nodeCell && cell.existingNodeId == InvalidNodeId) {
				cell.existingNodeId = node.id();
				break;
			}
		}
	}

	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "mergeactivetobackground");

	struct StampedNode {
		int nodeId;
		voxel::Region region;
	};
	core::DynamicArray<StampedNode> stampedNodes;
	stampedNodes.reserve(neededCells.size());
	int stampedCount = 0;

	for (const GridCell &cell : neededCells) {
		// Compute the overlap of the source with this grid cell (in world coords)
		voxel::Region cellSourceOverlap = sourceWorldRegion;
		cellSourceOverlap.cropTo(cell.cellRegion);

		if (cell.existingNodeId != InvalidNodeId) {
			// Existing node: expand if needed, then stamp
			scenegraph::SceneGraphNode &targetNode = _sceneGraph.node(cell.existingNodeId);
			voxel::RawVolume *targetVolume = targetNode.volume();
			if (targetVolume == nullptr) {
				continue;
			}

			const voxel::Region targetWorldRegion =
				_sceneGraph.sceneRegion(targetNode, _currentFrameIdx);
			const glm::ivec3 worldToLocal =
				targetVolume->region().getLowerCorner() - targetWorldRegion.getLowerCorner();

			// The local region we need to write into
			const voxel::Region neededLocalRegion(
				cellSourceOverlap.getLowerCorner() + worldToLocal,
				cellSourceOverlap.getUpperCorner() + worldToLocal);

			// Expand the volume if it doesn't contain the needed region
			const voxel::Region currentLocalRegion = targetVolume->region();
			if (!currentLocalRegion.containsRegion(neededLocalRegion)) {
				const voxel::Region expandedRegion(
					glm::min(currentLocalRegion.getLowerCorner(), neededLocalRegion.getLowerCorner()),
					glm::max(currentLocalRegion.getUpperCorner(), neededLocalRegion.getUpperCorner()));
				voxel::RawVolume *newVolume = voxelutil::resize(targetVolume, expandedRegion);
				if (newVolume == nullptr) {
					Log::warn("mergeactivetobackground: failed to expand node %i", cell.existingNodeId);
					continue;
				}
				targetNode.setVolume(newVolume);
				targetVolume = newVolume;
			}

			// Add missing source colors to the target palette before mapping
			palette::Palette &destPalette = targetNode.palette();
			bool paletteChanged = false;
			for (int i = 0; i < sourcePalette.colorCount(); ++i) {
				if (destPalette.tryAdd(sourcePalette.color(i), false)) {
					paletteChanged = true;
				}
			}
			if (paletteChanged) {
				destPalette.markDirty();
			}
			palette::PaletteLookup palLookup(destPalette);

			// Stamp all voxels including air (overwrite destination in overlap)
			int count = 0;
			const glm::ivec3 &srcLower = cellSourceOverlap.getLowerCorner();
			const glm::ivec3 &dstLower = neededLocalRegion.getLowerCorner();
			const int32_t overlapW = cellSourceOverlap.getWidthInVoxels();
			const int32_t overlapH = cellSourceOverlap.getHeightInVoxels();
			const int32_t overlapD = cellSourceOverlap.getDepthInVoxels();
			for (int32_t z = 0; z < overlapD; ++z) {
				for (int32_t y = 0; y < overlapH; ++y) {
					for (int32_t x = 0; x < overlapW; ++x) {
						const voxel::Voxel srcVoxel = worldSource->voxel(
							srcLower.x + x, srcLower.y + y, srcLower.z + z);
						voxel::Voxel destVoxel;
						if (voxel::isAir(srcVoxel.getMaterial())) {
							destVoxel = voxel::Voxel();
						} else {
							const int idx = palLookup.findClosestIndex(
								sourcePalette.color(srcVoxel.getColor()));
							destVoxel = voxel::createVoxel(destPalette,
								idx == palette::PaletteColorNotFound ? 0 : idx);
						}
						targetVolume->setVoxel(
							dstLower.x + x, dstLower.y + y, dstLower.z + z, destVoxel);
						++count;
					}
				}
			}
			if (count > 0) {
				stampedNodes.push_back(StampedNode{cell.existingNodeId, neededLocalRegion});
				stampedCount += count;
			}
		} else {
			// No existing node for this grid cell: create a new one
			// Single pass: stamp non-air source voxels into a new volume
			voxel::RawVolume *newVolume = new voxel::RawVolume(cell.cellRegion);
			palette::PaletteLookup palLookup(sourcePalette);

			const glm::ivec3 &srcLower = cellSourceOverlap.getLowerCorner();
			const int32_t overlapW = cellSourceOverlap.getWidthInVoxels();
			const int32_t overlapH = cellSourceOverlap.getHeightInVoxels();
			const int32_t overlapD = cellSourceOverlap.getDepthInVoxels();
			int count = 0;
			for (int32_t z = 0; z < overlapD; ++z) {
				for (int32_t y = 0; y < overlapH; ++y) {
					for (int32_t x = 0; x < overlapW; ++x) {
						const voxel::Voxel srcVoxel = worldSource->voxel(
							srcLower.x + x, srcLower.y + y, srcLower.z + z);
						if (voxel::isAir(srcVoxel.getMaterial())) {
							continue;
						}
						const int idx = palLookup.findClosestIndex(
							sourcePalette.color(srcVoxel.getColor()));
						const voxel::Voxel destVoxel = voxel::createVoxel(sourcePalette,
							idx == palette::PaletteColorNotFound ? 0 : idx);
						newVolume->setVoxel(srcLower.x + x, srcLower.y + y, srcLower.z + z,
							destVoxel);
						++count;
					}
				}
			}

			if (count == 0) {
				delete newVolume;
				continue;
			}

			scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
			newNode.setVolume(newVolume);
			newNode.setPalette(sourcePalette);
			newNode.setName("merged");
			const int newNodeId = moveNodeToSceneGraph(newNode, backgroundParentId);
			if (newNodeId != InvalidNodeId) {
				stampedNodes.push_back(StampedNode{newNodeId, cellSourceOverlap});
				stampedCount += count;
			}
		}
	}

	if (stampedCount == 0) {
		Log::warn("mergeactivetobackground: no voxels stamped for active node %i", sourceNodeId);
		return false;
	}

	// Show all hidden model nodes and unhide them in the mesh state so that
	// scheduleRegionExtraction does not skip extraction for hidden volumes
	for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		if (node.id() != sourceNodeId && !node.visible()) {
			nodeSetVisible(node.id(), true);
			_sceneRenderer->unhideNode(node.id());
		}
	}

	// Mark modified after nodes are visible so renderer processes the mesh updates
	for (const StampedNode &entry : stampedNodes) {
		modified(entry.nodeId, entry.region, SceneModifiedFlags::All);
	}

	_mementoHandler.markNodeRemove(_sceneGraph, *sourceNode);
	if (_sceneGraph.removeNode(sourceNodeId, false)) {
		_sceneRenderer->removeNode(sourceNodeId);
	}
	return true;
}

int SceneManager::mergeVisibleToTemp() {
	core::Buffer<int> visibleNodeIds;
	visibleNodeIds.reserve(_sceneGraph.size());
	for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (node.visible() && node.isModelNode()) {
			visibleNodeIds.push_back(node.id());
		}
	}

	if (visibleNodeIds.size() <= 1) {
		Log::warn("mergevisibletotemp: need at least 2 visible model nodes");
		return InvalidNodeId;
	}

	// Bake all visible nodes into world space and merge
	scenegraph::SceneGraph tempSceneGraph;
	for (int nodeId : visibleNodeIds) {
		const scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
		if (node == nullptr) {
			continue;
		}
		const scenegraph::FrameTransform &transform =
			_sceneGraph.transformForFrame(*node, _currentFrameIdx);
		const voxel::RawVolume *srcVolume = _sceneGraph.resolveVolume(*node);
		voxel::RawVolume *bakedVolume =
			voxelutil::applyTransformToVolume(*srcVolume, transform.worldMatrix(), node->pivot());
		if (bakedVolume == nullptr) {
			continue;
		}
		scenegraph::SceneGraphNode copiedNode(scenegraph::SceneGraphNodeType::Model);
		scenegraph::copyNode(*node, copiedNode, false, false);
		copiedNode.setVolume(bakedVolume);
		tempSceneGraph.emplace(core::move(copiedNode));
	}
	const scenegraph::SceneGraph::MergeResult &merged = tempSceneGraph.merge();
	if (!merged.hasVolume()) {
		Log::warn("mergevisibletotemp: merge produced no volume");
		return InvalidNodeId;
	}

	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "mergevisibletotemp");

	// Hide all source nodes
	for (int nodeId : visibleNodeIds) {
		nodeSetVisible(nodeId, false);
	}

	// Create the merged temporary node
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	newNode.setVolume(merged.volume());
	newNode.setPalette(merged.palette);
	newNode.setNormalPalette(merged.normalPalette);
	newNode.setName(_("merged_temp"));
	newNode.setVisible(true);

	const int newNodeId = moveNodeToSceneGraph(newNode);
	if (newNodeId == InvalidNodeId) {
		// Restore visibility on failure
		for (int nodeId : visibleNodeIds) {
			nodeSetVisible(nodeId, true);
		}
		Log::warn("mergevisibletotemp: failed to create merged node");
		return InvalidNodeId;
	}

	nodeActivate(newNodeId);
	return newNodeId;
}

void SceneManager::selectionInvert(int nodeId) {
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return;
	}
	// TODO: SELECTION: Box3D selection region is not updated here - and should we? there is no single region anymore
	// for inverting a box3d region. Maybe we should switch from box3d to another selection mode once invert was triggered in box3d mode?
	node->invertSelection();
	// Mark mesh dirty to trigger re-extraction with updated FlagOutline
	modified(nodeId, node->region(), SceneModifiedFlags::NoUndo);
}

void SceneManager::selectionUnselect(int nodeId) {
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return;
	}
	// Only re-extract where voxels actually had FlagOutline set
	const voxel::Region dirtyRegion = selectionCalculateRegion(*node);
	node->clearSelection();
	_modifierFacade.selectBrush().setBox3DSelectionRegion(voxel::Region::InvalidRegion);
	modified(nodeId, dirtyRegion.isValid() ? dirtyRegion : node->region(), SceneModifiedFlags::NoUndo);
}

void SceneManager::selectionSelectAll(int nodeId) {
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return;
	}
	voxel::RawVolume *volume = node->volume();
	if (volume == nullptr) {
		return;
	}
	node->select(volume->region());
	if (_modifierFacade.selectBrush().selectMode() == SelectMode::Box3D) {
		_modifierFacade.selectBrush().setBox3DSelectionRegion(volume->region());
	}
	// Mark mesh dirty to trigger re-extraction with updated FlagOutline
	modified(nodeId, node->region(), SceneModifiedFlags::NoUndo);
}

bool SceneManager::hasSelection(int nodeId) const {
	return selectionCalculateRegion(nodeId).isValid();
}

bool SceneManager::isSelected(int nodeId, const glm::ivec3 &pos) const {
	const scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
	if (node == nullptr || !node->isModelNode()) {
		return false;
	}
	const voxel::RawVolume *volume = node->volume();
	if (volume == nullptr) {
		return false;
	}
	const voxel::Voxel &voxel = volume->voxel(pos);
	return (voxel.getFlags() & voxel::FlagOutline) != 0;
}

voxel::Region SceneManager::selectionCalculateRegion(const scenegraph::SceneGraphNode &node) const {
	if (_selectionRegionCache.valid(node.id())) {
		return *_selectionRegionCache.value(node.id());
	}
	const voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	const voxel::Region &region = volume->regionForFlag(voxel::FlagOutline);
	_selectionRegionCache.set(node.id(), region);
	return region;
}

voxel::Region SceneManager::selectionCalculateRegion(int nodeId) const {
	const scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	return selectionCalculateRegion(*node);
}

void SceneManager::selectionSetBounds(int nodeId, const voxel::Region &region) {
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return;
	}
	const voxel::Region &vol = node->region();
	const voxel::Region clamped(glm::max(region.getLowerCorner(), vol.getLowerCorner()),
								glm::min(region.getUpperCorner(), vol.getUpperCorner()));
	if (!clamped.isValid()) {
		return;
	}
	// Only re-extract the union of old + new selection: those are the only voxels
	// whose FlagOutline bit changes. Everything outside is unchanged, so no mesh
	// re-extraction is needed there.
	voxel::Region dirtyRegion = clamped;
	const voxel::Region oldRegion = selectionCalculateRegion(*node);
	if (oldRegion.isValid()) {
		dirtyRegion.accumulate(oldRegion);
	}
	node->clearSelection();
	node->select(clamped);
	SelectBrush &selectBrush = _modifierFacade.selectBrush();
	if (selectBrush.selectMode() == SelectMode::Box3D) {
		selectBrush.setBox3DSelectionRegion(clamped);
	}
	modified(nodeId, dirtyRegion, SceneModifiedFlags::NoUndo);
}

void SceneManager::selectionSetEllipse(int nodeId) {
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return;
	}
	SelectBrush &brush = _modifierFacade.selectBrush();
	if (!brush.ellipseValid()) {
		return;
	}
	const glm::ivec3 &center = brush.ellipseCenter();
	const int radiusU = brush.ellipseRadiusU();
	const int radiusV = brush.ellipseRadiusV();
	const int depth = brush.ellipseDepth();
	const voxel::FaceNames face = brush.ellipseFace();
	int uAxis;
	int vAxis;
	SelectBrush::ellipseAxes(face, uAxis, vAxis);
	const int faceAxisIdx = math::getIndexForAxis(voxel::faceToAxis(face));
	const bool is3D = brush.ellipse3D();
	const bool positiveNormal = voxel::isPositiveFace(face);

	// Calculate the bounding region of the new ellipse
	const voxel::Region &vol = node->region();
	glm::ivec3 mins = center;
	glm::ivec3 maxs = center;
	mins[uAxis] -= radiusU;
	maxs[uAxis] += radiusU;
	mins[vAxis] -= radiusV;
	maxs[vAxis] += radiusV;
	if (positiveNormal) {
		mins[faceAxisIdx] -= depth;
	} else {
		maxs[faceAxisIdx] += depth;
	}
	voxel::Region ellipseRegion(mins, maxs);
	ellipseRegion.cropTo(vol);

	voxel::RawVolume *volume = node->volume();

	// Clear only the positions flagged by the previous ellipse (not all selections)
	voxel::Region dirtyRegion = ellipseRegion;
	core::DynamicArray<glm::ivec3> &history = brush.ellipseHistory();
	for (const glm::ivec3 &pos : history) {
		const voxel::Voxel &v = volume->voxel(pos);
		if (!voxel::isAir(v.getMaterial())) {
			voxel::Voxel modified = v;
			modified.setFlags(modified.getFlags() & ~voxel::FlagOutline);
			volume->setVoxel(pos, modified);
			dirtyRegion.accumulate(pos);
		}
	}
	history.clear();

	// Apply the new ellipse selection and record positions
	auto selectFunc = [&](int x, int y, int z, const voxel::Voxel &v) {
		const glm::ivec3 pos(x, y, z);
		if (SelectBrush::insideSelection(pos, center, radiusU, radiusV, depth, is3D,
										 uAxis, vAxis, faceAxisIdx, positiveNormal)) {
			voxel::Voxel modified = v;
			modified.setFlags(modified.getFlags() | voxel::FlagOutline);
			volume->setVoxel(x, y, z, modified);
			history.push_back(pos);
		}
	};
	if (depth > 1) {
		voxelutil::VisitSolid condition;
		voxelutil::visitVolume(*volume, ellipseRegion, selectFunc, condition);
	} else {
		voxelutil::visitSurfaceVolume(*volume, selectFunc);
	}

	modified(nodeId, dirtyRegion, SceneModifiedFlags::NoUndo);
}

void SceneManager::selectionFinalizeLasso(int nodeId) {
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return;
	}
	SelectBrush &brush = _modifierFacade.selectBrush();
	if (!brush.lassoAccumulating() || brush.lassoPath().size() < 3) {
		return;
	}
	// Copy path and axes before invalidating the brush (invalidateLasso clears them)
	const core::DynamicArray<glm::ivec3> path = brush.lassoPath();
	const int uAxis = brush.lassoUAxis();
	const int vAxis = brush.lassoVAxis();

	voxel::RawVolume *volume = node->volume();
	voxel::Region dirtyRegion = voxel::Region::InvalidRegion;

	// Restore edge history voxels first so they appear in the dirty region.
	// This ensures undo also reverts edge marks outside the polygon perimeter.
	if (brush.hasPendingChanges()) {
		dirtyRegion.accumulate(brush.revertChanges(volume));
	}
	brush.invalidateLasso();

	auto selectFunc = [&](int x, int y, int z, const voxel::Voxel &v) {
		const glm::ivec3 pos(x, y, z);
		if (!voxelutil::lassoContains(path, pos[uAxis], pos[vAxis], uAxis, vAxis)) {
			return;
		}
		voxel::Voxel modified = v;
		modified.setFlags(modified.getFlags() | voxel::FlagOutline);
		volume->setVoxel(pos, modified);
		dirtyRegion.accumulate(pos);
	};
	voxelutil::visitSurfaceVolume(*volume, selectFunc);

	if (dirtyRegion.isValid()) {
		modified(nodeId, dirtyRegion, SceneModifiedFlags::All);
	}
}

void SceneManager::selectionCancelLasso(int nodeId) {
	SelectBrush &brush = _modifierFacade.selectBrush();
	if (brush.hasPendingChanges()) {
		scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
		if (node != nullptr) {
			voxel::RawVolume *volume = node->volume();
			const voxel::Region dirtyRegion = brush.revertChanges(volume);
			if (dirtyRegion.isValid()) {
				modified(nodeId, dirtyRegion, SceneModifiedFlags::NoUndo);
			}
		}
	}
	brush.invalidateLasso();
}

void SceneManager::selectionLassoUndoVertex(int nodeId) {
	SelectBrush &brush = _modifierFacade.selectBrush();
	if (!brush.lassoAccumulating() || brush.lassoPath().size() < 2) {
		return;
	}
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return;
	}
	voxel::RawVolume *volume = node->volume();
	voxel::Region dirtyRegion = voxel::Region::InvalidRegion;

	// Restore all existing edge marks to their original state
	if (brush.hasPendingChanges()) {
		dirtyRegion.accumulate(brush.revertChanges(volume));
	}

	// Drop the last vertex and redraw the remaining edges
	brush.popLastLassoPathEntry();
	if (brush.lassoPath().size() >= 2) {
		brush.redrawEdgesOnVolume(volume, volume->region(), dirtyRegion);
	}

	if (dirtyRegion.isValid()) {
		modified(nodeId, dirtyRegion, SceneModifiedFlags::NoUndo);
	}
}

void SceneManager::resetLastTrace() {
	if (!_traceViaMouse) {
		return;
	}
	_lastRaytraceX = _lastRaytraceY = -1;
}

static bool shouldGetMerged(const scenegraph::SceneGraphNode &node, NodeMergeFlags flags) {
	bool add = false;
	if ((flags & NodeMergeFlags::Visible) == NodeMergeFlags::Visible) {
		add = node.visible();
	} else if ((flags & NodeMergeFlags::Invisible) == NodeMergeFlags::Invisible) {
		add = !node.visible();
	} else if ((flags & NodeMergeFlags::Locked) == NodeMergeFlags::Locked) {
		add = node.locked();
	} else if ((flags & NodeMergeFlags::All) == NodeMergeFlags::All) {
		add = true;
	}
	return add;
}

int SceneManager::mergeNodes(const core::Buffer<int>& nodeIds) {
	scenegraph::SceneGraph newSceneGraph;
	for (int nodeId : nodeIds) {
		const scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
		if (node == nullptr || !node->isAnyModelNode()) {
			continue;
		}
		const scenegraph::FrameTransform &transform = _sceneGraph.transformForFrame(*node, _currentFrameIdx);
		const voxel::RawVolume *srcVolume = _sceneGraph.resolveVolume(*node);
		voxel::RawVolume *bakedVolume =
		voxelutil::applyTransformToVolume(*srcVolume, transform.worldMatrix(), node->pivot());
		if (bakedVolume == nullptr) {
			continue;
		}
		scenegraph::SceneGraphNode copiedNode(scenegraph::SceneGraphNodeType::Model);
		scenegraph::copyNode(*node, copiedNode, false, false);
		copiedNode.setVolume(bakedVolume);
		newSceneGraph.emplace(core::move(copiedNode));
	}
	const scenegraph::SceneGraph::MergeResult &merged = newSceneGraph.merge();
	if (!merged.hasVolume()) {
		return InvalidNodeId;
	}

	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "merge");
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	newNode.setVolume(merged.volume());
	newNode.setPalette(merged.palette);
	newNode.setNormalPalette(merged.normalPalette);
	int parent = 0;
	if (const scenegraph::SceneGraphNode* firstNode = sceneGraphNode(nodeIds.front())) {
		newNode.setName(firstNode->name());
		newNode.setVisible(firstNode->visible());
		newNode.setLocked(firstNode->locked());
		newNode.setPivot(firstNode->pivot());
		newNode.setColor(firstNode->color());
		newNode.addProperties(firstNode->properties());
		parent = firstNode->parent();
	}

	int newNodeId = moveNodeToSceneGraph(newNode, parent);
	if (newNodeId == InvalidNodeId) {
		return newNodeId;
	}
	for (int nodeId : nodeIds) {
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			_mementoHandler.markNodeRemove(_sceneGraph, *node);
		}
	}
	for (int nodeId : nodeIds) {
		if (_sceneGraph.removeNode(nodeId, false)) {
			_sceneRenderer->removeNode(nodeId);
		}
	}
	return newNodeId;
}

int SceneManager::mergeNodes(NodeMergeFlags flags) {
	core::Buffer<int> nodeIds;
	nodeIds.reserve(_sceneGraph.size());
	for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (!shouldGetMerged(node, flags)) {
			continue;
		}
		nodeIds.push_back(node.id());
	}

	if (nodeIds.size() <= 1) {
		return InvalidNodeId;
	}

	return mergeNodes(nodeIds);
}

int SceneManager::mergeNodes(int nodeId1, int nodeId2) {
	if (!_sceneGraph.hasNode(nodeId1) || !_sceneGraph.hasNode(nodeId2)) {
		return InvalidNodeId;
	}
	voxel::RawVolume *volume1 = volume(nodeId1);
	if (volume1 == nullptr) {
		return InvalidNodeId;
	}
	voxel::RawVolume *volume2 = volume(nodeId2);
	if (volume2 == nullptr) {
		return InvalidNodeId;
	}
	core::Buffer<int> nodeIds(2);
	nodeIds[0] = nodeId1;
	nodeIds[1] = nodeId2;
	return mergeNodes(nodeIds);
}

void SceneManager::resetSceneState() {
	// this also resets the cursor voxel - but nodeActivate() will set it to the first usable index
	// that's why this call must happen before the nodeActivate() call.
	_modifierFacade.reset();
	int nodeId = (*_sceneGraph.beginModel()).id();
	for (const auto &entry : _sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (node.isModelNode() && node.visible()) {
			nodeId = node.id();
			break;
		}
	}
	_selectionRegionCache.invalidate();
	scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	// ensure that the first model node is active and re-select the first model node.
	// therefore we first "select" the root node, then switch back to the first model node.
	_sceneGraph.setActiveNode(_sceneGraph.root().id());
	nodeActivate(nodeId);
	_mementoHandler.clearStates();
	Log::debug("New volume for node %i", nodeId);
	_mementoHandler.markInitialSceneState(_sceneGraph);
	_dirty = false;
	_result = voxelutil::PickResult();
	_modifierFacade.setCursorVoxel(voxel::createVoxel(node.palette(), 0));
	{
		// TODO: happens in nodeActivate already
		setCursorPosition(cursorPosition(), voxel::FaceNames::Max, true);
		setReferencePosition(node.region().getCenter());
		resetLastTrace();
	}
	if (server().isRunning()) {
		SceneStateMessage msg(_sceneGraph);
		server().network().broadcast(msg, 0);
	}
}

void SceneManager::onNewNodeAdded(int newNodeId, bool isChildren) {
	if (newNodeId == InvalidNodeId) {
		return;
	}

	if (!isChildren) {
		_sceneGraph.updateTransforms();
	}

	if (scenegraph::SceneGraphNode *node = sceneGraphNode(newNodeId)) {
		const core::String &name = node->name();
		const scenegraph::SceneGraphNodeType type = node->type();
		const core::String &uuidStr = node->uuid().str();
		Log::debug("Adding node %i with name %s (type: %s, uuid: %s)", newNodeId, name.c_str(),
				   scenegraph::SceneGraphNodeTypeStr[(int)type], uuidStr.c_str());

		_mementoHandler.markNodeAdded(_sceneGraph, *node);

		for (int childId : node->children()) {
			onNewNodeAdded(childId, true);
		}

		markDirty();

		Log::debug("Add node %i to scene graph", newNodeId);
		if (type == scenegraph::SceneGraphNodeType::Model) {
			const voxel::Region &region = node->region();
			// update the whole volume
			_sceneRenderer->updateNodeRegion(newNodeId, region);

			_result = voxelutil::PickResult();
			if (!isChildren) {
				nodeActivate(newNodeId);
			}
		}
	}
}

int SceneManager::moveNodeToSceneGraph(scenegraph::SceneGraphNode &node, int parent) {
	const int newNodeId = scenegraph::moveNodeToSceneGraph(_sceneGraph, node, parent);
	onNewNodeAdded(newNodeId, false);
	return newNodeId;
}

bool SceneManager::loadSceneGraph(scenegraph::SceneGraph&& sceneGraph, bool disconnect) {
	core_trace_scoped(LoadSceneGraph);

	if (disconnect && client().isConnected() && !server().isRunning()) {
		disconnectFromServer();
	}

	_sceneGraph = core::move(sceneGraph);
	_sceneRenderer->clear();
	// stop any running animation
	core::getVar(cfg::VoxEditAnimationPlaying)->setVal(false);
	command::executeCommands("animate 0");

	const size_t nodesAdded = _sceneGraph.size();
	if (nodesAdded == 0) {
		const int modelSize = 128;
		if (_sceneGraph.empty(scenegraph::SceneGraphNodeType::Point)) {
			Log::warn("Failed to load any model volumes");
			const voxel::Region &region = voxel::Region::fromSize(modelSize);
			newScene(true, "", region);
			return false;
		} else {
			// only found points, let's create a new model node to please the editor
			addModelChild("", modelSize, modelSize, modelSize);
		}
	}
	resetSceneState();
	return true;
}

bool SceneManager::splitVolumes() {
	const int splitSize = _maxSuggestedVolumeSize->intVal();
	const glm::ivec3 maxSize(splitSize);
	scenegraph::SceneGraph newSceneGraph;
	if (scenegraph::splitVolumes(_sceneGraph, newSceneGraph, false, false, false, maxSize)) {
		return loadSceneGraph(core::move(newSceneGraph));
	}

	scenegraph::SceneGraph newSceneGraph2;
	if (scenegraph::splitVolumes(_sceneGraph, newSceneGraph2, false, true, false, maxSize)) {
		return loadSceneGraph(core::move(newSceneGraph2));
	}
	return false;
}

const scenegraph::SceneGraphNode *SceneManager::sceneGraphModelNode(int nodeId) const {
	const scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
	if (node == nullptr || node->type() != scenegraph::SceneGraphNodeType::Model) {
		return nullptr;
	}
	return node;
}

scenegraph::SceneGraphNode *SceneManager::sceneGraphModelNode(int nodeId) {
	scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
	if (node == nullptr || node->type() != scenegraph::SceneGraphNodeType::Model) {
		return nullptr;
	}
	return node;
}

scenegraph::SceneGraphNode *SceneManager::sceneGraphNode(int nodeId) {
	if (_sceneGraph.hasNode(nodeId)) {
		return &_sceneGraph.node(nodeId);
	}
	return nullptr;
}

scenegraph::SceneGraphNode *SceneManager::sceneGraphNodeByUUID(const core::UUID &uuid) {
	return _sceneGraph.findNodeByUUID(uuid);
}

const scenegraph::SceneGraphNode *SceneManager::sceneGraphNode(int nodeId) const {
	if (_sceneGraph.hasNode(nodeId)) {
		return &_sceneGraph.node(nodeId);
	}
	return nullptr;
}

const scenegraph::SceneGraph &SceneManager::sceneGraph() const {
	return _sceneGraph;
}

scenegraph::SceneGraph &SceneManager::sceneGraph() {
	return _sceneGraph;
}

bool SceneManager::setAnimation(const core::String &animation) {
	return _sceneGraph.setAnimation(animation);
}

bool SceneManager::addAnimation(const core::String &animation) {
	if (_sceneGraph.addAnimation(animation)) {
		_mementoHandler.markAnimationAdded(_sceneGraph, animation);
		return true;
	}
	return false;
}

bool SceneManager::duplicateAnimation(const core::String &animation, const core::String &newName) {
	if (_sceneGraph.duplicateAnimation(animation, newName)) {
		_mementoHandler.markAnimationAdded(_sceneGraph, animation);
		return true;
	}
	return false;
}

bool SceneManager::removeAnimation(const core::String &animation) {
	if (_sceneGraph.removeAnimation(animation)) {
		_mementoHandler.markAnimationRemoved(_sceneGraph, animation);
		return true;
	}
	return false;
}

bool SceneManager::setNewVolume(int nodeId, voxel::RawVolume* volume, bool deleteMesh) {
	core_trace_scoped(SetNewVolume);
	scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		return false;
	}
	return setSceneGraphNodeVolume(*node, volume);
}

bool SceneManager::setSceneGraphNodeVolume(scenegraph::SceneGraphNode &node, voxel::RawVolume* volume) {
	if (node.type() != scenegraph::SceneGraphNodeType::Model) {
		return false;
	}
	if (node.volume() == volume) {
		return true;
	}

	node.setVolume(volume);
	// the old volume pointer might no longer be used
	_sceneRenderer->removeNode(node.id());

	const voxel::Region& region = volume->region();
	_sceneRenderer->updateGridRegion(region);

	_dirty = false;
	_result = voxelutil::PickResult();
	setCursorPosition(cursorPosition(), _modifierFacade.cursorFace(), true);
	setReferencePosition(region.getLowerCenter());
	resetLastTrace();
	return true;
}

bool SceneManager::newScene(bool force, const core::String &name, voxel::RawVolume *v) {
	_sceneGraph.clear();
	_sceneRenderer->clear();

	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	newNode.setVolume(v);
	if (name.empty()) {
		newNode.setName("unnamed");
	} else {
		newNode.setName(name);
	}
	palette::Palette palette;
	const core::String &paletteName = core::getVar(cfg::VoxEditLastPalette)->strVal();
	if (!palette.load(paletteName.c_str())) {
		palette = voxel::getPalette();
	}
	newNode.setPalette(palette);
	const int nodeId = scenegraph::moveNodeToSceneGraph(_sceneGraph, newNode, 0);
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to add empty volume to new scene graph");
		return false;
	}
	setReferencePosition(v->region().getLowerCenter());
	resetSceneState();
	_lastFilename.clear();
	return true;
}

bool SceneManager::newScene(bool force, const core::String& name, const voxel::Region& region) {
	if (dirty() && !force) {
		return false;
	}
	voxel::RawVolume* v = new voxel::RawVolume(region);
	return newScene(force, name, v);
}

void SceneManager::nodeGroupRotate(math::Axis axis) {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphNode(groupNodeId);
		if (node == nullptr) {
			return;
		}
		const voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		voxel::RawVolume *newVolume = voxelutil::rotateAxis(v, axis);
		if (newVolume == nullptr) {
			return;
		}
		glm::vec3 pivot = node->pivot();
		voxel::Region r = newVolume->region();
		r.accumulate(v->region());
		setSceneGraphNodeVolume(*node, newVolume);
		modified(groupNodeId, r);
		for (const auto &kfAnim : node->allKeyFrames()) {
			for (auto &kf : kfAnim->second) {
				scenegraph::SceneGraphTransform &transform = kf.transform();
				transform.rotate(axis);
			}
		}
		_sceneGraph.updateTransforms();
		const int idx1 = (math::getIndexForAxis(axis) + 1) % 3;
		const int idx2 = (idx1 + 1) % 3;
		core::exchange(pivot[idx1], pivot[idx2]);
		node->setPivot(pivot);
		_mementoHandler.markKeyFramesChange(_sceneGraph, *node);
	});
}

static glm::vec3 rotateVec3AroundPivot(const glm::vec3 &pos, const glm::vec3 &pivot, math::Axis axis) {
	const glm::vec3 rel = pos - pivot;
	glm::vec3 result = pos;
	if (axis == math::Axis::Y) {
		// CCW in XZ plane - matches voxelutil::rotateAxis(Y) voxel mapping
		result.x = pivot.x - rel.z;
		result.z = pivot.z + rel.x;
	} else if (axis == math::Axis::X) {
		// CW in YZ plane - matches voxelutil::rotateAxis(X) voxel mapping
		result.y = pivot.y + rel.z;
		result.z = pivot.z - rel.y;
	} else {
		// CW in XY plane - matches voxelutil::rotateAxis(Z) voxel mapping
		result.x = pivot.x + rel.y;
		result.y = pivot.y - rel.x;
	}
	return result;
}

void SceneManager::nodeRotateAll(math::Axis axis) {
	const voxel::Region sceneReg = _sceneGraph.sceneRegion(_currentFrameIdx);
	if (!sceneReg.isValid()) {
		return;
	}
	const glm::vec3 scenePivot = sceneReg.calcCenterf();

	for (auto *e : _sceneGraph.nodes()) {
		scenegraph::SceneGraphNode &node = e->value;
		if (!node.isModelNode()) {
			continue;
		}
		const voxel::RawVolume *v = node.volume();
		if (v == nullptr) {
			continue;
		}
		voxel::RawVolume *newVolume = voxelutil::rotateAxis(v, axis);
		if (newVolume == nullptr) {
			continue;
		}
		const glm::vec3 oldRegionCenter = v->region().calcCenterf();
		const glm::vec3 newRegionCenter = newVolume->region().calcCenterf();

		glm::vec3 pivot = node.pivot();
		voxel::Region r = newVolume->region();
		r.accumulate(v->region());
		setSceneGraphNodeVolume(node, newVolume);
		modified(node.id(), r);

		for (const auto &kfAnim : node.allKeyFrames()) {
			for (auto &kf : kfAnim->second) {
				scenegraph::SceneGraphTransform &transform = kf.transform();
				// Rotate the volume's world-space center around the scene pivot,
				// then back-compute the new translation so the center lands correctly.
				const glm::vec3 worldCenter = transform.worldTranslation() + oldRegionCenter;
				const glm::vec3 newWorldCenter = rotateVec3AroundPivot(worldCenter, scenePivot, axis);
				transform.setWorldTranslation(newWorldCenter - newRegionCenter);
			}
		}
		_sceneGraph.updateTransforms();

		const int idx1 = (math::getIndexForAxis(axis) + 1) % 3;
		const int idx2 = (idx1 + 1) % 3;
		core::exchange(pivot[idx1], pivot[idx2]);
		node.setPivot(pivot);
		_mementoHandler.markKeyFramesChange(_sceneGraph, node);
	}
}

void SceneManager::nodeMoveVoxels(int nodeId, const glm::ivec3& m) {
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		return;
	}
	if (hasSelection(nodeId)) {
		// Move only the selected voxels (those with FlagOutline set)
		const voxel::Region &region = v->region();
		const glm::ivec3 &mins = region.getLowerCorner();
		const glm::ivec3 &maxs = region.getUpperCorner();

		// First pass: collect selected voxels and clear them
		core::DynamicArray<glm::ivec3> positions;
		core::DynamicArray<voxel::Voxel> voxels;
		for (int32_t z = mins.z; z <= maxs.z; ++z) {
			for (int32_t y = mins.y; y <= maxs.y; ++y) {
				for (int32_t x = mins.x; x <= maxs.x; ++x) {
					const voxel::Voxel &voxel = v->voxel(x, y, z);
					if ((voxel.getFlags() & voxel::FlagOutline) != 0) {
						positions.push_back(glm::ivec3(x, y, z));
						voxels.push_back(voxel);
						v->setVoxel(x, y, z, voxel::Voxel());
					}
				}
			}
		}

		// Second pass: place voxels at new positions
		for (size_t i = 0; i < positions.size(); ++i) {
			const glm::ivec3 newPos = positions[i] + m;
			if (region.containsPoint(newPos)) {
				v->setVoxel(newPos, voxels[i]);
			}
		}
	} else {
		v->move(m);
	}
	modified(nodeId, v->region());
}

void SceneManager::nodeGroupMoveVoxels(int x, int y, int z) {
	const glm::ivec3 v(x, y, z);
	nodeForeachGroup([&] (int groupNodeId) {
		nodeMoveVoxels(groupNodeId, v);
	});
}

void SceneManager::nodeShift(int nodeId, const glm::ivec3& m) {
	scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		return;
	}
	voxel::RawVolume *v = node->volume();
	if (v == nullptr) {
		return;
	}
	voxel::Region region = v->region();
	v->translate(m);
	region.accumulate(v->region());
	_dirtyRenderer = DirtyRendererLockedAxis | DirtyRendererGridRenderer;
	modified(nodeId, region);
}

void SceneManager::nodeGroupShift(int x, int y, int z) {
	const glm::ivec3 v(x, y, z);
	nodeForeachGroup([&] (int groupNodeId) {
		nodeShift(groupNodeId, v);
	});
}

bool SceneManager::setGridResolution(int resolution) {
	if (_modifierFacade.gridResolution() == resolution) {
		return false;
	}
	_modifierFacade.setGridResolution(resolution);
	setCursorPosition(cursorPosition(), _modifierFacade.cursorFace(), true);
	return true;
}

void SceneManager::render(voxelrender::RenderContext &renderContext, const video::Camera& camera, uint8_t renderMask) {
	renderContext.frameBuffer.bind(true);
	_sceneRenderer->updateLockedPlanes(_modifierFacade.lockedAxis(), _sceneGraph, cursorPosition());

	renderContext.frame = _currentFrameIdx;
	renderContext.sceneGraph = &_sceneGraph;

	const bool renderScene = (renderMask & RenderScene) != 0u;
	if (renderScene) {
		_sceneRenderer->renderScene(renderContext, camera);
	}
	const bool renderUI = (renderMask & RenderUI) != 0u;
	if (renderUI) {
		_sceneRenderer->renderUI(renderContext, camera);
		if (renderContext.isEditMode()) {
			const glm::mat4 &mat = worldMatrix(renderContext.frame, renderContext.applyTransforms());
			_modifierFacade.render(camera, activePalette(), mat);
		}
	}

	// If multisampling is enabled, resolve the multisampled framebuffer before unbinding
	if (renderContext.enableMultisampling) {
		const glm::ivec2 fbDim = renderContext.frameBuffer.dimension();
		video::blitFramebuffer(renderContext.frameBuffer.handle(), renderContext.resolveFrameBuffer.handle(),
							 video::ClearFlag::Color, fbDim.x, fbDim.y);
	}

	renderContext.frameBuffer.unbind();
}

int SceneManager::toNodeId(const command::CommandArgs& args, int defaultVal, const core::String &name) const {
	if (!args.has(name)) {
		return defaultVal;
	}
	const core::String &nodeId = args.str(name);
	core::UUID uuid(nodeId);
	if (uuid.isValid()) {
		const scenegraph::SceneGraphNode* node = _sceneGraph.findNodeByUUID(uuid);
		if (node != nullptr) {
			return node->id();
		}
	}
	return args.intVal(name, defaultVal);
}

void SceneManager::construct() {
	const core::VarDef voxEditColorWheel(cfg::VoxEditColorWheel, false, N_("Color wheel"), N_("Use the color wheel in the palette color editing"));
	core::Var::registerVar(voxEditColorWheel);
	const core::VarDef voxEditShowColorPicker(cfg::VoxEditShowColorPicker, false, N_("Color picker"), N_("Always show the color picker below the palette"));
	core::Var::registerVar(voxEditShowColorPicker);
	const core::VarDef voxEditModificationDismissMillis(cfg::VoxEditModificationDismissMillis, 1500, N_("Modification highlight"), N_("Milliseconds that a region should get highlighted in a few situations"));
	core::Var::registerVar(voxEditModificationDismissMillis);
	const core::VarDef voxEditRegionSizes(cfg::VoxEditRegionSizes, "", N_("Region sizes"), N_("Show fixed region sizes in the volume inspector"));
	core::Var::registerVar(voxEditRegionSizes);
	const core::VarDef voxEditLocalSpace(cfg::VoxEditLocalSpace, true, N_("Local transforms"), N_("Use local space for transforms"));
	core::Var::registerVar(voxEditLocalSpace);
	const core::VarDef voxEditShowgrid(cfg::VoxEditShowgrid, true, N_("Grid"), N_("Show the grid"));
	core::Var::registerVar(voxEditShowgrid);
	const core::VarDef voxEditShowlockedaxis(cfg::VoxEditShowlockedaxis, true, N_("Show locked axis"), N_("Show the currently locked axis"));
	core::Var::registerVar(voxEditShowlockedaxis);
	const core::VarDef voxEditShowaabb(cfg::VoxEditShowaabb, true, N_("Bounding box"), N_("Show the axis aligned bounding box"));
	core::Var::registerVar(voxEditShowaabb);
	const core::VarDef voxEditShowBones(cfg::VoxEditShowBones, false, N_("Bones"), N_("Show the bones in scene mode"));
	core::Var::registerVar(voxEditShowBones);
	const core::VarDef voxEditRendershadow(cfg::VoxEditRendershadow, false, N_("Render shadows"), N_("Render with shadows - make sure to set the scene lighting up properly"));
	core::Var::registerVar(voxEditRendershadow);
	const core::VarDef voxEditShadingMode(cfg::VoxEditShadingMode, 1, 0, 2, N_("Shading mode"), N_("Shading mode: 0=Unlit (pure colors), 1=Lit (no shadows), 2=Shadows"));
	core::Var::registerVar(voxEditShadingMode);
	const core::VarDef voxEditAnimationSpeed(cfg::VoxEditAnimationSpeed, 100, N_("Model animation speed"), N_("Millisecond delay between frames hide/unhide when using the scene graph panel play button to animate the models in the scene"));
	core::Var::registerVar(voxEditAnimationSpeed);
	const core::VarDef voxEditAutoNormalMode(cfg::VoxEditAutoNormalMode, 0, 0, 2, N_("Auto normal mode"), "Flat, Smooth, Smoother", core::CV_NOPERSIST);
	core::Var::registerVar(voxEditAutoNormalMode);
	const core::VarDef voxEditGridsize(cfg::VoxEditGridsize, 1, 1, 64, N_("Grid size"), N_("The size of the voxel grid"));
	core::Var::registerVar(voxEditGridsize);
	const core::VarDef voxEditPlaneSize(cfg::VoxEditPlaneSize, 100, 1, 1000, N_("Plane size"), N_("The size of the plane"));
	core::Var::registerVar(voxEditPlaneSize);
	const core::VarDef voxEditShowPlane(cfg::VoxEditShowPlane, true, N_("Plane"), N_("Show the plane"));
	core::Var::registerVar(voxEditShowPlane);
	const core::VarDef voxEditGrayInactive(cfg::VoxEditGrayInactive, false, N_("Grayscale"), N_("Render the inactive nodes in gray scale mode"));
	core::Var::registerVar(voxEditGrayInactive);
	const core::VarDef voxEditHideInactive(cfg::VoxEditHideInactive, false, N_("Only active"), N_("Hide the inactive nodes"));
	core::Var::registerVar(voxEditHideInactive);
	const core::VarDef voxEditSceneMode(cfg::VoxEditSceneMode, false, N_("Scene mode"), N_("Start in scene mode"));
	core::Var::registerVar(voxEditSceneMode);
	const core::VarDef voxEditImportSingleNode(cfg::VoxEditImportSingleNode, false, N_("Import as single node"), N_("Merge all nodes into a single node when importing a file"));
	core::Var::registerVar(voxEditImportSingleNode);
	const core::VarDef voxEditContinueSession(cfg::VoxEditContinueSession, false, N_("Continue last session"), N_("Reopen the last file and restore camera position on startup"));
	core::Var::registerVar(voxEditContinueSession);
	core::Var::registerVar(core::VarDef(cfg::VoxEditLastCameraTarget, "", nullptr, nullptr));
	core::Var::registerVar(core::VarDef(cfg::VoxEditLastCameraAngles, "", nullptr, nullptr));
	core::Var::registerVar(core::VarDef(cfg::VoxEditLastCameraDistance, "", nullptr, nullptr));
	const core::VarDef voxEditViewdistance(cfg::VoxEditViewdistance, 5000, 10, 5000, N_("View distance"), N_("Far plane for the camera"));
	core::Var::registerVar(voxEditViewdistance);
	const core::VarDef voxEditShowaxis(cfg::VoxEditShowaxis, true, N_("Show gizmo"), N_("Show the axis"));
	core::Var::registerVar(voxEditShowaxis);
	const core::VarDef voxEditCursorDetails(cfg::VoxEditCursorDetails, 1, 0, 3, N_("Cursor details"), N_("Print cursor details in edit mode - measure distance to reference position in mode 3"));
	core::Var::registerVar(voxEditCursorDetails);
	const core::VarDef voxEditAutoKeyFrame(cfg::VoxEditAutoKeyFrame, true, N_("Auto Keyframe"), N_("Automatically create keyframes when changing transforms"));
	core::Var::registerVar(voxEditAutoKeyFrame);
	const core::VarDef voxEditGizmoOperations(cfg::VoxEditGizmoOperations, 3, N_("Gizmo operations"), N_("Bitmask of gizmo operations in scene mode"));
	core::Var::registerVar(voxEditGizmoOperations);
	const core::VarDef voxEditGizmoPivot(cfg::VoxEditGizmoPivot, false, N_("Pivot"), N_("Activate the pivot mode for the gizmo in scene mode"), core::CV_NOPERSIST);
	core::Var::registerVar(voxEditGizmoPivot);
	const core::VarDef voxEditGizmoAllowAxisFlip(cfg::VoxEditGizmoAllowAxisFlip, true, N_("Flip axis"), N_("Flip axis or stay along the positive world/local axis"));
	core::Var::registerVar(voxEditGizmoAllowAxisFlip);
	const core::VarDef voxEditGizmoSnap(cfg::VoxEditGizmoSnap, true, N_("Snap to grid"), N_("Use the grid size for snap"));
	core::Var::registerVar(voxEditGizmoSnap);
	const core::VarDef voxEditModelGizmo(cfg::VoxEditModelGizmo, false, N_("Model gizmo"), N_("Show the gizmo to also translate the region"));
	core::Var::registerVar(voxEditModelGizmo);
	const core::VarDef voxEditBrushGizmo(cfg::VoxEditBrushGizmo, true, N_("Brush gizmo"), N_("Show the gizmo for brushes that support it"));
	core::Var::registerVar(voxEditBrushGizmo);
	const core::VarDef voxEditLastPalette(cfg::VoxEditLastPalette, palette::Palette::builtIn[0], N_("Last palette"), N_("The last used palette"));
	core::Var::registerVar(voxEditLastPalette);
	const core::VarDef voxEditViewports(cfg::VoxEditViewports, 2, 1, cfg::MaxViewports, N_("Viewports"), N_("The amount of viewports (not in simple ui mode)"));
	core::Var::registerVar(voxEditViewports);
	const core::VarDef voxEditMaxSuggestedVolumeSize(cfg::VoxEditMaxSuggestedVolumeSize, 128, 32, voxedit::MaxVolumeSize, N_("Max volume size"), N_("The maximum size of a volume before a few features are disabled (e.g. undo/autosave)"));
	core::Var::registerVar(voxEditMaxSuggestedVolumeSize);
	const core::VarDef voxEditViewMode(cfg::VoxEditViewMode, "default", N_("View mode"), N_("Configure the editor view mode"));
	core::Var::registerVar(voxEditViewMode);
	const core::VarDef voxEditTipOftheDay(cfg::VoxEditTipOftheDay, true, N_("Tip of the day"), N_("Show the tip of the day on startup"));
	core::Var::registerVar(voxEditTipOftheDay);
	const core::VarDef voxEditPopupTipOfTheDay(cfg::VoxEditPopupTipOfTheDay, false, N_("Tip of the day popup"), N_("Trigger opening of opup"), core::CV_NOPERSIST);
	core::Var::registerVar(voxEditPopupTipOfTheDay);
	const core::VarDef voxEditPopupWelcome(cfg::VoxEditPopupWelcome, false, N_("Welcome popup"), N_("Trigger opening of popup"), core::CV_NOPERSIST);
	core::Var::registerVar(voxEditPopupWelcome);
	const core::VarDef voxEditPopupMinecraftMapping(cfg::VoxEditPopupMinecraftMapping, false, N_("Minecraft mapping popup"), N_("Trigger opening of popup"), core::CV_NOPERSIST);
	core::Var::registerVar(voxEditPopupMinecraftMapping);
	const core::VarDef voxEditPopupAbout(cfg::VoxEditPopupAbout, false, N_("About popup"), N_("Trigger opening of popup"), core::CV_NOPERSIST);
	core::Var::registerVar(voxEditPopupAbout);
	const core::VarDef voxEditPopupRenameNode(cfg::VoxEditPopupRenameNode, false, N_("Rename node popup"), N_("Trigger opening of popup"), core::CV_NOPERSIST);
	core::Var::registerVar(voxEditPopupRenameNode);
	const core::VarDef voxEditPopupCreateAnimation(cfg::VoxEditPopupCreateAnimation, false, N_("Create animation popup"), N_("Trigger opening of popup"), core::CV_NOPERSIST);
	core::Var::registerVar(voxEditPopupCreateAnimation);

	const core::VarDef voxEditAnimationPlaying(cfg::VoxEditAnimationPlaying, false, N_("Animation playing"), N_("Update the children of a node when the transform of the node changes"), core::CV_NOPERSIST);
	core::Var::registerVar(voxEditAnimationPlaying);
	const core::VarDef voxEditAutoSaveSeconds(cfg::VoxEditAutoSaveSeconds, 180, N_("Autosave delay in seconds"), N_("Delay in second between autosaves - 0 disables autosaves"));
	_autoSaveSecondsDelay = core::Var::registerVar(voxEditAutoSaveSeconds);
	const core::VarDef voxEditTransformUpdateChildren(cfg::VoxEditTransformUpdateChildren, true, N_("Update children"), N_("Update the children of a node when the transform of the node changes"));
	_transformUpdateChildren = core::Var::registerVar(voxEditTransformUpdateChildren);
	_maxSuggestedVolumeSize = core::getVar(cfg::VoxEditMaxSuggestedVolumeSize);
	_lastDirectory = core::getVar(cfg::UILastDirectory);

	voxelformat::FormatConfig::init();

	_modifierFacade.construct();
	_mementoHandler.construct();
	_sceneRenderer->construct();
	_camMovement.construct();
	_server.construct();
	_client.construct();
	_soundManager.construct();

	command::Command::registerCommand("resizetoselection")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to resize"})
		.setHandler([&](const command::CommandArgs &args) {
			const int activeNodeId = toNodeId(args, activeNode());
			scenegraph::SceneGraphNode *node = sceneGraphModelNode(activeNodeId);
			if (node == nullptr) {
				return;
			}
			const voxel::Region &region = selectionCalculateRegion(*node);
			nodeResize(activeNodeId, region);
		}).setHelp(_("Resize the volume to the current selection"));

	command::Command::registerCommand("xs")
		.addArg({"script", command::ArgType::String, false, "", "Lua generator script filename"})
		.addArg({"args", command::ArgType::String, true, "", "Script arguments"})
		.setHandler([&] (const command::CommandArgs& args) {
			const core::String &script = args.str("script");
			const core::String luaCode = _luaApi.load(script);
			if (luaCode.empty()) {
				Log::error("Failed to load %s", script.c_str());
				return;
			}

			core::DynamicArray<core::String> luaArgs;
			const core::String &scriptArgs = args.str("args");
			if (!scriptArgs.empty()) {
				luaArgs.push_back(scriptArgs);
			}

			if (!runScript(luaCode, luaArgs)) {
				Log::error("Failed to execute %s", script.c_str());
			} else {
				Log::info("Executed script %s", script.c_str());
			}
		}).setHelp(_("Executes a lua script"))
			.setArgumentCompleter(voxelgenerator::scriptCompleter(_filesystem));

	for (int i = 0; i < lengthof(DIRECTIONS); ++i) {
		command::Command::registerActionButton(
				core::String::format("movecursor%s", DIRECTIONS[i].postfix),
				_move[i], _("Move the cursor by keys, not by viewport mouse trace"));
	}
	command::Command::registerCommand("palette_changeintensity")
		.addArg({"value", command::ArgType::Float, false, "", "Intensity scale value"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to apply the intensity change to"})
		.setHandler([&] (const command::CommandArgs& args) {
			const float scale = args.floatVal("value");
			const int nodeId = toNodeId(args, activeNode());
			scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
			palette::Palette &pal = node.palette();
			pal.changeIntensity(scale);
			_mementoHandler.markPaletteChange(_sceneGraph, node);
		}).setHelp(_("Change intensity by scaling the rgb values of the palette"));

	command::Command::registerCommand("palette_warmer")
		.addArg({"value", command::ArgType::Int, true, "10", "Warmth adjustment value"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to apply the warmth to"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
			palette::Palette &pal = node.palette();
			const uint8_t val = (uint8_t)args.intVal("value", 10);
			pal.changeWarmer(val);
			_mementoHandler.markPaletteChange(_sceneGraph, node);
		}).setHelp(_("Make the palette colors warmer"));

	command::Command::registerCommand("palette_colder")
		.addArg({"value", command::ArgType::Int, true, "10", "Coldness adjustment value"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to apply the coldness to"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
			palette::Palette &pal = node.palette();
			const uint8_t val = (uint8_t)args.intVal("value", 10);
			pal.changeColder(val);
			_mementoHandler.markPaletteChange(_sceneGraph, node);
		}).setHelp(_("Make the palette colors colder"));

	command::Command::registerCommand("palette_brighter")
		.addArg({"value", command::ArgType::Float, true, "0.2", "Brightness adjustment value"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to apply the brightness to"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
			palette::Palette &pal = node.palette();
			const float val = args.floatVal("value", 0.2f);
			pal.changeBrighter(val);
			_mementoHandler.markPaletteChange(_sceneGraph, node);
		}).setHelp(_("Make the palette colors brighter"));

	command::Command::registerCommand("palette_darker")
		.addArg({"value", command::ArgType::Float, true, "0.2", "Darkness adjustment value"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to apply the darkness to"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
			palette::Palette &pal = node.palette();
			const float val = args.floatVal("value", 0.2f);
			pal.changeDarker(val);
			_mementoHandler.markPaletteChange(_sceneGraph, node);
		}).setHelp(_("Make the palette colors darker"));

	command::Command::registerCommand("palette_addcolor")
		.addArg({"color", command::ArgType::String, false, "", "Color value (e.g. #FF0000, #FF0000AA, argb:AARRGGBB, \"255 0 0\", \"255,0,0,128\", \"rgba(255,0,0,128)\")"})
		.setHandler([&] (const command::CommandArgs& args) {
			const core::String colorStr = args.str("color");
			if (colorStr.empty()) {
				Log::warn("palette_addcolor: no color value given");
				return;
			}
			color::RGBA rgba;
			if (!color::parseColor(colorStr, rgba)) {
				Log::warn("palette_addcolor: could not parse color '%s'", colorStr.c_str());
				return;
			}
			const int nodeId = activeNode();
			scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
			if (node == nullptr) {
				Log::warn("palette_addcolor: no active model node");
				return;
			}
			palette::Palette &pal = node->palette();
			uint8_t index = 0;
			if (!pal.tryAdd(rgba, false, &index)) {
				if (pal.hasColor(rgba)) {
					Log::info("palette_addcolor: color already exists at index %u", index);
				} else {
					Log::warn("palette_addcolor: palette is full");
				}
				return;
			}
			pal.markSave();
			pal.markDirty();
			_mementoHandler.markPaletteChange(_sceneGraph, *node);
			Log::info("Added color (%u, %u, %u, %u) at palette index %u", rgba.r, rgba.g, rgba.b, rgba.a, index);
		}).setHelp(_("Add a color to the active node's palette. Supports: #RRGGBB, #RRGGBBAA, argb:AARRGGBB, \"R G B [A]\", \"R,G,B[,A]\", \"rgb(R,G,B)\", \"rgba(R,G,B,A)\""));

	command::Command::registerCommand("palette_removeunused")
		.addArg({"updatevoxels", command::ArgType::Bool, true, "false", "Update voxel colors"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to remove unused colors from"})
		.setHandler([&] (const command::CommandArgs& args) {
			const bool updateVoxels = args.boolVal("updatevoxels", false);
			const int nodeId = toNodeId(args, activeNode());
			nodeRemoveUnusedColors(nodeId, updateVoxels);
		}).setHelp(_("Remove unused colors from palette"));

	command::Command::registerCommand("palette_whitebalancing")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to apply the white balance to"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
			palette::Palette currentPal = node.palette();
			currentPal.whiteBalance();
			node.setPalette(currentPal);
			_mementoHandler.markPaletteChange(_sceneGraph, node);
		}).setHelp(_("Apply white balance to the current palette"));

	command::Command::registerCommand("palette_contraststretching")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to apply the contrast stretching to"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
			palette::Palette currentPal = node.palette();
			currentPal.constrastStretching();
			node.setPalette(currentPal);
			_mementoHandler.markPaletteChange(_sceneGraph, node);
		}).setHelp(_("Apply color stretching to the current palette"));

	command::Command::registerCommand("palette_applyall")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to apply the palette from"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			const scenegraph::SceneGraphNode &currentNode = _sceneGraph.node(nodeId);
			const palette::Palette &currentPal = currentNode.palette();
			memento::ScopedMementoGroup mementoGroup(_mementoHandler, "palette_applyall");
			for (const auto &entry : _sceneGraph.nodes()) {
				scenegraph::SceneGraphNode &node = entry->value;
				if (!node.isAnyModelNode()) {
					continue;
				}
				node.setPalette(currentPal);
				_mementoHandler.markPaletteChange(_sceneGraph, node);
			}
		}).setHelp(_("Apply the current palette to all model nodes"));

	command::Command::registerCommand("palette_sort")
		.addArg({"type", command::ArgType::String, false, "", "Sort type: hue|saturation|brightness|cielab|original"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to apply the sort to"})
		.setHandler([&] (const command::CommandArgs& args) {
			const core::String &type = args.str("type");
			const int nodeId = toNodeId(args, activeNode());
			scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
			palette::Palette &pal = node.palette();
			palette::PaletteView &palView = pal.view();
			if (type == "hue") {
				palView.sortHue();
			} else if (type == "brightness") {
				palView.sortBrightness();
			} else if (type == "cielab") {
				palView.sortCIELab();
			} else if (type == "saturation") {
				palView.sortSaturation();
			} else if (type == "original") {
				palView.sortOriginal();
			}
			_mementoHandler.markPaletteChange(_sceneGraph, node);
		}).setHelp(_("Change intensity by scaling the rgb values of the palette")).
			setArgumentCompleter(command::valueCompleter({"hue", "saturation", "brightness", "cielab", "original"}));

	command::Command::registerCommand("normpalette_removenormals")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to remove the normals from"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, InvalidNodeId);
			if (nodeId == InvalidNodeId) {
				nodeGroupRemoveNormals();
				return;
			}
			nodeRemoveNormals(nodeId);
		}).setHelp(_("Remove normal information from the palette"));

	command::Command::registerActionButton("zoom_in", _zoomIn, "Zoom in");
	command::Command::registerActionButton("zoom_out", _zoomOut, "Zoom out");
	command::Command::registerActionButton("camera_rotate", _rotate, "Rotate the camera");
	command::Command::registerActionButton("camera_pan", _pan, "Pan the camera");
	command::Command::registerCommand("mouse_node_select")
		.setHandler([&] (const command::CommandArgs&) {
			const int nodeId = traceScene();
			if (nodeId != InvalidNodeId) {
				Log::debug("switch active node to hovered from scene graph mode: %i", nodeId);
				nodeActivate(nodeId);
			}
		}).setHelp(_("Switch active node to hovered from scene graph mode"));

	command::Command::registerCommand("mouse_node_lock")
		.setHandler([&] (const command::CommandArgs&) {
			if (_camera == nullptr) {
				return;
			}
			const math::Ray &ray = _camera->mouseRay(_mouseCursor);
			const float rayLength = _camera->farPlane();

			struct OBBHit {
				int nodeId;
				float distance;
			};
			core::DynamicArray<OBBHit> hits;
			hits.reserve(_sceneGraph.size());

			for (auto entry : _sceneGraph.nodes()) {
				const scenegraph::SceneGraphNode &node = entry->second;
				if (!node.isAnyModelNode() || !node.visible()) {
					continue;
				}
				float distance = 0.0f;
				const voxel::Region &region = _sceneGraph.resolveRegion(node);
				const glm::vec3 pivot = node.pivot();
				const scenegraph::FrameTransform &transform = _sceneGraph.transformForFrame(node, _currentFrameIdx);
				const math::OBBF &obb = scenegraph::toOBB(true, region, pivot, transform);
				if (obb.intersect(ray.origin, ray.direction, distance)) {
					hits.push_back({node.id(), distance});
				}
			}

			hits.sort([] (const OBBHit &a, const OBBHit &b) {
				return a.distance > b.distance;
			});

			for (const OBBHit &hit : hits) {
				const scenegraph::SceneGraphNode &node = _sceneGraph.node(hit.nodeId);
				const voxel::RawVolume *v = _sceneRenderer->volumeForNode(node);
				if (v == nullptr) {
					continue;
				}
				const glm::mat4 model = _sceneGraph.worldMatrix(node, _currentFrameIdx, true);
				const glm::mat4 invModel = glm::inverse(model);
				const glm::vec3 localOrigin = glm::vec3(invModel * glm::vec4(ray.origin, 1.0f));
				const glm::vec3 localDir = glm::normalize(glm::vec3(invModel * glm::vec4(ray.direction, 0.0f)));
				static constexpr voxel::Voxel air;
				bool didHit = false;
				voxelutil::raycastWithEndpoints(v, localOrigin - voxelutil::RaycastOffset, localOrigin + localDir * rayLength - voxelutil::RaycastOffset, [&] (voxel::RawVolume::Sampler &sampler) {
					if (!sampler.voxel().isSameType(air)) {
						didHit = true;
						return false;
					}
					return true;
				});
				if (didHit) {
					scenegraph::SceneGraphNode &hitNode = _sceneGraph.node(hit.nodeId);
					hitNode.setLocked(!hitNode.locked());
					return;
				}
			}
		}).setHelp(_("Toggle lock on the hovered node"));

	command::Command::registerCommand("select")
		.addArg({"type", command::ArgType::String, false, "", "Selection type: all|none|invert"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to apply the selection to"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			const core::String &type = args.str("type");
			if (type == "none") {
				selectionUnselect(nodeId);
			} else if (type == "all") {
				selectionSelectAll(nodeId);
			} else if (type == "invert") {
				selectionInvert(nodeId);
			}
		}).setHelp(_("Select all, nothing or invert")).setArgumentCompleter(command::valueCompleter({"all", "none", "invert"}));

	command::Command::registerCommand("presentation")
		.setHandler([] (const command::CommandArgs& args) {
			command::Command::execute("hideall; animate 2000 true true");
		}).setHelp(_("Cycle through all scene objects"));

	command::Command::registerCommand("align")
		.setHandler([this] (const command::CommandArgs& args) {
			_sceneGraph.align();
			_sceneRenderer->clear();
			for (const auto &entry : _sceneGraph.nodes()) {
				const scenegraph::SceneGraphNode &node = entry->second;
				if (!node.isModelNode()) {
					continue;
				}
				modified(node.id(), _sceneGraph.resolveRegion(node));
			}
		}).setHelp(_("Allow to align all nodes on the floor next to each other without overlapping"));

	command::Command::registerCommand("modelssave")
		.addArg({"dir", command::ArgType::String, true, "", "Directory to save models to"})
		.setHandler([&] (const command::CommandArgs& args) {
			core::String dir = args.str("dir");
			if (dir.empty()) {
				dir = core::string::extractDir(getSuggestedFilename());
			}
			if (dir.empty()) {
				dir = _filesystem->homePath();
			}
			if (!saveModels(dir)) {
				Log::error("Failed to save models to dir: %s", dir.c_str());
			}
		}).setHelp(_("Save all model nodes into filenames represented by their node names")).setArgumentCompleter(command::dirCompleter(_filesystem, _lastDirectory));

	command::Command::registerCommand("modelsave")
		.addArg({"nodeid", command::ArgType::String, false, "", "Node ID or UUID to save"})
		.addArg({"file", command::ArgType::String, true, "", "File path to save to"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			core::String file = args.str("file");
			if (file.empty()) {
				file = core::String::format("node%i.vengi", nodeId);
			}
			if (!nodeSave(nodeId, file)) {
				Log::error("Failed to save node %i to file: %s", nodeId, file.c_str());
			}
		}).setHelp(_("Save a single node to the given path with their node names")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("newscene")
		.addArg({"name", command::ArgType::String, true, "", "Scene name"})
		.addArg({"width", command::ArgType::Int, true, "64", "Width"})
		.addArg({"height", command::ArgType::Int, true, "64", "Height"})
		.addArg({"depth", command::ArgType::Int, true, "64", "Depth"})
		.setHandler([&] (const command::CommandArgs& args) {
			const core::String &name = args.str("name");
			const int iw = args.intVal("width", 64) - 1;
			const int ih = args.intVal("height", 64) - 1;
			const int id = args.intVal("depth", 64) - 1;
			const voxel::Region region(glm::zero<glm::ivec3>(), glm::ivec3(iw, ih, id));
			if (!region.isValid()) {
				Log::warn("Invalid size provided (%i:%i:%i)", iw, ih, id);
				return;
			}
			if (!newScene(true, name, region)) {
				Log::warn("Could not create new scene");
			}
		}).setHelp(_("Create a new scene (with a given name and width, height, depth)"));

	command::Command::registerCommand("crop")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to crop"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			nodeCrop(nodeId);
		}).setHelp(_("Crop the given node to the voxel boundaries"));

	command::Command::registerCommand("splitobjects")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to split"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			nodeSplitObjects(nodeId);
		}).setHelp(_("Split the given node into multiple nodes"));

	command::Command::registerCommand("scaledown")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to scale"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			nodeScaleDown(nodeId);
		}).setHelp(_("Scale the given node down")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("scaleup")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to scale"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			nodeScaleUp(nodeId);
		}).setHelp(_("Scale the given node up")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("colortomodel")
		.addArg({"index", command::ArgType::Int, true, "", "Palette color index"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to create"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			if (args.has("index")) {
				const uint8_t index = (uint8_t)args.intVal("index");
				const voxel::Voxel voxel = voxel::createVoxel(activePalette(), index);
				nodeColorToNewNode(nodeId, voxel);
			} else {
				const voxel::Voxel voxel = _modifierFacade.cursorVoxel();
				nodeColorToNewNode(nodeId, voxel);
			}
		}).setHelp(_("Move the voxels of the current selected palette index or the given index into a new node"));

	command::Command::registerCommand("abortaction")
		.setHandler([&] (const command::CommandArgs& args) {
			_modifierFacade.abort();
		}).setHelp(_("Aborts the current modifier action"));

	command::Command::registerCommand("brushapply")
		.setHandler([&] (const command::CommandArgs& args) {
			_modifierFacade.brushApply();
		}).setHelp(_("Apply pending brush changes without switching brushes"));

	command::Command::registerCommand("fillhollow")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupFillHollow();
		}).setHelp(_("Fill the inner parts of closed models"));

	command::Command::registerCommand("hollow")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupHollow();
		}).setHelp(_("Remove non visible voxels"));

	command::Command::registerCommand("fill")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupFill();
		}).setHelp(_("Fill voxels in the current selection"));

	command::Command::registerCommand("clear")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupClear();
		}).setHelp(_("Remove all voxels in the current selection"));

	command::Command::registerCommand("deleteselected")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupDeleteSelected();
		}).setHelp(_("Remove selected voxels"));

	command::Command::registerCommand("colorselected")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupColorSelected(_modifierFacade.cursorVoxel().getColor());
		}).setHelp(_("Recolor selected voxels with the active palette color"));

	command::Command::registerCommand("deselectcolor")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupDeselectColor(_modifierFacade.cursorVoxel().getColor());
		}).setHelp(_("Deselect all voxels matching the active palette color"));

	command::Command::registerCommand("finalizelasso")
		.setHandler([&] (const command::CommandArgs& args) {
			selectionFinalizeLasso(sceneGraph().activeNode());
		}).setHelp(_("Close the lasso polygon and apply the selection to enclosed surface voxels"));

	command::Command::registerCommand("cancellasso")
		.setHandler([&] (const command::CommandArgs& args) {
			selectionCancelLasso(sceneGraph().activeNode());
		}).setHelp(_("Discard the in-progress lasso polygon without applying any selection"));

	command::Command::registerCommand("undolassovertex")
		.setHandler([&] (const command::CommandArgs& args) {
			selectionLassoUndoVertex(sceneGraph().activeNode());
		}).setHelp(_("Remove the last placed lasso vertex and redraw edges"));

	command::Command::registerCommand("selectonlycolor")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupSelectOnlyColor(_modifierFacade.cursorVoxel().getColor());
		}).setHelp(_("Deselect all voxels not matching the active palette color"));

	command::Command::registerCommand("selectonlyedges")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupSelectOnlyEdges();
		}).setHelp(_("Filter selection to only keep edge voxels where two or more faces meet"));

	command::Command::registerCommand("selectonlycorners")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupSelectOnlyCorners();
		}).setHelp(_("Filter selection to only keep corner voxels where three faces meet"));

	command::Command::registerCommand("selectonlywalledges")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupSelectOnlyWallEdges();
		}).setHelp(_("Filter selection to only keep wall edge voxels with at most one solid neighbor in a perpendicular ring"));

	command::Command::registerCommand("selectiongrow")
		.setHandler([&] (const command::CommandArgs& args) {
			nodeGroupSelectionGrow();
		}).setHelp(_("Expand the selection by one voxel in all directions"));

	command::Command::registerCommand("setreferenceposition")
		.addArg({"x", command::ArgType::Int, false, "", "X coordinate"})
		.addArg({"y", command::ArgType::Int, false, "", "Y coordinate"})
		.addArg({"z", command::ArgType::Int, false, "", "Z coordinate"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int x = args.intVal("x");
			const int y = args.intVal("y");
			const int z = args.intVal("z");
			setReferencePosition(glm::ivec3(x, y, z));
		}).setHelp(_("Set the reference position to the specified position"));

	command::Command::registerCommand("movecursor")
		.addArg({"x", command::ArgType::Int, false, "", "X offset"})
		.addArg({"y", command::ArgType::Int, false, "", "Y offset"})
		.addArg({"z", command::ArgType::Int, false, "", "Z offset"})
		.setHandler([this] (const command::CommandArgs& args) {
			const int x = args.intVal("x");
			const int y = args.intVal("y");
			const int z = args.intVal("z");
			moveCursor(x, y, z);
		}).setHelp(_("Move the cursor by the specified offsets"));

	command::Command::registerCommand("loadnormalpalette")
		.addArg({"name", command::ArgType::String, false, "", "Normal palette name"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to change"})
		.setHandler([this] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
			if (node == nullptr) {
				return;
			}
			const core::String &name = args.str("name");
			palette::NormalPalette normalPalette;
			if (normalPalette.load(name.c_str())) {
				node->setNormalPalette(normalPalette);
				_mementoHandler.markNormalPaletteChange(sceneGraph(), *node);
			}
		}).setHelp(_("Change the normal palette"));

	command::Command::registerCommand("loadpalette")
		.addArg({"name", command::ArgType::String, false, "", "Palette name (e.g. 'built-in:nippon' or 'lospec:id')"})
		.addArg({"searchbestcolors", command::ArgType::Bool, true, "false", "Search best matching colors"})
		.setHandler([this] (const command::CommandArgs& args) {
			const bool searchBestColors = args.boolVal("searchbestcolors", false);
			loadPalette(args.str("name"), searchBestColors, true);
		}).setHelp(_("Load a palette by name. E.g. 'built-in:nippon' or 'lospec:id'")).setArgumentCompleter(palette::paletteCompleter());

	command::Command::registerCommand("cursor")
		.addArg({"x", command::ArgType::Int, false, "", "X coordinate"})
		.addArg({"y", command::ArgType::Int, false, "", "Y coordinate"})
		.addArg({"z", command::ArgType::Int, false, "", "Z coordinate"})
		.setHandler([this] (const command::CommandArgs& args) {
			const int x = args.intVal("x");
			const int y = args.intVal("y");
			const int z = args.intVal("z");
			setCursorPosition(glm::ivec3(x, y, z), _modifierFacade.cursorFace(), true);
			_traceViaMouse = false;
		}).setHelp(_("Set the cursor to the specified position"));

	command::Command::registerCommand("setreferencepositiontocursor")
		.setHandler([&] (const command::CommandArgs& args) {
			setReferencePosition(cursorPosition());
		}).setHelp(_("Set the reference position to the current cursor position"));

	command::Command::registerCommand("modelsize")
		.addArg({"x", command::ArgType::Int, true, "", "X size"})
		.addArg({"y", command::ArgType::Int, true, "", "Y size"})
		.addArg({"z", command::ArgType::Int, true, "", "Z size"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to resize"})
		.setHandler([this] (const command::CommandArgs& args) {
			const int x = args.intVal("x", 1);
			const int y = args.intVal("y", x);
			const int z = args.intVal("z", y);
			const int nodeId = toNodeId(args, activeNode());
			nodeResize(nodeId, glm::ivec3(x, y, z));
		}).setHelp(_("Resize your current model node by the specified size"));

	command::Command::registerCommand("rescalecontent")
		.addArg({"x", command::ArgType::Int, false, "", "Target X size"})
		.addArg({"y", command::ArgType::Int, true, "", "Target Y size (defaults to X)"})
		.addArg({"z", command::ArgType::Int, true, "", "Target Z size (defaults to Y)"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to rescale"})
		.setHandler([this] (const command::CommandArgs& args) {
			const int x = args.intVal("x", 1);
			const int y = args.intVal("y", x);
			const int z = args.intVal("z", y);
			const int nodeId = toNodeId(args, activeNode());
			nodeRescaleContent(nodeId, glm::ivec3(x, y, z));
		}).setHelp(_("Rescale the content of a model node to the specified target size"));

	command::Command::registerCommand("shift")
		.addArg({"x", command::ArgType::Int, false, "", "X shift value"})
		.addArg({"y", command::ArgType::Int, false, "", "Y shift value"})
		.addArg({"z", command::ArgType::Int, false, "", "Z shift value"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int x = args.intVal("x");
			const int y = args.intVal("y");
			const int z = args.intVal("z");
			nodeGroupShift(x, y, z);
		}).setHelp(_("Shift the volume by the given values"));

	command::Command::registerCommand("center_referenceposition")
		.setHandler([&] (const command::CommandArgs& args) {
			const glm::ivec3& refPos = referencePosition();
			nodeForeachGroup([&](int groupNodeId) {
				const voxel::RawVolume *v = volume(groupNodeId);
				if (v == nullptr) {
					return;
				}
				const voxel::Region& region = v->region();
				const glm::ivec3& center = region.getCenter();
				const glm::ivec3& delta = refPos - center;
				nodeShift(groupNodeId, delta);
			});
		}).setHelp(_("Center the current active nodes at the reference position"));

	command::Command::registerCommand("center_origin")
		.setHandler([&](const command::CommandArgs &args) {
			nodeForeachGroup([&](int groupNodeId) {
				const voxel::RawVolume *v = volume(groupNodeId);
				if (v == nullptr) {
					return;
				}
				const voxel::Region& region = v->region();
				const glm::ivec3& delta = -region.getCenter();
				nodeShift(groupNodeId, delta);
			});
			setReferencePosition(glm::zero<glm::ivec3>());
		}).setHelp(_("Center the current active nodes at the origin"));

	command::Command::registerCommand("move")
		.addArg({"x", command::ArgType::Int, false, "", "X move value"})
		.addArg({"y", command::ArgType::Int, false, "", "Y move value"})
		.addArg({"z", command::ArgType::Int, false, "", "Z move value"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int x = args.intVal("x");
			const int y = args.intVal("y");
			const int z = args.intVal("z");
			nodeGroupMoveVoxels(x, y, z);
		}).setHelp(_("Move the voxels inside the volume by the given values without changing the volume bounds"));

	command::Command::registerCommand("copy")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to copy from"})
		.setHandler([&] (const command::CommandArgs& args) {
			nodeCopy(toNodeId(args, activeNode()));
		}).setHelp(_("Copy selection"));

	command::Command::registerCommand("paste")
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = activeNode();
			const voxel::Region &region = selectionCalculateRegion(nodeId);
			if (region.isValid()) {
				paste(region.getLowerCorner());
			} else {
				paste(referencePosition());
			}
		}).setHelp(_("Paste clipboard to current selection or reference position"));

	command::Command::registerCommand("pastecursor")
		.setHandler([&] (const command::CommandArgs& args) {
			paste(cursorPosition());
		}).setHelp(_("Paste clipboard to current cursor position"));

	command::Command::registerCommand("pastenewnode")
		.addArg({"nodeid", command::ArgType::String, true, "", "Parent node ID or UUID"})
		.setHandler([&] (const command::CommandArgs& args) {
			nodePasteAsNewNode(toNodeId(args, activeNode()));
		}).setHelp(_("Paste clipboard as a new node"));

	command::Command::registerCommand("cut")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to cut from"})
		.setHandler([&] (const command::CommandArgs& args) {
			nodeCut(toNodeId(args, activeNode()));
		}).setHelp(_("Cut selection"));

	command::Command::registerCommand("globalcopy")
		.setHandler([&] (const command::CommandArgs&) {
			globalCopy();
		}).setHelp(_("Copy selection to a file-based clipboard shared across editor instances"));

	command::Command::registerCommand("globalcopyvisible")
		.setHandler([&] (const command::CommandArgs&) {
			globalCopyVisible();
		}).setHelp(_("Merge all visible nodes and copy to the file-based clipboard"));

	command::Command::registerCommand("globalpaste")
		.setHandler([&] (const command::CommandArgs&) {
			globalPaste(cursorPosition());
		}).setHelp(_("Paste from the file-based clipboard shared across editor instances to the cursor position"));

	command::Command::registerCommand("globalpastetoref")
		.setHandler([&] (const command::CommandArgs&) {
			globalPaste(referencePosition());
		}).setHelp(_("Paste from the file-based clipboard shared across editor instances to the reference position"));

	command::Command::registerCommand("globalpastenode")
		.setHandler([&] (const command::CommandArgs&) {
			globalPasteNode(cursorPosition());
		}).setHelp(_("Paste from the file-based clipboard as a new node at the cursor position"));

	command::Command::registerCommand("splatmerge")
		.setHandler([&] (const command::CommandArgs&) {
			splatMerge(activeNode());
		}).setHelp(_("Merge active node voxels into all overlapping nodes and remove the source node"));

	command::Command::registerCommand("mergeactivetobackground")
		.setHandler([&] (const command::CommandArgs&) {
			mergeActiveToBackground();
		}).setHelp(_("Stamp active node onto all other nodes, overwriting their content in overlapping regions"));

	command::Command::registerCommand("mergevisibletotemp")
		.setHandler([&] (const command::CommandArgs&) {
			mergeVisibleToTemp();
		}).setHelp(_("Merge all visible nodes into a temporary editable node, hiding the originals"));

	command::Command::registerCommand("undo")
		.setHandler([&] (const command::CommandArgs& args) {
			undo();
		}).setHelp(_("Undo your last step"));

	command::Command::registerCommand("redo")
		.setHandler([&] (const command::CommandArgs& args) {
			redo();
		}).setHelp(_("Redo your last step"));

	// Normalize rotation amount: accepts degree multiples of 90 or raw counts (1-4).
	// Negative values wrap around. Values > 4 (after conversion) return 0 (no-op).
	auto normalizeRotationAmount = [](int n) -> int {
		if (n != 0 && n % 90 == 0) {
			n = n / 90;
		}
		if (n < 0) {
			n = ((n % 4) + 4) % 4;
		}
		if (n > 4) {
			return 0;
		}
		return n;
	};

	command::Command::registerCommand("rotate")
		.addArg({"axis", command::ArgType::String, false, "", "Axis to rotate around: x|y|z"})
		.addArg({"amount", command::ArgType::Int, true, "1", "Number of 90-degree rotations"})
		.setHandler([&, normalizeRotationAmount] (const command::CommandArgs& args) {
			const math::Axis axis = math::toAxis(args.str("axis"));
			const int n = normalizeRotationAmount(args.intVal("amount", 1));
			memento::ScopedMementoGroup group(_mementoHandler, "rotate");
			for (int i = 0; i < n; ++i) {
				nodeGroupRotate(axis);
			}
		}).setHelp(_("Rotate active nodes around the given axis"));

	command::Command::registerCommand("rotateall")
		.addArg({"axis", command::ArgType::String, false, "", "Axis to rotate around: x|y|z"})
		.addArg({"amount", command::ArgType::Int, true, "1", "Number of 90-degree rotations"})
		.setHandler([&, normalizeRotationAmount] (const command::CommandArgs& args) {
			const math::Axis axis = math::toAxis(args.str("axis"));
			const int n = normalizeRotationAmount(args.intVal("amount", 1));
			memento::ScopedMementoGroup group(_mementoHandler, "rotateall");
			for (int i = 0; i < n; ++i) {
				nodeRotateAll(axis);
			}
		}).setHelp(_("Rotate all model nodes around the given axis, preserving their relative positions"));

	command::Command::registerCommand("modelmerge")
		.addArg({"nodeid1", command::ArgType::String, true, "", "First node ID or UUID"})
		.addArg({"nodeid2", command::ArgType::String, true, "", "Second node ID or UUID"})
		.setHandler([&] (const command::CommandArgs& args) {
			int nodeId1;
			int nodeId2;
			if (args.has("nodeid1") && !args.has("nodeid2")) {
				nodeId2 = toNodeId(args, InvalidNodeId, "nodeid2");
				nodeId1 = _sceneGraph.prevModelNode(nodeId2);
			} else if (args.has("nodeid1") && args.has("nodeid2")) {
				nodeId1 = toNodeId(args, InvalidNodeId, "nodeid1");
				nodeId2 = toNodeId(args, InvalidNodeId, "nodeid2");
			} else {
				nodeId2 = activeNode();
				nodeId1 = _sceneGraph.prevModelNode(nodeId2);
			}
			mergeNodes(nodeId1, nodeId2);
		}).setHelp(_("Merge two given nodes or active model node with the next one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modelmergeall")
		.setHandler([&] (const command::CommandArgs& args) {
			mergeNodes(NodeMergeFlags::All);
		}).setHelp(_("Merge all nodes"));

	command::Command::registerCommand("modelmergevisible")
		.setHandler([&] (const command::CommandArgs& args) {
			mergeNodes(NodeMergeFlags::Visible);
		}).setHelp(_("Merge all visible nodes"));

	command::Command::registerCommand("modelmergelocked")
		.setHandler([&] (const command::CommandArgs& args) {
			mergeNodes(NodeMergeFlags::Locked);
		}).setHelp(_("Merge all locked nodes"));

	command::Command::registerCommand("animate")
		.addArg({"nodedelaymillis", command::ArgType::Int, false, "", "Delay between frames in milliseconds (0 to stop)"})
		.addArg({"enable", command::ArgType::Bool, true, "true", "Enable animation"})
		.addArg({"resetcamera", command::ArgType::Bool, true, "false", "Reset camera between frames"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int delayMs = args.intVal("nodedelaymillis");
			const bool enable = args.boolVal("enable", true);
			_animationResetCamera = args.boolVal("resetcamera", false);
			if (!enable) {
				_frameAnimationSpeed = 0.0;
				return;
			}
			_frameAnimationSpeed = (double)delayMs / 1000.0;
		}).setHelp(_("Animate all nodes with the given delay in millis between the frames"));

	command::Command::registerCommand("setcolor")
		.addArg({"index", command::ArgType::Int, false, "", "Palette color index"})
		.setHandler([&] (const command::CommandArgs& args) {
			const uint8_t index = (uint8_t)args.intVal("index");
			const voxel::Voxel voxel = voxel::createVoxel(activePalette(), index);
			_modifierFacade.setCursorVoxel(voxel);
		}).setHelp(_("Use the given index to select the color from the current palette"));

	command::Command::registerCommand("setcolorrgb")
		.addArg({"red", command::ArgType::Int, false, "", "Red component (0-255)"})
		.addArg({"green", command::ArgType::Int, false, "", "Green component (0-255)"})
		.addArg({"blue", command::ArgType::Int, false, "", "Blue component (0-255)"})
		.setHandler([&] (const command::CommandArgs& args) {
			const uint8_t red = (uint8_t)args.intVal("red");
			const uint8_t green = (uint8_t)args.intVal("green");
			const uint8_t blue = (uint8_t)args.intVal("blue");
			const color::RGBA color(red, green, blue);
			const int index = activePalette().getClosestMatch(color);
			const voxel::Voxel voxel = voxel::createVoxel(activePalette(), index);
			_modifierFacade.setCursorVoxel(voxel);
		}).setHelp(_("Set the current selected color by finding the closest rgb match in the palette"));

	command::Command::registerCommand("pickcolor")
		.setHandler([&] (const command::CommandArgs& args) {
			// during mouse movement, the current cursor position might be at an air voxel (this
			// depends on the mode you are editing in), thus we should use the cursor voxel in
			// that case
			if (_traceViaMouse && !voxel::isAir(hitCursorVoxel().getMaterial())) {
				_modifierFacade.setCursorVoxel(hitCursorVoxel());
				return;
			}
			// resolve the voxel via cursor position. This allows to use also get the proper
			// result if we moved the cursor via keys (and thus might have skipped tracing)
			if (const voxel::RawVolume *v = activeVolume()) {
				const voxel::Voxel& voxel = v->voxel(cursorPosition());
				_modifierFacade.setCursorVoxel(voxel);
			}
		}).setHelp(_("Pick the current selected color from current cursor voxel"));

	command::Command::registerCommand("flip")
		.addArg({"axis", command::ArgType::String, false, "", "Axis to flip around: x|y|z"})
		.setHandler([&] (const command::CommandArgs& args) {
			const math::Axis axis = math::toAxis(args.str("axis"));
			nodeGroupFlip(axis);
		}).setHelp(_("Flip the selected nodes around the given axis")).setArgumentCompleter(command::valueCompleter({"x", "y", "z"}));

	command::Command::registerCommand("transformreset")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			if (nodeId == InvalidNodeId) {
				nodeGroupResetTransform(InvalidKeyFrame);
				return;
			}
			nodeResetTransform(nodeId, InvalidKeyFrame);
		}).setHelp(_("Reset the transform of the current node and keyframe to the default transform"));

	command::Command::registerCommand("transformmirror")
		.addArg({"axis", command::ArgType::String, false, "", "Axis to mirror: x|y|z"})
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID"})
		.setHandler([&] (const command::CommandArgs& args) {
			const math::Axis axis = math::toAxis(args.str("axis"));
			const int nodeId = toNodeId(args, activeNode());
			nodeTransformMirror(nodeId, InvalidKeyFrame, axis);
		}).setHelp(_("Mirrors the transform at the given axes for the given node"));

	command::Command::registerCommand("modeladd")
		.addArg({"name", command::ArgType::String, true, "", "Model name"})
		.addArg({"width", command::ArgType::Int, true, "64", "Width of the volume"})
		.addArg({"height", command::ArgType::Int, true, "64", "Height of the volume"})
		.addArg({"depth", command::ArgType::Int, true, "64", "Depth of the volume"})
		.setHandler([&] (const command::CommandArgs& args) {
			const core::String &name = args.str("name");
			const int iw = args.intVal("width", 64);
			const int ih = args.intVal("height", iw);
			const int id = args.intVal("depth", ih);
			addModelChild(name, iw, ih, id);
		}).setHelp(_("Add a new model node to the current active node (with a given name and width, height, depth)"));

	command::Command::registerCommand("nodebaketransform")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, InvalidNodeId);
			if (nodeId == InvalidNodeId) {
				nodeForeachGroup([&] (int groupNodeId) {
					nodeBakeTransform(groupNodeId);
				});
			} else {
				nodeBakeTransform(nodeId);
			}
		}).setHelp(_("Bake the current transform into the voxel data for a particular node by id - or the current active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodedelete")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to delete"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
				nodeRemove(*node, false);
			}
		}).setHelp(_("Delete a particular node by id - or the current active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodelock")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to lock"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
				node->setLocked(true);
			}
		}).setHelp(_("Lock a particular node by id - or the current active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodetogglelock")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
				node->setLocked(!node->locked());
			}
		}).setHelp(_("Toggle the lock state of a particular node by id - or the current active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodeunlock")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to unlock"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
				node->setLocked(false);
			}
		}).setHelp(_("Unlock a particular node by id - or the current active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodeactivate")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to activate"})
		.setHandler([&] (const command::CommandArgs& args) {
			if (!args.has("nodeid")) {
				Log::info("Active node: %i", activeNode());
				return;
			}
			const int nodeId = toNodeId(args, activeNode());
			nodeActivate(nodeId);
		}).setHelp(_("Set or print the current active node")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodetogglevisible")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID"})
		.setHandler([&](const command::CommandArgs &args) {
			const int nodeId = toNodeId(args, activeNode());
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeSetVisible(nodeId, !node->visible());
			}
		}).setHelp(_("Toggle the visible state of a node")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("showall")
		.setHandler([&] (const command::CommandArgs& args) {
			for (auto iter = _sceneGraph.beginAll(); iter != _sceneGraph.end(); ++iter) {
				scenegraph::SceneGraphNode &node = *iter;
				node.setVisible(true);
			}
		}).setHelp(_("Show all nodes"));

	command::Command::registerCommand("hideall")
		.setHandler([&](const command::CommandArgs &args) {
			for (auto iter = _sceneGraph.beginAll(); iter != _sceneGraph.end(); ++iter) {
				scenegraph::SceneGraphNode &node = *iter;
				node.setVisible(false);
			}
		}).setHelp(_("Hide all nodes"));

	command::Command::registerCommand("nodeshowallchildren")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			_sceneGraph.visitChildren(nodeId, true, [] (scenegraph::SceneGraphNode &node) {
				node.setVisible(true);
			});
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				node->setVisible(true);
			}
		}).setHelp(_("Show all children nodes"));

	command::Command::registerCommand("nodehideallchildren")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID"})
		.setHandler([&](const command::CommandArgs &args) {
			const int nodeId = toNodeId(args, activeNode());
			_sceneGraph.visitChildren(nodeId, true, [] (scenegraph::SceneGraphNode &node) {
				node.setVisible(false);
			});
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				node->setVisible(false);
			}
		}).setHelp(_("Hide all children nodes"));

	command::Command::registerCommand("nodehideothers")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to keep visible"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			for (auto iter = _sceneGraph.beginAll(); iter != _sceneGraph.end(); ++iter) {
				scenegraph::SceneGraphNode &node = *iter;
				if (node.id() == nodeId) {
					node.setVisible(true);
					continue;
				}
				node.setVisible(false);
			}
		}).setHelp(_("Hide all model nodes except the active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modellockall")
		.setHandler([&](const command::CommandArgs &args) {
			for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
				scenegraph::SceneGraphNode &node = *iter;
				node.setLocked(true);
			}
		}).setHelp(_("Lock all nodes"));

	command::Command::registerCommand("modelunlockall")
		.setHandler([&] (const command::CommandArgs& args) {
			for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
				scenegraph::SceneGraphNode &node = *iter;
				node.setLocked(false);
			}
		}).setHelp(_("Unlock all nodes"));

	command::Command::registerCommand("noderename")
		.addArg({"nodeid", command::ArgType::String, false, "", "Node ID or UUID"})
		.addArg({"newname", command::ArgType::String, true, "", "New name"})
		.setHandler([&] (const command::CommandArgs& args) {
			// Two args: first is node ID, second is name
			const int nodeId = toNodeId(args, activeNode());
			const core::String &newName = args.str("newname");
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeRename(*node, newName);
			}
		}).setHelp(_("Change the name of a node")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodeduplicate")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to duplicate"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeDuplicate(*node);
			}
		}).setHelp(_("Duplicates the current node or the given node id")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modelref")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to reference"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeReference(*node);
			}
		}).setHelp(_("Create a node reference for the given node id")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modelunref")
		.addArg({"nodeid", command::ArgType::String, true, "", "Node ID or UUID to unreference"})
		.setHandler([&] (const command::CommandArgs& args) {
			const int nodeId = toNodeId(args, activeNode());
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeUnreference(*node);
			}
		}).setHelp(_("Unreference from model and allow to edit the voxels for this node"));

	command::Command::registerCommand("camera_activate")
		.addArg({"nodeid", command::ArgType::String, true, "", "Camera node ID or UUID"})
		.setHandler([&] (const command::CommandArgs& args) {
			video::Camera *camera = activeCamera();
			if (camera == nullptr) {
				Log::error("No active camera found");
				return;
			}
			const scenegraph::SceneGraphNodeCamera *cameraNode;
			if (!args.has("nodeid")) {
				cameraNode = _sceneGraph.activeCameraNode();
			} else {
				const int nodeId = toNodeId(args, InvalidNodeId);
				const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
				if (!node.isCameraNode()) {
					Log::error("The node %i (%s) is not a camera node", nodeId, node.name().c_str());
					return;
				}
				cameraNode = &scenegraph::toCameraNode(node);
			}
			if (cameraNode == nullptr) {
				Log::error("No active camera node found");
				return;
			}
			video::Camera nodeCamera = voxelrender::toCamera(_camera->size(), sceneGraph(), *cameraNode, currentFrame());
			camera->lerp(nodeCamera);
		}).setHelp(_("Interpolate to the camera node position and orientation"));

	command::Command::registerCommand("camera_rotation")
		.addArg({"mode", command::ArgType::String, true, "", "Rotation mode: target|eye (toggles if not specified)"})
		.setHandler([&] (const command::CommandArgs& args) {
			video::Camera *camera = activeCamera();
			if (camera == nullptr) {
				Log::error("No active camera found");
				return;
			}
			if (!args.has("mode")) {
				if (camera->rotationType() == video::CameraRotationType::Target) {
					camera->setRotationType(video::CameraRotationType::Eye);
				} else {
					camera->setRotationType(video::CameraRotationType::Target);
				}
				return;
			}
			const core::String &modeStr = args.str("mode");
			if (modeStr == "target") {
				camera->setRotationType(video::CameraRotationType::Target);
			} else if (modeStr == "eye") {
				camera->setRotationType(video::CameraRotationType::Eye);
			} else {
				Log::error("Unknown camera mode: %s (valid are: target, eye)", modeStr.c_str());
			}
		}).setHelp(_("Set or toggle the camera rotation mode (target or eye)")).setArgumentCompleter(command::valueCompleter({"target", "eye"}));

	command::Command::registerCommand("camera_target_reference")
		.setHandler([&] (const command::CommandArgs& args) {
			video::Camera *camera = activeCamera();
			if (camera == nullptr) {
				Log::error("No active camera found");
				return;
			}
			const glm::vec3 refPos = glm::vec3(referencePosition());
			camera->setTarget(refPos);
			camera->setRotationType(video::CameraRotationType::Target);
		}).setHelp(_("Set the camera orbit target to the reference point position"));

	command::Command::registerCommand("camera_projection")
		.addArg({"mode", command::ArgType::String, true, "", "Projection mode: perspective|orthogonal (toggles if not specified)"})
		.setHandler([&] (const command::CommandArgs& args) {
			video::Camera *camera = activeCamera();
			if (camera == nullptr) {
				Log::error("No active camera found");
				return;
			}
			if (!args.has("mode")) {
				if (camera->mode() == video::CameraMode::Perspective) {
					camera->setMode(video::CameraMode::Orthogonal);
				} else {
					camera->setMode(video::CameraMode::Perspective);
				}
				return;
			}
			const core::String &modeStr = args.str("mode");
			if (modeStr == "perspective") {
				camera->setMode(video::CameraMode::Perspective);
			} else if (modeStr == "orthogonal") {
				camera->setMode(video::CameraMode::Orthogonal);
			} else {
				Log::error("Unknown projection mode: %s (valid are: perspective, orthogonal)", modeStr.c_str());
			}
		}).setHelp(_("Set or toggle the camera projection mode (perspective or orthogonal)")).setArgumentCompleter(command::valueCompleter({"perspective", "orthogonal"}));

	command::Command::registerCommand("net_server_start")
		.addArg({"port", command::ArgType::Int, true, "", "Port to listen on (default: ve_netport cvar)"})
		.addArg({"iface", command::ArgType::String, true, "", "Interface IP to bind (default: ve_netserverinterface cvar)"})
		.setHandler([this](const command::CommandArgs &args) {
			const int port = args.intVal("port", core::getVar(cfg::VoxEditNetPort)->intVal());
			const core::String iface = args.str("iface", core::getVar(cfg::VoxEditNetServerInterface)->strVal());
			startLocalServer(port, iface);
		}).setHelp(_("Start the local voxedit server"));

	command::Command::registerCommand("net_server_stop")
		.setHandler([this](const command::CommandArgs &) {
			stopLocalServer();
		}).setHelp(_("Stop the local voxedit server"));

	command::Command::registerCommand("net_server_kick")
		.addArg({"clientid", command::ArgType::Int, false, "", "Client ID to kick from the server"})
		.setHandler([this](const command::CommandArgs &args) {
			const network::ClientId clientId = (network::ClientId)args.intVal("clientid");
			server().markForDisconnect(clientId);
		}).setHelp(_("Kick a client from the server"));

	command::Command::registerCommand("net_client_connect")
		.addArg({"host", command::ArgType::String, true, "", "Server hostname (default: ve_nethostname cvar)"})
		.addArg({"port", command::ArgType::Int, true, "", "Server port (default: ve_netport cvar)"})
		.setHandler([this](const command::CommandArgs &args) {
			const core::String host = args.str("host", core::getVar(cfg::VoxEditNetHostname)->strVal());
			const int port = args.intVal("port", core::getVar(cfg::VoxEditNetPort)->intVal());
			connectToServer(host, port);
		}).setHelp(_("Connect to a voxedit server"));

	command::Command::registerCommand("net_client_disconnect")
		.setHandler([this](const command::CommandArgs &) {
			disconnectFromServer();
		}).setHelp(_("Disconnect from the current voxedit server"));
}

void SceneManager::nodeRemoveUnusedColors(int nodeId, bool reindexPalette) {
	scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	node.removeUnusedColors(reindexPalette);
	if (reindexPalette) {
		memento::ScopedMementoGroup mementoGroup(_mementoHandler, "removeunusedcolors");
		_mementoHandler.markPaletteChange(_sceneGraph, node);
		modified(nodeId, _sceneGraph.resolveRegion(node));
	} else {
		_mementoHandler.markPaletteChange(_sceneGraph, node);
	}
}

int SceneManager::addPointChild(const core::String& name, const glm::ivec3& position, const glm::quat& orientation, const core::UUID &uuid) {
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Point, uuid);
	scenegraph::SceneGraphTransform transform;
	transform.setWorldTranslation(position);
	transform.setWorldOrientation(orientation);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	newNode.setTransform(keyFrameIdx, transform);
	newNode.setName(name);
	const int parentId = activeNode();
	const int nodeId = moveNodeToSceneGraph(newNode, parentId);
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to add point child node '%s' at position %i:%i:%i", name.c_str(), position.x, position.y, position.z);
		return InvalidNodeId;
	}
	return nodeId;
}

int SceneManager::addModelChild(const core::String& name, int width, int height, int depth, const core::UUID &uuid) {
	const voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
	if (!region.isValid()) {
		Log::warn("Invalid size provided (%i:%i:%i)", width, height, depth);
		return InvalidNodeId;
	}
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model, uuid);
	newNode.createVolume(region);
	newNode.setName(name);
	const int parentId = activeNode();
	const int nodeId = moveNodeToSceneGraph(newNode, parentId);
	return nodeId;
}

void SceneManager::nodeGroupFlip(math::Axis axis) {
	nodeForeachGroup([&](int groupNodeId) {
		voxel::RawVolume *v = volume(groupNodeId);
		if (v == nullptr) {
			return;
		}
		voxel::RawVolume* newVolume = voxelutil::mirrorAxis(v, axis);
		voxel::Region r = newVolume->region();
		r.accumulate(v->region());
		if (!setNewVolume(groupNodeId, newVolume)) {
			delete newVolume;
			return;
		}
		modified(groupNodeId, r);
	});
}

bool SceneManager::init() {
	++_initialized;
	if (_initialized > 1) {
		Log::debug("Already initialized");
		return true;
	}

	if (!_server.init()) {
		Log::error("Failed to initialize the server");
		return false;
	}
	if (!_client.init()) {
		Log::error("Failed to initialize the client");
		return false;
	}
	if (_soundManager.init()) {
		_chatSound = _soundManager.loadSound("chat-ping.wav");
	} else {
		Log::warn("Failed to initialize the sound manager");
	}
	if (!_mementoHandler.init()) {
		Log::error("Failed to initialize the memento handler");
		return false;
	}
	if (!_sceneRenderer->init()) {
		Log::error("Failed to initialize the scene renderer");
		return false;
	}
	if (!_modifierFacade.init()) {
		Log::error("Failed to initialize the modifier");
		return false;
	}
	if (!_camMovement.init()) {
		Log::error("Failed to initialize the movement controller");
		return false;
	}
	if (!_luaApi.init()) {
		Log::error("Failed to initialize the lua api");
		return false;
	}

	_gridSize = core::getVar(cfg::VoxEditGridsize);
	_lastAutoSave = _timeProvider->tickSeconds();
	_modifierFacade.setLockedAxis(math::Axis::None, true);
	return true;
}

bool SceneManager::isScriptRunning() const {
	return _luaApi.scriptStillRunning();
}

bool SceneManager::runScript(const core::String& luaCode, const core::DynamicArray<core::String>& args) {
	if (luaCode.empty()) {
		Log::warn("No script selected");
		return false;
	}
	const int nodeId = _sceneGraph.activeNode();
	const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	if (!node.isAnyModelNode()) {
		Log::warn("The given node (%s:%i) is not a model node", node.name().c_str(), nodeId);
		return false;
	}
	const voxel::Region &region = _sceneGraph.resolveRegion(node);
	_mementoHandler.beginGroup("lua script");
	// TODO: MEMENTO: there are still no memento states for direct node modifications during the script run
	//                we can e.g. set or modify the transforms, properties and so on of a node.
	_sceneGraph.registerListener(&_luaApiListener);
	if (!_luaApi.exec(luaCode, _sceneGraph, nodeId, region, _modifierFacade.cursorVoxel(), args)) {
		_sceneGraph.unregisterListener(&_luaApiListener);
		_mementoHandler.endGroup();
		return false;
	}
	return true;
}

bool SceneManager::runScriptSync(const core::String &luaCode, const core::DynamicArray<core::String> &args) {
	if (!runScript(luaCode, args)) {
		return false;
	}

	// run the coroutine to completion
	for (;;) {
		const voxelgenerator::ScriptState state = _luaApi.update(0.0);
		if (state == voxelgenerator::ScriptState::Error) {
			_sceneGraph.unregisterListener(&_luaApiListener);
			_mementoHandler.endGroup();
			return false;
		}
		if (state == voxelgenerator::ScriptState::Finished) {
			if (_sceneGraph.dirty()) {
				markDirty();
				_sceneRenderer->clear();
				_sceneGraph.markClean();
			}
			const voxelgenerator::LuaDirtyRegions &dirtyRegions = _luaApi.dirtyRegions();
			for (const auto &entry : dirtyRegions) {
				const int dirtyNodeId = entry->key;
				const voxel::Region &dirtyRegion = entry->value;
				if (dirtyRegion.isValid()) {
					modified(dirtyNodeId, dirtyRegion);
				}
			}
			_sceneGraph.unregisterListener(&_luaApiListener);
			_mementoHandler.endGroup();
			return true;
		}
		if (state == voxelgenerator::ScriptState::Inactive) {
			_sceneGraph.unregisterListener(&_luaApiListener);
			_mementoHandler.endGroup();
			return false;
		}
		// ScriptState::Running - continue the loop
	}
}

bool SceneManager::frameAnimationActive() const {
	return _frameAnimationSpeed > 0.0;
}

void SceneManager::animateFrames(double nowSeconds) {
	if (!frameAnimationActive()) {
		return;
	}
	if (_nextFrameSwitch > nowSeconds) {
		return;
	}
	_nextFrameSwitch = nowSeconds + _frameAnimationSpeed;

	if (_frameAnimationNodeId == InvalidNodeId) {
		_frameAnimationNodeId = (*_sceneGraph.beginModel()).id();
	}

	scenegraph::SceneGraphNode &prev = _sceneGraph.node(_frameAnimationNodeId);
	if (prev.isAnyModelNode()) {
		prev.setVisible(false);
	}

	_frameAnimationNodeId = _sceneGraph.nextModelNode(_frameAnimationNodeId);
	if (_frameAnimationNodeId == InvalidNodeId) {
		if (const auto *node = _sceneGraph.firstModelNode()) {
			_frameAnimationNodeId = node->id();
		} else {
			_frameAnimationSpeed = 0.0f;
		}
	}
	scenegraph::SceneGraphNode &node = _sceneGraph.node(_frameAnimationNodeId);
	if (node.isAnyModelNode()) {
		node.setVisible(true);
	}
	if (_animationResetCamera) {
		command::Command::execute("camera_reset");
	}
}

bool SceneManager::isLoading() const {
	return _loadingFuture.valid();
}

void SceneManager::stopLocalServer() {
	disconnectFromServer();
	server().stop();
}

void SceneManager::startLocalServer(int port, const core::String &iface) {
	_mementoHandler.registerListener(&_client);
	client().disconnect();
	server().start(port, iface);
	client().connect("localhost", port, true);
}

bool SceneManager::connectToServer(const core::String &host, int port) {
	_mementoHandler.registerListener(&_client);
	server().stop();
	client().disconnect();
	return client().connect(host, port, false);
}

void SceneManager::disconnectFromServer() {
	_mementoHandler.unregisterListener(&_client);
	client().disconnect();
}

bool SceneManager::startRecording(const core::String &filename) {
	stopPlayback();
	_mementoHandler.registerListener(&_recorder);
	if (!_recorder.startRecording(filename)) {
		_mementoHandler.unregisterListener(&_recorder);
		return false;
	}
	return true;
}

void SceneManager::stopRecording() {
	_mementoHandler.unregisterListener(&_recorder);
	_recorder.stopRecording();
}

bool SceneManager::isRecording() const {
	return _recorder.isRecording();
}

bool SceneManager::startPlayback(const core::String &filename) {
	stopRecording();
	return _player.startPlayback(filename);
}

void SceneManager::stopPlayback() {
	_player.stopPlayback();
}

bool SceneManager::isPlaying() const {
	return _player.isPlaying();
}

bool SceneManager::isPlaybackPaused() const {
	return _player.isPaused();
}

void SceneManager::setPlaybackPaused(bool paused) {
	_player.setPaused(paused);
}

float SceneManager::playbackSpeed() const {
	return _player.speed();
}

void SceneManager::setPlaybackSpeed(float speed) {
	_player.setSpeed(speed);
}

bool SceneManager::update(double nowSeconds) {
	core_trace_scoped(SceneManagerUpdate);
	if (_lsystemRunning) {
		stepLSystem();
	}
	updateDelta(nowSeconds);
	_server.update(nowSeconds);
	_client.update(nowSeconds);
	_soundManager.update(nowSeconds);
	_player.update(deltaSeconds());
	bool loadedNewScene = false;
	if (_loadingFuture.ready()) {
		if (loadSceneGraph(core::move(_loadingFuture.get()))) {
			_needAutoSave = false;
			_dirty = false;
			loadedNewScene = true;
		}
		_loadingFuture = {};
	}

	voxelgenerator::ScriptState state = _luaApi.update(nowSeconds);
	if (state == voxelgenerator::ScriptState::Error) {
		_sceneGraph.unregisterListener(&_luaApiListener);
		_mementoHandler.endGroup();
	} else if (state == voxelgenerator::ScriptState::Finished) {
		if (_sceneGraph.dirty()) {
			markDirty();
			// Scripts may replace volumes (e.g., via resize), making MeshState's
			// volume pointers stale. Clear them before modified() to prevent
			// use-after-free in scheduleRegionExtraction(). The render loop's
			// prepareModelNodes() will re-sync volumes on the next frame.
			_sceneRenderer->clear();
			_sceneGraph.markClean();
		}
		const voxelgenerator::LuaDirtyRegions &dirtyRegions = _luaApi.dirtyRegions();
		for (const auto &entry : dirtyRegions) {
			const int nodeId = entry->key;
			const voxel::Region &region = entry->value;
			if (region.isValid()) {
				modified(nodeId, region);
			}
		}
		_sceneGraph.unregisterListener(&_luaApiListener);
		_mementoHandler.endGroup();
	}
	video::Camera *camera = activeCamera();
	scenegraph::FrameIndex frameIdx = currentFrame();
	// TODO: Set to InvalidFrameIndex if transforms should not get applied
	_camMovement.update(_nowSeconds, camera, _sceneGraph, frameIdx);
	_modifierFacade.update(nowSeconds, camera);

	updateDirtyRendererStates();

	_sceneRenderer->update();
	setGridResolution(_gridSize->intVal());
	for (int i = 0; i < lengthof(DIRECTIONS); ++i) {
		if (!_move[i].pressed()) {
			continue;
		}
		_move[i].execute(nowSeconds, 0.125, [&] () {
			const Direction& dir = DIRECTIONS[i];
			moveCursor(dir.x, dir.y, dir.z);
		});
	}
	if (_zoomIn.pressed()) {
		_zoomIn.execute(nowSeconds, 0.02, [&] () {
			if (_camera != nullptr) {
				_camMovement.zoom(*_camera, 1.0f);
			}
		});
	} else if (_zoomOut.pressed()) {
		_zoomOut.execute(nowSeconds, 0.02, [&] () {
			if (_camera != nullptr) {
				_camMovement.zoom(*_camera, -1.0f);
			}
		});
	}
	if (_pan.pressed() && _camera != nullptr) {
		_pan.execute(nowSeconds, 0.0, [&] () {
			_camMovement.pan(*_camera, _mouseCursorDelta.x, _mouseCursorDelta.y);
		});
	}
	if (_rotate.pressed() && _camera != nullptr && !_fixedCamera) {
		_rotate.execute(nowSeconds, 0.0, [&] () {
			_camMovement.rotate(*_camera, (float)_mouseCursorDelta.x, (float)_mouseCursorDelta.y);
		});
	} else if (_mouseLookActive && _camera != nullptr && !_fixedCamera) {
		_camMovement.rotate(*_camera, (float)_mouseCursorDelta.x, (float)_mouseCursorDelta.y);
	}

	animateFrames(nowSeconds);
	autosave();
	return loadedNewScene;
}

void SceneManager::shutdown() {
	if (_initialized > 0) {
		--_initialized;
	}
	if (_initialized != 0) {
		return;
	}

	autosave();

	_luaApi.shutdown();

	_sceneRenderer->shutdown();
	if (_sceneGraph.isRegistered(&_luaApiListener)) {
		Log::error("Lua api listener still registered");
		_sceneGraph.unregisterListener(&_luaApiListener);
	}
	_selectionRegionCache.invalidate();
	_sceneGraph.clear();

	_camMovement.shutdown();
	_modifierFacade.shutdown();
	_mementoHandler.unregisterListener(&_client);
	_mementoHandler.unregisterListener(&_recorder);
	_mementoHandler.shutdown();
	_recorder.stopRecording();
	_player.stopPlayback();
	_server.shutdown();
	_client.shutdown();
	_soundManager.freeSound(_chatSound);
	_chatSound = nullptr;
	_soundManager.shutdown();

	command::Command::unregisterActionButton("zoom_in");
	command::Command::unregisterActionButton("zoom_out");
	command::Command::unregisterActionButton("camera_rotate");
	command::Command::unregisterActionButton("camera_pan");
}

void SceneManager::lsystemAbort() {
	if (_lsystemRunning) {
		_mementoHandler.endGroup();
		_lsystemRunning = false;
	}
	_lsystemExecState = {};
	_lsystemNodeId = InvalidNodeId;
}

void SceneManager::lsystem(const voxelgenerator::lsystem::LSystemConfig &conf) {
	lsystemAbort();
	_lsystemConfig = conf;
	_lsystemNodeId = activeNode();
	voxel::RawVolume *v = volume(_lsystemNodeId);
	if (v == nullptr) {
		return;
	}
	_lsystemVoxel = _modifierFacade.cursorVoxel();
	voxelgenerator::lsystem::prepareState(_lsystemConfig, _lsystemState);
	_lsystemRunning = true;
	_mementoHandler.beginGroup("LSystem generation");
}

void SceneManager::stepLSystem() {
	if (!_lsystemRunning) {
		return;
	}
	voxel::RawVolume *v = volume(_lsystemNodeId);
	if (v == nullptr) {
		_lsystemRunning = false;
		_mementoHandler.endGroup();
		return;
	}
	voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
	if (!voxelgenerator::lsystem::step(wrapper, _lsystemVoxel, _lsystemState, _lsystemExecState)) {
		_lsystemRunning = false;
		_mementoHandler.endGroup();
	}
	modified(_lsystemNodeId, wrapper.dirtyRegion());
}

float SceneManager::lsystemProgress() const {
	if (!_lsystemRunning || _lsystemState.sentence.empty()) {
		return 0.0f;
	}
	return (float)_lsystemExecState.index / (float)_lsystemState.sentence.size();
}

void SceneManager::setReferencePosition(const glm::ivec3& pos) {
	_modifierFacade.setReferencePosition(pos);
}

void SceneManager::moveCursor(int x, int y, int z) {
	glm::ivec3 p = cursorPosition();
	const int res = _modifierFacade.gridResolution();
	p.x += x * res;
	p.y += y * res;
	p.z += z * res;
	setCursorPosition(p, _modifierFacade.cursorFace(), true);
	_traceViaMouse = false;
	if (const voxel::RawVolume *v = activeVolume()) {
		const voxel::Voxel &voxel = v->voxel(cursorPosition());
		_modifierFacade.setHitCursorVoxel(voxel);
	}
}

void SceneManager::setCursorPosition(glm::ivec3 pos, voxel::FaceNames hitFace, bool force) {
	const voxel::RawVolume* v = activeVolume();
	if (v == nullptr) {
		return;
	}

	const int res = _modifierFacade.gridResolution();
	const voxel::Region& region = v->region();
	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3 delta = pos - mins;
	if (delta.x % res != 0) {
		pos.x = mins.x + (delta.x / res) * res;
	}
	if (delta.y % res != 0) {
		pos.y = mins.y + (delta.y / res) * res;
	}
	if (delta.z % res != 0) {
		pos.z = mins.z + (delta.z / res) * res;
	}

	const math::Axis lockedAxis = _modifierFacade.lockedAxis();

	// make a copy here - no reference - otherwise the comparison below won't
	// do anything else than comparing the same values.
	const glm::ivec3 oldCursorPos = cursorPosition();
	if (!force) {
		if ((lockedAxis & math::Axis::X) != math::Axis::None) {
			pos.x = oldCursorPos.x;
		}
		if ((lockedAxis & math::Axis::Y) != math::Axis::None) {
			pos.y = oldCursorPos.y;
		}
		if ((lockedAxis & math::Axis::Z) != math::Axis::None) {
			pos.z = oldCursorPos.z;
		}
	}

	if (!region.containsPoint(pos)) {
		pos = region.moveInto(pos.x, pos.y, pos.z);
	}
	_modifierFacade.setCursorPosition(pos, hitFace);
	if (oldCursorPos == pos) {
		return;
	}
	_dirtyRenderer |= DirtyRendererLockedAxis;
}

void SceneManager::updateDirtyRendererStates() {
	if (_dirtyRenderer & DirtyRendererLockedAxis) {
		_dirtyRenderer &= ~DirtyRendererLockedAxis;
		_sceneRenderer->updateLockedPlanes(_modifierFacade.lockedAxis(), _sceneGraph, cursorPosition());
	}
	if (_dirtyRenderer & DirtyRendererGridRenderer) {
		_dirtyRenderer &= ~DirtyRendererGridRenderer;
		_sceneRenderer->updateGridRegion(_sceneGraph.node(activeNode()).region());
	}
}

bool SceneManager::trace(bool sceneMode, bool force, const glm::mat4 &invModel) {
	if (_modifierFacade.isLocked()) {
		return false;
	}
	if (sceneMode) {
		return true;
	}
	if (_rotate.pressed()) {
		return false;
	}

	return mouseRayTrace(force, invModel);
}

int SceneManager::traceScene() {
	const int previousNodeId = activeNode();
	int nodeId = InvalidNodeId;
	core_trace_scoped(EditorSceneOnProcessUpdateRay);
	float intersectDist = _camera->farPlane();
	const math::Ray& ray = _camera->mouseRay(_mouseCursor);
	for (auto entry : _sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode& node = entry->second;
		if (previousNodeId == node.id()) {
			continue;
		}
		if (!node.isAnyModelNode()) {
			continue;
		}
		if (!node.visible()) {
			continue;
		}
		if (!_sceneRenderer->isVisible(node.id(), false)) {
			continue;
		}
		float distance = 0.0f;
		const voxel::Region& region = _sceneGraph.resolveRegion(node);
		const glm::vec3 pivot = node.pivot();
		const scenegraph::FrameTransform &transform = _sceneGraph.transformForFrame(node, _currentFrameIdx);
		const math::OBBF& obb = scenegraph::toOBB(true, region, pivot, transform);
		if (obb.intersect(ray.origin, ray.direction, distance)) {
			if (distance < intersectDist) {
				intersectDist = distance;
				nodeId = node.id();
			}
		}
	}
	Log::debug("Hovered node: %i", nodeId);
	return nodeId;
}

void SceneManager::updateCursor() {
	voxel::FaceNames hitFace = _result.hitFace;
	if (_modifierFacade.modifierTypeRequiresExistingVoxel()) {
		if (_result.didHit) {
			setCursorPosition(_result.hitVoxel, hitFace);
		} else if (_result.validPreviousPosition) {
			setCursorPosition(_result.previousPosition, hitFace);
		}
	} else if (_result.validPreviousPosition) {
		setCursorPosition(_result.previousPosition, hitFace);
	} else if (_result.didHit) {
		setCursorPosition(_result.hitVoxel, hitFace);
	}

	const voxel::RawVolume *v = activeVolume();
	if (_result.didHit && v != nullptr) {
		_modifierFacade.setHitCursorVoxel(v->voxel(_result.hitVoxel));
	} else {
		_modifierFacade.setHitCursorVoxel(voxel::Voxel());
	}
	if (v) {
		_modifierFacade.setVoxelAtCursor(v->voxel(_modifierFacade.cursorPosition()));
	}
}

bool SceneManager::mouseRayTrace(bool force, const glm::mat4 &invModel) {
	// mouse tracing is disabled - e.g. because the voxel cursor was moved by keyboard
	// shortcuts. In this case the execution of the modifier would result in a
	// re-execution of the trace. And that would move the voxel cursor to the mouse pos
	if (!_traceViaMouse) {
		return false;
	}
	// if the trace is not forced, and the mouse cursor position did not change, don't
	// re-execute the trace.
	if (_lastRaytraceX == _mouseCursor.x && _lastRaytraceY == _mouseCursor.y && !force) {
		return false;
	}
	const video::Camera *camera = activeCamera();
	if (camera == nullptr) {
		return false;
	}
	const int nodeId = activeNode();
	const scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		return false;
	}
	const voxel::RawVolume* v = _sceneRenderer->volumeForNode(*node);
	if (v == nullptr) {
		return false;
	}
	math::Ray ray = camera->mouseRay(_mouseCursor);
	ray.origin = glm::vec3(invModel * glm::vec4(ray.origin, 1.0f));
	ray.direction = glm::normalize(glm::vec3(invModel * glm::vec4(ray.direction, 0.0f)));
	const float rayLength = camera->farPlane();

	const glm::vec3& dirWithLength = ray.direction * rayLength;
	static constexpr voxel::Voxel air;

	Log::trace("Execute new trace for %i:%i (%i:%i)",
			_mouseCursor.x, _mouseCursor.y, _lastRaytraceX, _lastRaytraceY);

	core_trace_scoped(EditorSceneOnProcessUpdateRay);
	_lastRaytraceX = _mouseCursor.x;
	_lastRaytraceY = _mouseCursor.y;

	_result = {};
	_result.direction = ray.direction;

	const math::Axis lockedAxis = _modifierFacade.lockedAxis();
	// TODO: we could optionally limit the raycast to the selection

	const float offset = voxelutil::RaycastOffset;
	voxelutil::raycastWithEndpoints(v, ray.origin - offset, ray.origin + dirWithLength - offset, [&] (voxel::RawVolume::Sampler& sampler) {
		if (!_result.firstValidPosition && sampler.currentPositionValid()) {
			_result.firstPosition = sampler.position();
			_result.firstValidPosition = true;
		}

		if (!sampler.voxel().isSameType(air)) {
			_result.didHit = true;
			_result.hitVoxel = sampler.position();
			_result.hitFace = voxelutil::raycastFaceDetection(ray.origin, ray.direction, _result.hitVoxel, 0.0f, 1.0f).face;
			Log::debug("Raycast face hit: %i", (int)_result.hitFace);
			return false;
		}
		if (sampler.currentPositionValid()) {
			// while having an axis locked, we should end the trace if the ray leaves the
			// locked plane - but allow the ray to arrive at and travel along the plane so
			// that side faces of voxels on the plane can be selected
			if (lockedAxis != math::Axis::None && _result.validPreviousPosition) {
				const glm::ivec3& cursorPos = cursorPosition();
				bool currOnPlane = false;
				bool prevOnPlane = false;
				if ((lockedAxis & math::Axis::X) != math::Axis::None) {
					currOnPlane |= sampler.position()[0] == cursorPos[0];
					prevOnPlane |= _result.previousPosition[0] == cursorPos[0];
				}
				if ((lockedAxis & math::Axis::Y) != math::Axis::None) {
					currOnPlane |= sampler.position()[1] == cursorPos[1];
					prevOnPlane |= _result.previousPosition[1] == cursorPos[1];
				}
				if ((lockedAxis & math::Axis::Z) != math::Axis::None) {
					currOnPlane |= sampler.position()[2] == cursorPos[2];
					prevOnPlane |= _result.previousPosition[2] == cursorPos[2];
				}
				if (prevOnPlane && !currOnPlane) {
					// ray is leaving the locked plane - stop the trace and use the
					// last on-plane position that is already in _result.previousPosition.
					// use the locked axis to determine the face - the ray approaches the
					// plane from one side, so the face is the plane surface facing the ray
					if ((lockedAxis & math::Axis::X) != math::Axis::None) {
						_result.hitFace = ray.direction.x > 0.0f ? voxel::FaceNames::NegativeX : voxel::FaceNames::PositiveX;
					} else if ((lockedAxis & math::Axis::Y) != math::Axis::None) {
						_result.hitFace = ray.direction.y > 0.0f ? voxel::FaceNames::NegativeY : voxel::FaceNames::PositiveY;
					} else if ((lockedAxis & math::Axis::Z) != math::Axis::None) {
						_result.hitFace = ray.direction.z > 0.0f ? voxel::FaceNames::NegativeZ : voxel::FaceNames::PositiveZ;
					}
					return false;
				}
			}

			_result.validPreviousPosition = true;
			_result.previousPosition = sampler.position();
		} else if (_result.firstValidPosition && !_result.firstInvalidPosition) {
			_result.firstInvalidPosition = true;
			_result.hitVoxel = sampler.position();
			return false;
		}
		return true;
	});

	if (_result.firstInvalidPosition) {
		if (lockedAxis != math::Axis::None && !_result.didHit) {
			// ray exited the volume while on the locked plane without hitting a solid voxel
			if ((lockedAxis & math::Axis::X) != math::Axis::None) {
				_result.hitFace = ray.direction.x > 0.0f ? voxel::FaceNames::NegativeX : voxel::FaceNames::PositiveX;
			} else if ((lockedAxis & math::Axis::Y) != math::Axis::None) {
				_result.hitFace = ray.direction.y > 0.0f ? voxel::FaceNames::NegativeY : voxel::FaceNames::PositiveY;
			} else if ((lockedAxis & math::Axis::Z) != math::Axis::None) {
				_result.hitFace = ray.direction.z > 0.0f ? voxel::FaceNames::NegativeZ : voxel::FaceNames::PositiveZ;
			}
		} else {
			_result.hitFace = voxelutil::raycastFaceDetection(ray.origin, ray.direction, _result.hitVoxel, 0.0f, 1.0f).face;
		}
		Log::debug("Raycast face hit: %i", (int)_result.hitFace);
	}

	updateCursor();

	return true;
}

bool SceneManager::nodeUpdatePivot(scenegraph::SceneGraphNode &node, const glm::vec3 &pivot) {
	nodeSetPivot(node, pivot);
	_sceneGraph.updateTransforms(_transformUpdateChildren->boolVal());
	markDirty();
	_mementoHandler.markKeyFramesChange(_sceneGraph, node);
	return true;
}

bool SceneManager::nodeGroupUpdatePivot(const glm::vec3 &pivot) {
	nodeForeachGroup([&] (int groupNodeId) {
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(groupNodeId)) {
			nodeUpdatePivot(*node, pivot);
		}
	});
	return true;
}

bool SceneManager::nodeUpdatePivot(int nodeId, const glm::vec3 &pivot) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeUpdatePivot(*node, pivot);
	}
	return false;
}

bool SceneManager::nodeUpdateKeyFrameInterpolation(scenegraph::SceneGraphNode &node,
												   scenegraph::KeyFrameIndex keyFrameIdx,
												   scenegraph::InterpolationType interpolation) {
	if (node.keyFrames()->size() <= (size_t)keyFrameIdx) {
		Log::warn("Keyframe %i does not exist", (int)keyFrameIdx);
		return false;
	}
	node.keyFrame(keyFrameIdx).interpolation = interpolation;
	_mementoHandler.markKeyFramesChange(_sceneGraph, node);
	markDirty();
	return true;
}

bool SceneManager::nodeUpdateKeyFrameInterpolation(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx, scenegraph::InterpolationType interpolation) {
	if (nodeId == InvalidNodeId) {
		return false;
	}
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeUpdateKeyFrameInterpolation(*node, keyFrameIdx, interpolation);
	}
	return false;
}

bool SceneManager::nodeTransformMirror(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx,
									   math::Axis axis) {
	if (keyFrameIdx == InvalidKeyFrame) {
		scenegraph::FrameIndex frameIdx = currentFrame();
		keyFrameIdx = node.keyFrameForFrame(frameIdx);
	}
	scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
	scenegraph::SceneGraphTransform &transform = keyFrame.transform();
	if (axis == math::Axis::X) {
		transform.mirrorX();
	} else if ((axis & (math::Axis::X | math::Axis::Z)) == (math::Axis::X | math::Axis::Z)) {
		transform.mirrorXZ();
	} else if ((axis & (math::Axis::X | math::Axis::Y | math::Axis::Z)) ==
			   (math::Axis::X | math::Axis::Y | math::Axis::Z)) {
		transform.mirrorXYZ();
	} else {
		return false;
	}
	if (transform.dirty()) {
		transform.update(_sceneGraph, node, keyFrame.frameIdx, _transformUpdateChildren->boolVal());
		nodeKeyFramesChanged(node);
	}
	return true;
}

bool SceneManager::nodeTransformMirror(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx, math::Axis axis) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeTransformMirror(*node, keyFrameIdx, axis);
	}
	return false;
}

bool SceneManager::nodeGroupUpdateTransform(const glm::vec3 &angles, const glm::vec3 &scale, const glm::vec3 &translation,
											scenegraph::FrameIndex frameIdx, bool local) {
	nodeForeachGroup([&] (int groupNodeId) {
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(groupNodeId)) {
			const scenegraph::KeyFrameIndex keyFrameIdx = node->keyFrameForFrame(frameIdx);
			if (keyFrameIdx != InvalidKeyFrame) {
				nodeUpdateTransform(*node, angles, scale, translation, keyFrameIdx, local);
			}
		}
	});
	return true;
}

void SceneManager::nodeUpdatePartialVolume(scenegraph::SceneGraphNode &node, const voxel::RawVolume &volume) {
	if (!node.isAnyModelNode()) {
		return;
	}
	if (voxel::RawVolume *v = node.volume()) {
		v->copyInto(volume);
		modified(node.id(), volume.region());
	}
}

bool SceneManager::nodeUpdateTransform(int nodeId, const glm::vec3 &angles, const glm::vec3 &scale, const glm::vec3 &translation,
							 scenegraph::KeyFrameIndex keyFrameIdx, bool local) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeUpdateTransform(*node, angles, scale, translation, keyFrameIdx, local);
	}
	return false;
}

bool SceneManager::nodeUpdateTransform(int nodeId, const glm::mat4 &matrix,
									   scenegraph::KeyFrameIndex keyFrameIdx, bool local) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeUpdateTransform(*node, matrix, keyFrameIdx, local);
	}
	return false;
}

void SceneManager::nodeGroupResetTransform(scenegraph::KeyFrameIndex keyFrameIdx) {
	nodeForeachGroup([&] (int groupNodeId) {
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(groupNodeId)) {
			nodeResetTransform(*node, keyFrameIdx);
		}
	});
}

bool SceneManager::nodeResetTransform(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeResetTransform(*node, keyFrameIdx);
	}
	return false;
}

bool SceneManager::nodeAddKeyframe(scenegraph::SceneGraphNode &node, scenegraph::FrameIndex frameIdx) {
	const scenegraph::KeyFrameIndex newKeyFrameIdx = node.addKeyFrame(frameIdx);
	if (newKeyFrameIdx == InvalidKeyFrame) {
		Log::warn("Failed to add keyframe for frame %i", (int)frameIdx);
		return false;
	}
	Log::debug("node has %i keyframes", (int)node.keyFrames()->size());
	for (const auto& kf : *node.keyFrames()) {
		Log::debug("- keyframe %i", (int)kf.frameIdx);
	}
	if (newKeyFrameIdx > 0) {
		scenegraph::SceneGraphTransform copyFromTransform;
		if (node.isCameraNode() && activeCamera() != nullptr) {
			Log::debug("New keyframe for camera node %i at frame %i", node.id(), (int)frameIdx);
			if (video::Camera* camera = activeCamera()) {
				copyFromTransform.setWorldTranslation(camera->eye());
				copyFromTransform.setWorldOrientation(camera->orientation());
			}
		} else {
			scenegraph::KeyFrameIndex copyFromKeyFrameIdx = node.previousKeyFrameForFrame(frameIdx);
			core_assert(copyFromKeyFrameIdx != newKeyFrameIdx);
			copyFromTransform = node.keyFrame(copyFromKeyFrameIdx).transform();
			Log::debug("Assign transform from key frame %d to frame %d", (int)copyFromKeyFrameIdx, (int)frameIdx);
		}
		scenegraph::SceneGraphKeyFrame &copyToKeyFrame = node.keyFrame(newKeyFrameIdx);
		copyToKeyFrame.setTransform(copyFromTransform);
		_sceneGraph.updateTransforms();
		core_assert_msg(copyToKeyFrame.frameIdx == frameIdx, "Expected frame idx %d, got %d", (int)frameIdx, (int)copyToKeyFrame.frameIdx);
		_mementoHandler.markKeyFramesChange(_sceneGraph, node);
		markDirty();
		return true;
	}
	return false;
}

void SceneManager::nodeGroupAddKeyFrame(scenegraph::FrameIndex frameIdx) {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode &node = _sceneGraph.node(groupNodeId);
		nodeAddKeyframe(node, frameIdx);
	});
}

bool SceneManager::nodeAddKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeAddKeyframe(*node, frameIdx);
	}
	return false;
}

bool SceneManager::nodeAllAddKeyFrames(scenegraph::FrameIndex frameIdx) {
	for (auto iter = sceneGraph().beginAllModels(); iter != sceneGraph().end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		if (!node.hasKeyFrame(frameIdx)) {
			nodeAddKeyframe(node, frameIdx);
		}
	}
	return true;
}

void SceneManager::nodeGroupRemoveKeyFrame(scenegraph::FrameIndex frameIdx) {
	nodeForeachGroup([&](int groupNodeId) {
		scenegraph::SceneGraphNode &node = _sceneGraph.node(groupNodeId);
		nodeRemoveKeyFrame(node, frameIdx);
	});
}

bool SceneManager::nodeRemoveKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeRemoveKeyFrame(*node, frameIdx);
	}
	return false;
}

bool SceneManager::nodeRemoveKeyFrameByIndex(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeRemoveKeyFrameByIndex(*node, keyFrameIdx);
	}
	return false;
}

bool SceneManager::nodeRemoveKeyFrame(scenegraph::SceneGraphNode &node, scenegraph::FrameIndex frameIdx) {
	if (node.removeKeyFrame(frameIdx)) {
		_mementoHandler.markKeyFramesChange(_sceneGraph, node);
		markDirty();
		return true;
	}
	return false;
}

bool SceneManager::nodeRemoveKeyFrameByIndex(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx) {
	if (node.removeKeyFrameByIndex(keyFrameIdx)) {
		_mementoHandler.markKeyFramesChange(_sceneGraph, node);
		markDirty();
		return true;
	}
	return false;
}

void SceneManager::nodeKeyFramesChanged(scenegraph::SceneGraphNode &node) {
	_sceneGraph.invalidateFrameTransformCache(node.id());
	_mementoHandler.markKeyFramesChange(_sceneGraph, node);
	markDirty();
}

bool SceneManager::nodeShiftAllKeyframes(scenegraph::SceneGraphNode &node, const glm::vec3 &shift) {
	if (node.keyFrames() == nullptr) {
		return false;
	}
	for (auto &kf : *node.keyFrames()) {
		scenegraph::SceneGraphTransform &transform = kf.transform();
		const glm::vec3 &newLocalTranslation = transform.localTranslation() + shift;
		transform.setLocalTranslation(newLocalTranslation);
		transform.update(_sceneGraph, node, kf.frameIdx, false);
	}
	nodeKeyFramesChanged(node);
	return true;
}

bool SceneManager::nodeShiftAllKeyframes(int nodeId, const glm::vec3 &shift) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeShiftAllKeyframes(*node, shift);
	}
	return false;
}

bool SceneManager::nodeUpdateTransform(scenegraph::SceneGraphNode &node, const glm::vec3 &angles,
									   const glm::vec3 &scale, const glm::vec3 &translation,
									   scenegraph::KeyFrameIndex keyFrameIdx, bool local) {
	scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
	scenegraph::SceneGraphTransform &transform = keyFrame.transform();
	const glm::quat rot(glm::radians(angles));
	if (local) {
		transform.setLocalOrientation(rot);
		transform.setLocalScale(scale);
		transform.setLocalTranslation(translation);
	} else {
		transform.setWorldOrientation(rot);
		transform.setWorldScale(scale);
		transform.setWorldTranslation(translation);
	}
	if (transform.dirty()) {
		transform.update(_sceneGraph, node, keyFrame.frameIdx, _transformUpdateChildren->boolVal());
		nodeKeyFramesChanged(node);
	}
	return true;
}

bool SceneManager::nodeUpdateTransform(scenegraph::SceneGraphNode &node, const glm::mat4 &matrix,
									   scenegraph::KeyFrameIndex keyFrameIdx, bool local) {
	scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
	scenegraph::SceneGraphTransform &transform = keyFrame.transform();
	if (local) {
		transform.setLocalMatrix(matrix);
	} else {
		transform.setWorldMatrix(matrix);
	}
	if (transform.dirty()) {
		transform.update(_sceneGraph, node, keyFrame.frameIdx, _transformUpdateChildren->boolVal());
		nodeKeyFramesChanged(node);
	}

	return true;
}

void SceneManager::nodeSetPivot(scenegraph::SceneGraphNode &node, const glm::vec3 &pivot) {
	Log::debug("Set the pivot of node %i to %f:%f:%f", node.id(), pivot.x, pivot.y, pivot.z);
	// no memento states in here - this is just a helper
	const glm::vec3 oldPivot = node.pivot();
	if (node.setPivot(pivot)) {
		const glm::vec3 deltaPivot = pivot - oldPivot;
		const glm::vec3 size = node.region().getDimensionsInVoxels();
		node.localTranslate(deltaPivot * size);
	}
}

bool SceneManager::nodeResetTransform(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx) {
	nodeSetPivot(node, {0.0f, 0.0f, 0.0f});
	if (keyFrameIdx == InvalidKeyFrame) {
		scenegraph::FrameIndex frameIdx = currentFrame();
		keyFrameIdx = node.keyFrameForFrame(frameIdx);
	}
	scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
	scenegraph::SceneGraphTransform &transform = keyFrame.transform();
	transform.setLocalMatrix(glm::mat4(1.0f));
	_sceneGraph.updateTransforms();
	markDirty();
	_mementoHandler.markKeyFramesChange(_sceneGraph, node);
	return true;
}

int SceneManager::nodeReference(int nodeId) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		if (node->isReferenceNode()) {
			return nodeReference(node->reference());
		}
		return nodeReference(*node);
	}
	return InvalidNodeId;
}

bool SceneManager::nodeDuplicate(int nodeId, int *newNodeId) {
	if (nodeId == InvalidNodeId) {
		return false;
	}
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		nodeDuplicate(*node, newNodeId);
		return true;
	}
	return false;
}

bool SceneManager::nodeMove(int sourceNodeId, int targetNodeId, scenegraph::NodeMoveFlag flags) {
	if (_sceneGraph.changeParent(sourceNodeId, targetNodeId, flags)) {
		scenegraph::SceneGraphNode *node = sceneGraphNode(sourceNodeId);
		core_assert(node != nullptr);
		_mementoHandler.markNodeMoved(_sceneGraph, *node);
		markDirty();
		return true;
	}
	return false;
}

bool SceneManager::nodeSetProperty(int nodeId, const core::String &key, const core::String &value) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		if (node->setProperty(key, value)) {
			_mementoHandler.markNodePropertyChange(_sceneGraph, *node);
			return true;
		}
	}
	return false;
}

bool SceneManager::nodeRemoveProperty(int nodeId, const core::String &key) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		if (node->properties().remove(key)) {
			_mementoHandler.markNodePropertyChange(_sceneGraph, *node);
			return true;
		}
	}
	return false;
}

bool SceneManager::nodeSetIKConstraint(int nodeId, const scenegraph::IKConstraint &constraint) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		node->setIkConstraint(constraint);
		_mementoHandler.markIKConstraintChange(_sceneGraph, *node);
		markDirty();
		return true;
	}
	return false;
}

bool SceneManager::nodeRemoveIKConstraint(int nodeId) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		if (!node->hasIKConstraint()) {
			return false;
		}
		node->removeIkConstraint();
		_mementoHandler.markIKConstraintChange(_sceneGraph, *node);
		markDirty();
		return true;
	}
	return false;
}

bool SceneManager::nodeRename(int nodeId, const core::String &name) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeRename(*node, name);
	}
	return false;
}

bool SceneManager::nodeRemoveChildrenByType(scenegraph::SceneGraphNode &node, scenegraph::SceneGraphNodeType type) {
	core::Buffer<int> removeChildren;
	for (int childId : node.children()) {
		const scenegraph::SceneGraphNode &childNode = _sceneGraph.node(childId);
		if (childNode.type() == type) {
			removeChildren.push_back(childId);
		}
	}
	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "noderemovebytype");
	for (int childId : removeChildren) {
		if (!nodeRemove(childId, true)) {
			Log::error("Failed to remove child node %i", childId);
			return false;
		}
	}
	return true;
}

bool SceneManager::nodeRename(scenegraph::SceneGraphNode &node, const core::String &name) {
	if (node.name() == name) {
		return true;
	}
	node.setName(name);
	_mementoHandler.markNodeRenamed(_sceneGraph, node);
	markDirty();
	return true;
}

bool SceneManager::nodeSetVisible(int nodeId, bool visible) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		node->setVisible(visible);
		if (node->type() == scenegraph::SceneGraphNodeType::Group) {
			_sceneGraph.visitChildren(nodeId, true, [visible] (scenegraph::SceneGraphNode &childNode) {
				childNode.setVisible(visible);
			});
		}
		_sceneRenderer->markDirty();
		return true;
	}
	return false;
}

bool SceneManager::nodeSetLocked(int nodeId, bool locked) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		node->setLocked(locked);
		return true;
	}
	return false;
}

bool SceneManager::nodeRemove(int nodeId, bool recursive) {
	if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
		return nodeRemove(*node, recursive);
	}
	return false;
}

bool SceneManager::exceedsMaxSuggestedVolumeSize() const {
	const int maxDim = _maxSuggestedVolumeSize->intVal();
	const int64_t maxVoxels = (int64_t)maxDim * (int64_t)maxDim * (int64_t)maxDim;
	const voxel::Region &region = _sceneGraph.maxRegion();
	return region.voxels() > maxVoxels;
}

void SceneManager::markDirty() {
	_sceneGraph.markMaxFramesDirty();
	// we only autosave if the volumes in the scene graph are not exceeding the
	// max suggested voxel count
	_needAutoSave = !exceedsMaxSuggestedVolumeSize();
	_dirty = true;
	_sceneRenderer->markDirty();
}

bool SceneManager::nodeRemove(scenegraph::SceneGraphNode &node, bool recursive) {
	const int nodeId = node.id();
	core::String name = node.name();
	Log::debug("Delete node %i with name %s", nodeId, name.c_str());
	core::Buffer<int> removeReferenceNodes;
	for (auto iter = _sceneGraph.begin(scenegraph::SceneGraphNodeType::ModelReference); iter != _sceneGraph.end(); ++iter) {
		if ((*iter).reference() == nodeId) {
			removeReferenceNodes.push_back((*iter).id());
		}
	}
	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "noderemove");
	for (int refNodeId : removeReferenceNodes) {
		nodeRemove(_sceneGraph.node(refNodeId), recursive);
	}
	if (recursive) {
		// create a copy to prevent concurrent modification errors
		scenegraph::SceneGraphNodeChildren childrenCopy = node.children();
		for (int child : childrenCopy) {
			if (!nodeRemove(child, recursive)) {
				Log::error("Failed to remove child node %i", child);
				return false;
			}
		}
	}
	_mementoHandler.markNodeRemove(_sceneGraph, node);
	if (!_sceneGraph.removeNode(nodeId, false)) {
		Log::error("Failed to remove node with id %i", nodeId);
		return false;
	}
	_sceneRenderer->removeNode(nodeId);
	if (_sceneGraph.empty()) {
		const voxel::Region &region = voxel::Region::fromSize(32);
		scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
		newNode.createVolume(region);
		if (name.empty()) {
			newNode.setName("unnamed");
		} else {
			newNode.setName(name);
		}
		moveNodeToSceneGraph(newNode);
	} else {
		markDirty();
	}
	return true;
}

void SceneManager::nodeDuplicate(const scenegraph::SceneGraphNode &node, int *newNodeId) {
	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "nodeduplicate");
	const int nodeId = scenegraph::copyNodeToSceneGraph(_sceneGraph, node, node.parent(), true);
	onNewNodeAdded(nodeId, false);
	if (newNodeId) {
		*newNodeId = nodeId;
	}
}

int SceneManager::nodeReference(const scenegraph::SceneGraphNode &node) {
	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "nodereference");
	const int newNodeId = scenegraph::createNodeReference(_sceneGraph, node);
	onNewNodeAdded(newNodeId, false);
	return newNodeId;
}

bool SceneManager::isValidReferenceNode(const scenegraph::SceneGraphNode &node) const {
	if (node.type() != scenegraph::SceneGraphNodeType::ModelReference) {
		Log::error("Node %i is not a reference model", node.id());
		return false;
	}
	if (!_sceneGraph.hasNode(node.reference())) {
		Log::error("Node %i is not valid anymore - referenced node doesn't exist", node.id());
		return false;
	}
	return true;
}

bool SceneManager::nodeUnreference(scenegraph::SceneGraphNode &node) {
	if (!isValidReferenceNode(node)) {
		return false;
	}
	if (scenegraph::SceneGraphNode* referencedNode = sceneGraphNode(node.reference())) {
		if (referencedNode->type() != scenegraph::SceneGraphNodeType::Model) {
			Log::error("Referenced node is no model node - failed to unreference");
			return false;
		}
		if (!node.unreferenceModelNode(*referencedNode)) {
			return false;
		}
		modified(node.id(), node.volume()->region());
		return true;
	}
	Log::error("Referenced node is wasn't found - failed to unreference");
	return false;
}

bool SceneManager::nodeUnreference(int nodeId) {
	if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
		return nodeUnreference(*node);
	}
	return false;
}

bool SceneManager::nodeReduceColors(scenegraph::SceneGraphNode &node, const core::Buffer<uint8_t> &srcPalIdx, uint8_t targetPalIdx) {
	voxel::RawVolume *v = _sceneGraph.resolveVolume(node);
	if (v == nullptr) {
		return false;
	}
	palette::Palette &palette = node.palette();
	const voxel::Voxel replacementVoxel = voxel::createVoxel(palette, targetPalIdx);
	voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
	auto func = [&wrapper, &srcPalIdx, replacementVoxel](int x, int y, int z, const voxel::Voxel &voxel) {
		for (uint8_t srcPal : srcPalIdx) {
			if (voxel.getColor() == srcPal) {
				wrapper.setVoxel(x, y, z, replacementVoxel);
				break;
			}
		}
	};
	voxelutil::visitVolumeParallel(wrapper, func);
	modified(node.id(), wrapper.dirtyRegion());
	return true;
}

bool SceneManager::nodeRemoveColor(scenegraph::SceneGraphNode &node, uint8_t palIdx) {
	voxel::RawVolume *v = _sceneGraph.resolveVolume(node);
	if (v == nullptr) {
		return false;
	}
	palette::Palette &palette = node.palette();
	const uint8_t replacement = palette.findReplacement(palIdx);
	if (replacement != palIdx && palette.removeColor(palIdx)) {
		memento::ScopedMementoGroup mementoGroup(_mementoHandler, "removecolor");
		palette.markSave();
		const voxel::Voxel replacementVoxel = voxel::createVoxel(palette, replacement);
		_mementoHandler.markPaletteChange(_sceneGraph, node);
		voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
		auto func = [&wrapper, replacementVoxel](int x, int y, int z, const voxel::Voxel &) {
			wrapper.setVoxel(x, y, z, replacementVoxel);
		};
		voxelutil::visitVolumeParallel(wrapper, func, voxelutil::VisitVoxelColor(palIdx));
		modified(node.id(), wrapper.dirtyRegion());
		if (_modifierFacade.cursorVoxel().getColor() == palIdx) {
			_modifierFacade.setCursorVoxel(replacementVoxel);
		}
		return true;
	}
	return false;
}

bool SceneManager::nodeReduceColors(int nodeId, const core::Buffer<uint8_t> &srcPalIdx, uint8_t targetPalIdx) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeReduceColors(*node, srcPalIdx, targetPalIdx);
	}
	return false;
}

bool SceneManager::nodeQuantizeColors(scenegraph::SceneGraphNode &node, const core::Buffer<uint8_t> &selectedIndices, int targetColorCount) {
	const int selectedCount = (int)selectedIndices.size();
	if (selectedCount <= 1 || targetColorCount <= 0 || targetColorCount >= selectedCount) {
		return false;
	}

	voxel::RawVolume *v = _sceneGraph.resolveVolume(node);
	if (v == nullptr) {
		return false;
	}

	palette::Palette &palette = node.palette();
	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "quantizecolors");
	_mementoHandler.markPaletteChange(_sceneGraph, node);

	// collect the RGBA colors for the selected indices
	color::RGBA inputColors[palette::PaletteMaxColors];
	for (int i = 0; i < selectedCount; ++i) {
		inputColors[i] = palette.color(selectedIndices[i]);
	}

	// quantize to target count
	color::RGBA quantizedColors[palette::PaletteMaxColors];
	const color::ColorReductionType reductionType =
		color::toColorReductionType(core::getVar(cfg::CoreColorReduction)->strVal().c_str());
	const int quantizedCount = color::quantize(quantizedColors, (size_t)targetColorCount, inputColors, (size_t)selectedCount, reductionType);
	if (quantizedCount <= 0) {
		return false;
	}

	// for each selected palette index, find the closest quantized color
	// group[i] = index into quantizedColors that this selected index maps to
	int mapping[palette::PaletteMaxColors];
	for (int i = 0; i < selectedCount; ++i) {
		float minDist = FLT_MAX;
		int bestMatch = 0;
		const color::RGBA c = inputColors[i];
		for (int q = 0; q < quantizedCount; ++q) {
			const float dist = color::getDistance(c, quantizedColors[q], color::Distance::Approximation);
			if (dist < minDist) {
				minDist = dist;
				bestMatch = q;
			}
		}
		mapping[i] = bestMatch;
	}

	// for each quantized color group, pick the first selected index as the "keeper",
	// remap voxels from all other indices in the group to the keeper, then clear them
	for (int q = 0; q < quantizedCount; ++q) {
		int keeperSelectedIdx = -1;
		core::Buffer<uint8_t> srcPalIdx;
		for (int i = 0; i < selectedCount; ++i) {
			if (mapping[i] != q) {
				continue;
			}
			if (keeperSelectedIdx == -1) {
				keeperSelectedIdx = i;
			} else {
				srcPalIdx.push_back(selectedIndices[i]);
			}
		}
		if (keeperSelectedIdx == -1) {
			continue;
		}
		const uint8_t keeperPalIdx = selectedIndices[keeperSelectedIdx];
		// set the keeper to the quantized color
		palette.setColor(keeperPalIdx, quantizedColors[q]);

		if (!srcPalIdx.empty()) {
			// remap voxels from the other indices to the keeper
			const voxel::Voxel replacementVoxel = voxel::createVoxel(palette, keeperPalIdx);
			voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
			auto func = [&wrapper, &srcPalIdx, replacementVoxel](int x, int y, int z, const voxel::Voxel &voxel) {
				for (uint8_t srcPal : srcPalIdx) {
					if (voxel.getColor() == srcPal) {
						wrapper.setVoxel(x, y, z, replacementVoxel);
						break;
					}
				}
			};
			voxelutil::visitVolumeParallel(wrapper, func);
			modified(node.id(), wrapper.dirtyRegion());

			// clear the colors that were quantized away
			for (uint8_t idx : srcPalIdx) {
				palette.setColor(idx, color::RGBA(0, 0, 0, 0));
			}
		}
	}

	palette.markSave();
	palette.markDirty();
	return true;
}

bool SceneManager::nodeQuantizeColors(int nodeId, const core::Buffer<uint8_t> &selectedIndices, int targetColorCount) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeQuantizeColors(*node, selectedIndices, targetColorCount);
	}
	return false;
}

bool SceneManager::nodeRemoveColor(int nodeId, uint8_t palIdx) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeRemoveColor(*node, palIdx);
	}
	return false;
}

bool SceneManager::nodeDuplicateColor(scenegraph::SceneGraphNode &node, uint8_t palIdx) {
	palette::Palette &palette = node.palette();
	palette.duplicateColor(palIdx);
	palette.markSave();
	_mementoHandler.markPaletteChange(_sceneGraph, node);
	return true;
}

bool SceneManager::nodeDuplicateColor(int nodeId, uint8_t palIdx) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeDuplicateColor(*node, palIdx);
	}
	return false;
}

bool SceneManager::nodeRemoveAlpha(scenegraph::SceneGraphNode &node, uint8_t palIdx) {
	palette::Palette &palette = node.palette();
	color::RGBA c = palette.color(palIdx);
	if (c.a == 255) {
		return false;
	}
	c.a = 255;
	palette.setColor(palIdx, c);
	palette.markSave();
	_mementoHandler.markPaletteChange(_sceneGraph, node);
	nodeUpdateVoxelType(node.id(), palIdx, voxel::VoxelType::Generic);
	return true;
}

bool SceneManager::nodeRemoveAlpha(int nodeId, uint8_t palIdx) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeRemoveAlpha(*node, palIdx);
	}
	return false;
}

bool SceneManager::nodeSetMaterial(scenegraph::SceneGraphNode &node, uint8_t palIdx, palette::MaterialProperty material, float value) {
	palette::Palette &palette = node.palette();
	palette.setMaterialValue(palIdx, material, value);
	palette.markSave();
	_mementoHandler.markPaletteChange(_sceneGraph, node);
	return true;
}

bool SceneManager::nodeSetMaterial(int nodeId, uint8_t palIdx, palette::MaterialProperty material, float value) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeSetMaterial(*node, palIdx, material, value);
	}
	return false;
}

bool SceneManager::nodeSetColor(scenegraph::SceneGraphNode &node, uint8_t palIdx, const color::RGBA &color) {
	palette::Palette &palette = node.palette();
	const bool existingColor = palIdx < palette.colorCount();
	const bool oldHasAlpha = palette.color(palIdx).a != 255;
	palette.setColor(palIdx, color);
	const bool newHasAlpha = color.a != 255;
	if (existingColor) {
		if (oldHasAlpha && !newHasAlpha) {
			nodeUpdateVoxelType(node.id(), palIdx, voxel::VoxelType::Generic);
		} else if (!oldHasAlpha && newHasAlpha) {
			nodeUpdateVoxelType(node.id(), palIdx, voxel::VoxelType::Transparent);
		}
	}
	palette.markSave();
	_mementoHandler.markPaletteChange(_sceneGraph, node);
	return true;
}

bool SceneManager::nodeSetColor(int nodeId, uint8_t palIdx, const color::RGBA &color) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeSetColor(*node, palIdx, color);
	}
	return false;
}

void SceneManager::nodeForeachGroup(const core::Function<void(int)>& f) {
	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "group");
	_sceneGraph.foreachGroup(f);
}

void SceneManager::nodeGroupRemoveNormals() {
	nodeForeachGroup([&] (int groupNodeId) {
		nodeRemoveNormals(groupNodeId);
	});
}

bool SceneManager::nodeRemoveNormals(int nodeId) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		if (!node->isAnyModelNode()) {
			return false;
		}
		voxel::RawVolume *v = _sceneGraph.resolveVolume(*node);
		if (v == nullptr) {
			return false;
		}
		voxel::RawVolumeWrapper wrapper = _modifierFacade.createRawVolumeWrapper(v);
		auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &voxel) {
			if (voxel.getNormal() == NO_NORMAL) {
				return;
			}
			voxel::Voxel newVoxel = voxel::createVoxel(voxel.getMaterial(), voxel.getColor(), NO_NORMAL, voxel.getFlags());
			newVoxel.setNormalReset();
			wrapper.setVoxel(x, y, z, newVoxel);
		};
		voxelutil::visitVolumeParallel(wrapper, func);
		modified(node->id(), wrapper.dirtyRegion(), SceneModifiedFlags::NoResetTrace);
		return true;
	}
	return false;
}

bool SceneManager::nodeActivate(int nodeId) {
	if (nodeId == InvalidNodeId) {
		return false;
	}
	if (!_sceneGraph.hasNode(nodeId)) {
		Log::warn("Given node id %i doesn't exist", nodeId);
		return false;
	}
	if (_sceneGraph.activeNode() == nodeId) {
		return true;
	}
	Log::debug("Activate node %i", nodeId);
	// cancel any in-progress brush operation before switching nodes
	_modifierFacade.abort();
	const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	// a node switch will disable the locked axis as the positions might have changed anyway
	modifier().setLockedAxis(math::Axis::X | math::Axis::Y | math::Axis::Z, true);
	_sceneGraph.setActiveNode(nodeId);
	const palette::Palette &palette = node.palette();
	for (int i = 0; i < palette.colorCount(); ++i) {
		if (palette.color(i).a > 0) {
			_modifierFacade.setCursorVoxel(voxel::createVoxel(palette, i));
			break;
		}
	}

	if (node.isModelNode()) {
		const voxel::Region& region = node.region();
		_dirtyRenderer |= DirtyRendererGridRenderer;
		if (!region.containsPoint(referencePosition())) {
			const glm::ivec3 pivot = region.getLowerCorner() + glm::ivec3(node.pivot() * glm::vec3(region.getDimensionsInVoxels()));
			setReferencePosition(glm::ivec3(pivot));
		}
		if (!region.containsPoint(cursorPosition())) {
			setCursorPosition(region.getCenter(), voxel::FaceNames::Max);
		}
		resetLastTrace();
	}
	return true;
}

bool SceneManager::empty() const {
	return _sceneGraph.empty();
}

void SceneManager::setActiveCamera(video::Camera *camera, bool fixedCamera) {
	_fixedCamera = fixedCamera;
	if (_camera == camera) {
		return;
	}
	_camera = camera;
	resetLastTrace();
}

core::String SceneManager::getSuggestedFilename(const core::String &extension) const {
	if (extension.first() == '.') {
		return getSuggestedFilename(extension.substr(1));
	}
	const io::FileDescription &fileDesc = _lastFilename;
	core::String name = "scene";
	core::String ext = extension;
	if (!fileDesc.empty()) {
		name = core::string::stripExtension(fileDesc.name);
		if (ext.empty()) {
			ext = core::string::extractExtension(fileDesc.name);
		}
	}
	if (ext.empty()) {
		ext = fileDesc.desc.mainExtension();
	}
	if (ext.empty()) {
		ext = voxelformat::VENGIFormat::format().mainExtension();
	}
	return name + "." + ext;
}

const voxel::Region &SceneManager::sliceRegion() const {
	return _sceneRenderer->sliceRegion();
}

void SceneManager::setSliceRegion(const voxel::Region &region) {
	_sceneRenderer->setSliceRegion(region);
}

bool SceneManager::isSliceModeActive() const {
	return _sceneRenderer->isSliceModeActive();
}

}
