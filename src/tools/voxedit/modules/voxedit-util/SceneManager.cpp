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
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/UTF8.h"
#include "core/collection/DynamicArray.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "math/AABB.h"
#include "math/Axis.h"
#include "math/Random.h"
#include "math/Ray.h"
#include "video/Renderer.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "video/Types.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeMoveWrapper.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelfont/VoxelFont.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/SceneGraphUtil.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelgenerator/TreeGenerator.h"
#include "voxelrender/ImageGenerator.h"
#include "voxelrender/RawVolumeRenderer.h"
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
#include "MementoHandler.h"
#include "SceneUtil.h"
#include "tool/Clipboard.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace voxedit {

SceneManager::~SceneManager() {
	core_assert_msg(_initialized == 0, "SceneManager was not properly shut down");
}

bool SceneManager::loadPalette(const core::String& paletteName, bool searchBestColors) {
	voxel::Palette palette;

	if (core::string::startsWith(paletteName, "node:")) {
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
	const image::ImagePtr& img = image::loadImage(file);
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
	const image::ImagePtr& img = image::loadImage(file);
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
	const image::ImagePtr& img = image::loadImage(file);
	if (!img->isLoaded()) {
		return false;
	}
	voxel::RawVolumeWrapper wrapper(v);
	const voxel::Voxel dirtVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel grassVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
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
	const image::ImagePtr& img = image::loadImage(file);
	if (!img->isLoaded()) {
		return false;
	}
	voxel::RawVolumeWrapper wrapper(v);
	voxel::PaletteLookup palLookup(node->palette());
	const voxel::Voxel dirtVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
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
	io::FileDescription autoSaveFilename;
	if (_lastFilename.empty()) {
		autoSaveFilename.set("autosave-noname.vox");
	} else {
		if (core::string::startsWith(_lastFilename.c_str(), "autosave-")) {
			autoSaveFilename = _lastFilename;
		} else {
			const io::FilePtr file = io::filesystem()->open(_lastFilename.name);
			const core::String& p = file->path();
			const core::String& f = file->fileName();
			const core::String& e = file->extension();
			autoSaveFilename.set(core::string::format("%sautosave-%s.%s",
					p.c_str(), f.c_str(), e.c_str()), &_lastFilename.desc);
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
		Log::warn("Node with id %i wasn't found", nodeId);
		return true;
	}
	if (node->type() != voxelformat::SceneGraphNodeType::Model) {
		Log::warn("Given node is no model node");
		return false;
	}
	voxelformat::SceneGraph newSceneGraph;
	voxelformat::SceneGraphNode newNode;
	voxelformat::copyNode(*node, newNode, false);
	newSceneGraph.emplace(core::move(newNode));
	if (voxelformat::saveFormat(filePtr, &_lastFilename.desc, newSceneGraph, voxelrender::volumeThumbnail)) {
		Log::info("Saved node %i to %s", nodeId, filePtr->name().c_str());
		return true;
	}
	Log::warn("Failed to save node %i to %s", nodeId, filePtr->name().c_str());
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

void SceneManager::updateVoxelType(int nodeId, uint8_t palIdx, voxel::VoxelType newType) {
	voxel::RawVolumeWrapper wrapper(volume(nodeId));
	voxelutil::visitVolume(wrapper, [&wrapper, palIdx, newType](int x, int y, int z, const voxel::Voxel &v) {
		if (v.getColor() != palIdx) {
			return;
		}
		wrapper.setVoxel(x, y, z, voxel::createVoxel(newType, palIdx));
	});
	modified(nodeId, wrapper.dirtyRegion());
}

bool SceneManager::saveModels(const core::String& dir) {
	bool state = false;
	for (const voxelformat::SceneGraphNode & node : _sceneGraph) {
		const core::String filename = core::string::path(dir, node.name() + ".vengi");
		state |= saveNode(node.id(), filename);
	}
	return state;
}

bool SceneManager::save(const io::FileDescription& file, bool autosave) {
	if (_sceneGraph.empty()) {
		Log::warn("No volumes for saving found");
		return false;
	}

	if (file.empty()) {
		Log::warn("No filename given for saving");
		return false;
	}
	const io::FilePtr& filePtr = io::filesystem()->open(file.name, io::FileMode::SysWrite);
	if (!filePtr->validHandle()) {
		Log::warn("Failed to open the given file '%s' for writing", file.c_str());
		return false;
	}

	if (voxelformat::saveFormat(filePtr, &_lastFilename.desc, _sceneGraph, voxelrender::volumeThumbnail)) {
		if (!autosave) {
			_dirty = false;
			_lastFilename = file;
		}
		core::Var::get(cfg::VoxEditLastFile)->setVal(filePtr->name());
		_needAutoSave = false;
		return true;
	}
	Log::warn("Failed to save to desired format");
	return false;
}

static void mergeIfNeeded(voxelformat::SceneGraph &newSceneGraph) {
	if (newSceneGraph.size() > voxelrender::RawVolumeRenderer::MAX_VOLUMES) {
		const voxelformat::SceneGraph::MergedVolumePalette &merged = newSceneGraph.merge(true);
		newSceneGraph.clear();
		voxelformat::SceneGraphNode node;
		node.setVolume(merged.first, true);
		node.setPalette(merged.second);
		newSceneGraph.emplace(core::move(node));
	}
}

bool SceneManager::import(const core::String& file) {
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

bool SceneManager::load(const io::FileDescription& file) {
	if (file.empty()) {
		return false;
	}
	const io::FilePtr& filePtr = io::filesystem()->open(file.name);
	if (!filePtr->validHandle()) {
		Log::error("Failed to open model file '%s'", file.c_str());
		return false;
	}

	if (_loadingFuture.valid()) {
		Log::error("Failed to load '%s' - still loading another model", file.c_str());
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
	_lastFilename.set(filePtr->name(), &file.desc);
	return true;
}

bool SceneManager::load(const io::FileDescription& file, const uint8_t *data, size_t size) {
	voxelformat::SceneGraph newSceneGraph;
	io::MemoryReadStream stream(data, size);
	voxelformat::loadFormat(file.name, stream, newSceneGraph);
	mergeIfNeeded(newSceneGraph);
	if (loadSceneGraph(core::move(newSceneGraph))) {
		_needAutoSave = false;
		_dirty = false;
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
	_traceViaMouse = true;
}

void SceneManager::modified(int nodeId, const voxel::Region& modifiedRegion, bool markUndo, uint64_t renderRegionMillis) {
	Log::debug("Modified node %i, record undo state: %s", nodeId, markUndo ? "true" : "false");
	voxel::logRegion("Modified", modifiedRegion);
	if (markUndo) {
		voxelformat::SceneGraphNode &node = _sceneGraph.node(nodeId);
		_mementoHandler.markModification(node, modifiedRegion);
	}
	if (modifiedRegion.isValid()) {
		_sceneRenderer.updateNodeRegion(nodeId, modifiedRegion, renderRegionMillis);
	}
	markDirty();
	resetLastTrace();
}

void SceneManager::colorToNewNode(const voxel::Voxel voxelColor) {
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
	voxelutil::rescaleVolume(*srcVolume, _sceneGraph.node(nodeId).palette(), *destVolume);
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
	if (!region.isValid()) {
		return;
	}
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

	if (activeNode() == nodeId) {
		const glm::ivec3 &refPos = referencePosition();
		if (!region.containsPoint(refPos)) {
			setReferencePosition(region.getCenter());
		}
	}
}

void SceneManager::resizeAll(const glm::ivec3& size) {
	_sceneGraph.foreachGroup([&] (int nodeId) {
		resize(nodeId, size);
	});
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

bool SceneManager::setActivePalette(const voxel::Palette &palette, bool searchBestColors) {
	const int nodeId = activeNode();
	if (!_sceneGraph.hasNode(nodeId)) {
		Log::warn("Failed to set the active palette - node with id %i not found", nodeId);
		return false;
	}
	voxelformat::SceneGraphNode &node = _sceneGraph.node(nodeId);
	if (node.type() != voxelformat::SceneGraphNodeType::Model) {
		Log::warn("Failed to set the active palette - node with id %i is no model node", nodeId);
		return false;
	}
	if (searchBestColors) {
		voxel::RawVolume *v = node.volume();
		voxel::RawVolumeWrapper wrapper(v);
		core_assert(v != nullptr);
		const voxel::Palette &oldPalette = node.palette();
		const int voxels = voxelutil::visitVolume(wrapper, [&wrapper, &palette, &oldPalette](int x, int y, int z, const voxel::Voxel &voxel) {
			const core::RGBA rgba = oldPalette.color(voxel.getColor());
			const int newColor = palette.getClosestMatch(rgba);
			if (newColor != -1) {
				voxel::Voxel newVoxel(voxel::VoxelType::Generic, newColor);
				wrapper.setVoxel(x, y, z, newVoxel);
			}
		});
		if (!voxels) {
			_mementoHandler.markPaletteChange(node, wrapper.dirtyRegion());
			node.setPalette(palette);
			return true;
		}
		if (!wrapper.dirtyRegion().isValid()) {
			Log::warn("remapping palette indices failed: %i entries in the old palette", oldPalette.colorCount());
		} else {
			modified(nodeId, wrapper.dirtyRegion());
		}
		_mementoHandler.markPaletteChange(node, wrapper.dirtyRegion());
		node.setPalette(palette);
	} else {
		_mementoHandler.markPaletteChange(node);
		node.setPalette(palette);
	}
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

bool SceneManager::mementoRename(const MementoState& s) {
	Log::debug("Memento: rename of node %i (%s)", s.nodeId, s.name.c_str());
	return nodeRename(s.nodeId, s.name);
}

bool SceneManager::mementoPaletteChange(const MementoState& s) {
	Log::debug("Memento: palette change of node %i to %s", s.nodeId, s.name.c_str());
	if (voxelformat::SceneGraphNode* node = sceneGraphNode(s.nodeId)) {
		node->setPalette(*s.palette.value());
		if (s.hasVolumeData()) {
			mementoModification(s);
		}
		markDirty();
		return true;
	}
	return false;
}

bool SceneManager::mementoModification(const MementoState& s) {
	Log::debug("Memento: modification in volume of node %i (%s)", s.nodeId, s.name.c_str());
	if (voxelformat::SceneGraphNode *node = sceneGraphNode(s.nodeId)) {
		if (node->region() != s.dataRegion()) {
			node->setVolume(new voxel::RawVolume(s.dataRegion()), true);
		}
		MementoData::toVolume(node->volume(), s.data);
		node->setName(s.name);
		if (s.palette.hasValue()) {
			node->setPalette(*s.palette.value());
		}
		modified(node->id(), s.data.region(), false);
		return true;
	}
	Log::warn("Failed to handle memento state - node id %i not found (%s)", s.nodeId, s.name.c_str());
	return false;
}

bool SceneManager::mementoStateToNode(const MementoState &s) {
	voxelformat::SceneGraphNodeType type = voxelformat::SceneGraphNodeType::Model;
	if (!s.hasVolumeData()) {
		type = voxelformat::SceneGraphNodeType::Group;
	}
	voxelformat::SceneGraphNode node(type);
	if (type == voxelformat::SceneGraphNodeType::Model) {
		node.setVolume(new voxel::RawVolume(s.dataRegion()), true);
		MementoData::toVolume(node.volume(), s.data);
		const glm::vec3 pivot = node.region().getPivot();
		const glm::vec3 dimensions = node.region().getDimensionsInVoxels();
		node.setPivot(0, pivot, dimensions);
		if (s.palette.hasValue()) {
			node.setPalette(*s.palette.value());
		}
	}
	node.setName(s.name);
	const int newNodeId = addNodeToSceneGraph(node, s.parentId);
	_mementoHandler.updateNodeId(s.nodeId, newNodeId);
	return newNodeId != -1;
}

bool SceneManager::mementoStateExecute(const MementoState &s, bool isRedo) {
	core_assert(s.valid());
	ScopedMementoHandlerLock lock(_mementoHandler);
	if (s.type == MementoType::SceneNodeRenamed) {
		return mementoRename(s);
	}
	if (s.type == MementoType::SceneNodePaletteChanged) {
		return mementoPaletteChange(s);
	}
	if (s.type == MementoType::SceneNodeMove) {
		Log::debug("Memento: move of node %i (%s) (new parent %i)", s.nodeId, s.name.c_str(), s.parentId);
		return nodeMove(s.nodeId, s.parentId);
	}
	if (s.type == MementoType::SceneNodeTransform) {
		Log::debug("Memento: transform of node %i", s.nodeId);
		if (voxelformat::SceneGraphNode *node = sceneGraphNode(s.nodeId)) {
			voxelformat::SceneGraphTransform &transform = node->keyFrame(s.keyFrame).transform();
			transform.setWorldMatrix(s.worldMatrix);
			transform.update(_sceneGraph, *node, s.keyFrame);
			return true;
		}
		return false;
	}
	if (s.type == MementoType::Modification) {
		return mementoModification(s);
	}
	if (isRedo) {
		if (s.type == MementoType::SceneNodeRemoved) {
			Log::debug("Memento: remove of node %i (%s) from parent %i", s.nodeId, s.name.c_str(), s.parentId);
			return nodeRemove(s.nodeId, true);
		}
		if (s.type == MementoType::SceneNodeAdded) {
			Log::debug("Memento: add node (%s) to parent %i", s.name.c_str(), s.parentId);
			return mementoStateToNode(s);
		}
	} else {
		if (s.type == MementoType::SceneNodeRemoved) {
			Log::debug("Memento: remove of node (%s) from parent %i", s.name.c_str(), s.parentId);
			return mementoStateToNode(s);
		}
		if (s.type == MementoType::SceneNodeAdded) {
			Log::debug("Memento: add node (%s) to parent %i", s.name.c_str(), s.parentId);
			return nodeRemove(s.nodeId, true);
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

	const MementoState& s = _mementoHandler.undo();
	return mementoStateExecute(s, false);
}

bool SceneManager::doRedo() {
	if (!mementoHandler().canRedo()) {
		Log::debug("Nothing to redo");
		return false;
	}

	const MementoState& s = _mementoHandler.redo();
	return mementoStateExecute(s, true);
}

bool SceneManager::copy() {
	const Selection& selection = _modifier.selection();
	if (!selection.isValid()) {
		return false;
	}
	const int nodeId = activeNode();
	voxel::RawVolume* model = volume(nodeId);
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
	const int64_t dismissMillis = core::Var::getSafe(cfg::VoxEditModificationDismissMillis)->intVal();
	modified(nodeId, modifiedRegion, true, dismissMillis);
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
	voxelformat::SceneGraph newSceneGraph;
	for (int nodeId : nodeIds) {
		voxelformat::SceneGraphNode copiedNode;
		const voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId);
		if (node == nullptr || node->type() != voxelformat::SceneGraphNodeType::Model) {
			continue;
		}
		voxelformat::copyNode(*node, copiedNode, true);
		newSceneGraph.emplace(core::move(copiedNode));
	}
	voxelformat::SceneGraph::MergedVolumePalette merged = newSceneGraph.merge();
	if (merged.first == nullptr) {
		return -1;
	}

	voxelformat::SceneGraphNode newNode(voxelformat::SceneGraphNodeType::Model);
	int parent = 0;
	if (voxelformat::SceneGraphNode* firstNode = sceneGraphNode(nodeIds.front())) {
		voxelformat::copyNode(*firstNode, newNode, false);
	}
	newNode.setVolume(merged.first, true);
	newNode.setPalette(merged.second);

	int newNodeId = addNodeToSceneGraph(newNode, parent);
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
	if (!_sceneGraph.hasNode(nodeId1) || !_sceneGraph.hasNode(nodeId2)) {
		return -1;
	}
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
	// this also resets the cursor voxel - but nodeActive() will set it to the first usable index
	// that's why this call must happen before the nodeActive() call.
	_modifier.reset();
	voxelformat::SceneGraphNode &node = *_sceneGraph.begin();
	nodeActivate(node.id());
	_mementoHandler.clearStates();
	Log::debug("New volume for node %i", node.id());
	_mementoHandler.markModification(node, voxel::Region::InvalidRegion);
	_dirty = false;
	_result = voxelutil::PickResult();
	_modifier.setCursorVoxel(voxel::createVoxel(node.palette(), 0));
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

		for (voxelformat::SceneGraphKeyFrame &keyFrame : node->keyFrames()) {
			keyFrame.transform().update(sceneGraph(), *node, keyFrame.frameIdx);
		}

		_mementoHandler.markNodeAdded(*node);
		markDirty();

		Log::debug("Add node %i to scene graph", newNodeId);
		if (type == voxelformat::SceneGraphNodeType::Model) {
			// update the whole volume
			_sceneRenderer.updateNodeRegion(newNodeId, region);

			_result = voxelutil::PickResult();
			nodeActivate(newNodeId);
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
	_sceneRenderer.clear();

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

void SceneManager::updateGridRenderer(const voxel::Region& region) {
	_sceneRenderer.updateGridRegion(region);
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
	_sceneRenderer.clear();

	voxel::RawVolume* v = new voxel::RawVolume(region);
	voxelformat::SceneGraphNode node;
	node.setVolume(v, true);
	if (name.empty()) {
		node.setName("unnamed");
	} else {
		node.setName(name);
	}
	const int nodeId = voxelformat::addNodeToSceneGraph(_sceneGraph, node, 0);
	if (nodeId == -1) {
		Log::error("Failed to add empty volume to new scene graph");
		return false;
	}
	glm::ivec3 center = v->region().getCenter();
	center.y = region.getLowerY();
	setReferencePosition(center);
	resetSceneState();
	_lastFilename.clear();
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
		const glm::vec3 &pivot = node->transform(_currentFrameIdx).pivot();
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
	voxel::RawVolume *v = node->volume();
	if (v == nullptr) {
		return;
	}
	voxel::Region region = v->region();
	v->translate(m);
	region.accumulate(v->region());
	modified(nodeId, region);
}

void SceneManager::shift(int x, int y, int z) {
	const glm::ivec3 v(x, y, z);
	_sceneGraph.foreachGroup([&] (int nodeId) {
		shift(nodeId, v);
	});
}

void SceneManager::exchangeColors(int nodeId, uint8_t palIdx1, uint8_t palIdx2) {
	voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		return;
	}
	voxel::RawVolume *v = node->volume();
	if (v == nullptr) {
		return;
	}
	const voxel::Palette &palette = node->palette();
	voxel::RawVolumeWrapper wrapper(v);
	voxelutil::visitVolume(*v, [&wrapper, &palette, palIdx1, palIdx2] (int x, int y, int z, const voxel::Voxel& voxel) {
		if (voxel.getColor() == palIdx1) {
			wrapper.setVoxel(x, y, z, voxel::createVoxel(palette, palIdx2));
		} else if (voxel.getColor() == palIdx2) {
			wrapper.setVoxel(x, y, z, voxel::createVoxel(palette, palIdx1));
		}
	});
	if (wrapper.dirtyRegion().isValid()) {
		modified(nodeId, wrapper.dirtyRegion());
	}
}

bool SceneManager::setGridResolution(int resolution) {
	if (_modifier.gridResolution() == resolution) {
		return false;
	}
	_modifier.setGridResolution(resolution);
	setCursorPosition(cursorPosition(), true);
	return true;
}

void SceneManager::render(voxelrender::RenderContext &renderContext, const video::Camera& camera, uint8_t renderMask) {
	const bool renderScene = (renderMask & RenderScene) != 0u;
	if (renderScene) {
		_sceneRenderer.renderScene(renderContext, camera, _sceneGraph, _currentFrameIdx);
	}
	const bool renderUI = (renderMask & RenderUI) != 0u;
	if (renderUI) {
		_sceneRenderer.renderUI(renderContext, camera, _sceneGraph);
		_modifier.render(camera);
	}
}

void SceneManager::construct() {
	_modifier.construct();
	_mementoHandler.construct();
	_sceneRenderer.construct();

	_autoSaveSecondsDelay = core::Var::get(cfg::VoxEditAutoSaveSeconds, "180");

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
				_move[i]);
	}
	command::Command::registerCommand("palette_changeintensity", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: palette_changeintensity [value]");
			return;
		}
		const float scale = core::string::toFloat(args[0]);
		const int nodeId = activeNode();
		voxelformat::SceneGraphNode &node = _sceneGraph.node(nodeId);
		voxel::Palette &pal = node.palette();
		pal.changeIntensity(scale);
		_mementoHandler.markPaletteChange(node);
	}).setHelp("Change intensity by scaling the rgb values of the palette");

	command::Command::registerCommand("palette_sort", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: palette_sort [hue|saturation|brightness|cielab]");
			return;
		}
		const core::String &type = args[0];
		const int nodeId = activeNode();
		voxelformat::SceneGraphNode &node = _sceneGraph.node(nodeId);
		voxel::Palette &pal = node.palette();
		if (type == "hue") {
			pal.sortHue();
		} else if (type == "brightness") {
			pal.sortBrightness();
		} else if (type == "cielab") {
			pal.sortCIELab();
		} else if (type == "saturation") {
			pal.sortSaturation();
		}
		_mementoHandler.markPaletteChange(node);
	}).setHelp("Change intensity by scaling the rgb values of the palette");

	command::Command::registerActionButton("zoom_in", _zoomIn);
	command::Command::registerActionButton("zoom_out", _zoomOut);
	command::Command::registerActionButton("camera_rotate", _rotate);
	command::Command::registerActionButton("camera_pan", _pan);
	command::Command::registerCommand("mouse_node_select", [&] (const command::CmdArgs&) {
		if (_sceneModeNodeIdTrace != -1) {
			Log::debug("switch active node to hovered from scene graph mode: %i", _sceneModeNodeIdTrace);
			nodeActivate(_sceneModeNodeIdTrace);
		}
	}).setHelp("Switch active node to hovered from scene graph mode");

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

	command::Command::registerCommand("text", [this] (const command::CmdArgs& args) {
		if (args.size() != 2) {
			Log::info("Usage: text <string> <size>");
			return;
		}
		const core::String &str = args[0];
		const int size = args[1].toInt();
		renderText(str.c_str(), size);
	}).setHelp("Render a character to the reference position");

	command::Command::registerCommand("layerssave", [&] (const command::CmdArgs& args) {
		core::String dir = ".";
		if (!args.empty()) {
			dir = args[0];
		}
		if (!saveModels(dir)) {
			Log::error("Failed to save models to dir: %s", dir.c_str());
		}
	}).setHelp("Save all nodes into filenames represented by their node names");

	command::Command::registerCommand("layersave", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc < 1) {
			Log::info("Usage: layersave <nodeid> [<file>]");
			return;
		}
		const int nodeId = core::string::toInt(args[0]);
		core::String file = core::string::format("node%i.vox", nodeId);
		if (args.size() == 2) {
			file = args[1];
		}
		if (!saveNode(nodeId, file)) {
			Log::error("Failed to save node %i to file: %s", nodeId, file.c_str());
		}
	}).setHelp("Save a single node to the given path with their node names");

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
	}).setHelp("Crop the current active node to the voxel boundaries");

	command::Command::registerCommand("scale", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		int nodeId = activeNode();
		if (argc == 1) {
			nodeId = core::string::toInt(args[0]);
		}
		scale(nodeId);
	}).setHelp("Scale the current active node or the given node down");

	command::Command::registerCommand("colortolayer", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc < 1) {
			const voxel::Voxel voxel = _modifier.cursorVoxel();
			colorToNewNode(voxel);
		} else {
			const uint8_t index = core::string::toInt(args[0]);
			const voxel::Voxel voxel = voxel::createVoxel(activePalette(), index);
			colorToNewNode(voxel);
		}
	}).setHelp("Move the voxels of the current selected palette index or the given index into a new node");

	command::Command::registerCommand("abortaction", [&] (const command::CmdArgs& args) {
		_modifier.aabbAbort();
	}).setHelp("Aborts the current modifier action");

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
	}).setHelp("Set the reference position to the current cursor position");

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
	}).setHelp("Resize your volume about given x, y and z size");

	command::Command::registerCommand("layersize", [this] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc == 1) {
			const int size = core::string::toInt(args[0]);
			resize(activeNode(), glm::ivec3(size));
		} else if (argc == 3) {
			glm::ivec3 size;
			for (int i = 0; i < argc; ++i) {
				size[i] = core::string::toInt(args[i]);
			}
			resize(activeNode(), size);
		} else {
			resize(activeNode(), glm::ivec3(1));
		}
	}).setHelp("Resize your current node about given x, y and z size");

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
	}).setHelp("Center the current active nodes at the reference position");

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
	}).setHelp("Center the current active nodes at the origin");

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
	}).setHelp("Rotate active nodes by the given angles (in degree)");

	command::Command::registerCommand("layermerge", [&] (const command::CmdArgs& args) {
		int nodeId1;
		int nodeId2;
		if (args.size() == 2) {
			nodeId1 = core::string::toInt(args[0]);
			nodeId2 = core::string::toInt(args[1]);
		} else {
			nodeId2 = activeNode();
			nodeId1 = _sceneGraph.prevModelNode(nodeId2);
		}
		mergeNodes(nodeId1, nodeId2);
	}).setHelp("Merge two given nodes or active model node with the next one");

	command::Command::registerCommand("layermergeall", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::All);
	}).setHelp("Merge all nodes");

	command::Command::registerCommand("layermergevisible", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::Visible);
	}).setHelp("Merge all visible nodes");

	command::Command::registerCommand("layermergelocked", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::Locked);
	}).setHelp("Merge all locked nodes");

	command::Command::registerCommand("animate", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: animate <nodedelaymillis> <0|1>");
			Log::info("nodedelay of 0 will stop the animation, too");
			return;
		}
		if (args.size() == 2) {
			if (!core::string::toBool(args[1])) {
				_animationSpeed = 0.0;
				return;
			}
		}
		_animationSpeed = core::string::toDouble(args[0]) / 1000.0;
	}).setHelp("Animate all nodes with the given delay in millis between the frames");

	command::Command::registerCommand("setcolor", [&] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: setcolor <index>");
			return;
		}
		const uint8_t index = core::string::toInt(args[0]);
		const voxel::Voxel voxel = voxel::createVoxel(activePalette(), index);
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
		const voxel::Voxel voxel = voxel::createVoxel(activePalette(), index);
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
	}).setHelp("Flip the selected nodes around the given axis");

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
	}).setHelp("Add a new model node (with a given name and width, height, depth - all optional)");

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
	}).setHelp("Lock a particular node by id - or the current active one");

	command::Command::registerCommand("togglelayerlock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(!node->locked());
		}
	}).setHelp("Toggle the lock state of a particular node by id - or the current active one");

	command::Command::registerCommand("layerunlock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (voxelformat::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(false);
		}
	}).setHelp("Unlock a particular node by id - or the current active one");

	command::Command::registerCommand("layeractive", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Active node: %i", activeNode());
			return;
		}
		const int nodeId = core::string::toInt(args[0]);
		nodeActivate(nodeId);
	}).setHelp("Set or print the current active node");

	command::Command::registerCommand("togglelayerstate", [&](const command::CmdArgs &args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			node->setVisible(!node->visible());
		}
	}).setHelp("Toggle the visible state of a node");

	command::Command::registerCommand("layerhideall", [&](const command::CmdArgs &args) {
		for (voxelformat::SceneGraphNode &node : _sceneGraph) {
			node.setVisible(false);
		}
	}).setHelp("Hide all nodes");

	command::Command::registerCommand("layerlockall", [&](const command::CmdArgs &args) {
		for (voxelformat::SceneGraphNode &node : _sceneGraph) {
			node.setLocked(true);
		}
	}).setHelp("Lock all nodes");

	command::Command::registerCommand("layerunlockall", [&] (const command::CmdArgs& args) {
		for (voxelformat::SceneGraphNode &node : _sceneGraph) {
			node.setLocked(false);
		}
	}).setHelp("Unlock all nodes");

	command::Command::registerCommand("layerhideothers", [&] (const command::CmdArgs& args) {
		for (voxelformat::SceneGraphNode &node : _sceneGraph) {
			if (node.id() == activeNode()) {
				node.setVisible(true);
				continue;
			}
			node.setVisible(false);
		}
	}).setHelp("Hide all model nodes except the active one");

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
	}).setHelp("Show all nodes");

	command::Command::registerCommand("layerduplicate", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			nodeDuplicate(*node);
		}
	}).setHelp("Duplicates the current node or the given node id");
}

void SceneManager::renderText(const char *str, int size, int thickness, int spacing, const char *font) {
	if (!_voxelFont.init(font)) {
		Log::error("Failed to initialize voxel font with %s", font);
		return;
	}
	voxel::RawVolume *v = activeVolume();
	if (v == nullptr) {
		return;
	}
	voxel::RawVolumeWrapper wrapper(v);

	const char **s = &str;
	glm::ivec3 pos = referencePosition();
	for (int c = core::utf8::next(s); c != -1; c = core::utf8::next(s)) {
		pos.x += _voxelFont.renderCharacter(c, size, thickness, pos, wrapper, _modifier.cursorVoxel());
		pos.x += spacing;
	}

	modified(activeNode(), wrapper.dirtyRegion());
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

bool SceneManager::init() {
	++_initialized;
	if (_initialized > 1) {
		Log::debug("Already initialized");
		return true;
	}

	voxel::Palette palette;
	if (!palette.load(core::Var::getSafe(cfg::VoxEditLastPalette)->strVal().c_str())) {
		palette = voxel::getPalette();
	}
	if (!_mementoHandler.init()) {
		Log::error("Failed to initialize the memento handler");
		return false;
	}
	if (!_sceneRenderer.init()) {
		Log::error("Failed to initialize the scene renderer");
		return false;
	}
	if (!_modifier.init()) {
		Log::error("Failed to initialize the modifier");
		return false;
	}

	if (!_luaGenerator.init()) {
		Log::error("Failed to initialize the lua generator bindings");
		return false;
	}

	_gridSize = core::Var::getSafe(cfg::VoxEditGridsize);
	const core::TimeProviderPtr& timeProvider = app::App::getInstance()->timeProvider();
	_lastAutoSave = timeProvider->tickSeconds();

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
		const int roundTrip = modelCount + _currentAnimationModelIdx;
		for (int modelIdx = _currentAnimationModelIdx + 1; modelIdx < roundTrip; ++modelIdx) {
			voxelformat::SceneGraphNode *node = _sceneGraph[_currentAnimationModelIdx];
			core_assert_always(node != nullptr);
			node->setVisible(false);
			_currentAnimationModelIdx = modelIdx % modelCount;
			node = _sceneGraph[_currentAnimationModelIdx];
			core_assert_always(node != nullptr);
			node->setVisible(true);
			return;
		}
	}
}

void SceneManager::zoom(video::Camera& camera, float level) const {
	camera.zoom(level);
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
	_sceneRenderer.update();
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

	autosave();

	_sceneRenderer.shutdown();
	_sceneGraph.clear();
	_mementoHandler.clearStates();

	_luaGenerator.shutdown();
	_modifier.shutdown();
	_mementoHandler.shutdown();
	_voxelFont.shutdown();

	command::Command::unregisterActionButton("zoom_in");
	command::Command::unregisterActionButton("zoom_out");
	command::Command::unregisterActionButton("camera_rotate");
	command::Command::unregisterActionButton("camera_pan");
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
}

void SceneManager::moveCursor(int x, int y, int z) {
	glm::ivec3 p = cursorPosition();
	const int res = _modifier.gridResolution();
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

	const int res = _modifier.gridResolution();
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
	// TODO: multiple different viewport....
	_modifier.setCursorPosition(pos, _result.hitFace);
	if (oldCursorPos == pos) {
		return;
	}
	_sceneRenderer.updateLockedPlanes(_lockedAxis, _sceneGraph, cursorPosition());
}

bool SceneManager::trace(bool sceneMode, bool force) {
	if (_modifier.isLocked()) {
		return false;
	}
	if (sceneMode) {
		traceScene(force);
		return true;
	}

	return mouseRayTrace(force);
}

void SceneManager::traceScene(bool force) {
	if (_sceneModeNodeIdTrace != -1) {
		// if the trace is not forced, and the mouse cursor position did not change, don't
		// re-execute the trace.
		if (_lastRaytraceX == _mouseCursor.x && _lastRaytraceY == _mouseCursor.y && !force) {
			return;
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
		const math::OBB<float>& obb = toOBB(true, region, node.transformForFrame(_currentFrameIdx));
		if (obb.intersect(ray.origin, ray.direction, _camera->farPlane(), distance)) {
			if (distance < intersectDist) {
				intersectDist = distance;
				_sceneModeNodeIdTrace = node.id();
			}
		}
	}
	Log::trace("Hovered node: %i", _sceneModeNodeIdTrace);
}

void SceneManager::updateCursor() {
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

	const voxel::RawVolume *v = activeVolume();
	if (_result.didHit) {
		_modifier.setHitCursorVoxel(v->voxel(_result.hitVoxel));
	} else {
		_modifier.setHitCursorVoxel(voxel::Voxel());
	}
	_modifier.setVoxelAtCursor(v->voxel(_modifier.cursorPosition()));
}

bool SceneManager::mouseRayTrace(bool force) {
	// mouse tracing is disabled - e.g. because the voxel cursor was moved by keyboard
	// shortcuts. In this case the execution of the modifier would result in a
	// re-execution of the trace. And that would move the voxel cursor to the mouse pos
	if (!_traceViaMouse) {
		return false;
	}
	// if the trace is not forced, and the mouse cursor position did not change, don't
	// re-execute the trace.
	if (_lastRaytraceX == _mouseCursor.x && _lastRaytraceY == _mouseCursor.y && !force) {
		return true;
	}
	video::Camera *camera = activeCamera();
	if (camera == nullptr) {
		return false;
	}
	const voxel::RawVolume* v = activeVolume();
	if (v == nullptr) {
		return false;
	}
	const math::Ray& ray = camera->mouseRay(_mouseCursor);
	const float rayLength = camera->farPlane();

	const glm::vec3& dirWithLength = ray.direction * rayLength;
	static constexpr voxel::Voxel air;

	Log::trace("Execute new trace for %i:%i (%i:%i)",
			_mouseCursor.x, _mouseCursor.y, _lastRaytraceX, _lastRaytraceY);

	core_trace_scoped(EditorSceneOnProcessUpdateRay);
	_lastRaytraceX = _mouseCursor.x;
	_lastRaytraceY = _mouseCursor.y;

	_result.didHit = false;
	_result.validPreviousPosition = false;
	_result.firstInvalidPosition = false;
	_result.firstValidPosition = false;
	_result.direction = ray.direction;
	_result.hitFace = voxel::FaceNames::Max;
	voxelutil::raycastWithDirection(activeVolume(), ray.origin, dirWithLength, [&] (voxel::RawVolume::Sampler& sampler) {
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

	updateCursor();

	return true;
}

void SceneManager::setLockedAxis(math::Axis axis, bool unlock) {
	if (unlock) {
		_lockedAxis &= ~axis;
	} else {
		_lockedAxis |= axis;
	}
	_sceneRenderer.updateLockedPlanes(_lockedAxis, _sceneGraph, cursorPosition());
}

bool SceneManager::nodeUpdateTransform(int nodeId, const glm::mat4 &localMatrix, const glm::mat4 *deltaMatrix,
									   voxelformat::KeyFrameIndex keyFrameIdx) {
	if (nodeId == -1) {
		nodeForeachGroup([&] (int nodeId) {
			if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeUpdateTransform(*node, localMatrix, deltaMatrix, keyFrameIdx);
			}
		});
		return true;
	}
	if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeUpdateTransform(*node, localMatrix, deltaMatrix, keyFrameIdx);
	}
	return false;
}

bool SceneManager::nodeAddKeyframe(voxelformat::SceneGraphNode &node, voxelformat::FrameIndex frameIdx) {
	// TODO: memento state
	if (node.addKeyFrame(frameIdx) == InvalidKeyFrame) {
		Log::error("Failed to add keyframe for frame %i", (int)frameIdx);
		return false;
	}
	const voxelformat::KeyFrameIndex newKeyFrameIdx = node.keyFrameForFrame(frameIdx);
	if (newKeyFrameIdx > 0) {
		node.keyFrame(newKeyFrameIdx).setTransform(node.keyFrame(newKeyFrameIdx - 1).transform());
		return true;
	}
	return false;
}

bool SceneManager::nodeAddKeyFrame(int nodeId, voxelformat::FrameIndex frameIdx) {
	if (nodeId == -1) {
		nodeForeachGroup([&](int nodeId) {
			voxelformat::SceneGraphNode &node = _sceneGraph.node(nodeId);
			nodeAddKeyframe(node, frameIdx);
		});
		return true;
	}
	voxelformat::SceneGraphNode &node = _sceneGraph.node(nodeId);
	return nodeAddKeyframe(node, frameIdx);
}

bool SceneManager::nodeRemoveKeyFrame(int nodeId, voxelformat::FrameIndex frameIdx) {
	if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeRemoveKeyFrame(*node, frameIdx);
	}
	return false;
}

bool SceneManager::nodeRemoveKeyFrameByIndex(int nodeId, voxelformat::KeyFrameIndex keyFrameIdx) {
	if (voxelformat::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeRemoveKeyFrameByIndex(*node, keyFrameIdx);
	}
	return false;
}

bool SceneManager::nodeRemoveKeyFrame(voxelformat::SceneGraphNode &node, voxelformat::FrameIndex frameIdx) {
	// TODO: memento state
	return node.removeKeyFrame(frameIdx);
}

bool SceneManager::nodeRemoveKeyFrameByIndex(voxelformat::SceneGraphNode &node, voxelformat::KeyFrameIndex keyFrameIdx) {
	// TODO: memento state
	return node.removeKeyFrameByIndex(keyFrameIdx);
}

bool SceneManager::nodeUpdateTransform(voxelformat::SceneGraphNode &node, const glm::mat4 &localMatrix,
									   const glm::mat4 *deltaMatrix, voxelformat::KeyFrameIndex keyFrameIdx) {
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
	transform.setLocalScale(scale);
	transform.update(_sceneGraph, node, keyFrame.frameIdx);

	_mementoHandler.markNodeTransform(node, keyFrameIdx);
	markDirty();

	return true;
}

bool SceneManager::nodeMove(int sourceNodeId, int targetNodeId) {
	if (_sceneGraph.changeParent(sourceNodeId, targetNodeId)) {
		voxelformat::SceneGraphNode *node = sceneGraphNode(sourceNodeId);
		core_assert(node != nullptr);
		_mementoHandler.markNodeMoved(targetNodeId, sourceNodeId);
		markDirty();
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
	markDirty();
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

void SceneManager::markDirty() {
	_needAutoSave = true;
	_dirty = true;
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
	markDirty();
	if (_sceneGraph.empty()) {
		const voxel::Region region(glm::ivec3(0), glm::ivec3(31));
		newScene(true, name, region);
		// TODO: allow to undo the removal of the last node
	}
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
	if (node.type() == voxelformat::SceneGraphNodeType::Camera) {
		const voxelformat::SceneGraphNodeCamera& cameraNode = voxelformat::toCameraNode(node);
		video::Camera nodeCamera = voxelrender::toCamera(activeCamera()->size(), cameraNode);
		nodeCamera.update(0.0f);
		activeCamera()->lerp(nodeCamera);
		return false;
	} else if (node.type() != voxelformat::SceneGraphNodeType::Model) {
		Log::warn("Given node id %i is no model node", nodeId);
		return false;
	}
	_sceneGraph.setActiveNode(nodeId);
	const voxel::Palette &palette = node.palette();
	for (int i = 0; i < palette.colorCount(); ++i) {
		if (palette.color(i).a > 0) {
			_modifier.setCursorVoxel(voxel::createVoxel(palette, i));
			break;
		}
	}
	const voxel::Region& region = node.region();
	updateGridRenderer(region);
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
