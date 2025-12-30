/**
 * @file
 */

#include "SceneManager.h"

#include "app/Async.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "core/ArrayLength.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include "io/FormatDescription.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include "math/Axis.h"
#include "math/Random.h"
#include "math/Ray.h"
#include "memento/MementoHandler.h"
#include "metric/MetricFacade.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"
#include "palette/PaletteCompleter.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "scenegraph/SceneGraphUtil.h"
#include "scenegraph/SceneUtil.h"
#include "video/Camera.h"
#include "voxedit-util/ModelNodeSettings.h"
#include "voxedit-util/modifier/SceneModifiedFlags.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"
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
#include "voxelrender/SceneGraphRenderer.h"
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
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"

#include "Clipboard.h"
#include "Config.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/transform.hpp>

namespace voxedit {

inline auto nodeCompleter(const scenegraph::SceneGraph &sceneGraph) {
	return [&] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		int i = 0;
		for (const auto &entry : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = entry->value;
			if (!node.isAnyModelNode()) {
				continue;
			}
			matches.push_back(core::string::toString(node.id()));
		}
		return i;
	};
}

SceneManager::SceneManager(const core::TimeProviderPtr &timeProvider, const io::FilesystemPtr &filesystem,
						   const SceneRendererPtr &sceneRenderer, const ModifierRendererPtr &modifierRenderer,
						   const SelectionManagerPtr &selectionManager)
	: _timeProvider(timeProvider), _sceneRenderer(sceneRenderer),
	  _modifierFacade(this, modifierRenderer, selectionManager), _luaApi(filesystem),
	  _luaApiListener(this, _mementoHandler, _sceneGraph), _filesystem(filesystem), _selectionManager(selectionManager),
	  _client(this) {
	server().setState(&_sceneGraph);
}

SceneManager::~SceneManager() {
	core_assert_msg(_initialized == 0, "SceneManager was not properly shut down");
}

bool SceneManager::loadPalette(const core::String& paletteName, bool searchBestColors, bool save) {
	palette::Palette palette;

	const bool isNodePalette = core::string::startsWith(paletteName, "node:");
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
	core::Var::getSafe(cfg::VoxEditLastPalette)->setVal(paletteName);

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
		core::Var::getSafe(cfg::VoxEditLastPalette)->setVal(paletteName);
	}

	if (!setActive) {
		return true;
	}
	return setActivePalette(palette, searchBestColors);
}

bool SceneManager::calculateNormals(int nodeId, voxel::Connectivity connectivity, bool recalcAll, bool fillAndHollow) {
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
		const io::FilePtr &file = _filesystem->open(_lastFilename.name);
		const core::String &filename = file->fileName();
		const core::String &prefix = core::string::startsWith(filename, "autosave-") ? "" : "autosave-";
		const core::String &ext = file->extension();
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

bool SceneManager::saveNode(int nodeId, const core::String& file) {
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
		newNode.setVolume(_sceneGraph.resolveVolume(*node));
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

void SceneManager::fillHollow() {
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

void SceneManager::fill() {
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

void SceneManager::clear() {
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

void SceneManager::hollow() {
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
		return;
	}
	voxel::RawVolume *v = volume(nodeId);
	if (v == nullptr) {
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
		state |= saveNode(node.id(), filename);
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
			core::Var::get(cfg::UIFileDialogLastFile)->setVal(file.name);
		}
		_needAutoSave = false;
		return true;
	}
	Log::warn("Failed to save to desired format (%s)", file.c_str());
	return false;
}

static void mergeIfNeeded(scenegraph::SceneGraph &newSceneGraph) {
	if (newSceneGraph.size() > voxel::MAX_VOLUMES) {
		// TODO: don't merge everything blindly. Check if it's enough to merge single nodes here.
		// TODO: PERF: merging will take a lot of time for large scenes
		Log::debug("Merging scenegraph nodes as the number of volumes exceeds the limit of %i", voxel::MAX_VOLUMES);
		const scenegraph::SceneGraph::MergeResult &merged = newSceneGraph.merge();
		if (!merged.hasVolume()) {
			Log::error("Failed to merge the scenegraph nodes");
			return;
		}
		newSceneGraph.clear();
		scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
		newNode.setVolume(merged.volume(), true);
		newNode.setPalette(merged.palette);
		newNode.setNormalPalette(merged.normalPalette);
		newSceneGraph.emplace(core::move(newNode));
		Log::debug("Merging successful");
	}
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
	mergeIfNeeded(newSceneGraph);

	scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
	groupNode.setName(core::string::extractFilename(file));
	int importGroupNodeId = _sceneGraph.emplace(core::move(groupNode), activeNode());
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
			mergeIfNeeded(newSceneGraph);
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
		mergeIfNeeded(newSceneGraph);
		return core::move(newSceneGraph);
	});
	_lastFilename.set(file.name, &file.desc);
	core::Var::get(cfg::UIFileDialogLastFile)->setVal(file.name);
	return true;
}

bool SceneManager::load(const io::FileDescription& file, const uint8_t *data, size_t size) {
	scenegraph::SceneGraph newSceneGraph;
	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add(file.name, data, size);
	voxelformat::LoadContext loadCtx;
	voxelformat::loadFormat(file, archive, newSceneGraph, loadCtx);
	mergeIfNeeded(newSceneGraph);
	if (loadSceneGraph(core::move(newSceneGraph))) {
		_needAutoSave = false;
		_dirty = false;
		_lastFilename.clear();
	}
	return true;
}

void SceneManager::setMousePos(int x, int y) {
	if (_mouseCursor.x == x && _mouseCursor.y == y) {
		return;
	}
	_mouseCursor.x = x;
	_mouseCursor.y = y;
	// moving the mouse would trigger mouse tracing again
	// TODO: maybe only do this if a mouse button was pressed?
	_traceViaMouse = true;
}

glm::mat4 SceneManager::worldMatrix(scenegraph::FrameIndex frameIdx, bool applyTransforms) const {
	const int nodeId = _sceneGraph.activeNode();
	const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	return _sceneGraph.worldMatrix(node, frameIdx, applyTransforms);
}

bool SceneManager::supportsEditMode() const {
	const int nodeId = _sceneGraph.activeNode();
	const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	return node.isModelNode();
}

void SceneManager::modified(int nodeId, const voxel::Region& modifiedRegion, SceneModifiedFlags flags, uint64_t renderRegionMillis) {
	const bool markUndo = (flags & SceneModifiedFlags::MarkUndo) == SceneModifiedFlags::MarkUndo;
	Log::debug("Modified node %i, record undo state: %s", nodeId, markUndo ? "true" : "false");
	voxel::logRegion("Modified", modifiedRegion);
	if (markUndo) {
		const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		_mementoHandler.markModification(_sceneGraph, node, modifiedRegion);
	}
	if (modifiedRegion.isValid()) {
		Log::debug("Modify region for nodeid %i", nodeId);
		_sceneRenderer->updateNodeRegion(nodeId, modifiedRegion, renderRegionMillis);
	}
	markDirty();
	const bool resetTrace = (flags & SceneModifiedFlags::ResetTrace) == SceneModifiedFlags::ResetTrace;
	if (resetTrace) {
		resetLastTrace();
	}
}

int SceneManager::colorToNewNode(const voxel::Voxel voxelColor) {
	const int nodeId = _sceneGraph.activeNode();
	return colorToNewNode(nodeId, voxelColor);
}

int SceneManager::colorToNewNode(int nodeId, const voxel::Voxel voxelColor) {
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
	newNode.setVolume(newVolume, true);
	newNode.setName(core::String::format("color: %i", (int)voxelColor.getColor()));
	return moveNodeToSceneGraph(newNode, node.parent());
}

void SceneManager::scaleUp(int nodeId) {
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

void SceneManager::scaleDown(int nodeId) {
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

void SceneManager::splitObjects() {
	const int nodeId = activeNode();
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
		newNode.setVolume(newVolume, true);
		newNode.setName(node->name());
		newNode.setPalette(node->palette());
		moveNodeToSceneGraph(newNode, nodeId);
	}
}

void SceneManager::crop() {
	const int nodeId = activeNode();
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
	const glm::ivec3 oldMins = oldRegion.getLowerCorner();
	const glm::ivec3 oldMaxs = oldRegion.getUpperCorner();
	const glm::ivec3 mins = region.getLowerCorner();
	const glm::ivec3 maxs = region.getUpperCorner();
	if (glm::all(glm::greaterThanEqual(maxs, oldMaxs)) && glm::all(glm::lessThanEqual(mins, oldMins))) {
		// we don't have to re-extract a mesh if only new empty voxels were added.
		modified(nodeId, voxel::Region::InvalidRegion);
	} else {
		memento::ScopedMementoGroup mementoGroup(_mementoHandler, "resize");
		const core::Buffer<voxel::Region> &regions = voxel::Region::subtract(oldRegion, region);
		for (const voxel::Region &r : regions) {
			modified(nodeId, r);
		}
	}

	if (activeNode() == nodeId) {
		const glm::ivec3 &refPos = referencePosition();
		if (!region.containsPoint(refPos)) {
			setReferencePosition(region.getCenter());
		}
	}
}

void SceneManager::resizeAll(const glm::ivec3& size) {
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
	return _sceneGraph.setAnimations(*animations);
}

bool SceneManager::mementoKeyFrames(const memento::MementoState& s) {
	const core::String &uuidStr = s.nodeUUID.str();
	Log::debug("Memento: keyframes of node %s (%s)", uuidStr.c_str(), s.name.c_str());
	if (scenegraph::SceneGraphNode *node = sceneGraphNodeByUUID(s.nodeUUID)) {
		_sceneGraph.setAllKeyFramesForNode(*node, s.keyFrames);
		node->setPivot(s.pivot);
		_sceneGraph.updateTransforms();
		return true;
	}
	return false;
}

bool SceneManager::mementoPaletteChange(const memento::MementoState &s) {
	const core::String &uuidStr = s.nodeUUID.str();
	Log::debug("Memento: palette change of node %s to %s", uuidStr.c_str(), s.name.c_str());
	if (scenegraph::SceneGraphNode* node = sceneGraphNodeByUUID(s.nodeUUID)) {
		node->setPalette(s.palette);
		return true;
	}
	return false;
}

bool SceneManager::mementoNormalPaletteChange(const memento::MementoState &s) {
	const core::String &uuidStr = s.nodeUUID.str();
	Log::debug("Memento: normal palette change of node %s to %s", uuidStr.c_str(), s.name.c_str());
	if (scenegraph::SceneGraphNode* node = sceneGraphNodeByUUID(s.nodeUUID)) {
		node->setNormalPalette(s.normalPalette);
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
		newNode.setVolume(new voxel::RawVolume(s.dataRegion()), true);
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
				return nodeRemove(*node, true);
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
				return nodeRemove(*node, true);
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
	return true;
}

bool SceneManager::redo(int n) {
	Log::debug("redo %i steps", n);
	for (int i = 0; i < n; ++i) {
		if (!doRedo()) {
			return false;
		}
	}
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
	for (int i = n; i >= 0; --i) {
		const memento::MementoState& s = group.states[i];
		if (!mementoStateExecute(s, false)) {
			Log::error("Failed to undo memento state %i", (int)s.type);
			return false;
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

bool SceneManager::saveSelection(const io::FileDescription& file) {
	if (!_selectionManager->hasSelection()) {
		return false;
	}
	const int nodeId = activeNode();
	const scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		Log::warn("Node with id %i wasn't found", nodeId);
		return true;
	}
	if (node->type() != scenegraph::SceneGraphNodeType::Model) {
		Log::warn("Given node is no model node");
		return false;
	}

	voxelformat::SaveContext saveCtx;
	saveCtx.thumbnailCreator = voxelrender::volumeThumbnail;

	const io::ArchivePtr &archive = io::openFilesystemArchive(_filesystem);

	const voxel::RawVolume *volume = _sceneGraph.resolveVolume(*node);
	scenegraph::SceneGraph newSceneGraph;
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	scenegraph::copyNode(*node, newNode, false);
	voxel::RawVolume *v = _selectionManager->copy(*volume);
	newNode.setVolume(v, true);
	newSceneGraph.emplace(core::move(newNode));
	if (!voxelformat::saveFormat(newSceneGraph, file.name, &file.desc, archive, saveCtx)) {
		Log::warn("Failed to save node %i to %s", nodeId, file.name.c_str());
		return false;
	}
	Log::info("Saved node %i to %s", nodeId, file.name.c_str());
	return true;
}

bool SceneManager::copy() {
	if (!_selectionManager->hasSelection()) {
		Log::debug("Nothing selected yet - failed to copy");
		return false;
	}
	const int nodeId = activeNode();
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return false;
	}
	voxel::ClipboardData voxelData(node->volume(), node->palette(), false);
	_copy = voxedit::tool::copy(voxelData, _selectionManager);
	return _copy;
}

bool SceneManager::pasteAsNewNode() {
	if (!_copy) {
		Log::debug("Nothing copied yet - failed to paste");
		return false;
	}
	const int nodeId = activeNode();
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return false;
	}
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	scenegraph::copyNode(*node, newNode, false);
	newNode.setVolume(new voxel::RawVolume(*_copy.volume), true);
	newNode.setPalette(*_copy.palette);
	return moveNodeToSceneGraph(newNode, node->parent()) != InvalidNodeId;
}

bool SceneManager::paste(const glm::ivec3& pos) {
	if (!_copy) {
		Log::debug("Nothing copied yet - failed to paste");
		return false;
	}
	const int nodeId = activeNode();
	scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
	if (node == nullptr) {
		return false;
	}
	voxel::Region modifiedRegion;
	voxel::ClipboardData voxelData(node->volume(), node->palette(), false);
	voxedit::tool::paste(voxelData, _copy, pos, modifiedRegion);
	if (!modifiedRegion.isValid()) {
		Log::debug("Failed to paste");
		return false;
	}
	const int64_t dismissMillis = core::Var::getSafe(cfg::VoxEditModificationDismissMillis)->intVal();
	modified(nodeId, modifiedRegion, SceneModifiedFlags::All, dismissMillis);
	return true;
}

bool SceneManager::cut() {
	if (!_selectionManager->hasSelection()) {
		Log::debug("Nothing selected - failed to cut");
		return false;
	}
	const int nodeId = activeNode();
	scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	if (node.volume() == nullptr) {
		return false;
	}
	voxel::Region modifiedRegion;
	voxel::ClipboardData voxelData(node.volume(), node.palette(), false);
	_copy = voxedit::tool::cut(voxelData, _selectionManager, modifiedRegion);
	if (!_copy) {
		Log::debug("Failed to cut");
		return false;
	}
	if (!modifiedRegion.isValid()) {
		Log::debug("Failed to cut");
		_copy = {};
		return false;
	}
	const int64_t dismissMillis = core::Var::getSafe(cfg::VoxEditModificationDismissMillis)->intVal();
	modified(nodeId, modifiedRegion, SceneModifiedFlags::All, dismissMillis);
	return true;
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
		scenegraph::SceneGraphNode copiedNode(scenegraph::SceneGraphNodeType::Model);
		const scenegraph::SceneGraphNode *node = sceneGraphModelNode(nodeId);
		if (node == nullptr) {
			continue;
		}
		scenegraph::copyNode(*node, copiedNode, true);
		newSceneGraph.emplace(core::move(copiedNode));
	}
	const scenegraph::SceneGraph::MergeResult &merged = newSceneGraph.merge();
	if (!merged.hasVolume()) {
		return InvalidNodeId;
	}

	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "merge");
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	int parent = 0;
	if (scenegraph::SceneGraphNode* firstNode = sceneGraphNode(nodeIds.front())) {
		scenegraph::copyNode(*firstNode, newNode, false);
	}
	scenegraph::SceneGraphTransform &transform = newNode.keyFrame(0).transform();
	transform.setWorldTranslation(glm::vec3(0.0f));
	newNode.setVolume(merged.volume(), true);
	newNode.setPalette(merged.palette);
	newNode.setNormalPalette(merged.normalPalette);
	if (const scenegraph::SceneGraphNode* firstNode = sceneGraphNode(nodeIds.front())) {
		newNode.setKeyFrames(firstNode->keyFrames());
		newNode.setName(firstNode->name());
		newNode.setVisible(firstNode->visible());
		newNode.setLocked(firstNode->locked());
		newNode.setPivot(firstNode->pivot());
		newNode.setColor(firstNode->color());
		newNode.addProperties(firstNode->properties());
		const glm::ivec3 newPos = -newNode.volume()->region().getLowerCorner() + firstNode->volume()->region().getLowerCorner();
		newNode.volume()->region().shift(newPos);
	}

	int newNodeId = moveNodeToSceneGraph(newNode, parent);
	if (newNodeId == InvalidNodeId) {
		return newNodeId;
	}
	for (int nodeId : nodeIds) {
		nodeRemove(nodeId, false);
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
	scenegraph::SceneGraphNode &node = *_sceneGraph.beginModel();
	// ensure that the first model node is active and re-select the first model node.
	// therefore we first "select" the root node, then switch back to the first model node.
	_sceneGraph.setActiveNode(_sceneGraph.root().id());
	nodeActivate(node.id());
	_mementoHandler.clearStates();
	Log::debug("New volume for node %i", node.id());
	_mementoHandler.markInitialSceneState(_sceneGraph);
	_dirty = false;
	_result = voxelutil::PickResult();
	_modifierFacade.setCursorVoxel(voxel::createVoxel(node.palette(), 0));
	setCursorPosition(cursorPosition(), voxel::FaceNames::Max, true);
	setReferencePosition(node.region().getCenter());
	resetLastTrace();
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

	bool createDiff = core::Var::get("ve_diff", "false")->boolVal();
	if (createDiff) {
		for (const auto &entry : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = entry->second;
			if (!node.isModelNode()) {
				continue;
			}

			scenegraph::SceneGraphNode *existingNode = _sceneGraph.findNodeByName(node.name());
			if (existingNode == nullptr) {
				const int activeNode = _sceneGraph.activeNode();
				existingNode = &_sceneGraph.node(activeNode);
			}

			voxel::RawVolume *v = voxelutil::diffVolumes(existingNode->volume(), node.volume());
			if (v == nullptr) {
				Log::info("No diff between volumes of node %s", node.name().c_str());
				continue;
			}
			scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
			newNode.setVolume(v, true);
			newNode.setName("Diff " + node.name());
			_sceneGraph.emplace(core::move(newNode), existingNode->id());
		}
		return true;
	}

	_sceneGraph = core::move(sceneGraph);
	_sceneRenderer->clear();

	const size_t nodesAdded = _sceneGraph.size();
	if (nodesAdded == 0) {
		if (_sceneGraph.empty(scenegraph::SceneGraphNodeType::Point)) {
			Log::warn("Failed to load any model volumes");
			const voxel::Region region(glm::ivec3(0), glm::ivec3(size() - 1));
			newScene(true, "", region);
			return false;
		} else {
			// only found points, let's create a new model node to please the editor
			addModelChild("", size(), size(), size());
		}
	}
	resetSceneState();
	return true;
}

bool SceneManager::splitVolumes() {
	scenegraph::SceneGraph newSceneGraph;
	if (scenegraph::splitVolumes(_sceneGraph, newSceneGraph, false, false)) {
		return loadSceneGraph(core::move(newSceneGraph));
	}

	scenegraph::SceneGraph newSceneGraph2;
	if (scenegraph::splitVolumes(_sceneGraph, newSceneGraph2, false, true)) {
		return loadSceneGraph(core::move(newSceneGraph2));
	}
	return false;
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

	node.setVolume(volume, true);
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
	newNode.setVolume(v, true);
	if (name.empty()) {
		newNode.setName("unnamed");
	} else {
		newNode.setName(name);
	}
	palette::Palette palette;
	const core::String &paletteName = core::Var::getSafe(cfg::VoxEditLastPalette)->strVal();
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

void SceneManager::rotate(math::Axis axis) {
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

void SceneManager::nodeMoveVoxels(int nodeId, const glm::ivec3& m) {
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		return;
	}

	if (_selectionManager->hasSelection()) {
		// TODO: SELECTION: only move the selected voxels
		Log::warn("Moving only the selected voxels is not implemented yet");
	} else {
		v->move(m);
	}
	modified(nodeId, v->region());
}

void SceneManager::move(int x, int y, int z) {
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

void SceneManager::shift(int x, int y, int z) {
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

void SceneManager::construct() {
	core::Var::get(cfg::VoxEditColorWheel, "false", _("Use the color wheel in the palette color editing"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowColorPicker, "false", _("Always show the color picker below the palette"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditModificationDismissMillis, "1500", _("Milliseconds that a region should get highlighted in a few situations"));
	core::Var::get(cfg::VoxEditRegionSizes, "", _("Show fixed region sizes in the volume inspector"), core::Var::ivec3ListValidator<1, 256>);
	core::Var::get(cfg::VoxEditLocalSpace, "true", _("Use local space for transforms"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowgrid, "true", _("Show the grid"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowlockedaxis, "true", _("Show the currently locked axis"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowaabb, "true", _("Show the axis aligned bounding box"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowBones, "false", _("Show the bones in scene mode"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditRendershadow, "false", _("Render with shadows - make sure to set the scene lighting up properly"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShadingMode, "1", _("Shading mode: 0=Unlit (pure colors), 1=Lit (no shadows), 2=Shadows"), core::Var::minMaxValidator<0, 2>);
	core::Var::get(cfg::VoxEditAnimationSpeed, "100", _("Millisecond delay between frames hide/unhide when using the scene graph panel play button to animate the models in the scene"));
	core::Var::get(cfg::VoxEditAutoNormalMode, "0", core::CV_NOPERSIST, "Flat, Smooth, Smoother", core::Var::minMaxValidator<0, 2>);
	core::Var::get(cfg::VoxEditGridsize, "1", _("The size of the voxel grid"), core::Var::minMaxValidator<1, 64>);
	core::Var::get(cfg::VoxEditPlaneSize, "100", _("The size of the plane"), core::Var::minMaxValidator<1, 1000>);
	core::Var::get(cfg::VoxEditShowPlane, "true", _("Show the plane"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGrayInactive, "false", _("Render the inactive nodes in gray scale mode"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditHideInactive, "false", _("Hide the inactive nodes"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditViewdistance, "5000", _("Far plane for the camera"));
	core::Var::get(cfg::VoxEditShowaxis, "true", _("Show the axis"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditCursorDetails, "1", _("Print cursor details in edit mode - measure distance to reference position in mode 3"), core::Var::minMaxValidator<0, 3>);
	core::Var::get(cfg::VoxEditAutoKeyFrame, "true", _("Automatically create keyframes when changing transforms"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGizmoOperations, "3", _("Bitmask of gizmo operations in scene mode"));
	core::Var::get(cfg::VoxEditGizmoPivot, "false", core::CV_NOPERSIST, _("Activate the pivot mode for the gizmo in scene mode"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGizmoAllowAxisFlip, "true", _("Flip axis or stay along the positive world/local axis"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGizmoSnap, "true", _("Use the grid size for snap"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditModelGizmo, "false", _("Show the gizmo to also translate the region"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditLastPalette, palette::Palette::builtIn[0]);
	core::Var::get(cfg::VoxEditViewports, "2", _("The amount of viewports (not in simple ui mode)"), core::Var::minMaxValidator<2, cfg::MaxViewports>);
	core::Var::get(cfg::VoxEditMaxSuggestedVolumeSize, "128", _("The maximum size of a volume before a few features are disabled (e.g. undo/autosave)"), core::Var::minMaxValidator<32, voxedit::MaxVolumeSize>);
	core::Var::get(cfg::VoxEditMaxSuggestedVolumeSizePreview, "32", _("The maximum size of the preview volume"), core::Var::minMaxValidator<16, voxedit::MaxVolumeSize>);
	core::Var::get(cfg::VoxEditViewMode, "default", _("Configure the editor view mode"));
	core::Var::get(cfg::VoxEditTipOftheDay, "true", _("Show the tip of the day on startup"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditPopupTipOfTheDay, "false", core::CV_NOPERSIST, _("Trigger opening of popup"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditPopupWelcome, "false", core::CV_NOPERSIST, _("Trigger opening of popup"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditPopupMinecraftMapping, "false", core::CV_NOPERSIST, _("Trigger opening of popup"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditPopupAbout, "false", core::CV_NOPERSIST, _("Trigger opening of popup"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditPopupRenameNode, "false", core::CV_NOPERSIST, _("Trigger opening of popup"), core::Var::boolValidator);
	core::Var::get(cfg::VoxEditPopupCreateAnimation, "false", core::CV_NOPERSIST, _("Trigger opening of popup"), core::Var::boolValidator);

	core::Var::get(cfg::VoxEditAnimationPlaying, "false", core::CV_NOPERSIST, _("Update the children of a node when the transform of the node changes"), core::Var::boolValidator);
	_autoSaveSecondsDelay = core::Var::get(cfg::VoxEditAutoSaveSeconds, "180", -1, _("Delay in second between autosaves - 0 disables autosaves"));
	_transformUpdateChildren = core::Var::get(cfg::VoxEditTransformUpdateChildren, "true", -1, _("Update the children of a node when the transform of the node changes"), core::Var::boolValidator);
	_maxSuggestedVolumeSize = core::Var::getSafe(cfg::VoxEditMaxSuggestedVolumeSize);
	_lastDirectory = core::Var::getSafe(cfg::UILastDirectory);

	voxelformat::FormatConfig::init();

	_modifierFacade.construct();
	_mementoHandler.construct();
	_sceneRenderer->construct();
	_camMovement.construct();
	_server.construct();
	_client.construct();

	command::Command::registerCommand("resizetoselection", [&](const command::CmdArgs &args) {
		const voxel::Region &region = modifier().selectionMgr()->region();
		nodeResize(sceneGraph().activeNode(), region);
	}).setHelp(_("Resize the volume to the current selection"));

	command::Command::registerCommand("xs", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::error("Usage: xs <lua-generator-script-filename> [help]");
			return;
		}
		const core::String luaCode = _luaApi.load(args[0]);
		if (luaCode.empty()) {
			Log::error("Failed to load %s", args[0].c_str());
			return;
		}

		core::DynamicArray<core::String> luaArgs;
		for (size_t i = 1; i < args.size(); ++i) {
			luaArgs.push_back(args[i]);
		}

		if (!runScript(luaCode, luaArgs)) {
			Log::error("Failed to execute %s", args[0].c_str());
		} else {
			Log::info("Executed script %s", args[0].c_str());
		}
	}).setHelp(_("Executes a lua script"))
		.setArgumentCompleter(voxelgenerator::scriptCompleter(_filesystem));

	for (int i = 0; i < lengthof(DIRECTIONS); ++i) {
		command::Command::registerActionButton(
				core::String::format("movecursor%s", DIRECTIONS[i].postfix),
				_move[i], _("Move the cursor by keys, not by viewport mouse trace"));
	}
	command::Command::registerCommand("palette_changeintensity", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: palette_changeintensity [value]");
			return;
		}
		const float scale = core::string::toFloat(args[0]);
		const int nodeId = activeNode();
		scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		palette::Palette &pal = node.palette();
		pal.changeIntensity(scale);
		_mementoHandler.markPaletteChange(_sceneGraph, node);
	}).setHelp(_("Change intensity by scaling the rgb values of the palette"));

	command::Command::registerCommand("palette_warmer", [&] (const command::CmdArgs& args) {
		const int nodeId = activeNode();
		scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		palette::Palette &pal = node.palette();
		const uint8_t val = args.empty() ? 10 : core::string::toInt(args[0]);
		pal.changeWarmer(val);
		_mementoHandler.markPaletteChange(_sceneGraph, node);
	}).setHelp(_("Make the palette colors warmer"));

	command::Command::registerCommand("palette_colder", [&] (const command::CmdArgs& args) {
		const int nodeId = activeNode();
		scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		palette::Palette &pal = node.palette();
		const uint8_t val = args.empty() ? 10 : core::string::toInt(args[0]);
		pal.changeColder(val);
		_mementoHandler.markPaletteChange(_sceneGraph, node);
	}).setHelp(_("Make the palette colors colder"));

	command::Command::registerCommand("palette_brighter", [&] (const command::CmdArgs& args) {
		const int nodeId = activeNode();
		scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		palette::Palette &pal = node.palette();
		const float val = args.empty() ? 0.2f : core::string::toFloat(args[0]);
		pal.changeBrighter(val);
		_mementoHandler.markPaletteChange(_sceneGraph, node);
	}).setHelp(_("Make the palette colors brighter"));

	command::Command::registerCommand("palette_darker", [&] (const command::CmdArgs& args) {
		const int nodeId = activeNode();
		scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		palette::Palette &pal = node.palette();
		const float val = args.empty() ? 0.2f : core::string::toFloat(args[0]);
		pal.changeDarker(val);
		_mementoHandler.markPaletteChange(_sceneGraph, node);
	}).setHelp(_("Make the palette colors darker"));

	command::Command::registerCommand("palette_removeunused", [&] (const command::CmdArgs& args) {
		const bool updateVoxels = args.empty() ? false : core::string::toBool(args[0]);
		const int nodeId = activeNode();
		nodeRemoveUnusedColors(nodeId, updateVoxels);
	}).setHelp(_("Remove unused colors from palette"));

	command::Command::registerCommand("palette_whitebalancing", [&] (const command::CmdArgs& args) {
		const int nodeId = activeNode();
		scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		palette::Palette currentPal = node.palette();
		currentPal.whiteBalance();
		node.setPalette(currentPal);
		_mementoHandler.markPaletteChange(_sceneGraph, node);
	}).setHelp(_("Apply white balance to the current palette"));

	command::Command::registerCommand("palette_contraststretching", [&] (const command::CmdArgs& args) {
		const int nodeId = activeNode();
		scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		palette::Palette currentPal = node.palette();
		currentPal.constrastStretching();
		node.setPalette(currentPal);
		_mementoHandler.markPaletteChange(_sceneGraph, node);
	}).setHelp(_("Apply color stretching to the current palette"));

	command::Command::registerCommand("palette_applyall", [&] (const command::CmdArgs& args) {
		const int nodeId = activeNode();
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

	command::Command::registerCommand("palette_sort", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: palette_sort [hue|saturation|brightness|cielab|original]");
			return;
		}
		const core::String &type = args[0];
		const int nodeId = activeNode();
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

	command::Command::registerCommand("normpalette_removenormals", [&] (const command::CmdArgs& args) {
		const int nodeId = activeNode();
		nodeRemoveNormals(nodeId);
	}).setHelp(_("Remove normal information from the palette"));

	command::Command::registerActionButton("zoom_in", _zoomIn, "Zoom in");
	command::Command::registerActionButton("zoom_out", _zoomOut, "Zoom out");
	command::Command::registerActionButton("camera_rotate", _rotate, "Rotate the camera");
	command::Command::registerActionButton("camera_pan", _pan, "Pan the camera");
	command::Command::registerCommand("mouse_node_select", [&] (const command::CmdArgs&) {
		const int nodeId = traceScene();
		if (nodeId != InvalidNodeId) {
			Log::debug("switch active node to hovered from scene graph mode: %i", nodeId);
			nodeActivate(nodeId);
		}
	}).setHelp(_("Switch active node to hovered from scene graph mode"));

	command::Command::registerCommand("select", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: select [all|none|invert]");
			return;
		}
		if (scenegraph::SceneGraphNode *node = sceneGraphModelNode(activeNode())) {
			if (args[0] == "none") {
				_selectionManager->unselect(*node->volume());
			} else if (args[0] == "all") {
				_selectionManager->selectAll(*node->volume());
			} else if (args[0] == "invert") {
				_selectionManager->invert(*node->volume());
			}
		}
	}).setHelp(_("Select all, nothing or invert")).setArgumentCompleter(command::valueCompleter({"all", "none", "invert"}));

	command::Command::registerCommand("presentation", [] (const command::CmdArgs& args) {
		command::Command::execute("hideall; animate 2000 true true");
	}).setHelp(_("Cycle through all scene objects"));

	command::Command::registerCommand("align", [this] (const command::CmdArgs& args) {
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

	command::Command::registerCommand("modelssave", [&] (const command::CmdArgs& args) {
		core::String dir = core::string::extractDir(getSuggestedFilename());
		if (!args.empty()) {
			dir = args[0];
		}
		if (dir.empty()) {
			dir = _filesystem->homePath();
		}
		if (!saveModels(dir)) {
			Log::error("Failed to save models to dir: %s", dir.c_str());
		}
	}).setHelp(_("Save all model nodes into filenames represented by their node names")).setArgumentCompleter(command::dirCompleter(_filesystem, _lastDirectory));

	command::Command::registerCommand("modelsave", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc < 1) {
			Log::info("Usage: modelsave <nodeid> [<file>]");
			return;
		}
		const int nodeId = core::string::toInt(args[0]);
		core::String file = core::String::format("node%i.vengi", nodeId);
		if (args.size() == 2) {
			file = args[1];
		}
		if (!saveNode(nodeId, file)) {
			Log::error("Failed to save node %i to file: %s", nodeId, file.c_str());
		}
	}).setHelp(_("Save a single node to the given path with their node names")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("newscene", [&] (const command::CmdArgs& args) {
		const char *name = args.size() > 0 ? args[0].c_str() : "";
		const char *width = args.size() > 1 ? args[1].c_str() : "64";
		const char *height = args.size() > 2 ? args[2].c_str() : width;
		const char *depth = args.size() > 3 ? args[3].c_str() : height;
		const int iw = core::string::toInt(width) - 1;
		const int ih = core::string::toInt(height) - 1;
		const int id = core::string::toInt(depth) - 1;
		const voxel::Region region(glm::zero<glm::ivec3>(), glm::ivec3(iw, ih, id));
		if (!region.isValid()) {
			Log::warn("Invalid size provided (%i:%i:%i)", iw, ih, id);
			return;
		}
		if (!newScene(true, name, region)) {
			Log::warn("Could not create new scene");
		}
	}).setHelp(_("Create a new scene (with a given name and width, height, depth - all optional)"));

	command::Command::registerCommand("crop", [&] (const command::CmdArgs& args) {
		crop();
	}).setHelp(_("Crop the current active node to the voxel boundaries"));

	command::Command::registerCommand("splitobjects", [&] (const command::CmdArgs& args) {
		splitObjects();
	}).setHelp(_("Split the current active node into multiple nodes"));

	command::Command::registerCommand("scaledown", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		int nodeId = activeNode();
		if (argc == 1) {
			nodeId = core::string::toInt(args[0]);
		}
		scaleDown(nodeId);
	}).setHelp(_("Scale the current active node or the given node down")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("scaleup", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		int nodeId = activeNode();
		if (argc == 1) {
			nodeId = core::string::toInt(args[0]);
		}
		scaleUp(nodeId);
	}).setHelp(_("Scale the current active node or the given node up")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("colortomodel", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc < 1) {
			const voxel::Voxel voxel = _modifierFacade.cursorVoxel();
			colorToNewNode(voxel);
		} else {
			const uint8_t index = core::string::toInt(args[0]);
			const voxel::Voxel voxel = voxel::createVoxel(activePalette(), index);
			colorToNewNode(voxel);
		}
	}).setHelp(_("Move the voxels of the current selected palette index or the given index into a new node"));

	command::Command::registerCommand("abortaction", [&] (const command::CmdArgs& args) {
		_modifierFacade.abort();
	}).setHelp(_("Aborts the current modifier action"));

	command::Command::registerCommand("fillhollow", [&] (const command::CmdArgs& args) {
		fillHollow();
	}).setHelp(_("Fill the inner parts of closed models"));

	command::Command::registerCommand("hollow", [&] (const command::CmdArgs& args) {
		hollow();
	}).setHelp(_("Remove non visible voxels"));

	command::Command::registerCommand("fill", [&] (const command::CmdArgs& args) {
		fill();
	}).setHelp(_("Fill voxels in the current selection"));

	command::Command::registerCommand("clear", [&] (const command::CmdArgs& args) {
		clear();
	}).setHelp(_("Remove all voxels in the current selection"));

	command::Command::registerCommand("setreferenceposition", [&] (const command::CmdArgs& args) {
		if (args.size() != 3) {
			Log::info("Expected to get x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		setReferencePosition(glm::ivec3(x, y, z));
	}).setHelp(_("Set the reference position to the specified position"));

	command::Command::registerCommand("movecursor", [this] (const command::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Expected to get relative x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		moveCursor(x, y, z);
	}).setHelp(_("Move the cursor by the specified offsets"));

	command::Command::registerCommand("normalpalette", [this] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Expected to get the normal palette name");
			return;
		}
		const int nodeId = activeNode();
		scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
		if (node == nullptr) {
			return;
		}
		const core::String &name = args[0];
		palette::NormalPalette normalPalette;
		if (normalPalette.load(name.c_str())) {
			node->setNormalPalette(normalPalette);
			_mementoHandler.markNormalPaletteChange(sceneGraph(), *node);
		}
	}).setHelp(_("Change the normal palette"));

	command::Command::registerCommand("loadpalette", [this] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Expected to get the palette NAME as part of palette-NAME.[png|lua]");
			return;
		}
		bool searchBestColors = false;
		loadPalette(args[0], searchBestColors, true);
	}).setHelp(_("Load a palette by name. E.g. 'built-in:nippon' or 'lospec:id'")).setArgumentCompleter(palette::paletteCompleter());

	command::Command::registerCommand("cursor", [this] (const command::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Expected to get x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		setCursorPosition(glm::ivec3(x, y, z), _modifierFacade.cursorFace(), true);
		_traceViaMouse = false;
	}).setHelp(_("Set the cursor to the specified position"));

	command::Command::registerCommand("setreferencepositiontocursor", [&] (const command::CmdArgs& args) {
		setReferencePosition(cursorPosition());
	}).setHelp(_("Set the reference position to the current cursor position"));

	command::Command::registerCommand("resize", [this] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc == 1) {
			const int size = core::string::toInt(args[0]);
			resizeAll(glm::ivec3(size));
		} else if (argc == 3) {
			glm::ivec3 size;
			for (int i = 0; i < argc; ++i) {
				size[i] = core::string::toInt(args[i]);
			}
			resizeAll(size);
		} else {
			resizeAll(glm::ivec3(1));
		}
	}).setHelp(_("Resize your volume about given x, y and z size"));

	command::Command::registerCommand("modelsize", [this] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc == 1) {
			const int size = core::string::toInt(args[0]);
			nodeResize(activeNode(), glm::ivec3(size));
		} else if (argc == 3) {
			glm::ivec3 size;
			for (int i = 0; i < argc; ++i) {
				size[i] = core::string::toInt(args[i]);
			}
			nodeResize(activeNode(), size);
		} else {
			nodeResize(activeNode(), glm::ivec3(1));
		}
	}).setHelp(_("Resize your current model node about given x, y and z size"));

	command::Command::registerCommand("shift", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc != 3) {
			Log::info("Expected to get x, y and z values");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		shift(x, y, z);
	}).setHelp(_("Shift the volume by the given values"));

	command::Command::registerCommand("center_referenceposition", [&] (const command::CmdArgs& args) {
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

	command::Command::registerCommand("center_origin", [&](const command::CmdArgs &args) {
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

	command::Command::registerCommand("move", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc != 3) {
			Log::info("Expected to get x, y and z values");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		move(x, y, z);
	}).setHelp(_("Move the voxels inside the volume by the given values without changing the volume bounds"));

	command::Command::registerCommand("copy", [&] (const command::CmdArgs& args) {
		copy();
	}).setHelp(_("Copy selection"));

	command::Command::registerCommand("paste", [&] (const command::CmdArgs& args) {
		if (_selectionManager->hasSelection()) {
			paste(_selectionManager->region().getLowerCorner());
		} else {
			paste(referencePosition());
		}
	}).setHelp(_("Paste clipboard to current selection or reference position"));

	command::Command::registerCommand("pastecursor", [&] (const command::CmdArgs& args) {
		paste(cursorPosition());
	}).setHelp(_("Paste clipboard to current cursor position"));

	command::Command::registerCommand("pastenewnode", [&] (const command::CmdArgs& args) {
		pasteAsNewNode();
	}).setHelp(_("Paste clipboard as a new node"));

	command::Command::registerCommand("cut", [&] (const command::CmdArgs& args) {
		cut();
	}).setHelp(_("Cut selection"));

	command::Command::registerCommand("undo", [&] (const command::CmdArgs& args) {
		undo();
	}).setHelp(_("Undo your last step"));

	command::Command::registerCommand("redo", [&] (const command::CmdArgs& args) {
		redo();
	}).setHelp(_("Redo your last step"));

	command::Command::registerCommand("rotate", [&] (const command::CmdArgs& args) {
		if (args.size() < 1) {
			Log::info("Usage: rotate <x|y|z> <amount:1>");
			return;
		}
		const math::Axis axis = math::toAxis(args[0]);
		int n = 1;
		if (args.size() > 1) {
			n = core::string::toInt(args[1]);
		}
		memento::ScopedMementoGroup group(_mementoHandler, "rotate");
		for (int i = 0; i < n; ++i) {
			rotate(axis);
		}
	}).setHelp(_("Rotate active nodes around the given axis"));

	command::Command::registerCommand("modelmerge", [&] (const command::CmdArgs& args) {
		int nodeId1;
		int nodeId2;
		if (args.size() == 1) {
			if (args[0] == "all") {
				const scenegraph::SceneGraph::MergeResult &merged = _sceneGraph.merge();
				if (!merged.hasVolume()) {
					Log::warn("Merging failed");
					return;
				}
				newScene(true, "merged", merged.volume());
				if (auto *node = _sceneGraph.firstModelNode()) {
					node->setPalette(merged.palette);
					node->setNormalPalette(merged.normalPalette);
				}
				return;
			}
			nodeId2 = core::string::toInt(args[0]);
			nodeId1 = _sceneGraph.prevModelNode(nodeId2);
		} else if (args.size() == 2) {
			nodeId1 = core::string::toInt(args[0]);
			nodeId2 = core::string::toInt(args[1]);
		} else {
			nodeId2 = activeNode();
			nodeId1 = _sceneGraph.prevModelNode(nodeId2);
		}
		mergeNodes(nodeId1, nodeId2);
	}).setHelp(_("Merge two given nodes or active model node with the next one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modelmergeall", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::All);
	}).setHelp(_("Merge all nodes"));

	command::Command::registerCommand("modelmergevisible", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::Visible);
	}).setHelp(_("Merge all visible nodes"));

	command::Command::registerCommand("modelmergelocked", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::Locked);
	}).setHelp(_("Merge all locked nodes"));

	command::Command::registerCommand("animate", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: animate <nodedelaymillis> <0|1> <resetcamera>");
			Log::info("nodedelay of 0 will stop the animation, too");
			return;
		}
		if (args.size() == 2) {
			_animationResetCamera = false;
			if (!core::string::toBool(args[1])) {
				_frameAnimationSpeed = 0.0;
				return;
			}
		} else if (args.size() == 3) {
			_animationResetCamera = core::string::toBool(args[2]);
		}
		_frameAnimationSpeed = core::string::toDouble(args[0]) / 1000.0;
	}).setHelp(_("Animate all nodes with the given delay in millis between the frames"));

	command::Command::registerCommand("setcolor", [&] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: setcolor <index>");
			return;
		}
		const uint8_t index = core::string::toInt(args[0]);
		const voxel::Voxel voxel = voxel::createVoxel(activePalette(), index);
		_modifierFacade.setCursorVoxel(voxel);
	}).setHelp(_("Use the given index to select the color from the current palette"));

	command::Command::registerCommand("setcolorrgb", [&] (const command::CmdArgs& args) {
		if (args.size() != 3) {
			Log::info("Usage: setcolorrgb <red> <green> <blue> (color range 0-255)");
			return;
		}
		const uint8_t red = core::string::toInt(args[0]);
		const uint8_t green = core::string::toInt(args[1]);
		const uint8_t blue = core::string::toInt(args[2]);
		const color::RGBA color(red, green, blue);
		const int index = activePalette().getClosestMatch(color);
		const voxel::Voxel voxel = voxel::createVoxel(activePalette(), index);
		_modifierFacade.setCursorVoxel(voxel);
	}).setHelp(_("Set the current selected color by finding the closest rgb match in the palette"));

	command::Command::registerCommand("pickcolor", [&] (const command::CmdArgs& args) {
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

	command::Command::registerCommand("flip", [&] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: flip <x|y|z>");
			return;
		}
		const math::Axis axis = math::toAxis(args[0]);
		flip(axis);
	}).setHelp(_("Flip the selected nodes around the given axis")).setArgumentCompleter(command::valueCompleter({"x", "y", "z"}));

	command::Command::registerCommand("transformreset", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		int nodeId = activeNode();
		if (argc == 1) {
			nodeId = core::string::toInt(args[0]);
		}
		nodeResetTransform(nodeId, InvalidKeyFrame);
	}).setHelp(_("Reset the transform of the current node and keyframe to the default transform"));

	command::Command::registerCommand("transformmirror", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc < 1) {
			Log::info("Usage: transformmirror <x|y|z> [nodeid]");
			return;
		}
		const math::Axis axis = math::toAxis(args[0]);
		int nodeId = activeNode();
		if (argc == 2) {
			nodeId = core::string::toInt(args[1]);
		}
		nodeTransformMirror(nodeId, InvalidKeyFrame, axis);
	}).setHelp(_("Mirrors the transform at the given axes for the given node"));

	command::Command::registerCommand("modeladd", [&] (const command::CmdArgs& args) {
		const char *name = args.size() > 0 ? args[0].c_str() : "";
		const char *width = args.size() > 1 ? args[1].c_str() : "64";
		const char *height = args.size() > 2 ? args[2].c_str() : width;
		const char *depth = args.size() > 3 ? args[3].c_str() : height;
		const int iw = core::string::toInt(width);
		const int ih = core::string::toInt(height);
		const int id = core::string::toInt(depth);
		addModelChild(name, iw, ih, id);
	}).setHelp(_("Add a new model node (with a given name and width, height, depth - all optional)"));

	command::Command::registerCommand("nodedelete", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			nodeRemove(*node, false);
		}
	}).setHelp(_("Delete a particular node by id - or the current active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodelock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(true);
		}
	}).setHelp(_("Lock a particular node by id - or the current active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodetogglelock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(!node->locked());
		}
	}).setHelp(_("Toggle the lock state of a particular node by id - or the current active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodeunlock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(false);
		}
	}).setHelp(_("Unlock a particular node by id - or the current active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodeactivate", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Active node: %i", activeNode());
			return;
		}
		const int nodeId = core::string::toInt(args[0]);
		nodeActivate(nodeId);
	}).setHelp(_("Set or print the current active node")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodetogglevisible", [&](const command::CmdArgs &args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			nodeSetVisible(nodeId, !node->visible());
		}
	}).setHelp(_("Toggle the visible state of a node")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("showall", [&] (const command::CmdArgs& args) {
		for (auto iter = _sceneGraph.beginAll(); iter != _sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			node.setVisible(true);
		}
	}).setHelp(_("Show all nodes"));

	command::Command::registerCommand("hideall", [&](const command::CmdArgs &args) {
		for (auto iter = _sceneGraph.beginAll(); iter != _sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			node.setVisible(false);
		}
	}).setHelp(_("Hide all nodes"));

	command::Command::registerCommand("nodeshowallchildren", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		_sceneGraph.visitChildren(nodeId, true, [] (scenegraph::SceneGraphNode &node) {
			node.setVisible(true);
		});
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			node->setVisible(true);
		}
	}).setHelp(_("Show all children nodes"));

	command::Command::registerCommand("nodehideallchildren", [&](const command::CmdArgs &args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		_sceneGraph.visitChildren(nodeId, true, [] (scenegraph::SceneGraphNode &node) {
			node.setVisible(false);
		});
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			node->setVisible(false);
		}
	}).setHelp(_("Hide all children nodes"));

	command::Command::registerCommand("nodehideothers", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		for (auto iter = _sceneGraph.beginAll(); iter != _sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			if (node.id() == nodeId) {
				node.setVisible(true);
				continue;
			}
			node.setVisible(false);
		}
	}).setHelp(_("Hide all model nodes except the active one")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modellockall", [&](const command::CmdArgs &args) {
		for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			node.setLocked(true);
		}
	}).setHelp(_("Lock all nodes"));

	command::Command::registerCommand("modelunlockall", [&] (const command::CmdArgs& args) {
		for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			node.setLocked(false);
		}
	}).setHelp(_("Unlock all nodes"));

	command::Command::registerCommand("noderename", [&] (const command::CmdArgs& args) {
		if (args.size() == 1) {
			const int nodeId = activeNode();
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeRename(*node, args[0]);
			}
		} else if (args.size() == 2) {
			const int nodeId = core::string::toInt(args[0]);
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeRename(*node, args[1]);
			}
		} else {
			Log::info("Usage: noderename [<nodeid>] newname");
		}
	}).setHelp(_("Rename the current node or the given node id")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodeduplicate", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			nodeDuplicate(*node);
		}
	}).setHelp(_("Duplicates the current node or the given node id")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modelref", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			nodeReference(*node);
		}
	}).setHelp(_("Create a node reference for the given node id")).setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modelunref", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			nodeUnreference(*node);
		}
	}).setHelp(_("Unreference from model and allow to edit the voxels for this node"));

	command::Command::registerCommand("cam_activate", [&] (const command::CmdArgs& args) {
		video::Camera *camera = activeCamera();
		if (camera == nullptr) {
			Log::error("No active camera found");
			return;
		}
		const scenegraph::SceneGraphNodeCamera *cameraNode;
		if (args.empty()) {
			cameraNode = _sceneGraph.activeCameraNode();
		} else {
			const int nodeId = core::string::toInt(args[0]);
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
	newNode.setVolume(new voxel::RawVolume(region), true);
	newNode.setName(name);
	const int parentId = activeNode();
	const int nodeId = moveNodeToSceneGraph(newNode, parentId);
	return nodeId;
}

void SceneManager::flip(math::Axis axis) {
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

	_gridSize = core::Var::getSafe(cfg::VoxEditGridsize);
	_lastAutoSave = _timeProvider->tickSeconds();

	voxel::Region maxRegion(0, _maxSuggestedVolumeSize->intVal() - 1);
	_selectionManager->setMaxRegionSize(maxRegion);

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
		command::Command::execute("resetcamera");
	}
}

void SceneManager::zoom(video::Camera& camera, float level) {
	_camMovement.zoom(camera, level, deltaSeconds());
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

void SceneManager::connectToServer(const core::String &host, int port) {
	_mementoHandler.registerListener(&_client);
	server().stop();
	client().disconnect();
	client().connect(host, port, false);
}

void SceneManager::disconnectFromServer() {
	_mementoHandler.unregisterListener(&_client);
	client().disconnect();
}

bool SceneManager::update(double nowSeconds) {
	core_trace_scoped(SceneManagerUpdate);
	if (_lsystemRunning) {
		stepLSystem();
	}
	updateDelta(nowSeconds);
	_server.update(nowSeconds);
	_client.update(nowSeconds);
	bool loadedNewScene = false;
	if (_loadingFuture.ready()) {
		if (loadSceneGraph(core::move(_loadingFuture.get()))) {
			_needAutoSave = false;
			_dirty = false;
			loadedNewScene = true;
		}
		_loadingFuture = {};
	}

	if (_maxSuggestedVolumeSize->isDirty()) {
		voxel::Region maxRegion(0, _maxSuggestedVolumeSize->intVal() - 1);
		_selectionManager->setMaxRegionSize(maxRegion);
		_maxSuggestedVolumeSize->markClean();
	}

	voxelgenerator::ScriptState state = _luaApi.update(nowSeconds);
	if (state == voxelgenerator::ScriptState::Error) {
		_sceneGraph.unregisterListener(&_luaApiListener);
		_mementoHandler.endGroup();
	} else if (state == voxelgenerator::ScriptState::Finished) {
		const voxel::Region dirtyRegion = _luaApi.dirtyRegion();
		if (dirtyRegion.isValid()) {
			modified(activeNode(), dirtyRegion);
		}
		if (_sceneGraph.dirty()) {
			markDirty();
			_sceneRenderer->clear();
			_sceneGraph.markClean();
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
				zoom(*_camera, 1.0f);
			}
		});
	} else if (_zoomOut.pressed()) {
		_zoomOut.execute(nowSeconds, 0.02, [&] () {
			if (_camera != nullptr) {
				zoom(*_camera, -1.0f);
			}
		});
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
	_sceneGraph.clear();

	_camMovement.shutdown();
	_modifierFacade.shutdown();
	_mementoHandler.unregisterListener(&_client);
	_mementoHandler.shutdown();
	_server.shutdown();
	_client.shutdown();

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
			// while having an axis locked, we should end the trace if we hit the plane
			if (lockedAxis != math::Axis::None) {
				const glm::ivec3& cursorPos = cursorPosition();
				if ((lockedAxis & math::Axis::X) != math::Axis::None) {
					if (sampler.position()[0] == cursorPos[0]) {
						return false;
					}
				}
				if ((lockedAxis & math::Axis::Y) != math::Axis::None) {
					if (sampler.position()[1] == cursorPos[1]) {
						return false;
					}
				}
				if ((lockedAxis & math::Axis::Z) != math::Axis::None) {
					if (sampler.position()[2] == cursorPos[2]) {
						return false;
					}
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
		_result.hitFace = voxelutil::raycastFaceDetection(ray.origin, ray.direction, _result.hitVoxel, 0.0f, 1.0f).face;
		Log::debug("Raycast face hit: %i", (int)_result.hitFace);
	}

	updateCursor();

	return true;
}

bool SceneManager::nodeUpdatePivot(scenegraph::SceneGraphNode &node, const glm::vec3 &pivot) {
	nodeSetPivot(node, pivot);
	_sceneGraph.updateTransforms();
	markDirty();
	_mementoHandler.markKeyFramesChange(_sceneGraph, node);
	return true;
}

bool SceneManager::nodeUpdatePivotGroup(const glm::vec3 &pivot) {
	nodeForeachGroup([&] (int groupNodeId) {
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(groupNodeId)) {
			nodeUpdatePivot(*node, pivot);
		}
	});
	return true;
}

bool SceneManager::nodeUpdatePivot(int nodeId, const glm::vec3 &pivot) {
	if (nodeId == InvalidNodeId) {
		return false;
	}
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

bool SceneManager::nodeUpdateTransformGroup(const glm::vec3 &angles, const glm::vec3 &scale, const glm::vec3 &translation,
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
	if (nodeId == InvalidNodeId) {
		nodeForeachGroup([&] (int groupNodeId) {
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(groupNodeId)) {
				nodeUpdateTransform(*node, matrix, keyFrameIdx, local);
			}
		});
		return true;
	}
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeUpdateTransform(*node, matrix, keyFrameIdx, local);
	}
	return false;
}

bool SceneManager::nodeResetTransform(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx) {
	if (nodeId == InvalidNodeId) {
		nodeForeachGroup([&] (int groupNodeId) {
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(groupNodeId)) {
				nodeResetTransform(*node, keyFrameIdx);
			}
		});
		return true;
	}
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
		scenegraph::KeyFrameIndex copyFromKeyFrameIdx = node.previousKeyFrameForFrame(frameIdx);
		core_assert(copyFromKeyFrameIdx != newKeyFrameIdx);
		scenegraph::SceneGraphTransform copyFromTransform = node.keyFrame(copyFromKeyFrameIdx).transform();
		Log::debug("Assign transform from key frame %d to frame %d", (int)copyFromKeyFrameIdx, (int)frameIdx);
		scenegraph::SceneGraphKeyFrame &copyToKeyFrame = node.keyFrame(newKeyFrameIdx);
		copyToKeyFrame.setTransform(copyFromTransform);
		core_assert_msg(copyToKeyFrame.frameIdx == frameIdx, "Expected frame idx %d, got %d", (int)frameIdx, (int)copyToKeyFrame.frameIdx);
		_mementoHandler.markKeyFramesChange(_sceneGraph, node);
		markDirty();
		return true;
	}
	return false;
}

bool SceneManager::nodeAddKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx) {
	if (nodeId == InvalidNodeId) {
		nodeForeachGroup([&](int groupNodeId) {
			scenegraph::SceneGraphNode &node = _sceneGraph.node(groupNodeId);
			nodeAddKeyframe(node, frameIdx);
		});
		return true;
	}
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

bool SceneManager::nodeRemoveKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx) {
	if (nodeId == InvalidNodeId) {
		nodeForeachGroup([&](int groupNodeId) {
			scenegraph::SceneGraphNode &node = _sceneGraph.node(groupNodeId);
			nodeRemoveKeyFrame(node, frameIdx);
		});
		return true;
	}
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
	const int maxVoxels = maxDim * maxDim * maxDim;
	const voxel::Region &region = _sceneGraph.maxRegion();
	return region.voxels() > maxVoxels;
}

void SceneManager::markDirty() {
	_sceneGraph.markMaxFramesDirty();
	// we only autosave if the volumes in the scene graph are not exceeding the
	// max suggested voxel count
	_needAutoSave = !exceedsMaxSuggestedVolumeSize();
	_dirty = true;
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
		const voxel::Region region(glm::ivec3(0), glm::ivec3(31));
		scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
		newNode.setVolume(new voxel::RawVolume(region), true);
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

void SceneManager::nodeForeachGroup(const std::function<void(int)>& f) {
	memento::ScopedMementoGroup mementoGroup(_mementoHandler, "group");
	_sceneGraph.foreachGroup(f);
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

bool SceneManager::cameraRotate() const {
	return _rotate.pressed();
}

bool SceneManager::cameraPan() const {
	return _pan.pressed();
}

void SceneManager::setActiveCamera(video::Camera *camera) {
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
