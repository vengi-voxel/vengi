/**
 * @file
 */

#include "SceneManager.h"

#include "app/App.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/collection/DynamicArray.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "math/AABB.h"
#include "math/Axis.h"
#include "math/Random.h"
#include "math/Ray.h"
#include "video/Renderer.h"
#include "video/ScopedState.h"
#include "video/Types.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeMoveWrapper.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/SceneGraphUtil.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelgenerator/TreeGenerator.h"
#include "voxelutil/Picking.h"
#include "voxelutil/Raycast.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeMover.h"
#include "voxelutil/VolumeRescaler.h"
#include "voxelutil/VolumeResizer.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#include "voxelutil/ImageUtils.h"

#include "AxisUtil.h"
#include "Config.h"
#include "CustomBindingContext.h"
#include "MementoHandler.h"
#include "tool/Clipboard.h"

#ifdef VOXEDIT_ANIMATION
#include "animation/Animation.h"
#include "anim/AnimationLuaSaver.h"
#include "attrib/ShadowAttributes.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace voxedit {

SceneManager::~SceneManager() {
	core_assert_msg(_initialized == 0, "SceneManager was not properly shut down");
}

bool SceneManager::loadPalette(const core::String& paletteName) {
	voxel::Palette palette;
	if (!palette.load(paletteName.c_str())) {
		return false;
	}
	if (!setActivePalette(palette)) {
		return false;
	}
	core::Var::getSafe(cfg::VoxEditLastPalette)->setVal(paletteName);
	return true;
}

bool SceneManager::importPalette(const core::String& file) {
	voxel::Palette palette;
	if (!voxelformat::importPalette(file, palette)) {
		Log::warn("Failed to import a palette from file '%s'", file.c_str());
		return false;
	}

	core::String paletteName(core::string::extractFilename(file.c_str()));
	const core::String &paletteFilename = core::string::format("palette-%s.png", paletteName.c_str());
	const io::FilesystemPtr &fs = io::filesystem();
	const io::FilePtr &pngFile = fs->open(paletteFilename, io::FileMode::Write);
	if (palette.save(pngFile->name().c_str())) {
		core::Var::getSafe(cfg::VoxEditLastPalette)->setVal(paletteName);
	} else {
		Log::warn("Failed to write palette image");
	}

	return setActivePalette(palette);
}

bool SceneManager::importAsVolume(const core::String &file, int maxDepth, bool bothSides) {
	const image::ImagePtr& img = image::loadImage(file, false);
	voxel::RawVolume *v = voxelutil::importAsVolume(img, maxDepth, bothSides);
	if (v == nullptr) {
		return false;
	}
	voxelformat::SceneGraphNode node;
	node.setVolume(v, true);
	node.setName(core::string::extractFilename(img->name().c_str()));
	return addNodeToSceneGraph(node) != -1;
}

bool SceneManager::importAsPlane(const core::String& file) {
	const image::ImagePtr& img = image::loadImage(file, false);
	voxel::RawVolume *v = voxelutil::importAsPlane(img);
	if (v == nullptr) {
		return false;
	}
	voxelformat::SceneGraphNode node;
	node.setVolume(v, true);
	node.setName(core::string::extractFilename(img->name().c_str()));
	return addNodeToSceneGraph(node) != -1;
}

bool SceneManager::importHeightmap(const core::String& file) {
	const int nodeId = activeNode();
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		return false;
	}
	const image::ImagePtr& img = image::loadImage(file, false);
	if (!img->isLoaded()) {
		return false;
	}
	voxel::RawVolumeWrapper wrapper(v);
	const voxel::Voxel dirtVoxel = voxel::createColorVoxel(voxel::VoxelType::Dirt, 0);
	const voxel::Voxel grassVoxel = voxel::createColorVoxel(voxel::VoxelType::Grass, 0);
	voxelutil::importHeightmap(wrapper, img, dirtVoxel, grassVoxel);
	modified(nodeId, wrapper.dirtyRegion());
	return true;
}

bool SceneManager::importColoredHeightmap(const core::String& file) {
	const int nodeId = activeNode();
	voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId);
	core_assert(node != nullptr);
	voxel::RawVolume* v = node->volume();
	if (v == nullptr) {
		return false;
	}
	const image::ImagePtr& img = image::loadImage(file, false);
	if (!img->isLoaded()) {
		return false;
	}
	voxel::RawVolumeWrapper wrapper(v);
	voxel::PaletteLookup palLookup(node->palette());
	const voxel::Voxel dirtVoxel = voxel::createColorVoxel(voxel::VoxelType::Dirt, 0);
	voxelutil::importColoredHeightmap(wrapper, palLookup, img, dirtVoxel);
	modified(nodeId, wrapper.dirtyRegion());
	return true;
}

void SceneManager::autosave() {
	if (!_needAutoSave) {
		return;
	}
	const core::TimeProviderPtr& timeProvider = app::App::getInstance()->timeProvider();
	const double delay = (double)_autoSaveSecondsDelay->intVal();
	if (_lastAutoSave + delay > timeProvider->tickSeconds()) {
		return;
	}
	core::String autoSaveFilename;
	if (_lastFilename.empty()) {
		autoSaveFilename = "autosave-noname.vox";
	} else {
		if (core::string::startsWith(_lastFilename.c_str(), "autosave-")) {
			autoSaveFilename = _lastFilename;
		} else {
			const io::FilePtr file = io::filesystem()->open(_lastFilename);
			const core::String& p = file->path();
			const core::String& f = file->fileName();
			const core::String& e = file->extension();
			autoSaveFilename = core::string::format("%sautosave-%s.%s",
					p.c_str(), f.c_str(), e.c_str());
		}
	}
	if (save(autoSaveFilename, true)) {
		Log::info("Autosave file %s", autoSaveFilename.c_str());
	} else {
		Log::warn("Failed to autosave");
	}
	_lastAutoSave = timeProvider->tickSeconds();
}

bool SceneManager::saveNode(int nodeId, const core::String& file) {
	const io::FilePtr& filePtr = io::filesystem()->open(file, io::FileMode::SysWrite);
	if (!filePtr->validHandle()) {
		Log::warn("Failed to open the given file '%s' for writing", file.c_str());
		return false;
	}
	const voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		return true;
	}
	voxelformat::SceneGraph newSceneGraph;
	voxelformat::SceneGraphNode newNode;
	voxelformat::copyNode(*node, newNode, false);
	newSceneGraph.emplace(core::move(newNode));
	if (voxelformat::saveFormat(filePtr, newSceneGraph)) {
		Log::info("Saved layer %i to %s", nodeId, filePtr->name().c_str());
		return true;
	}
	Log::warn("Failed to save layer %i to %s", nodeId, filePtr->name().c_str());
	return false;
}

void SceneManager::fillHollow() {
	_sceneGraph.foreachGroup([&] (int nodeId) {
		voxel::RawVolume *v = volume(nodeId);
		voxel::RawVolumeWrapper wrapper = _modifier.createRawVolumeWrapper(v);
		voxelutil::fillHollow(wrapper, _modifier.cursorVoxel());
		modified(nodeId, wrapper.dirtyRegion());
	});
}

void SceneManager::fillPlane(const image::ImagePtr &image) {
	const int nodeId = activeNode();
	if (nodeId == -1) {
		return;
	}
	voxel::RawVolume *v = volume(nodeId);
	voxel::RawVolumeWrapper wrapper = _modifier.createRawVolumeWrapper(v);
	const glm::ivec3 &pos = _modifier.cursorPosition();
	const voxel::FaceNames face = _modifier.cursorFace();
	const voxel::Voxel &hitVoxel = hitCursorVoxel();
	voxelutil::fillPlane(wrapper, image, hitVoxel, pos, face);
	modified(nodeId, wrapper.dirtyRegion());
}

bool SceneManager::saveModels(const core::String& dir) {
	bool state = false;
	for (const voxelformat::SceneGraphNode & node : _sceneGraph) {
		const core::String filename = core::string::path(dir, node.name() + ".qb");
		state |= saveNode(node.id(), filename);
	}
	return state;
}

bool SceneManager::save(const core::String& file, bool autosave) {
	if (_sceneGraph.empty()) {
		Log::warn("No volumes for saving found");
		return false;
	}

	if (file.empty()) {
		Log::warn("No filename given for saving");
		return false;
	}
	const io::FilePtr& filePtr = io::filesystem()->open(file, io::FileMode::SysWrite);
	if (!filePtr->validHandle()) {
		Log::warn("Failed to open the given file '%s' for writing", file.c_str());
		return false;
	}

	if (voxelformat::saveFormat(filePtr, _sceneGraph)) {
		if (!autosave) {
			_dirty = false;
			_lastFilename = file;
		}
		core::Var::get(cfg::VoxEditLastFile)->setVal(file);
		_needAutoSave = false;
		return true;
	}
	Log::warn("Failed to save to desired format");
	return false;
}

static void mergeIfNeeded(voxelformat::SceneGraph &newSceneGraph) {
	if (newSceneGraph.size() > voxelrender::RawVolumeRenderer::MAX_VOLUMES) {
		const voxelformat::SceneGraph::MergedVolumePalette &merged = newSceneGraph.merge();
		newSceneGraph.clear();
		voxelformat::SceneGraphNode node;
		node.setVolume(merged.first, true);
		node.setPalette(merged.second);
		newSceneGraph.emplace(core::move(node));
	}
}

bool SceneManager::prefab(const core::String& file) {
	if (file.empty()) {
		Log::error("Can't import model: No file given");
		return false;
	}
	const io::FilePtr& filePtr = io::filesystem()->open(file);
	if (!filePtr->validHandle()) {
		Log::error("Failed to open model file %s", file.c_str());
		return false;
	}
	voxelformat::SceneGraph newSceneGraph;
	io::FileStream stream(filePtr);
	if (!voxelformat::loadFormat(filePtr->name(), stream, newSceneGraph)) {
		Log::error("Failed to load %s", file.c_str());
		return false;
	}
	mergeIfNeeded(newSceneGraph);

	voxelformat::SceneGraphNode groupNode(voxelformat::SceneGraphNodeType::Group);
	groupNode.setName(core::string::extractFilename(file));
	int newNodeId = _sceneGraph.emplace(core::move(groupNode), activeNode());
	bool state = true;
	for (voxelformat::SceneGraphNode& node : newSceneGraph) {
		state |= addNodeToSceneGraph(node, newNodeId) != -1;
	}

	return state;
}

bool SceneManager::load(const core::String& file) {
	if (file.empty()) {
		return false;
	}
	const io::FilePtr& filePtr = io::filesystem()->open(file);
	if (!filePtr->validHandle()) {
		Log::error("Failed to open model file '%s'", file.c_str());
		return false;
	}
	core::ThreadPool& threadPool = app::App::getInstance()->threadPool();
	_loadingFuture = threadPool.enqueue([filePtr] () {
		voxelformat::SceneGraph newSceneGraph;
		io::FileStream stream(filePtr);
		voxelformat::loadFormat(filePtr->name(), stream, newSceneGraph);
		mergeIfNeeded(newSceneGraph);
		// TODO: stuff that happens in RawVolumeRenderer::extractRegion and
		// RawVolumeRenderer::scheduleExtractions should happen here
		return core::move(newSceneGraph);
	});
	_lastFilename = filePtr->fileName() + "." + filePtr->extension();
	return true;
}

void SceneManager::setMousePos(int x, int y) {
	if (_mouseCursor.x == x && _mouseCursor.y == y) {
		return;
	}
	_mouseCursor.x = x;
	_mouseCursor.y = y;
	_traceViaMouse = true;
}

#ifdef VOXEDIT_ANIMATION
void SceneManager::handleAnimationViewUpdate(int nodeId) {
	if (!_animationUpdate && _animationNodeIdDirtyState == -1) {
		// the first layer
		_animationNodeIdDirtyState = nodeId;
	} else if (_animationUpdate) {
		// a second layer was modified (maybe a group action)
		_animationNodeIdDirtyState = -1;
	}
	_animationUpdate = true;
}
#endif

void SceneManager::queueRegionExtraction(int nodeId, const voxel::Region& region) {
	bool addNew = true;
	for (const auto& r : _extractRegions) {
		if (r.nodeId != nodeId) {
			continue;
		}
		if (r.region.containsRegion(region)) {
			addNew = false;
			break;
		}
	}
	if (addNew) {
		_extractRegions.push_back({region, nodeId});
	}
}

void SceneManager::modified(int nodeId, const voxel::Region& modifiedRegion, bool markUndo) {
	Log::debug("Modified node %i, undo state: %s", nodeId, markUndo ? "true" : "false");
	voxel::logRegion("Modified", modifiedRegion);
	if (markUndo) {
		voxelformat::SceneGraphNode &node = _sceneGraph.node(nodeId);
		_mementoHandler.markModification(node, modifiedRegion);
	}
	if (modifiedRegion.isValid()) {
		queueRegionExtraction(nodeId, modifiedRegion);
	}
	_dirty = true;
	_needAutoSave = true;
#ifdef VOXEDIT_ANIMATION
	handleAnimationViewUpdate(nodeId);
#endif
	resetLastTrace();
}

void SceneManager::colorToNewLayer(const voxel::Voxel voxelColor) {
	voxel::RawVolume* newVolume = new voxel::RawVolume(_sceneGraph.groupRegion());
	_sceneGraph.foreachGroup([&] (int nodeId) {
		voxel::RawVolumeWrapper wrapper(volume(nodeId));
		voxelutil::visitVolume(wrapper, [&] (int32_t x, int32_t y, int32_t z, const voxel::Voxel& voxel) {
			if (voxel.getColor() == voxelColor.getColor()) {
				newVolume->setVoxel(x, y, z, voxel);
				wrapper.setVoxel(x, y, z, voxel::Voxel());
			}
		});
		modified(nodeId, wrapper.dirtyRegion());
	});
	voxelformat::SceneGraphNode node;
	node.setVolume(newVolume, true);
	node.setName(core::string::format("color: %i", (int)voxelColor.getColor()));
	addNodeToSceneGraph(node);
}

void SceneManager::scale(int nodeId) {
	voxel::RawVolume* srcVolume = volume(nodeId);
	if (srcVolume == nullptr) {
		return;
	}
	const voxel::Region srcRegion = srcVolume->region();
	const glm::ivec3& targetDimensionsHalf = (srcRegion.getDimensionsInVoxels() / 2) - 1;
	if (targetDimensionsHalf.x < 0 || targetDimensionsHalf.y < 0 || targetDimensionsHalf.z < 0) {
		Log::debug("Can't scale anymore");
		return;
	}
	const voxel::Region destRegion(srcRegion.getLowerCorner(), srcRegion.getLowerCorner() + targetDimensionsHalf);
	voxel::RawVolume* destVolume = new voxel::RawVolume(destRegion);
	voxelutil::rescaleVolume(*srcVolume, *destVolume);
	if (!setNewVolume(nodeId, destVolume, true)) {
		delete destVolume;
		return;
	}
	modified(nodeId, srcRegion);
}

void SceneManager::crop() {
	const int nodeId = activeNode();
	voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		return;
	}
	if (_volumeRenderer.empty(*node)) {
		Log::info("Empty volumes can't be cropped");
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

void SceneManager::resize(int nodeId, const glm::ivec3& size) {
	voxel::RawVolume* v = volume(nodeId);
	voxel::Region region = v->region();
	region.shiftUpperCorner(size);
	resize(nodeId, region);
}

void SceneManager::resize(int nodeId, const voxel::Region &region) {
	voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		return;
	}
	voxel::RawVolume* v = node->volume();
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
		// we don't have to reextract a mesh if only new empty voxels were added.
		modified(nodeId, voxel::Region::InvalidRegion);
	} else {
		// TODO: assemble the 6 surroundings to optimize this for big volumes
		modified(nodeId, newVolume->region());
	}
}

void SceneManager::resizeAll(const glm::ivec3& size) {
	const glm::ivec3 refPos = referencePosition();
	_sceneGraph.foreachGroup([&] (int nodeId) {
		resize(nodeId, size);
	});
	if (_sceneGraph.region().containsPoint(refPos)) {
		setReferencePosition(refPos);
	}
}

voxel::RawVolume* SceneManager::volume(int nodeId) {
	voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId);
	core_assert_msg(node != nullptr, "Node with id %i wasn't found in the scene graph", nodeId);
	return node->volume();
}

const voxel::RawVolume* SceneManager::volume(int nodeId) const {
	const voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId);
	core_assert_msg(node != nullptr, "Node with id %i wasn't found in the scene graph", nodeId);
	return node->volume();
}

int SceneManager::activeNode() const {
	// This must return a model node that has a volume attached
	return _sceneGraph.activeNode();
}

voxel::Palette &SceneManager::activePalette() const {
	const int nodeId = activeNode();
	if (!_sceneGraph.hasNode(nodeId)) {
		return _sceneGraph.firstPalette();
	}
	return _sceneGraph.node(nodeId).palette();
}

bool SceneManager::setActivePalette(const voxel::Palette &palette) {
	const int nodeId = activeNode();
	if (!_sceneGraph.hasNode(nodeId)) {
		Log::warn("Failed to set the active palette");
		return false;
	}
	voxelformat::SceneGraphNode &node = _sceneGraph.node(nodeId);
	node.setPalette(palette);
	return true;
}

voxel::RawVolume* SceneManager::activeVolume() {
	const int nodeId = activeNode();
	if (nodeId == -1) {
		Log::error("No active node in scene graph");
		return nullptr;
	}
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		Log::error("No active volume found for node id %i", nodeId);
	}
	return v;
}

// TODO: this should also update _dirty
bool SceneManager::undo() {
	if (!mementoHandler().canUndo()) {
		Log::debug("Nothing to undo");
		return false;
	}

	const MementoState& s = _mementoHandler.undo();
	core_assert(s.valid());
	ScopedMementoHandlerLock lock(_mementoHandler);
	if (s.type == MementoType::SceneNodeRenamed) {
		Log::debug("Memento: Undo rename of node %i (%s)", s.nodeId, s.name.c_str());
		if (voxelformat::SceneGraphNode* node = sceneGraphNode(s.nodeId)) {
			node->setName(s.name);
			return true;
		}
		return false;
	} else if (s.type == MementoType::SceneNodeMove) {
		Log::debug("Memento: Undo move of node %i (%s) (move back to parent %i)", s.nodeId, s.name.c_str(), s.parentId);
		return nodeMove(s.nodeId, s.parentId);
	} else if (s.type == MementoType::SceneNodeTransform) {
		Log::debug("Memento: Undo transform of node %i", s.nodeId);
		return nodeUpdateTransform(s.nodeId, s.localMatrix, nullptr, s.keyFrame, false);
	} else if (s.type == MementoType::SceneNodeRemoved) {
		voxel::RawVolume* v = MementoData::toVolume(s.data);
		voxelformat::SceneGraphNode node(voxelformat::SceneGraphNodeType::Model);
		node.setName(s.name);
		node.setVolume(v, true);
		const glm::vec3 rp = referencePosition();
		const glm::vec3 size = v->region().getDimensionsInVoxels();
		node.setPivot(0, rp, size);
		Log::debug("Memento: Undo remove of node (%s) from parent %i", s.name.c_str(), s.parentId);
		const int newNodeId = addNodeToSceneGraph(node, s.parentId);
		_mementoHandler.updateNodeId(s.nodeId, newNodeId);
		return newNodeId != -1;
	} else if (s.type == MementoType::SceneNodeAdded) {
		Log::debug("Memento: Remove node %i", s.nodeId);
		return nodeRemove(s.nodeId, true);
	} else if (s.type == MementoType::Modification) {
		Log::debug("Memento: Undo modification in volume of node %i (%s)", s.nodeId, s.name.c_str());
		voxel::RawVolume* v = MementoData::toVolume(s.data);
		if (voxelformat::SceneGraphNode *node = sceneGraphNode(s.nodeId)) {
			node->setVolume(v, true);
			node->setName(s.name);
			modified(node->id(), s.region, false);
			_volumeRenderer.prepare(_sceneGraph);
			return true;
		} else {
			Log::warn("Failed to undo - node id %i not found (%s)", s.nodeId, s.name.c_str());
			delete v;
			return false;
		}
	}
	return true;
}

bool SceneManager::redo() {
	if (!mementoHandler().canRedo()) {
		Log::debug("Nothing to redo");
		return false;
	}

	const MementoState& s = _mementoHandler.redo();
	core_assert(s.valid());
	ScopedMementoHandlerLock lock(_mementoHandler);
	if (s.type == MementoType::SceneNodeRenamed) {
		Log::debug("Memento: Redo rename of node %i to %s", s.nodeId, s.name.c_str());
		if (voxelformat::SceneGraphNode* node = sceneGraphNode(s.nodeId)) {
			node->setName(s.name);
			return true;
		}
		return false;
	} else if (s.type == MementoType::SceneNodeMove) {
		Log::debug("Memento: Redo move of node %i (%s) (new parent %i)", s.nodeId, s.name.c_str(), s.parentId);
		return nodeMove(s.nodeId, s.parentId);
	} else if (s.type == MementoType::SceneNodeTransform) {
		Log::debug("Memento: Undo transform of node %i", s.nodeId);
		return nodeUpdateTransform(s.nodeId, s.localMatrix, nullptr, s.keyFrame, false);
	} else if (s.type == MementoType::SceneNodeRemoved) {
		Log::debug("Memento: Redo remove of node %i (%s) from parent %i", s.nodeId, s.name.c_str(), s.parentId);
		return nodeRemove(s.nodeId, true);
	} else if (s.type == MementoType::SceneNodeAdded) {
		voxel::RawVolume* v = MementoData::toVolume(s.data);
		voxelformat::SceneGraphNode node(voxelformat::SceneGraphNodeType::Model);
		node.setName(s.name);
		node.setVolume(v, true);
		const glm::vec3 rp = referencePosition();
		const glm::vec3 size = v->region().getDimensionsInVoxels();
		node.setPivot(0, rp, size);
		Log::debug("Memento: Redo add node (%s) to parent %i", s.name.c_str(), s.parentId);
		const int newNodeId = addNodeToSceneGraph(node, s.parentId);
		_mementoHandler.updateNodeId(s.nodeId, newNodeId);
		return newNodeId != -1;
	} else if (s.type == MementoType::Modification) {
		Log::debug("Memento: Redo modification in volume of node %i (%s)", s.nodeId, s.name.c_str());
		voxel::RawVolume* v = MementoData::toVolume(s.data);
		if (voxelformat::SceneGraphNode *node = sceneGraphNode(s.nodeId)) {
			node->setVolume(v, true);
			node->setName(s.name);
			modified(node->id(), s.region, false);
			_volumeRenderer.prepare(_sceneGraph);
			return true;
		} else {
			Log::warn("Failed to redo - node id %i not found (%s)", s.nodeId, s.name.c_str());
			delete v;
			return false;
		}
	}
	return true;
}

bool SceneManager::copy() {
	const Selection& selection = _modifier.selection();
	if (!selection.isValid()) {
		return false;
	}
	const int nodeId = activeNode();
	voxel::RawVolume* model = volume(nodeId);
	if (_copy != nullptr) {
		delete _copy;
	}
	_copy = voxedit::tool::copy(model, selection);
	return true;
}

bool SceneManager::paste(const glm::ivec3& pos) {
	if (_copy == nullptr) {
		Log::debug("Nothing copied yet - failed to paste");
		return false;
	}
	const int nodeId = activeNode();
	voxel::RawVolume* model = volume(nodeId);
	voxel::Region modifiedRegion;
	voxedit::tool::paste(model, _copy, pos, modifiedRegion);
	if (!modifiedRegion.isValid()) {
		Log::debug("Failed to paste");
		return false;
	}
	modified(nodeId, modifiedRegion);
	return true;
}

bool SceneManager::cut() {
	const Selection& selection = _modifier.selection();
	if (!selection.isValid()) {
		Log::debug("Nothing selected - failed to cut");
		return false;
	}
	const int nodeId = activeNode();
	voxel::RawVolume* model = volume(nodeId);
	if (_copy != nullptr) {
		delete _copy;
	}
	voxel::Region modifiedRegion;
	_copy = voxedit::tool::cut(model, selection, modifiedRegion);
	if (_copy == nullptr) {
		Log::debug("Failed to cut");
		return false;
	}
	modified(nodeId, modifiedRegion);
	return true;
}

void SceneManager::resetLastTrace() {
	_sceneModeNodeIdTrace = -1;
	if (!_traceViaMouse) {
		return;
	}
	_lastRaytraceX = _lastRaytraceY = -1;
}

static bool shouldGetMerged(const voxelformat::SceneGraphNode &node, NodeMergeFlags flags) {
	bool add = false;
	if ((flags & NodeMergeFlags::All) != NodeMergeFlags::None) {
		add = true;
	} else if ((flags & NodeMergeFlags::Visible) != NodeMergeFlags::None) {
		add = node.visible();
	} else if ((flags & NodeMergeFlags::Invisible) != NodeMergeFlags::None) {
		add = !node.visible();
	} else if ((flags & NodeMergeFlags::Locked) != NodeMergeFlags::None) {
		add = node.locked();
	}
	return add;
}

int SceneManager::mergeNodes(const core::DynamicArray<int>& nodeIds) {
	voxel::Region mergedRegion = voxel::Region::InvalidRegion;
	core::DynamicArray<glm::ivec3> translations;
	core::DynamicArray<voxel::RawVolume*> volumes;

	volumes.reserve(nodeIds.size());
	translations.reserve(nodeIds.size());

	// calculate the new target region - use the scenegraph translation for this
	for (int nodeId : nodeIds) {
		voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId);
		core_assert(node != nullptr);
		voxel::Region region = node->region();
		const voxelformat::SceneGraphTransform &transform = node->transform(_currentFrameIdx);
		region.shift(transform.worldTranslation());
		if (mergedRegion.isValid()) {
			mergedRegion.accumulate(region);
		} else {
			mergedRegion = region;
		}
		translations.push_back(transform.worldTranslation());
		volumes.push_back(node->volume());
	}

	if (!mergedRegion.isValid()) {
		return -1;
	}

	voxel::RawVolume* merged = new voxel::RawVolume(mergedRegion);
	for (size_t i = 0; i < volumes.size(); ++i) {
		const voxel::RawVolume* v = volumes[i];
		const voxel::Region& sr = v->region();
		voxel::Region dr = sr;
		dr.shift(translations[i]);
		voxelutil::mergeVolumes(merged, v, dr, sr);
	}
	merged->translate(-mergedRegion.getLowerCorner());

	voxelformat::SceneGraphNode node;
	node.setVolume(merged, true);
	int parent = 0;
	if (voxelformat::SceneGraphNode* firstNode = sceneGraphNode(nodeIds.front())) {
		parent = firstNode->parent();
		const size_t numKeyFrames = firstNode->keyFrames().size();
		for (size_t i = 0; i < numKeyFrames; ++i) {
			node.setTransform(i, firstNode->transform(i));
		}
	}
	int newNodeId = addNodeToSceneGraph(node, parent);
	if (newNodeId == -1) {
		return -1;
	}
	for (int nodeId : nodeIds) {
		nodeRemove(nodeId, false);
	}
	return newNodeId;
}

int SceneManager::mergeNodes(NodeMergeFlags flags) {
	core::DynamicArray<int> nodeIds;
	nodeIds.reserve(_sceneGraph.size());
	for (voxelformat::SceneGraphNode &node : _sceneGraph) {
		if (!shouldGetMerged(node, flags)) {
			continue;
		}
		nodeIds.push_back(node.id());
	}

	if (nodeIds.size() <= 1) {
		return -1;
	}

	return mergeNodes(nodeIds);
}

int SceneManager::mergeNodes(int nodeId1, int nodeId2) {
	voxel::RawVolume *volume1 = volume(nodeId1);
	if (volume1 == nullptr) {
		return -1;
	}
	voxel::RawVolume *volume2 = volume(nodeId2);
	if (volume2 == nullptr) {
		return -1;
	}
	core::DynamicArray<int> nodeIds(2);
	nodeIds[0] = nodeId1;
	nodeIds[1] = nodeId2;
	return mergeNodes(nodeIds);
}

void SceneManager::resetSceneState() {
#ifdef VOXEDIT_ANIMATION
	_animationNodeIdDirtyState = -1;
	_animationIdx = 0;
#endif
	voxelformat::SceneGraphNode &node = *_sceneGraph.begin();
	nodeActivate(node.id());
	if (_sceneGraph.size() > 1) {
		setEditMode(EditMode::Scene);
	} else {
		setEditMode(EditMode::Model);
	}
	_mementoHandler.clearStates();
	Log::debug("New volume for node %i", node.id());
	_mementoHandler.markModification(node, voxel::Region::InvalidRegion);
	_dirty = false;
	_modifier.reset();
	_result = voxelutil::PickResult();
	setCursorPosition(cursorPosition(), true);
	setReferencePosition(node.region().getCenter());
	resetLastTrace();
}

void SceneManager::onNewNodeAdded(int newNodeId) {
	if (newNodeId == -1) {
		return;
	}
	if (voxelformat::SceneGraphNode *node = sceneGraphNode(newNodeId)) {
		const voxel::Region &region = node->region();
		const core::String &name = node->name();
		const voxelformat::SceneGraphNodeType type = node->type();
		Log::debug("Adding node %i with name %s", newNodeId, name.c_str());
		_mementoHandler.markNodeAdded(*node);

		Log::debug("Add node %i to scene graph", newNodeId);
		if (type == voxelformat::SceneGraphNodeType::Model) {
			// update the whole volume
			queueRegionExtraction(newNodeId, region);

			_result = voxelutil::PickResult();
			_needAutoSave = true;
			_dirty = true;

			nodeActivate(newNodeId);
#ifdef VOXEDIT_ANIMATION
			handleAnimationViewUpdate(newNodeId);
#endif
		}
	}
}

int SceneManager::addNodeToSceneGraph(voxelformat::SceneGraphNode &node, int parent) {
	const int newNodeId = voxelformat::addNodeToSceneGraph(_sceneGraph, node, parent);
	onNewNodeAdded(newNodeId);
	return newNodeId;
}

bool SceneManager::loadSceneGraph(voxelformat::SceneGraph&& sceneGraph) {
	core_trace_scoped(LoadSceneGraph);
	_sceneGraph = core::move(sceneGraph);
	_volumeRenderer.clear();

	const size_t nodesAdded = _sceneGraph.size();
	if (nodesAdded == 0) {
		Log::warn("Failed to load any model volumes");
		const voxel::Region region(glm::ivec3(0), glm::ivec3(size() - 1));
		newScene(true, "", region);
		return false;
	}
	resetSceneState();
	return true;
}

math::AABB<float> SceneManager::toAABB(const voxel::Region& region) const {
	return math::AABB<float>(glm::floor(region.getLowerCornerf()), glm::floor(glm::vec3(region.getUpperCornerf() + 1.0f)));
}

math::OBB<float> SceneManager::toOBB(const voxel::Region& region, const voxelformat::SceneGraphTransform &transform) const {
	core_assert(region.isValid());
	if (_editMode == EditMode::Scene) {
		const glm::vec3 &extents = transform.pivot() * glm::vec3(region.getDimensionsInVoxels());
		const glm::vec3 &center = (region.getLowerCornerf() + transform.worldTranslation()) + extents;
		const glm::mat4 &matrix = transform.worldMatrix();
		return math::OBB<float>(center, extents, matrix);
	}
	return math::OBB<float>(glm::floor(region.getLowerCornerf()), glm::floor(glm::vec3(region.getUpperCornerf() + 1.0f)));
}

void SceneManager::updateGridRenderer(const voxel::Region& region) {
	_gridRenderer.update(toAABB(region));
}

voxelformat::SceneGraphNode *SceneManager::sceneGraphNode(int nodeId) {
	if (_sceneGraph.hasNode(nodeId)) {
		return &_sceneGraph.node(nodeId);
	}
	return nullptr;
}

const voxelformat::SceneGraphNode *SceneManager::sceneGraphNode(int nodeId) const {
	if (_sceneGraph.hasNode(nodeId)) {
		return &_sceneGraph.node(nodeId);
	}
	return nullptr;
}

const voxelformat::SceneGraph &SceneManager::sceneGraph() {
	return _sceneGraph;
}

// TODO: handle deleteMesh somehow
bool SceneManager::setNewVolume(int nodeId, voxel::RawVolume* volume, bool deleteMesh) {
	core_trace_scoped(SetNewVolume);
	voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		return false;
	}
	return setSceneGraphNodeVolume(*node, volume);
}

bool SceneManager::setSceneGraphNodeVolume(voxelformat::SceneGraphNode &node, voxel::RawVolume* volume) {
	node.setVolume(volume, true);

	const voxel::Region& region = volume->region();
	updateGridRenderer(region);
	updateAABBMesh();

	_dirty = false;
	_result = voxelutil::PickResult();
	setCursorPosition(cursorPosition(), true);
	glm::ivec3 center = region.getCenter();
	center.y = region.getLowerY();
	setReferencePosition(center);
	resetLastTrace();
	return true;
}

bool SceneManager::newScene(bool force, const core::String& name, const voxel::Region& region) {
	if (dirty() && !force) {
		return false;
	}
	_sceneGraph.clear();
	_volumeRenderer.clear();

	voxel::RawVolume* v = new voxel::RawVolume(region);
	voxelformat::SceneGraphNode node;
	node.setVolume(v, true);
	if (name.empty()) {
		node.setName("unnamed");
	} else {
		node.setName(name);
	}
	const glm::vec3 rp = v->region().getPivot();
	const glm::vec3 size = v->region().getDimensionsInVoxels();
	node.setPivot(0, rp, size);
	const int nodeId = voxelformat::addNodeToSceneGraph(_sceneGraph, node, 0);
	if (nodeId == -1) {
		Log::error("Failed to add empty volume to new scene graph");
		return false;
	}
	glm::ivec3 center = v->region().getCenter();
	center.y = region.getLowerY();
	setReferencePosition(center);
	resetSceneState();
	return true;
}

void SceneManager::rotate(int angleX, int angleY, int angleZ) {
	const glm::vec3 angle(angleX, angleY, angleZ);
	_sceneGraph.foreachGroup([&](int nodeId) {
		voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId);
		if (node == nullptr) {
			return;
		}
		const voxel::RawVolume *model = node->volume();
		const glm::vec3 pivot = node->transform(_currentFrameIdx).pivot();
		voxel::RawVolume *newVolume = voxelutil::rotateVolume(model, angle, pivot);
		voxel::Region r = newVolume->region();
		r.accumulate(model->region());
		setSceneGraphNodeVolume(*node, newVolume);
		modified(nodeId, r);
	});
}

void SceneManager::move(int nodeId, const glm::ivec3& m) {
	const voxel::RawVolume* model = volume(nodeId);
	voxel::RawVolume* newVolume = new voxel::RawVolume(model->region());
	voxel::RawVolumeMoveWrapper wrapper(newVolume);
	voxelutil::moveVolume(&wrapper, model, m);
	if (!setNewVolume(nodeId, newVolume)) {
		delete newVolume;
		return;
	}
	modified(nodeId, newVolume->region());
}

void SceneManager::move(int x, int y, int z) {
	const glm::ivec3 v(x, y, z);
	_sceneGraph.foreachGroup([&] (int nodeId) {
		move(nodeId, v);
	});
}

void SceneManager::shift(int nodeId, const glm::ivec3& m) {
	voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		return;
	}
	node->translate(m, _currentFrameIdx);
	updateAABBMesh();
	const voxelformat::KeyFrameIndex keyFrameIdx = node->keyFrameForFrame(_currentFrameIdx);
	_mementoHandler.markNodeTransform(*node, keyFrameIdx);
}

void SceneManager::shift(int x, int y, int z) {
	const glm::ivec3 v(x, y, z);
	_sceneGraph.foreachGroup([&] (int nodeId) {
		shift(nodeId, v);
	});
}

bool SceneManager::setGridResolution(int resolution) {
	const bool ret = gridRenderer().setGridResolution(resolution);
	if (!ret) {
		return false;
	}

	const int res = gridRenderer().gridResolution();
	_modifier.setGridResolution(res);
	setCursorPosition(cursorPosition(), true);

	return true;
}

#ifdef VOXEDIT_ANIMATION
void SceneManager::renderAnimation(const video::Camera& camera) {
	attrib::ShadowAttributes attrib;
	const double deltaFrameSeconds = app::App::getInstance()->deltaFrameSeconds();
	if (_animationUpdate) {
		for (voxelformat::SceneGraphNode& node : _sceneGraph) {
			if (_animationNodeIdDirtyState >= 0 && _animationNodeIdDirtyState != node.id()) {
				Log::debug("Don't update layer %i", node.id());
				continue;
			}
			const core::String& value = node.property("type");
			if (value.empty()) {
				Log::debug("No type metadata found on layer %i", node.id());
				continue;
			}
			const int characterMeshTypeId = core::string::toInt(value);
			const animation::AnimationSettings& animSettings = animationEntity().animationSettings();
			const core::String& path = animSettings.paths[characterMeshTypeId];
			if (path.empty()) {
				Log::debug("No path found for layer %i", node.id());
				continue;
			}
			voxel::Mesh mesh;
			_volumeRenderer.toMesh(node, &mesh);
			const core::String& fullPath = animSettings.fullPath(characterMeshTypeId);
			_animationCache->putMesh(fullPath.c_str(), mesh);
			Log::debug("Updated mesh on layer %i for path %s", node.id(), fullPath.c_str());
		}
		if (!animationEntity().initMesh(_animationCache)) {
			Log::warn("Failed to update the mesh");
		}
		_animationUpdate = false;
		_animationNodeIdDirtyState = -1;
	}
	animationEntity().update(deltaFrameSeconds, attrib);
	_animationRenderer.render(animationEntity(), camera);
}
#endif

void SceneManager::updateAABBMesh() {
	Log::debug("Update aabb mesh");
	_shapeBuilder.clear();
	for (voxelformat::SceneGraphNode &node : _sceneGraph) {
		const voxel::RawVolume* v = node.volume();
		const voxel::Region& region = v->region();
		if (node.id() == activeNode()) {
			_shapeBuilder.setColor(core::Color::White);
		} else {
			_shapeBuilder.setColor(core::Color::Gray);
		}
		_shapeBuilder.obb(toOBB(region, node.transformForFrame(_currentFrameIdx)));
	}
	_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
}

void SceneManager::render(const video::Camera& camera, const glm::ivec2 &size, uint8_t renderMask) {
	const bool depthTest = video::enable(video::State::DepthTest);
	const bool renderScene = (renderMask & RenderScene) != 0u;
	if (renderScene) {
		_volumeRenderer.resize(size);
		_volumeRenderer.prepare(_sceneGraph, _currentFrameIdx, _hideInactive->boolVal(), _grayInactive->boolVal());
		_volumeRenderer.render(camera, _renderShadow, false);
		extractVolume();
	}
	const bool renderUI = (renderMask & RenderUI) != 0u;
	if (renderUI) {
		if (_editMode == EditMode::Scene) {
			if (_showAabbVar->boolVal()) {
				_shapeRenderer.render(_aabbMeshIndex, camera);
			}
		} else {
			const int nodeId = activeNode();
			voxelformat::SceneGraphNode *n = sceneGraphNode(nodeId);
			const voxel::Region& region = n->volume()->region();
			_gridRenderer.render(camera, toAABB(region));

			_modifier.render(camera);

			if (_renderLockAxis) {
				video::ScopedState blend(video::State::Blend, true);
				for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
					// TODO: fix z-fighting
					_shapeRenderer.render(_planeMeshIndex[i], camera);
				}
			}
		}
		if (!depthTest) {
			video::disable(video::State::DepthTest);
		}
		_shapeRenderer.render(_referencePointMesh, camera, _referencePointModelMatrix);
	} else if (!depthTest) {
		video::disable(video::State::DepthTest);
	}
}

void SceneManager::construct() {
	_modifier.construct();
	_mementoHandler.construct();
	_volumeRenderer.construct();

	command::Command::registerCommand("xs", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::error("Usage: xs <lua-generator-script-filename> [help]");
			return;
		}
		const core::String luaScript = _luaGenerator.load(args[0]);
		if (luaScript.empty()) {
			Log::error("Failed to load %s", args[0].c_str());
			return;
		}

		core::DynamicArray<core::String> luaArgs;
		for (size_t i = 1; i < args.size(); ++i) {
			luaArgs.push_back(args[i]);
		}

		if (!runScript(luaScript, luaArgs)) {
			Log::error("Failed to execute %s", args[0].c_str());
		} else {
			Log::info("Executed script %s", args[0].c_str());
		}
	}).setHelp("Executes a lua script to modify the current active volume")
		.setArgumentCompleter(voxelgenerator::scriptCompleter(io::filesystem()));

	for (int i = 0; i < lengthof(DIRECTIONS); ++i) {
		command::Command::registerActionButton(
				core::string::format("movecursor%s", DIRECTIONS[i].postfix),
				_move[i]).setBindingContext(BindingContext::Model);
	}

	command::Command::registerActionButton("zoom_in", _zoomIn).setBindingContext(BindingContext::Editing);
	command::Command::registerActionButton("zoom_out", _zoomOut).setBindingContext(BindingContext::Editing);
	command::Command::registerActionButton("camera_rotate", _rotate).setBindingContext(BindingContext::Editing);
	command::Command::registerActionButton("camera_pan", _pan).setBindingContext(BindingContext::Editing);
	command::Command::registerCommand("mouse_layer_select", [&] (const command::CmdArgs&) {
		if (_sceneModeNodeIdTrace != -1) {
			Log::debug("switch active node to hovered from scene graph mode: %i", _sceneModeNodeIdTrace);
			nodeActivate(_sceneModeNodeIdTrace);
		}
	}).setHelp("Switch active node to hovered from scene graph mode").setBindingContext(BindingContext::Scene);

	command::Command::registerCommand("select", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: select [all|none|invert]");
			return;
		}
		if (args[0] == "none") {
			_modifier.unselect();
		} else if (args[0] == "all") {
			voxel::RawVolume* v = activeVolume();
			if (!v) {
				return;
			}
			const voxel::Region &region = v->region();
			_modifier.select(region.getLowerCorner(), region.getUpperCorner());
		}
	}).setHelp("Unselect all");

#ifdef VOXEDIT_ANIMATION
	command::Command::registerCommand("animation_cycle", [this] (const command::CmdArgs& argv) {
		int offset = 1;
		if (argv.size() > 0) {
			offset = core::string::toInt(argv[0]);
		}
		_animationIdx += offset;
		while (_animationIdx < 0) {
			_animationIdx += (core::enumVal(animation::Animation::MAX) + 1);
		}
		_animationIdx %= (core::enumVal(animation::Animation::MAX) + 1);
		Log::info("current animation idx: %i", _animationIdx);
		animationEntity().setAnimation((animation::Animation)_animationIdx, true);
	}).setHelp("Cycle between all possible animations");

	command::Command::registerCommand("animation_save", [&] (const command::CmdArgs& args) {
		core::String name = "entity";
		if (!args.empty()) {
			name = args[0];
		}
		saveAnimationEntity(name.c_str());
	}).setHelp("Save the animation models and config values");
#endif

	command::Command::registerCommand("togglescene", [this] (const command::CmdArgs& args) {
		toggleEditMode();
	}).setHelp("Toggle scene mode on/off").setBindingContext(voxedit::BindingContext::Editing);

	command::Command::registerCommand("layerssave", [&] (const command::CmdArgs& args) {
		core::String dir = ".";
		if (!args.empty()) {
			dir = args[0];
		}
		if (!saveModels(dir)) {
			Log::error("Failed to save models to dir: %s", dir.c_str());
		}
	}).setHelp("Save all models into filenames represented by their layer/node names");

	command::Command::registerCommand("layersave", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc < 1) {
			Log::info("Usage: layersave <nodeid> [<file>]");
			return;
		}
		const int nodeId = core::string::toInt(args[0]);
		core::String file = core::string::format("layer%i.vox", nodeId);
		if (args.size() == 2) {
			file = args[1];
		}
		if (!saveNode(nodeId, file)) {
			Log::error("Failed to save node %i to file: %s", nodeId, file.c_str());
		}
	}).setHelp("Save a single model to the given path with their layer/node names");

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
	}).setHelp("Create a new scene (with a given name and width, height, depth - all optional)");

	command::Command::registerCommand("crop", [&] (const command::CmdArgs& args) {
		crop();
	}).setHelp("Crop the current layer to the voxel boundaries");

	command::Command::registerCommand("scale", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		int nodeId = activeNode();
		if (argc == 1) {
			nodeId = core::string::toInt(args[0]);
		}
		scale(nodeId);
	}).setHelp("Scale the current layer or given layer down");

	command::Command::registerCommand("colortolayer", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc < 1) {
			const voxel::Voxel voxel = _modifier.cursorVoxel();
			colorToNewLayer(voxel);
		} else {
			const uint8_t index = core::string::toInt(args[0]);
			const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
			colorToNewLayer(voxel);
		}
	}).setHelp("Move the voxels of the current selected palette index or the given index into a new layer");

	command::Command::registerCommand("abortaction", [&] (const command::CmdArgs& args) {
		_modifier.aabbAbort();
	}).setHelp("Aborts the current modifier action").setBindingContext(BindingContext::Model);

	command::Command::registerCommand("fillhollow", [&] (const command::CmdArgs& args) {
		fillHollow();
	}).setHelp("Fill the inner parts of closed models");

	command::Command::registerCommand("setreferenceposition", [&] (const command::CmdArgs& args) {
		if (args.size() != 3) {
			Log::info("Expected to get x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		setReferencePosition(glm::ivec3(x, y, z));
	}).setHelp("Set the reference position to the specified position");

	command::Command::registerCommand("movecursor", [this] (const command::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Expected to get relative x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		moveCursor(x, y, z);
	}).setHelp("Move the cursor by the specified offsets");

	command::Command::registerCommand("loadpalette", [this] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Expected to get the palette NAME as part of palette-NAME.[png|lua]");
			return;
		}
		loadPalette(args[0]);
	}).setHelp("Load an existing palette by name. E.g. 'built-in:nippon'");

	command::Command::registerCommand("cursor", [this] (const command::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Expected to get x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		setCursorPosition(glm::ivec3(x, y, z), true);
	}).setHelp("Set the cursor to the specified position");

	command::Command::registerCommand("setreferencepositiontocursor", [&] (const command::CmdArgs& args) {
		setReferencePosition(cursorPosition());
	}).setHelp("Set the reference position to the current cursor position").setBindingContext(BindingContext::Model);

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
	}).setHelp("Resize your volume about given x, y and z size").setBindingContext(BindingContext::Editing);

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
	}).setHelp("Shift the volume by the given values");

	command::Command::registerCommand("center_referenceposition", [&] (const command::CmdArgs& args) {
		const glm::ivec3& refPos = referencePosition();
		_sceneGraph.foreachGroup([&] (int nodeId) {
			const auto* v = volume(nodeId);
			if (v == nullptr) {
				return;
			}
			const voxel::Region& region = v->region();
			const glm::ivec3& center = region.getCenter();
			const glm::ivec3& delta = refPos - center;
			shift(nodeId, delta);
		});
	}).setHelp("Center the current active layers at the reference position");

	command::Command::registerCommand("center_origin", [&] (const command::CmdArgs& args) {
		_sceneGraph.foreachGroup([&] (int nodeId) {
			const auto* v = volume(nodeId);
			if (v == nullptr) {
				return;
			}
			const voxel::Region& region = v->region();
			const glm::ivec3& delta = -region.getCenter();
			shift(nodeId, delta);
		});
		setReferencePosition(glm::zero<glm::ivec3>());
	}).setHelp("Center the current active layers at the origin");

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
	}).setHelp("Move the voxels inside the volume by the given values");

	command::Command::registerCommand("copy", [&] (const command::CmdArgs& args) {
		copy();
	}).setHelp("Copy selection");

	command::Command::registerCommand("paste", [&] (const command::CmdArgs& args) {
		const Selection& selection = _modifier.selection();
		if (selection.isValid()) {
			paste(selection.getLowerCorner());
		} else {
			paste(referencePosition());
		}
	}).setHelp("Paste clipboard to current selection or reference position");

	command::Command::registerCommand("pastecursor", [&] (const command::CmdArgs& args) {
		paste(_modifier.cursorPosition());
	}).setHelp("Paste clipboard to current cursor position");

	command::Command::registerCommand("cut", [&] (const command::CmdArgs& args) {
		cut();
	}).setHelp("Cut selection");

	command::Command::registerCommand("undo", [&] (const command::CmdArgs& args) {
		undo();
	}).setHelp("Undo your last step");

	command::Command::registerCommand("redo", [&] (const command::CmdArgs& args) {
		redo();
	}).setHelp("Redo your last step");

	command::Command::registerCommand("rotate", [&] (const command::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Usage: rotate <x> <y> <z>");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		rotate(x, y, z);
	}).setHelp("Rotate active layers by the given angles (in degree)");

	command::Command::registerCommand("layermerge", [&] (const command::CmdArgs& args) {
		int nodeId1;
		int nodeId2;
		if (args.size() == 2) {
			nodeId1 = core::string::toInt(args[0]);
			nodeId2 = core::string::toInt(args[1]);
		} else {
			nodeId1 = activeNode();
			// FIXME: this layer id might be an empty slot
			nodeId2 = nodeId1 + 1;
		}
		mergeNodes(nodeId1, nodeId2);
	}).setHelp("Merge two given layers or active layer with the one below");

	command::Command::registerCommand("layermergeall", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::All);
	}).setHelp("Merge all layers");

	command::Command::registerCommand("layermergevisible", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::Visible);
	}).setHelp("Merge all visible layers");

	command::Command::registerCommand("layermergelocked", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::Locked);
	}).setHelp("Merge all locked layers");

	command::Command::registerCommand("animate", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: animate <framedelaymillis> <0|1>");
			Log::info("framedelay of 0 will stop the animation, too");
			return;
		}
		if (args.size() == 2) {
			if (!core::string::toBool(args[1])) {
				_animationSpeed = 0.0;
				return;
			}
		}
		_animationSpeed = core::string::toDouble(args[0]) / 1000.0;
	}).setHelp("Animate all visible layers with the given delay in millis between the frames");

	command::Command::registerCommand("setcolor", [&] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: setcolor <index>");
			return;
		}
		const uint8_t index = core::string::toInt(args[0]);
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
		_modifier.setCursorVoxel(voxel);
	}).setHelp("Use the given index to select the color from the current palette");

	command::Command::registerCommand("setcolorrgb", [&] (const command::CmdArgs& args) {
		if (args.size() != 3) {
			Log::info("Usage: setcolorrgb <red> <green> <blue> (color range 0-255)");
			return;
		}
		const float red = core::string::toFloat(args[0]);
		const float green = core::string::toFloat(args[1]);
		const float blue = core::string::toFloat(args[2]);
		const glm::vec4 color(red / 255.0f, green / 255.0, blue / 255.0, 1.0f);
		core::DynamicArray<glm::vec4> materialColors;
		const voxel::Palette &palette = activePalette();
		palette.toVec4f(materialColors);
		const int index = core::Color::getClosestMatch(color, materialColors);
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
		_modifier.setCursorVoxel(voxel);
	}).setHelp("Set the current selected color by finding the closest rgb match in the palette");

	command::Command::registerCommand("pickcolor", [&] (const command::CmdArgs& args) {
		// during mouse movement, the current cursor position might be at an air voxel (this
		// depends on the mode you are editing in), thus we should use the cursor voxel in
		// that case
		if (_traceViaMouse && !voxel::isAir(hitCursorVoxel().getMaterial())) {
			_modifier.setCursorVoxel(hitCursorVoxel());
			return;
		}
		// resolve the voxel via cursor position. This allows to use also get the proper
		// result if we moved the cursor via keys (and thus might have skipped tracing)
		const glm::ivec3& cursorPos = _modifier.cursorPosition();
		if (const voxel::RawVolume *v = activeVolume()) {
			const voxel::Voxel& voxel = v->voxel(cursorPos);
			_modifier.setCursorVoxel(voxel);
		}
	}).setHelp("Pick the current selected color from current cursor voxel");

	command::Command::registerCommand("flip", [&] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: flip <x|y|z>");
			return;
		}
		const math::Axis axis = math::toAxis(args[0]);
		flip(axis);
	}).setHelp("Flip the selected layers around the given axis");

	command::Command::registerCommand("lock", [&] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: lock <x|y|z>");
			return;
		}
		const math::Axis axis = math::toAxis(args[0]);
		const bool unlock = (_lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp("Toggle locked mode for the given axis at the current cursor position");

	command::Command::registerCommand("lockx", [&] (const command::CmdArgs& args) {
		const math::Axis axis = math::Axis::X;
		const bool unlock = (_lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp("Toggle locked mode for the x axis at the current cursor position");

	command::Command::registerCommand("locky", [&] (const command::CmdArgs& args) {
		const math::Axis axis = math::Axis::Y;
		const bool unlock = (_lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp("Toggle locked mode for the y axis at the current cursor position");

	command::Command::registerCommand("lockz", [&] (const command::CmdArgs& args) {
		const math::Axis axis = math::Axis::Z;
		const bool unlock = (_lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp("Toggle locked mode for the z axis at the current cursor position");

	command::Command::registerCommand("centerplane", [&] (const command::CmdArgs& args) {
		modifier().setCenterMode(!modifier().centerMode());
	}).setHelp("Toggle center plane building");

	command::Command::registerCommand("layeradd", [&] (const command::CmdArgs& args) {
		const char *name = args.size() > 0 ? args[0].c_str() : "";
		const char *width = args.size() > 1 ? args[1].c_str() : "64";
		const char *height = args.size() > 2 ? args[2].c_str() : width;
		const char *depth = args.size() > 3 ? args[3].c_str() : height;
		const int iw = core::string::toInt(width);
		const int ih = core::string::toInt(height);
		const int id = core::string::toInt(depth);
		addModelChild(name, iw, ih, id);
	}).setHelp("Add a new layer (with a given name and width, height, depth - all optional)");

	command::Command::registerCommand("layerdelete", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			if (node->type() == voxelformat::SceneGraphNodeType::Model) {
				nodeRemove(*node, false);
			}
		}
	}).setHelp("Delete a particular node by id - or the current active one");

	command::Command::registerCommand("layerlock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(true);
		}
	}).setHelp("Lock a particular layer by id - or the current active one");

	command::Command::registerCommand("togglelayerlock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(!node->locked());
		}
	}).setHelp("Toggle the lock state of a particular layer by id - or the current active one");

	command::Command::registerCommand("layerunlock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(false);
		}
	}).setHelp("Unlock a particular layer by id - or the current active one");

	command::Command::registerCommand("layeractive", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Active node: %i", activeNode());
			return;
		}
		const int nodeId = core::string::toInt(args[0]);
		nodeActivate(nodeId);
	}).setHelp("Set or print the current active layer");

	command::Command::registerCommand("togglelayerstate", [&](const command::CmdArgs &args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			node->setVisible(!node->visible());
		}
	}).setHelp("Toggle the visible state of a layer");

	command::Command::registerCommand("layerhideall", [&](const command::CmdArgs &args) {
		for (voxelformat::SceneGraphNode &node : _sceneGraph) {
			node.setVisible(false);
		}
	}).setHelp("Hide all layers");

	command::Command::registerCommand("layerlockall", [&](const command::CmdArgs &args) {
		for (voxelformat::SceneGraphNode &node : _sceneGraph) {
			node.setLocked(true);
		}
	}).setHelp("Lock all layers");

	command::Command::registerCommand("layerunlockall", [&] (const command::CmdArgs& args) {
		for (voxelformat::SceneGraphNode &node : _sceneGraph) {
			node.setLocked(false);
		}
	}).setHelp("Unlock all layers");

	command::Command::registerCommand("layerhideothers", [&] (const command::CmdArgs& args) {
		for (voxelformat::SceneGraphNode &node : _sceneGraph) {
			if (node.id() == activeNode()) {
				node.setVisible(true);
				continue;
			}
			node.setVisible(false);
		}
	}).setHelp("Hide all layers except the active one");

	command::Command::registerCommand("layerrename", [&] (const command::CmdArgs& args) {
		if (args.size() == 1) {
			const int nodeId = activeNode();
			if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeRename(*node, args[0]);
			}
		} else if (args.size() == 2) {
			const int nodeId = core::string::toInt(args[0]);
			if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeRename(*node, args[1]);
			}
		} else {
			Log::info("Usage: layerrename [<nodeid>] newname");
		}
	}).setHelp("Rename the current node or the given node id");

	command::Command::registerCommand("layershowall", [&] (const command::CmdArgs& args) {
		for (voxelformat::SceneGraphNode &node : _sceneGraph) {
			node.setVisible(true);
		}
	}).setHelp("Show all layers");

	command::Command::registerCommand("layerduplicate", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			nodeDuplicate(*node);
		}
	}).setHelp("Duplicates the current node or the given node id");

	_showAabbVar = core::Var::getSafe(cfg::VoxEditShowaabb);
	_grayInactive = core::Var::getSafe(cfg::VoxEditGrayInactive);
	_hideInactive = core::Var::getSafe(cfg::VoxEditHideInactive);
}

int SceneManager::addModelChild(const core::String& name, int width, int height, int depth) {
	const voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
	if (!region.isValid()) {
		Log::warn("Invalid size provided (%i:%i:%i)", width, height, depth);
		return -1;
	}
	voxelformat::SceneGraphNode modelNode;
	modelNode.setVolume(new voxel::RawVolume(region), true);
	modelNode.setName(name);
	const int parentId = activeNode();
	const int nodeId = addNodeToSceneGraph(modelNode, parentId);
	return nodeId;
}

void SceneManager::flip(math::Axis axis) {
	_sceneGraph.foreachGroup([&] (int nodeId) {
		auto* v = volume(nodeId);
		if (v == nullptr) {
			return;
		}
		voxel::RawVolume* newVolume = voxelutil::mirrorAxis(v, axis);
		voxel::Region r = newVolume->region();
		r.accumulate(v->region());
		if (!setNewVolume(nodeId, newVolume)) {
			delete newVolume;
			return;
		}
		modified(nodeId, r);
	});
}

void SceneManager::toggleEditMode() {
	if (_editMode == EditMode::Model) {
		setEditMode(EditMode::Scene);
	} else if (_editMode == EditMode::Scene) {
		setEditMode(EditMode::Model);
	}
}

void SceneManager::setEditMode(EditMode mode) {
	_editMode = mode;
#ifdef VOXEDIT_ANIMATION
	_animationUpdate = false;
#endif
	if (_editMode == EditMode::Scene) {
		_modifier.aabbAbort();
		_volumeRenderer.setSceneMode(true);
	} else if (_editMode == EditMode::Model) {
		_volumeRenderer.setSceneMode(false);
#ifdef VOXEDIT_ANIMATION
	} else if (_editMode == EditMode::Animation) {
		_animationUpdate = true;
#endif
	}
	updateAABBMesh();
	// don't abort or toggle any other mode
}

bool SceneManager::init() {
	++_initialized;
	if (_initialized > 1) {
		Log::debug("Already initialized");
		return true;
	}

	core::String paletteName = core::Var::getSafe(cfg::VoxEditLastPalette)->strVal();
	if (paletteName.empty()) {
		paletteName = voxel::Palette::getDefaultPaletteName();
	}
	voxel::Palette palette;
	if (!palette.load(paletteName.c_str())) {
		Log::warn("Failed to load the palette data");
	}
	if (!voxel::initPalette(palette)) {
		Log::warn("Failed to initialize the palette data for %s, falling back to default", paletteName.c_str());
		if (!voxel::initDefaultPalette()) {
			Log::error("Failed to initialize the palette data");
			return false;
		}
	}
	if (!_mementoHandler.init()) {
		Log::error("Failed to initialize the memento handler");
		return false;
	}
	if (!_volumeRenderer.init(video::getWindowSize())) {
		Log::error("Failed to initialize the volume renderer");
		return false;
	}
	if (!_shapeRenderer.init()) {
		Log::error("Failed to initialize the shape renderer");
		return false;
	}
	if (!_gridRenderer.init()) {
		Log::error("Failed to initialize the grid renderer");
		return false;
	}
	if (!_modifier.init()) {
		Log::error("Failed to initialize the modifier");
		return false;
	}
#ifdef VOXEDIT_ANIMATION
	if (!_volumeCache.init()) {
		Log::error("Failed to initialize the volume cache");
		return false;
	}
	if (!_animationSystem.init()) {
		Log::error("Failed to initialize the animation system");
		return false;
	}
	if (!_animationRenderer.init()) {
		Log::error("Failed to initialize the character renderer");
		return false;
	}
	_animationRenderer.setClearColor(core::Color::Clear);
	const auto& meshCache = core::make_shared<voxelformat::MeshCache>();
	_animationCache = core::make_shared<animation::AnimationCache>(meshCache);
	if (!_animationCache->init()) {
		Log::error("Failed to initialize the character mesh cache");
		return false;
	}
#endif

	if (!_luaGenerator.init()) {
		Log::error("Failed to initialize the lua generator bindings");
		return false;
	}

	_autoSaveSecondsDelay = core::Var::get(cfg::VoxEditAutoSaveSeconds, "180");
	_ambientColor = core::Var::get(cfg::VoxEditAmbientColor, "1.0 1.0 1.0");
	_diffuseColor = core::Var::get(cfg::VoxEditDiffuseColor, "1.0 1.0 1.0");
	_cameraZoomSpeed = core::Var::get(cfg::VoxEditCameraZoomSpeed, "10.0");
	const core::TimeProviderPtr& timeProvider = app::App::getInstance()->timeProvider();
	_lastAutoSave = timeProvider->tickSeconds();

	for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
		_planeMeshIndex[i] = -1;
	}

	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(core::Color::SteelBlue, 0.8f));
	_shapeBuilder.sphere(8, 6, 0.5f);
	_referencePointMesh = _shapeRenderer.create(_shapeBuilder);

	_lockedAxis = math::Axis::None;
	return true;
}

bool SceneManager::runScript(const core::String& script, const core::DynamicArray<core::String>& args) {
	const int nodeId = activeNode();
	voxel::RawVolume* volume = this->volume(nodeId);
	const voxel::Region& region = _modifier.createRegion(volume);
	voxel::Region dirtyRegion = voxel::Region::InvalidRegion;
	const bool retVal = _luaGenerator.exec(script, _sceneGraph, nodeId, region, _modifier.cursorVoxel(), dirtyRegion, args);
	modified(nodeId, dirtyRegion);
	return retVal;
}

bool SceneManager::animateActive() const {
	return _animationSpeed > 0.0;
}

void SceneManager::animate(double nowSeconds) {
	if (!animateActive()) {
		return;
	}
	if (_nextFrameSwitch <= nowSeconds) {
		_nextFrameSwitch = nowSeconds + _animationSpeed;
		const int modelCount = (int)_sceneGraph.size(voxelformat::SceneGraphNodeType::Model);
		const int roundTrip = modelCount + _currentAnimationLayer;
		for (int modelIdx = _currentAnimationLayer + 1; modelIdx < roundTrip; ++modelIdx) {
			voxelformat::SceneGraphNode *node = _sceneGraph[_currentAnimationLayer];
			core_assert_always(node != nullptr);
			node->setVisible(false);
			_currentAnimationLayer = modelIdx % modelCount;
			node = _sceneGraph[_currentAnimationLayer];
			core_assert_always(node != nullptr);
			node->setVisible(true);
			return;
		}
	}
}

void SceneManager::zoom(video::Camera& camera, float level) const {
	const float cameraSpeed = _cameraZoomSpeed->floatVal();
	const float value = cameraSpeed * level;
	camera.zoom(value);
}

bool SceneManager::isLoading() const {
	return _loadingFuture.valid();
}

bool SceneManager::update(double nowSeconds) {
	bool loadedNewScene = false;
	if (_loadingFuture.valid()) {
		using namespace std::chrono_literals;
		std::future_status status = _loadingFuture.wait_for(1ms);
		if (status == std::future_status::ready) {
			if (loadSceneGraph(core::move(_loadingFuture.get()))) {
				_needAutoSave = false;
				_dirty = false;
				loadedNewScene = true;
			}
			_loadingFuture = std::future<voxelformat::SceneGraph>();
		}
	}
	_modifier.update(nowSeconds);
	_volumeRenderer.update();
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

	_volumeRenderer.setAmbientColor(_ambientColor->vec3Val());
	_volumeRenderer.setDiffuseColor(_diffuseColor->vec3Val());
	animate(nowSeconds);
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

	if (_copy) {
		delete _copy;
		_copy = nullptr;
	}

	_volumeRenderer.shutdown();
	_sceneGraph.clear();

	_luaGenerator.shutdown();
	_mementoHandler.shutdown();
	_modifier.shutdown();
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_gridRenderer.shutdown();
	_mementoHandler.clearStates();
#ifdef VOXEDIT_ANIMATION
	_volumeCache.shutdown();
	_animationRenderer.shutdown();
	if (_animationCache) {
		_animationCache->shutdown();
	}
	_character.shutdown();
	_bird.shutdown();
	_animationSystem.shutdown();
#endif

	_referencePointMesh = -1;
	_aabbMeshIndex = -1;

	command::Command::unregisterActionButton("zoom_in");
	command::Command::unregisterActionButton("zoom_out");
	command::Command::unregisterActionButton("camera_rotate");
	command::Command::unregisterActionButton("camera_pan");
}

#ifdef VOXEDIT_ANIMATION
animation::AnimationEntity& SceneManager::animationEntity() {
	if (_entityType == animation::AnimationSettings::Type::Character) {
		return _character;
	}
	return _bird;
}

bool SceneManager::saveAnimationEntity(const char *name) {
	_dirty = false;
	// TODO: race and gender
	const core::String& chrName = core::string::format("chr/human-male-%s", name);
	const core::String& luaFilePath = animation::luaFilename(chrName.c_str());
	const core::String luaDir(core::string::extractPath(luaFilePath));
	io::filesystem()->createDir(luaDir);
	const io::FilePtr& luaFile = io::filesystem()->open(luaFilePath, io::FileMode::SysWrite);
	const animation::AnimationSettings& animSettings = animationEntity().animationSettings();
	if (saveAnimationEntityLua(animSettings, animationEntity().skeletonAttributes(), name, luaFile)) {
		Log::info("Wrote lua script: %s", luaFile->name().c_str());
	} else {
		Log::error("Failed to write lua script: %s", luaFile->name().c_str());
	}

	const int mountCount = (int)_sceneGraph.size();
	for (int i = 0; i < mountCount; ++i) {
		voxelformat::SceneGraphNode *node = _sceneGraph[i];
		core_assert_always(node != nullptr);
		const voxel::RawVolume* v = node->volume();
		if (v == nullptr) {
			continue;
		}
		const core::String& value = node->property("type");
		if (value.empty()) {
			const core::String& unknown = core::string::format("%i-%s-%s.vox", (int)i, node->name().c_str(), name);
			Log::warn("No type metadata found on layer %i. Saving to %s", (int)i, unknown.c_str());
			if (!saveNode((int)i, unknown)) {
				Log::warn("Failed to save unknown layer to %s", unknown.c_str());
				_dirty = true;
			}
			continue;
		}
		const int characterMeshTypeId = core::string::toInt(value);
		const core::String& fullPath = animSettings.fullPath(characterMeshTypeId, name);
		if (!saveNode((int)i, fullPath)) {
			Log::warn("Failed to save type %i to %s", characterMeshTypeId, fullPath.c_str());
			_dirty = true;
		}
	}

	return true;
}

bool SceneManager::loadAnimationEntity(const core::String& luaFile) {
	const core::String& lua = io::filesystem()->load(luaFile);
	animation::AnimationSettings settings;
	if (!animation::loadAnimationSettings(lua, settings, nullptr)) {
		Log::warn("Failed to initialize the animation settings for %s", luaFile.c_str());
		return false;
	}
	_entityType = settings.type();
	if (_entityType == animation::AnimationSettings::Type::Max) {
		Log::warn("Failed to detect the entity type for %s", luaFile.c_str());
		return false;
	}

	if (!animationEntity().initSettings(lua)) {
		Log::warn("Failed to initialize the animation settings and attributes for %s", luaFile.c_str());
	}

	voxelformat::SceneGraph newSceneGraph;
	if (!_volumeCache.getVolumes(animationEntity().animationSettings(), newSceneGraph)) {
		Log::warn("Failed to load scene graph for animation settings");
		return false;
	}

	if (!loadSceneGraph(core::move(newSceneGraph))) {
		Log::warn("Failed to load scene graph");
	}
	setEditMode(EditMode::Animation);
	animationEntity().setAnimation(animation::Animation::IDLE, true);

	return true;
}
#endif

bool SceneManager::extractVolume() {
	core_trace_scoped(SceneManagerExtract);
	const size_t n = _extractRegions.size();
	if (n <= 0) {
		return false;
	}
	Log::debug("Extract the meshes for %i regions", (int)n);
	for (size_t i = 0; i < n; ++i) {
		const voxel::Region& region = _extractRegions[i].region;
		if (voxelformat::SceneGraphNode* node = sceneGraphNode(_extractRegions[i].nodeId)) {
			if (!_volumeRenderer.extractRegion(*node, region)) {
				Log::error("Failed to extract the model mesh");
			}
			Log::debug("Extract node %i", _extractRegions[i].nodeId);
			voxel::logRegion("Extraction", region);
		}
	}
	_extractRegions.clear();
	return true;
}

void SceneManager::lsystem(const core::String &axiom, const core::DynamicArray<voxelgenerator::lsystem::Rule> &rules, float angle, float length,
		float width, float widthIncrement, int iterations, float leavesRadius) {
	math::Random random;
	const int nodeId = activeNode();
	voxel::RawVolumeWrapper wrapper(volume(nodeId));
	voxelgenerator::lsystem::generate(wrapper, referencePosition(), axiom, rules, angle, length, width, widthIncrement, iterations, random, leavesRadius);
	modified(nodeId, wrapper.dirtyRegion());
}

void SceneManager::createTree(const voxelgenerator::TreeContext& ctx) {
	math::Random random(ctx.cfg.seed);
	const int nodeId = activeNode();
	voxel::RawVolumeWrapper wrapper(volume(nodeId));
	voxelgenerator::tree::createTree(wrapper, ctx, random);
	modified(nodeId, wrapper.dirtyRegion());
}

void SceneManager::setReferencePosition(const glm::ivec3& pos) {
	_modifier.setReferencePosition(pos);
	const glm::ivec3 &refPos = _modifier.referencePosition();
	const glm::vec3 posAligned((float)refPos.x + 0.5f, (float)refPos.y + 0.5f, (float)refPos.z + 0.5f);
	_referencePointModelMatrix = glm::translate(posAligned);
}

void SceneManager::moveCursor(int x, int y, int z) {
	glm::ivec3 p = cursorPosition();
	const int res = gridRenderer().gridResolution();
	p.x += x * res;
	p.y += y * res;
	p.z += z * res;
	setCursorPosition(p, true);
	_traceViaMouse = false;
	if (const voxel::RawVolume *v = activeVolume()) {
		const voxel::Voxel &voxel = v->voxel(cursorPosition());
		_modifier.setHitCursorVoxel(voxel);
	}
}

void SceneManager::setCursorPosition(glm::ivec3 pos, bool force) {
	const voxel::RawVolume* v = activeVolume();
	if (v == nullptr) {
		return;
	}

	const int res = gridRenderer().gridResolution();
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
	// make a copy here - no reference - otherwise the comparison below won't
	// do anything else than comparing the same values.
	const glm::ivec3 oldCursorPos = cursorPosition();
	if (!force) {
		if ((_lockedAxis & math::Axis::X) != math::Axis::None) {
			pos.x = oldCursorPos.x;
		}
		if ((_lockedAxis & math::Axis::Y) != math::Axis::None) {
			pos.y = oldCursorPos.y;
		}
		if ((_lockedAxis & math::Axis::Z) != math::Axis::None) {
			pos.z = oldCursorPos.z;
		}
	}

	if (!region.containsPoint(pos)) {
		pos = region.moveInto(pos.x, pos.y, pos.z);
	}
	_modifier.setCursorPosition(pos, _result.hitFace);
	if (oldCursorPos == pos) {
		return;
	}
	updateLockedPlane(math::Axis::X);
	updateLockedPlane(math::Axis::Y);
	updateLockedPlane(math::Axis::Z);
}

bool SceneManager::trace(bool force, voxelutil::PickResult *result) {
	if (result) {
		*result = _result;
	}
	if (_editMode == EditMode::Scene) {
		if (_sceneModeNodeIdTrace != -1) {
			// if the trace is not forced, and the mouse cursor position did not change, don't
			// re-execute the trace.
			if (_lastRaytraceX == _mouseCursor.x && _lastRaytraceY == _mouseCursor.y && !force) {
				return true;
			}
		}
		_sceneModeNodeIdTrace = -1;
		core_trace_scoped(EditorSceneOnProcessUpdateRay);
		_lastRaytraceX = _mouseCursor.x;
		_lastRaytraceY = _mouseCursor.y;
		float intersectDist = _camera->farPlane();
		const math::Ray& ray = _camera->mouseRay(_mouseCursor);
		for (voxelformat::SceneGraphNode &node : _sceneGraph) {
			if (!node.visible()) {
				continue;
			}
			const voxel::Region& region = node.region();
			float distance = 0.0f;
			const math::OBB<float>& obb = toOBB(region, node.transformForFrame(_currentFrameIdx));
			if (obb.intersect(ray.origin, ray.direction, _camera->farPlane(), distance)) {
				if (distance < intersectDist) {
					intersectDist = distance;
					_sceneModeNodeIdTrace = node.id();
				}
			}
		}
		Log::trace("Hovered node: %i", _sceneModeNodeIdTrace);
		return true;
	} else if (_editMode != EditMode::Model) {
		return false;
	}

	// mouse tracing is disabled - e.g. because the voxel cursor was moved by keyboard
	// shortcuts in this case the execution of the modifier would result in a
	// re-execution of the trace. And that would move the voxel cursor to the mouse pos
	if (!_traceViaMouse) {
		return false;
	}
	// if the trace is not forced, and the mouse cursor position did not change, don't
	// re-execute the trace.
	if (_lastRaytraceX == _mouseCursor.x && _lastRaytraceY == _mouseCursor.y && !force) {
		return true;
	}
	if (_camera == nullptr) {
		return false;
	}
	const voxel::RawVolume* v = activeVolume();
	if (v == nullptr) {
		return false;
	}
	Log::trace("Execute new trace for %i:%i (%i:%i)",
			_mouseCursor.x, _mouseCursor.y, _lastRaytraceX, _lastRaytraceY);

	core_trace_scoped(EditorSceneOnProcessUpdateRay);
	_lastRaytraceX = _mouseCursor.x;
	_lastRaytraceY = _mouseCursor.y;

	const math::Ray& ray = _camera->mouseRay(_mouseCursor);
	float rayLength = _camera->farPlane();
	const glm::vec3& dirWithLength = ray.direction * rayLength;
	static constexpr voxel::Voxel air;

	_result.didHit = false;
	_result.validPreviousPosition = false;
	_result.firstInvalidPosition = false;
	_result.firstValidPosition = false;
	_result.direction = ray.direction;
	_result.hitFace = voxel::FaceNames::Max;
	voxelutil::raycastWithDirection(v, ray.origin, dirWithLength, [&] (voxel::RawVolume::Sampler& sampler) {
		if (!_result.firstValidPosition && sampler.currentPositionValid()) {
			_result.firstPosition = sampler.position();
			_result.firstValidPosition = true;
		}

		if (sampler.voxel() != air) {
			_result.didHit = true;
			_result.hitVoxel = sampler.position();
			_result.hitFace = voxel::raycastFaceDetection(ray.origin, ray.direction, _result.hitVoxel, 0.0f, 1.0f);
			Log::debug("Raycast face hit: %i", (int)_result.hitFace);
			return false;
		}
		if (sampler.currentPositionValid()) {
			// while having an axis locked, we should end the trace if we hit the plane
			if (_lockedAxis != math::Axis::None) {
				const glm::ivec3& cursorPos = cursorPosition();
				if ((_lockedAxis & math::Axis::X) != math::Axis::None) {
					if (sampler.position()[0] == cursorPos[0]) {
						return false;
					}
				}
				if ((_lockedAxis & math::Axis::Y) != math::Axis::None) {
					if (sampler.position()[1] == cursorPos[1]) {
						return false;
					}
				}
				if ((_lockedAxis & math::Axis::Z) != math::Axis::None) {
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
		_result.hitFace = voxel::raycastFaceDetection(ray.origin, ray.direction, _result.hitVoxel, 0.0f, 1.0f);
		Log::debug("Raycast face hit: %i", (int)_result.hitFace);
	}

	if (_modifier.modifierTypeRequiresExistingVoxel()) {
		if (_result.didHit) {
			setCursorPosition(_result.hitVoxel);
		} else if (_result.validPreviousPosition) {
			setCursorPosition(_result.previousPosition);
		}
	} else if (_result.validPreviousPosition) {
		setCursorPosition(_result.previousPosition);
	} else if (_result.didHit) {
		setCursorPosition(_result.hitVoxel);
	}

	if (_result.didHit) {
		_modifier.setHitCursorVoxel(v->voxel(_result.hitVoxel));
	} else {
		_modifier.setHitCursorVoxel(voxel::Voxel());
	}
	_modifier.setVoxelAtCursor(v->voxel(_modifier.cursorPosition()));

	if (result) {
		*result = _result;
	}

	return true;
}

void SceneManager::updateLockedPlane(math::Axis axis) {
	if (axis == math::Axis::None) {
		return;
	}
	const int index = math::getIndexForAxis(axis);
	int32_t& meshIndex = _planeMeshIndex[index];
	if ((_lockedAxis & axis) == math::Axis::None) {
		if (meshIndex != -1) {
			_shapeRenderer.deleteMesh(meshIndex);
			meshIndex = -1;
		}
		return;
	}

	const glm::vec4 colors[] = {
		core::Color::LightRed,
		core::Color::LightGreen,
		core::Color::LightBlue
	};
	updateShapeBuilderForPlane(_shapeBuilder, _sceneGraph.region(), false, cursorPosition(), axis, core::Color::alpha(colors[index], 0.4f));
	_shapeRenderer.createOrUpdate(meshIndex, _shapeBuilder);
}

void SceneManager::setLockedAxis(math::Axis axis, bool unlock) {
	if (unlock) {
		_lockedAxis &= ~axis;
	} else {
		_lockedAxis |= axis;
	}
	updateLockedPlane(math::Axis::X);
	updateLockedPlane(math::Axis::Y);
	updateLockedPlane(math::Axis::Z);
}

bool SceneManager::nodeUpdateTransform(int nodeId, const glm::mat4 &localMatrix, const glm::mat4 *deltaMatrix,
									   voxelformat::KeyFrameIndex keyFrameIdx, bool memento) {
	if (nodeId != -1) {
		if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			return nodeUpdateTransform(*node, localMatrix, deltaMatrix, keyFrameIdx, memento);
		}
		return false;
	}
	nodeForeachGroup([&] (int nodeId) {
		if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			nodeUpdateTransform(*node, localMatrix, deltaMatrix, keyFrameIdx, memento);
		}
	});
	return true;
}

bool SceneManager::nodeUpdateTransform(voxelformat::SceneGraphNode &node, const glm::mat4 &localMatrix, const glm::mat4 *deltaMatrix, voxelformat::KeyFrameIndex keyFrameIdx, bool memento) {
	glm::vec3 translation;
	glm::quat orientation;
	glm::vec3 scale;
	glm::vec3 skew;
	glm::vec4 perspective;
	voxelformat::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
	voxelformat::SceneGraphTransform &transform = keyFrame.transform();
	glm::decompose(localMatrix, scale, orientation, translation, skew, perspective);
	transform.setLocalTranslation(translation);
	transform.setLocalOrientation(orientation);
	// transform.setLocalScale(glm::length(scale)); // TODO: broken
	transform.update(_sceneGraph, node, keyFrame.frameIdx);

	if (memento) {
		_mementoHandler.markNodeTransform(node, keyFrameIdx);
	}

	updateAABBMesh();
	return true;
}

bool SceneManager::nodeMove(int sourceNodeId, int targetNodeId) {
	if (_sceneGraph.changeParent(sourceNodeId, targetNodeId)) {
		voxelformat::SceneGraphNode *node = sceneGraphNode(sourceNodeId);
		core_assert(node != nullptr);
		_mementoHandler.markNodeMoved(targetNodeId, sourceNodeId);
		return true;
	}
	return false;
}

bool SceneManager::nodeRename(int nodeId, const core::String &name) {
	if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeRename(*node, name);
	}
	return false;
}

bool SceneManager::nodeRename(voxelformat::SceneGraphNode &node, const core::String &name) {
	node.setName(name);
	_mementoHandler.markNodeRenamed(node);
	return true;
}

bool SceneManager::nodeSetVisible(int nodeId, bool visible) {
	if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		node->setVisible(visible);
		return true;
	}
	return false;
}

bool SceneManager::nodeSetLocked(int nodeId, bool visible) {
	if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		node->setLocked(visible);
		return true;
	}
	return false;
}

bool SceneManager::nodeRemove(int nodeId, bool recursive) {
	if (voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId)) {
		return nodeRemove(*node, recursive);
	}
	return false;
}

bool SceneManager::nodeRemove(voxelformat::SceneGraphNode &node, bool recursive) {
	const int nodeId = node.id();
	const core::String &name = node.name();
	Log::debug("Delete node %i with name %s", nodeId, name.c_str());
	_mementoHandler.markNodeRemoved(node);
	if (!_sceneGraph.removeNode(nodeId, recursive)) {
		Log::error("Failed to remove node with id %i", nodeId);
		// TODO: _mementoHandler.removeLast();
		return false;
	}
	_needAutoSave = true;
	_dirty = true;
	if (_sceneGraph.empty()) {
		const voxel::Region region(glm::ivec3(0), glm::ivec3(31));
		newScene(true, name, region);
		// TODO: allow to undo the removal of the last node
	}
	updateAABBMesh();
	return true;
}

void SceneManager::nodeDuplicate(const voxelformat::SceneGraphNode &node) {
	if (node.type() != voxelformat::SceneGraphNodeType::Model) {
		return;
	}
	const int newNodeId = voxelformat::addNodeToSceneGraph(_sceneGraph, node, node.parent());
	onNewNodeAdded(newNodeId);
#if 0
	for (int childNodeId : node.children()) {
		// TODO: duplicate children
	}
#endif
}

void SceneManager::nodeForeachGroup(const std::function<void(int)>& f) {
	_sceneGraph.foreachGroup(f);
}

bool SceneManager::nodeActivate(int nodeId) {
	if (!_sceneGraph.hasNode(nodeId)) {
		Log::warn("Given node id %i doesn't exist", nodeId);
		return false;
	}
	if (_sceneGraph.activeNode() == nodeId) {
		return true;
	}
	Log::debug("Activate node %i", nodeId);
	voxelformat::SceneGraphNode &node = _sceneGraph.node(nodeId);
	if (node.type() != voxelformat::SceneGraphNodeType::Model) {
		Log::warn("Given node id %i is no model node", nodeId);
		return false;
	}
	_sceneGraph.setActiveNode(nodeId);
	voxel::overridePalette(node.palette());
	const voxel::Region& region = node.region();
	updateGridRenderer(region);
	updateAABBMesh();
	if (!region.containsPoint(referencePosition())) {
		const uint32_t frame = 0;
		const glm::ivec3 pivot = region.getLowerCorner() + glm::ivec3(node.transform(frame).pivot() * glm::vec3(region.getDimensionsInVoxels()));
		setReferencePosition(glm::ivec3(pivot));
	}
	if (!region.containsPoint(cursorPosition())) {
		setCursorPosition(node.region().getCenter());
	}
	resetLastTrace();
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

}
