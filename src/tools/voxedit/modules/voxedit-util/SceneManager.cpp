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
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "math/AABB.h"
#include "math/Axis.h"
#include "math/Random.h"
#include "math/Ray.h"
#include "video/Camera.h"
#include "video/Renderer.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "video/Types.h"
#include "voxedit-util/modifier/ModifierRenderer.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeMoveWrapper.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelfont/VoxelFont.h"
#include "voxelformat/Format.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelgenerator/TreeGenerator.h"
#include "voxelrender/ImageGenerator.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/SceneGraphRenderer.h"
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

inline auto nodeCompleter(const scenegraph::SceneGraph &sceneGraph) {
	return [&] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		int i = 0;
		for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &modelNode = *iter;
			matches.push_back(core::string::toString(modelNode.id()));
		}
		return i;
	};
}

inline auto paletteCompleter() {
	return [&] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		int i = 0;
		for (i = 0; i < lengthof(voxel::Palette::builtIn); ++i) {
			if (core::string::startsWith(voxel::Palette::builtIn[i], str.c_str())) {
				matches.push_back(voxel::Palette::builtIn[i]);
			}
		}
		return i;
	};
}

SceneManager::SceneManager() : SceneManager(core::make_shared<SceneRenderer>(), core::make_shared<ModifierRenderer>()) {
}

SceneManager::SceneManager(const SceneRendererPtr &sceneRenderer, const ModifierRendererPtr &modifierRenderer)
	: _sceneRenderer(sceneRenderer), _modifier(modifierRenderer) {
}

SceneManager::~SceneManager() {
	core_assert_msg(_initialized == 0, "SceneManager was not properly shut down");
}

bool SceneManager::loadPalette(const core::String& paletteName, bool searchBestColors, bool save) {
	voxel::Palette palette;

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
		const core::String &paletteFilename = core::string::format("palette-%s.png", filename.c_str());
		const io::FilesystemPtr &fs = io::filesystem();
		const io::FilePtr &pngFile = fs->open(paletteFilename, io::FileMode::Write);
		if (!palette.save(pngFile->name().c_str())) {
			Log::warn("Failed to write palette image: %s", paletteFilename.c_str());
		}
	}

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
	scenegraph::SceneGraphNode newNode;
	newNode.setVolume(v, true);
	newNode.setName(core::string::extractFilename(img->name().c_str()));
	return addNodeToSceneGraph(newNode) != InvalidNodeId;
}

bool SceneManager::importAsPlane(const core::String& file) {
	const image::ImagePtr& img = image::loadImage(file);
	voxel::RawVolume *v = voxelutil::importAsPlane(img);
	if (v == nullptr) {
		return false;
	}
	scenegraph::SceneGraphNode newNode;
	newNode.setVolume(v, true);
	newNode.setName(core::string::extractFilename(img->name().c_str()));
	return addNodeToSceneGraph(newNode) != InvalidNodeId;
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
	scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId);
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
	const int delay = _autoSaveSecondsDelay->intVal();
	if (delay <= 0 || _lastAutoSave + (double)delay > timeProvider->tickSeconds()) {
		return;
	}
	io::FileDescription autoSaveFilename;
	if (_lastFilename.empty()) {
		autoSaveFilename.set("autosave-noname." + voxelformat::vengi().exts[0]);
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
	const scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		Log::warn("Node with id %i wasn't found", nodeId);
		return true;
	}
	if (node->type() != scenegraph::SceneGraphNodeType::Model) {
		Log::warn("Given node is no model node");
		return false;
	}
	scenegraph::SceneGraph newSceneGraph;
	scenegraph::SceneGraphNode newNode;
	scenegraph::copyNode(*node, newNode, false);
	if (node->isReference()) {
		newNode.setVolume(_sceneGraph.resolveVolume(*node), false);
	}
	newSceneGraph.emplace(core::move(newNode));
	voxelformat::SaveContext saveCtx;
	saveCtx.thumbnailCreator = voxelrender::volumeThumbnail;
	if (voxelformat::saveFormat(filePtr, &_lastFilename.desc, newSceneGraph, saveCtx)) {
		Log::info("Saved node %i to %s", nodeId, filePtr->name().c_str());
		return true;
	}
	Log::warn("Failed to save node %i to %s", nodeId, filePtr->name().c_str());
	return false;
}

void SceneManager::fillHollow() {
	_sceneGraph.foreachGroup([&] (int nodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
		if (node == nullptr || node->type() != scenegraph::SceneGraphNodeType::Model) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		voxel::RawVolumeWrapper wrapper = _modifier.createRawVolumeWrapper(v);
		voxelutil::fillHollow(wrapper, _modifier.cursorVoxel());
		modified(nodeId, wrapper.dirtyRegion());
	});
}

void SceneManager::hollow() {
	_sceneGraph.foreachGroup([&](int nodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
		if (node == nullptr || node->type() != scenegraph::SceneGraphNodeType::Model) {
			return;
		}
		voxel::RawVolume *v = node->volume();
		if (v == nullptr) {
			return;
		}
		voxel::RawVolumeWrapper wrapper = _modifier.createRawVolumeWrapper(v);
		core::DynamicArray<glm::ivec3> filled;
		voxelutil::visitUndergroundVolume(wrapper, [&filled](int x, int y, int z, const voxel::Voxel &voxel) {
			filled.emplace_back(x, y, z);
		});
		for (const glm::ivec3 &pos : filled) {
			wrapper.setVoxel(pos, voxel::Voxel());
		}
		modified(nodeId, wrapper.dirtyRegion());
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
	voxel::RawVolumeWrapper wrapper = _modifier.createRawVolumeWrapper(v);
	const glm::ivec3 &pos = _modifier.cursorPosition();
	const voxel::FaceNames face = _modifier.cursorFace();
	const voxel::Voxel hitVoxel/* = hitCursorVoxel()*/; // TODO: should be an option
	voxelutil::fillPlane(wrapper, image, hitVoxel, pos, face);
	modified(nodeId, wrapper.dirtyRegion());
}

void SceneManager::updateVoxelType(int nodeId, uint8_t palIdx, voxel::VoxelType newType) {
	voxel::RawVolume *v = volume(nodeId);
	if (v == nullptr) {
		return;
	}
	voxel::RawVolumeWrapper wrapper(v);
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
	for (auto iter = _sceneGraph.beginAllModels(); iter != _sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
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

	voxelformat::SaveContext saveCtx;
	saveCtx.thumbnailCreator = voxelrender::volumeThumbnail;
	if (voxelformat::saveFormat(filePtr, &file.desc, _sceneGraph, saveCtx)) {
		if (!autosave) {
			_dirty = false;
			_lastFilename = file;
			core::Var::get(cfg::VoxEditLastFile)->setVal(filePtr->name());
		}
		_needAutoSave = false;
		return true;
	}
	Log::warn("Failed to save to desired format");
	return false;
}

static void mergeIfNeeded(scenegraph::SceneGraph &newSceneGraph) {
	if (newSceneGraph.size() > voxelrender::RawVolumeRenderer::MAX_VOLUMES) {
		const scenegraph::SceneGraph::MergedVolumePalette &merged = newSceneGraph.merge(true);
		newSceneGraph.clear();
		scenegraph::SceneGraphNode newNode;
		newNode.setVolume(merged.first, true);
		newNode.setPalette(merged.second);
		newSceneGraph.emplace(core::move(newNode));
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
	scenegraph::SceneGraph newSceneGraph;
	io::FileStream stream(filePtr);
	voxelformat::LoadContext loadCtx;
	io::FileDescription fileDesc;
	fileDesc.set(filePtr->name());
	if (!voxelformat::loadFormat(fileDesc, stream, newSceneGraph, loadCtx)) {
		Log::error("Failed to load %s", file.c_str());
		return false;
	}
	mergeIfNeeded(newSceneGraph);

	scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
	groupNode.setName(core::string::extractFilename(file));
	int newNodeId = _sceneGraph.emplace(core::move(groupNode), activeNode());
	bool state = false;
	for (auto iter = newSceneGraph.beginAllModels(); iter != newSceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		state |= addNodeToSceneGraph(node, newNodeId) != InvalidNodeId;
	}

	return state;
}

bool SceneManager::importDirectory(const core::String& directory, const io::FormatDescription *format, int depth) {
	if (directory.empty()) {
		return false;
	}
	core::DynamicArray<io::FilesystemEntry> entities;
	io::filesystem()->list(directory, entities, format ? format->wildCard() : "", depth);
	if (entities.empty()) {
		Log::info("Could not find any model in %s", directory.c_str());
		return false;
	}
	bool state = false;
	scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
	groupNode.setName(core::string::extractFilename(directory));
	int importGroupNodeId = _sceneGraph.emplace(core::move(groupNode), activeNode());

	for (const auto &e : entities) {
		if (format == nullptr && !voxelformat::isModelFormat(e.name)) {
			continue;
		}
		scenegraph::SceneGraph newSceneGraph;
		io::FilePtr filePtr = io::filesystem()->open(e.fullPath, io::FileMode::SysRead);
		io::FileStream stream(filePtr);
		voxelformat::LoadContext loadCtx;
		io::FileDescription fileDesc;
		fileDesc.set(filePtr->name(), format);
		if (!voxelformat::loadFormat(fileDesc, stream, newSceneGraph, loadCtx)) {
			Log::error("Failed to load %s", e.fullPath.c_str());
		} else {
			mergeIfNeeded(newSceneGraph);
			for (auto iter = newSceneGraph.beginModel(); iter != newSceneGraph.end(); ++iter) {
				scenegraph::SceneGraphNode &node = *iter;
				state |= addNodeToSceneGraph(node, importGroupNodeId) != InvalidNodeId;
			}
		}
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
	_loadingFuture = threadPool.enqueue([filePtr, file] () {
		scenegraph::SceneGraph newSceneGraph;
		io::FileStream stream(filePtr);
		voxelformat::LoadContext loadCtx;
		voxelformat::loadFormat(file, stream, newSceneGraph, loadCtx);
		mergeIfNeeded(newSceneGraph);
		// TODO: stuff that happens in RawVolumeRenderer::extractRegion and
		// RawVolumeRenderer::scheduleExtractions should happen here
		return core::move(newSceneGraph);
	});
	_lastFilename.set(filePtr->name(), &file.desc);
	return true;
}

bool SceneManager::load(const io::FileDescription& file, const uint8_t *data, size_t size) {
	scenegraph::SceneGraph newSceneGraph;
	io::MemoryReadStream stream(data, size);
	voxelformat::LoadContext loadCtx;
	voxelformat::loadFormat(file, stream, newSceneGraph, loadCtx);
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
	_traceViaMouse = true;
}

void SceneManager::modified(int nodeId, const voxel::Region& modifiedRegion, bool markUndo, uint64_t renderRegionMillis) {
	Log::debug("Modified node %i, record undo state: %s", nodeId, markUndo ? "true" : "false");
	voxel::logRegion("Modified", modifiedRegion);
	if (markUndo) {
		scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		_mementoHandler.markModification(node, modifiedRegion);
	}
	if (modifiedRegion.isValid()) {
		_sceneRenderer->updateNodeRegion(nodeId, modifiedRegion, renderRegionMillis);
	}
	markDirty();
	resetLastTrace();
}

void SceneManager::colorToNewNode(const voxel::Voxel voxelColor) {
	const voxel::Region &region = _sceneGraph.groupRegion();
	if (!region.isValid()) {
		Log::warn("Invalid node region");
		return;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(region);
	_sceneGraph.foreachGroup([&] (int nodeId) {
		voxel::RawVolume *v = volume(nodeId);
		if (v == nullptr) {
			return;
		}
		voxel::RawVolumeWrapper wrapper(v);
		voxelutil::visitVolume(wrapper, [&] (int32_t x, int32_t y, int32_t z, const voxel::Voxel& voxel) {
			if (voxel.getColor() == voxelColor.getColor()) {
				newVolume->setVoxel(x, y, z, voxel);
				wrapper.setVoxel(x, y, z, voxel::Voxel());
			}
		});
		modified(nodeId, wrapper.dirtyRegion());
	});
	scenegraph::SceneGraphNode newNode;
	newNode.setVolume(newVolume, true);
	newNode.setName(core::string::format("color: %i", (int)voxelColor.getColor()));
	addNodeToSceneGraph(newNode);
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

void SceneManager::crop() {
	const int nodeId = activeNode();
	scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId);
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
	if (v == nullptr) {
		return;
	}
	voxel::Region region = v->region();
	region.shiftUpperCorner(size);
	resize(nodeId, region);
}

void SceneManager::resize(int nodeId, const voxel::Region &region) {
	if (!region.isValid()) {
		return;
	}
	scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId);
	if (node == nullptr) {
		return;
	}
	voxel::RawVolume* v = _sceneGraph.resolveVolume(*node);
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
	if (nodeId == InvalidNodeId) {
		return nullptr;
	}
	scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId);
	core_assert_msg(node != nullptr, "Node with id %i wasn't found in the scene graph", nodeId);
	if (node == nullptr) {
		return nullptr;
	}
	// TODO: use _sceneGraph.resolveVolume(*node); here, too?
	return node->volume();
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
	scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	if (node.type() != scenegraph::SceneGraphNodeType::Model) {
		Log::warn("Failed to set the active palette - node with id %i is no model node", nodeId);
		return false;
	}
	if (searchBestColors) {
		const voxel::Region dirtyRegion = node.remapToPalette(palette);
		if (dirtyRegion.isValid()) {
			_mementoHandler.markPaletteChange(node, dirtyRegion);
			node.setPalette(palette);
			return true;
		}
		if (!dirtyRegion.isValid()) {
			Log::warn("Remapping palette indices failed");
		} else {
			modified(nodeId, dirtyRegion);
		}
		_mementoHandler.markPaletteChange(node, dirtyRegion);
		node.setPalette(palette);
	} else {
		_mementoHandler.markPaletteChange(node);
		node.setPalette(palette);
	}
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

bool SceneManager::mementoRename(const MementoState& s) {
	Log::debug("Memento: rename of node %i (%s)", s.nodeId, s.name.c_str());
	return nodeRename(s.nodeId, s.name);
}

bool SceneManager::mementoProperties(const MementoState& s) {
	Log::debug("Memento: properties of node %i (%s)", s.nodeId, s.name.c_str());
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(s.nodeId)) {
		node->properties().clear();
		core_assert(s.properties.hasValue());
		node->addProperties(*s.properties.value());
		return true;
	}
	return false;
}
bool SceneManager::mementoKeyFrames(const MementoState& s) {
	Log::debug("Memento: keyframes of node %i (%s)", s.nodeId, s.name.c_str());
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(s.nodeId)) {
		node->setAllKeyFrames(*s.keyFrames.value(), _sceneGraph.activeAnimation());
		return true;
	}
	return false;
}

bool SceneManager::mementoPaletteChange(const MementoState& s) {
	Log::debug("Memento: palette change of node %i to %s", s.nodeId, s.name.c_str());
	if (scenegraph::SceneGraphNode* node = sceneGraphNode(s.nodeId)) {
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
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(s.nodeId)) {
		if (node->region() != s.dataRegion()) {
			voxel::RawVolume *v = new voxel::RawVolume(s.dataRegion());
			if (!setSceneGraphNodeVolume(*node, v)) {
				delete v;
			}
		} else {
			node->setPivot(s.dataRegion().pivot());
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
	scenegraph::SceneGraphNodeType type = s.nodeType;
	if (type == scenegraph::SceneGraphNodeType::Max) {
		if (!s.hasVolumeData()) {
			type = scenegraph::SceneGraphNodeType::Group;
		} else {
			type = scenegraph::SceneGraphNodeType::Model;
		}
	}
	scenegraph::SceneGraphNode newNode(type);
	if (type == scenegraph::SceneGraphNodeType::Model) {
		newNode.setVolume(new voxel::RawVolume(s.dataRegion()), true);
		MementoData::toVolume(newNode.volume(), s.data);
		if (s.palette.hasValue()) {
			newNode.setPalette(*s.palette.value());
		}
	}
	if (type == scenegraph::SceneGraphNodeType::ModelReference) {
		newNode.setReference(s.referenceId);
	}
	if (s.keyFrames.hasValue()) {
		newNode.setAllKeyFrames(*s.keyFrames.value(), _sceneGraph.activeAnimation());
	}
	if (s.properties.hasValue()) {
		newNode.properties().clear();
		newNode.addProperties(*s.properties.value());
	}
	if (s.data.region().isValid()) {
		newNode.setPivot(s.region.pivot());
	}
	newNode.setName(s.name);
	const int newNodeId = addNodeToSceneGraph(newNode, s.parentId);
	_mementoHandler.updateNodeId(s.nodeId, newNodeId);
	return newNodeId != InvalidNodeId;
}

bool SceneManager::mementoStateExecute(const MementoState &s, bool isRedo) {
	core_assert(s.valid());
	ScopedMementoHandlerLock lock(_mementoHandler);
	if (s.type == MementoType::SceneNodeRenamed) {
		return mementoRename(s);
	}
	if (s.type == MementoType::SceneNodeKeyFrames) {
		return mementoKeyFrames(s);
	}
	if (s.type == MementoType::SceneNodeProperties) {
		return mementoProperties(s);
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
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(s.nodeId)) {
			node->setPivot(s.pivot);
			scenegraph::SceneGraphTransform &transform = node->keyFrame(s.keyFrameIdx).transform();
			transform.setWorldMatrix(s.worldMatrix);
			transform.update(_sceneGraph, *node, s.keyFrameIdx);
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

bool SceneManager::saveSelection(const io::FileDescription& file) {
	const Selections& selections = _modifier.selections();
	if (selections.empty()) {
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
	const io::FilePtr& filePtr = io::filesystem()->open(file.name, io::FileMode::SysWrite);
	if (!filePtr->validHandle()) {
		Log::warn("Failed to open the given file '%s' for writing", file.c_str());
		return false;
	}
	for (const Selection &selection : selections) {
		scenegraph::SceneGraph newSceneGraph;
		scenegraph::SceneGraphNode newNode;
		scenegraph::copyNode(*node, newNode, false);
		newNode.setVolume(new voxel::RawVolume(_sceneGraph.resolveVolume(*node), selection), true);
		newSceneGraph.emplace(core::move(newNode));
		voxelformat::SaveContext saveCtx;
		saveCtx.thumbnailCreator = voxelrender::volumeThumbnail;
		if (voxelformat::saveFormat(filePtr, &file.desc, newSceneGraph, saveCtx)) {
			Log::info("Saved node %i to %s", nodeId, filePtr->name().c_str());
		} else {
			Log::warn("Failed to save node %i to %s", nodeId, filePtr->name().c_str());
			return false;
		}
	}
	return true;
}

bool SceneManager::copy() {
	const Selections& selections = _modifier.selections();
	if (selections.empty()) {
		return false;
	}
	voxel::RawVolume* v = activeVolume();
	if (v == nullptr) {
		return false;
	}
	_copy = voxedit::tool::copy(v, selections);
	return true;
}

bool SceneManager::pasteAsNewNode() {
	if (_copy == nullptr) {
		Log::debug("Nothing copied yet - failed to paste");
		return false;
	}
	const int nodeId = activeNode();
	const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	scenegraph::copyNode(node, newNode, false);
	newNode.setVolume(new voxel::RawVolume(*_copy), true);
	return addNodeToSceneGraph(newNode, node.parent()) != InvalidNodeId;
}

bool SceneManager::paste(const glm::ivec3& pos) {
	if (_copy == nullptr) {
		Log::debug("Nothing copied yet - failed to paste");
		return false;
	}
	const int nodeId = activeNode();
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		return false;
	}
	voxel::Region modifiedRegion;
	voxedit::tool::paste(v, _copy, pos, modifiedRegion);
	if (!modifiedRegion.isValid()) {
		Log::debug("Failed to paste");
		return false;
	}
	const int64_t dismissMillis = core::Var::getSafe(cfg::VoxEditModificationDismissMillis)->intVal();
	modified(nodeId, modifiedRegion, true, dismissMillis);
	return true;
}

bool SceneManager::cut() {
	const Selections& selections = _modifier.selections();
	if (selections.empty()) {
		Log::debug("Nothing selected - failed to cut");
		return false;
	}
	const int nodeId = activeNode();
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		return false;
	}
	voxel::Region modifiedRegion;
	_copy = voxedit::tool::cut(v, selections, modifiedRegion);
	if (_copy == nullptr) {
		Log::debug("Failed to cut");
		return false;
	}
	if (!modifiedRegion.isValid()) {
		Log::debug("Failed to cut");
		delete _copy;
		return false;
	}
	const int64_t dismissMillis = core::Var::getSafe(cfg::VoxEditModificationDismissMillis)->intVal();
	modified(nodeId, modifiedRegion, true, dismissMillis);
	return true;
}

void SceneManager::resetLastTrace() {
	_sceneModeNodeIdTrace = InvalidNodeId;
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

int SceneManager::mergeNodes(const core::DynamicArray<int>& nodeIds) {
	scenegraph::SceneGraph newSceneGraph;
	for (int nodeId : nodeIds) {
		scenegraph::SceneGraphNode copiedNode;
		const scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
		if (node == nullptr || node->type() != scenegraph::SceneGraphNodeType::Model) {
			continue;
		}
		scenegraph::copyNode(*node, copiedNode, true);
		newSceneGraph.emplace(core::move(copiedNode));
	}
	bool applyTransformPosition = true;
	scenegraph::SceneGraph::MergedVolumePalette merged = newSceneGraph.merge(applyTransformPosition);
	if (merged.first == nullptr) {
		return InvalidNodeId;
	}

	scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
	int parent = 0;
	if (scenegraph::SceneGraphNode* firstNode = sceneGraphNode(nodeIds.front())) {
		scenegraph::copyNode(*firstNode, newNode, false);
	}
	if (applyTransformPosition) {
		scenegraph::SceneGraphTransform &transform = newNode.keyFrame(0).transform();
		transform.setWorldTranslation(glm::vec3(0.0f));
	}
	newNode.setVolume(merged.first, true);
	newNode.setPalette(merged.second);

	int newNodeId = addNodeToSceneGraph(newNode, parent);
	if (newNodeId == InvalidNodeId) {
		return newNodeId;
	}
	for (int nodeId : nodeIds) {
		nodeRemove(nodeId, false);
	}
	return newNodeId;
}

int SceneManager::mergeNodes(NodeMergeFlags flags) {
	core::DynamicArray<int> nodeIds;
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
	core::DynamicArray<int> nodeIds(2);
	nodeIds[0] = nodeId1;
	nodeIds[1] = nodeId2;
	return mergeNodes(nodeIds);
}

void SceneManager::resetSceneState() {
	// this also resets the cursor voxel - but nodeActive() will set it to the first usable index
	// that's why this call must happen before the nodeActive() call.
	_modifier.reset();
	scenegraph::SceneGraphNode &node = *_sceneGraph.beginModel();
	nodeActivate(node.id());
	_mementoHandler.clearStates();
	Log::debug("New volume for node %i", node.id());
	// TODO: what about the memento states of the other nodes
	_mementoHandler.markInitialNodeState(node);
	_dirty = false;
	_result = voxelutil::PickResult();
	_modifier.setCursorVoxel(voxel::createVoxel(node.palette(), 0));
	setCursorPosition(cursorPosition(), true);
	setReferencePosition(node.region().getCenter());
	resetLastTrace();
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
		Log::debug("Adding node %i with name %s", newNodeId, name.c_str());

		_mementoHandler.markNodeAdded(*node);

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

int SceneManager::addNodeToSceneGraph(scenegraph::SceneGraphNode &node, int parent) {
	const int newNodeId = scenegraph::addNodeToSceneGraph(_sceneGraph, node, parent, false);
	onNewNodeAdded(newNodeId, false);
	return newNodeId;
}

bool SceneManager::loadSceneGraph(scenegraph::SceneGraph&& sceneGraph) {
	core_trace_scoped(LoadSceneGraph);
	_sceneGraph = core::move(sceneGraph);
	_sceneRenderer->clear();

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

bool SceneManager::splitVolumes() {
	scenegraph::SceneGraph newSceneGraph;
	if (scenegraph::splitVolumes(_sceneGraph, newSceneGraph, false, false)) {
		return loadSceneGraph(core::move(newSceneGraph));
	} else if (scenegraph::splitVolumes(_sceneGraph, newSceneGraph, false, true)) {
		return loadSceneGraph(core::move(newSceneGraph));
	}
	return false;
}

void SceneManager::updateGridRenderer(const voxel::Region& region) {
	_sceneRenderer->updateGridRegion(region);
}

scenegraph::SceneGraphNode *SceneManager::sceneGraphNode(int nodeId) {
	if (_sceneGraph.hasNode(nodeId)) {
		return &_sceneGraph.node(nodeId);
	}
	return nullptr;
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
		// TODO: memento
		//_mementoHandler.markAddedAnimation(animation);
		return true;
	}
	return false;
}

bool SceneManager::duplicateAnimation(const core::String &animation, const core::String &newName) {
	if (_sceneGraph.duplicateAnimation(animation, newName)) {
		// TODO: memento
		//_mementoHandler.markAddedAnimation(animation);
		return true;
	}
	return false;
}

bool SceneManager::removeAnimation(const core::String &animation) {
	if (_sceneGraph.removeAnimation(animation)) {
		// TODO: memento
		//_mementoHandler.markRemovedAnimation(animation);
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
	_sceneRenderer->nodeRemove(node.id());

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
	_sceneRenderer->clear();

	voxel::RawVolume* v = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode newNode;
	newNode.setVolume(v, true);
	if (name.empty()) {
		newNode.setName("unnamed");
	} else {
		newNode.setName(name);
	}
	const int nodeId = scenegraph::addNodeToSceneGraph(_sceneGraph, newNode, 0);
	if (nodeId == InvalidNodeId) {
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

void SceneManager::rotate(math::Axis axis) {
	_sceneGraph.foreachGroup([&](int nodeId) {
		scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId);
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
		voxel::Region r = newVolume->region();
		r.accumulate(v->region());
		setSceneGraphNodeVolume(*node, newVolume);
		modified(nodeId, r);
	});
}

void SceneManager::move(int nodeId, const glm::ivec3& m) {
	const voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		return;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(v->region());
	voxel::RawVolumeMoveWrapper wrapper(newVolume);
	voxelutil::moveVolume(&wrapper, v, m);
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
	modified(nodeId, region);
}

void SceneManager::shift(int x, int y, int z) {
	const glm::ivec3 v(x, y, z);
	_sceneGraph.foreachGroup([&] (int nodeId) {
		shift(nodeId, v);
	});
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
		_sceneRenderer->renderScene(renderContext, camera, _sceneGraph, _currentFrameIdx);
	}
	const bool renderUI = (renderMask & RenderUI) != 0u;
	if (renderUI) {
		_sceneRenderer->renderUI(renderContext, camera, _sceneGraph);
		if (!renderContext.sceneMode) {
			_modifier.render(camera, activePalette());
		}
	}
}

void SceneManager::construct() {
	_modifier.construct();
	_mementoHandler.construct();
	_sceneRenderer->construct();
	_movement.construct();

	_autoSaveSecondsDelay = core::Var::get(cfg::VoxEditAutoSaveSeconds, "180", -1, "Delay in second between autosaves - 0 disables autosaves");
	_movementSpeed = core::Var::get(cfg::VoxEditMovementSpeed, "180.0f");

	command::Command::registerCommand("xs", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::error("Usage: xs <lua-generator-script-filename> [help]");
			return;
		}
		const core::String luaCode = _modifier.scriptBrush().luaGenerator().load(args[0]);
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
	}).setHelp("Executes a lua script")
		.setArgumentCompleter(voxelgenerator::scriptCompleter(io::filesystem()));

	for (int i = 0; i < lengthof(DIRECTIONS); ++i) {
		command::Command::registerActionButton(
				core::string::format("movecursor%s", DIRECTIONS[i].postfix),
				_move[i], "Move the cursor by keys, not but viewport mouse trace");
	}
	command::Command::registerCommand("palette_changeintensity", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: palette_changeintensity [value]");
			return;
		}
		const float scale = core::string::toFloat(args[0]);
		const int nodeId = activeNode();
		scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
		voxel::Palette &pal = node.palette();
		pal.changeIntensity(scale);
		_mementoHandler.markPaletteChange(node);
	}).setHelp("Change intensity by scaling the rgb values of the palette");

	command::Command::registerCommand("palette_removeunused", [&] (const command::CmdArgs& args) {
		const int nodeId = activeNode();
		removeUnusedColors(nodeId);
	}).setHelp("Remove unused colors from palette");

	command::Command::registerCommand("palette_sort", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: palette_sort [hue|saturation|brightness|cielab]");
			return;
		}
		const core::String &type = args[0];
		const int nodeId = activeNode();
		scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
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
	}).setHelp("Change intensity by scaling the rgb values of the palette").
		setArgumentCompleter(command::valueCompleter({"hue", "saturation", "brightness", "cielab"}));

	command::Command::registerActionButton("zoom_in", _zoomIn, "Zoom in");
	command::Command::registerActionButton("zoom_out", _zoomOut, "Zoom out");
	command::Command::registerActionButton("camera_rotate", _rotate, "Rotate the camera");
	command::Command::registerActionButton("camera_pan", _pan, "Pan the camera");
	command::Command::registerCommand("mouse_node_select", [&] (const command::CmdArgs&) {
		if (_sceneModeNodeIdTrace != InvalidNodeId) {
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
			if (const scenegraph::SceneGraphNode *node = sceneGraphNode(activeNode())) {
				const voxel::Region &region = node->region();
				if (region.isValid()) {
					_modifier.select(region.getLowerCorner(), region.getUpperCorner());
				}
			}
		} else if (args[0] == "invert") {
			if (const scenegraph::SceneGraphNode *node = sceneGraphNode(activeNode())) {
				_modifier.invert(node->region());
			}
		}
	}).setHelp("Select all nothing or invert").setArgumentCompleter(command::valueCompleter({"all", "none", "invert"}));

	command::Command::registerCommand("text", [this] (const command::CmdArgs& args) {
		if (args.size() != 2) {
			Log::info("Usage: text <string> <size>");
			return;
		}
		const core::String &str = args[0];
		const int size = args[1].toInt();
		renderText(str.c_str(), size);
	}).setHelp("Render characters at the reference position");

	command::Command::registerCommand("modelssave", [&] (const command::CmdArgs& args) {
		core::String dir = ".";
		if (!args.empty()) {
			dir = args[0];
		}
		if (!saveModels(dir)) {
			Log::error("Failed to save models to dir: %s", dir.c_str());
		}
	}).setHelp("Save all model nodes into filenames represented by their node names");

	command::Command::registerCommand("modelsave", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		if (argc < 1) {
			Log::info("Usage: modelsave <nodeid> [<file>]");
			return;
		}
		const int nodeId = core::string::toInt(args[0]);
		core::String file = core::string::format("node%i.vengi", nodeId);
		if (args.size() == 2) {
			file = args[1];
		}
		if (!saveNode(nodeId, file)) {
			Log::error("Failed to save node %i to file: %s", nodeId, file.c_str());
		}
	}).setHelp("Save a single node to the given path with their node names").setArgumentCompleter(nodeCompleter(_sceneGraph));

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

	command::Command::registerCommand("scaledown", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		int nodeId = activeNode();
		if (argc == 1) {
			nodeId = core::string::toInt(args[0]);
		}
		scaleDown(nodeId);
	}).setHelp("Scale the current active node or the given node down").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("scaleup", [&] (const command::CmdArgs& args) {
		const int argc = (int)args.size();
		int nodeId = activeNode();
		if (argc == 1) {
			nodeId = core::string::toInt(args[0]);
		}
		scaleUp(nodeId);
	}).setHelp("Scale the current active node or the given node up").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("colortomodel", [&] (const command::CmdArgs& args) {
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
		_modifier.stop();
	}).setHelp("Aborts the current modifier action");

	command::Command::registerCommand("fillhollow", [&] (const command::CmdArgs& args) {
		fillHollow();
	}).setHelp("Fill the inner parts of closed models");

	command::Command::registerCommand("hollow", [&] (const command::CmdArgs& args) {
		hollow();
	}).setHelp("Remove non visible voxels");

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
		bool searchBestColors = false;
		loadPalette(args[0], searchBestColors, true);
	}).setHelp("Load a palette by name. E.g. 'built-in:nippon' or 'lospec:id'").setArgumentCompleter(paletteCompleter());

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

	command::Command::registerCommand("modelsize", [this] (const command::CmdArgs& args) {
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
	}).setHelp("Resize your current model node about given x, y and z size");

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
		_sceneGraph.foreachGroup([&](int nodeId) {
			const voxel::RawVolume *v = volume(nodeId);
			if (v == nullptr) {
				return;
			}
			const voxel::Region& region = v->region();
			const glm::ivec3& center = region.getCenter();
			const glm::ivec3& delta = refPos - center;
			shift(nodeId, delta);
		});
	}).setHelp("Center the current active nodes at the reference position");

	command::Command::registerCommand("center_origin", [&](const command::CmdArgs &args) {
		_sceneGraph.foreachGroup([&](int nodeId) {
			const voxel::RawVolume *v = volume(nodeId);
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
	}).setHelp("Move the voxels inside the volume by the given values without changing the volume bounds");

	command::Command::registerCommand("copy", [&] (const command::CmdArgs& args) {
		copy();
	}).setHelp("Copy selection");

	command::Command::registerCommand("paste", [&] (const command::CmdArgs& args) {
		const Selections& selections = _modifier.selections();
		if (!selections.empty()) {
			voxel::Region r = selections[0];
			for (const Selection &region : selections) {
				r.accumulate(region);
			}
			paste(r.getLowerCorner());
		} else {
			paste(referencePosition());
		}
	}).setHelp("Paste clipboard to current selection or reference position");

	command::Command::registerCommand("pastecursor", [&] (const command::CmdArgs& args) {
		paste(_modifier.cursorPosition());
	}).setHelp("Paste clipboard to current cursor position");

	command::Command::registerCommand("pastenewnode", [&] (const command::CmdArgs& args) {
		pasteAsNewNode();
	}).setHelp("Paste clipboard as a new node");

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
		if (args.size() < 1) {
			Log::info("Usage: rotate <x|y|z>");
			return;
		}
		const math::Axis axis = math::toAxis(args[0]);
		rotate(axis);
	}).setHelp("Rotate active nodes around the given axis");

	command::Command::registerCommand("modelmerge", [&] (const command::CmdArgs& args) {
		int nodeId1;
		int nodeId2;
		if (args.size() == 1) {
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
	}).setHelp("Merge two given nodes or active model node with the next one").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modelmergeall", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::All);
	}).setHelp("Merge all nodes");

	command::Command::registerCommand("modelsmergevisible", [&] (const command::CmdArgs& args) {
		mergeNodes(NodeMergeFlags::Visible);
	}).setHelp("Merge all visible nodes");

	command::Command::registerCommand("modelsmergelocked", [&] (const command::CmdArgs& args) {
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
	}).setHelp("Flip the selected nodes around the given axis").setArgumentCompleter(command::valueCompleter({"x", "y", "z"}));

	command::Command::registerCommand("lock", [&] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: lock <x|y|z>");
			return;
		}
		const math::Axis axis = math::toAxis(args[0]);
		const bool unlock = (_lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp("Toggle locked mode for the given axis at the current cursor position").setArgumentCompleter(command::valueCompleter({"x", "y", "z"}));

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

	command::Command::registerCommand("modeladd", [&] (const command::CmdArgs& args) {
		const char *name = args.size() > 0 ? args[0].c_str() : "";
		const char *width = args.size() > 1 ? args[1].c_str() : "64";
		const char *height = args.size() > 2 ? args[2].c_str() : width;
		const char *depth = args.size() > 3 ? args[3].c_str() : height;
		const int iw = core::string::toInt(width);
		const int ih = core::string::toInt(height);
		const int id = core::string::toInt(depth);
		addModelChild(name, iw, ih, id);
	}).setHelp("Add a new model node (with a given name and width, height, depth - all optional)");

	command::Command::registerCommand("nodedelete", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			nodeRemove(*node, false);
		}
	}).setHelp("Delete a particular node by id - or the current active one").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodelock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(true);
		}
	}).setHelp("Lock a particular node by id - or the current active one").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodetogglelock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(!node->locked());
		}
	}).setHelp("Toggle the lock state of a particular node by id - or the current active one").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodeunlock", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode* node = sceneGraphNode(nodeId)) {
			node->setLocked(false);
		}
	}).setHelp("Unlock a particular node by id - or the current active one").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodeactivate", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Active node: %i", activeNode());
			return;
		}
		const int nodeId = core::string::toInt(args[0]);
		nodeActivate(nodeId);
	}).setHelp("Set or print the current active node").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodetogglevisible", [&](const command::CmdArgs &args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			node->setVisible(!node->visible());
		}
	}).setHelp("Toggle the visible state of a node").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("showall", [&] (const command::CmdArgs& args) {
		for (auto iter = _sceneGraph.beginAll(); iter != _sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			node.setVisible(true);
		}
	}).setHelp("Show all nodes");

	command::Command::registerCommand("hideall", [&](const command::CmdArgs &args) {
		for (auto iter = _sceneGraph.beginAll(); iter != _sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			node.setVisible(false);
		}
	}).setHelp("Hide all nodes");

	command::Command::registerCommand("nodeshowallchildren", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		_sceneGraph.visitChildren(nodeId, true, [] (scenegraph::SceneGraphNode &node) {
			node.setVisible(true);
		});
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			node->setVisible(true);
		}
	}).setHelp("Show all children nodes");

	command::Command::registerCommand("nodehideallchildren", [&](const command::CmdArgs &args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		_sceneGraph.visitChildren(nodeId, true, [] (scenegraph::SceneGraphNode &node) {
			node.setVisible(false);
		});
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			node->setVisible(false);
		}
	}).setHelp("Hide all children nodes");

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
	}).setHelp("Hide all model nodes except the active one").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modellockall", [&](const command::CmdArgs &args) {
		for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			node.setLocked(true);
		}
	}).setHelp("Lock all nodes");

	command::Command::registerCommand("modelunlockall", [&] (const command::CmdArgs& args) {
		for (auto iter = _sceneGraph.beginModel(); iter != _sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			node.setLocked(false);
		}
	}).setHelp("Unlock all nodes");

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
	}).setHelp("Rename the current node or the given node id").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("nodeduplicate", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			nodeDuplicate(*node);
		}
	}).setHelp("Duplicates the current node or the given node id").setArgumentCompleter(nodeCompleter(_sceneGraph));

	command::Command::registerCommand("modelref", [&] (const command::CmdArgs& args) {
		const int nodeId = args.size() > 0 ? core::string::toInt(args[0]) : activeNode();
		if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
			nodeReference(*node);
		}
	}).setHelp("Create a node reference for the given node id").setArgumentCompleter(nodeCompleter(_sceneGraph));
}

void SceneManager::removeUnusedColors(int nodeId) {
	scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	const voxel::RawVolume *v = node.volume();
	if (v == nullptr) {
		return;
	}
	core::Array<bool, voxel::PaletteMaxColors> usedColors;
	usedColors.fill(false);

	voxel::Palette &pal = node.palette();
	voxelutil::visitVolume(*v, [&usedColors] (int x, int y, int z, const voxel::Voxel& voxel) {
		usedColors[voxel.getColor()] = true;
		return true;
	});
	int unused = 0;
	for (size_t i = 0; i < usedColors.size(); ++i) {
		if (!usedColors[i]) {
			++unused;
		}
	}
	if (unused >= voxel::PaletteMaxColors) {
		Log::warn("Removing all colors from the palette is not allowed");
		return;
	}
	for (size_t i = 0; i < usedColors.size(); ++i) {
		if (!usedColors[i]) {
			pal.color(i) = core::RGBA(0);
		}
	}
	pal.markDirty();
	pal.markSave();
	_mementoHandler.markPaletteChange(node);
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
	voxel::RawVolumeWrapper wrapper = _modifier.createRawVolumeWrapper(v);
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
		return InvalidNodeId;
	}
	scenegraph::SceneGraphNode newNode;
	newNode.setVolume(new voxel::RawVolume(region), true);
	newNode.setName(name);
	const int parentId = activeNode();
	const int nodeId = addNodeToSceneGraph(newNode, parentId);
	return nodeId;
}

void SceneManager::flip(math::Axis axis) {
	_sceneGraph.foreachGroup([&](int nodeId) {
		voxel::RawVolume *v = volume(nodeId);
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
	if (!_sceneRenderer->init()) {
		Log::error("Failed to initialize the scene renderer");
		return false;
	}
	if (!_modifier.init()) {
		Log::error("Failed to initialize the modifier");
		return false;
	}
	if (!_movement.init()) {
		Log::error("Failed to initialize the movement controller");
		return false;
	}

	_gridSize = core::Var::getSafe(cfg::VoxEditGridsize);
	_hideInactive = core::Var::getSafe(cfg::VoxEditHideInactive);
	const core::TimeProviderPtr& timeProvider = app::App::getInstance()->timeProvider();
	_lastAutoSave = timeProvider->tickSeconds();

	_lockedAxis = math::Axis::None;
	return true;
}

bool SceneManager::runScript(const core::String& luaCode, const core::DynamicArray<core::String>& args) {
	const int nodeId = activeNode();
	voxel::RawVolume* v = volume(nodeId);
	if (v == nullptr) {
		return false;
	}

	_modifier.setBrushType(BrushType::Script);
	_modifier.scriptBrush().setScriptCode(luaCode, args);

	_modifier.start();
	auto callback = [nodeId, this](const voxel::Region &region, ModifierType type, bool markUndo) {
		if (_sceneGraph.hasNode(nodeId)) {
			modified(nodeId, region, markUndo);
		}
	};
	const bool state = _modifier.execute(_sceneGraph, v, callback);
	_modifier.stop();
	if (!state) {
		Log::warn("Failed to execute script");
	}
	return state;
}

bool SceneManager::animateActive() const {
	return _animationSpeed > 0.0;
}

void SceneManager::animate(double nowSeconds) {
	if (!animateActive()) {
		return;
	}
	if (_nextFrameSwitch > nowSeconds) {
		return;
	}
	_nextFrameSwitch = nowSeconds + _animationSpeed;

	if (_currentAnimationNodeId == InvalidNodeId) {
		_currentAnimationNodeId = (*_sceneGraph.beginModel()).id();
	}

	scenegraph::SceneGraphNode &prev = _sceneGraph.node(_currentAnimationNodeId);
	if (prev.isAnyModelNode()) {
		prev.setVisible(false);
	}

	_currentAnimationNodeId = _sceneGraph.nextModelNode(_currentAnimationNodeId);
	if (_currentAnimationNodeId == InvalidNodeId) {
		_currentAnimationNodeId = _sceneGraph.nextModelNode(_sceneGraph.root().id());
	}
	scenegraph::SceneGraphNode &node = _sceneGraph.node(_currentAnimationNodeId);
	if (node.isAnyModelNode()) {
		node.setVisible(true);
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
			_loadingFuture = std::future<scenegraph::SceneGraph>();
		}
	}

	_movement.update(nowSeconds);
	video::Camera *camera = activeCamera();
	if (camera != nullptr && camera->rotationType() == video::CameraRotationType::Eye) {
		const glm::vec3& moveDelta = _movement.moveDelta(_movementSpeed->floatVal());
		camera->move(moveDelta);
	}

	_modifier.update(nowSeconds);
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

	_sceneRenderer->shutdown();
	_sceneGraph.clear();
	_mementoHandler.clearStates();

	_movement.shutdown();
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
	voxel::RawVolume *v = volume(nodeId);
	if (v == nullptr) {
		return;
	}
	voxel::RawVolumeWrapper wrapper(v);
	voxelgenerator::lsystem::generate(wrapper, referencePosition(), axiom, rules, angle, length, width, widthIncrement, iterations, random, leavesRadius);
	modified(nodeId, wrapper.dirtyRegion());
}

void SceneManager::createTree(const voxelgenerator::TreeContext& ctx) {
	math::Random random(ctx.cfg.seed);
	const int nodeId = activeNode();
	voxel::RawVolume *v = volume(nodeId);
	if (v == nullptr) {
		return;
	}
	voxel::RawVolumeWrapper wrapper(v);
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
	_sceneRenderer->updateLockedPlanes(_lockedAxis, _sceneGraph, cursorPosition());
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
	if (_sceneModeNodeIdTrace != InvalidNodeId) {
		// if the trace is not forced, and the mouse cursor position did not change, don't
		// re-execute the trace.
		if (_lastRaytraceX == _mouseCursor.x && _lastRaytraceY == _mouseCursor.y && !force) {
			return;
		}
	}
	_sceneModeNodeIdTrace = InvalidNodeId;
	core_trace_scoped(EditorSceneOnProcessUpdateRay);
	_lastRaytraceX = _mouseCursor.x;
	_lastRaytraceY = _mouseCursor.y;
	float intersectDist = _camera->farPlane();
	const math::Ray& ray = _camera->mouseRay(_mouseCursor);
	const bool hideInactive = _hideInactive->boolVal();
	for (auto entry : _sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode& node = entry->second;
		if (!node.isAnyModelNode()) {
			continue;
		}
		if (!node.visible() || (hideInactive && node.id() != activeNode())) {
			continue;
		}
		float distance = 0.0f;
		const voxel::Region& region = _sceneGraph.resolveRegion(node);
		const glm::vec3 pivot = _sceneGraph.resolvePivot(node);
		const scenegraph::SceneGraphTransform &transform = node.transformForFrame(_currentFrameIdx);
		const math::OBB<float>& obb = toOBB(true, region, pivot, transform);
		if (obb.intersect(ray.origin, ray.direction, distance)) {
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
	if (_result.didHit && v != nullptr) {
		_modifier.setHitCursorVoxel(v->voxel(_result.hitVoxel));
	} else {
		_modifier.setHitCursorVoxel(voxel::Voxel());
	}
	if (v) {
		_modifier.setVoxelAtCursor(v->voxel(_modifier.cursorPosition()));
	}
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
	const video::Camera *camera = activeCamera();
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

	// TODO: we could optionally limit the raycast to the selection

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

	updateCursor();

	return true;
}

void SceneManager::setLockedAxis(math::Axis axis, bool unlock) {
	if (unlock) {
		_lockedAxis &= ~axis;
	} else {
		_lockedAxis |= axis;
	}
	_sceneRenderer->updateLockedPlanes(_lockedAxis, _sceneGraph, cursorPosition());
}

bool SceneManager::nodeUpdateTransform(int nodeId, const glm::mat4 &localMatrix, const glm::mat4 *deltaMatrix,
									   scenegraph::KeyFrameIndex keyFrameIdx) {
	if (nodeId == InvalidNodeId) {
		nodeForeachGroup([&] (int nodeId) {
			if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
				nodeUpdateTransform(*node, localMatrix, deltaMatrix, keyFrameIdx);
			}
		});
		return true;
	}
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeUpdateTransform(*node, localMatrix, deltaMatrix, keyFrameIdx);
	}
	return false;
}

bool SceneManager::nodeAddKeyframe(scenegraph::SceneGraphNode &node, scenegraph::FrameIndex frameIdx) {
	const scenegraph::KeyFrameIndex newKeyFrameIdx = node.addKeyFrame(frameIdx);
	if (newKeyFrameIdx == InvalidKeyFrame) {
		Log::warn("Failed to add keyframe for frame %i", (int)frameIdx);
		return false;
	}
	Log::error("node has %i keyframes", (int)node.keyFrames()->size());
	for (const auto& kf : *node.keyFrames()) {
		Log::error("- keyframe %i", (int)kf.frameIdx);
	}
	if (newKeyFrameIdx > 0) {
		node.keyFrame(newKeyFrameIdx).setTransform(node.keyFrame(newKeyFrameIdx - 1).transform());
		_mementoHandler.markKeyFramesChange(node);
		markDirty();
		return true;
	}
	return false;
}

bool SceneManager::nodeAddKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx) {
	if (nodeId == InvalidNodeId) {
		nodeForeachGroup([&](int nodeId) {
			scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
			nodeAddKeyframe(node, frameIdx);
		});
		return true;
	}
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		return nodeAddKeyframe(*node, frameIdx);
	}
	return false;
}

bool SceneManager::nodeRemoveKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx) {
	if (nodeId == InvalidNodeId) {
		nodeForeachGroup([&](int nodeId) {
			scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
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
		_mementoHandler.markKeyFramesChange(node);
		markDirty();
		return true;
	}
	return false;
}

bool SceneManager::nodeRemoveKeyFrameByIndex(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx) {
	if (node.removeKeyFrameByIndex(keyFrameIdx)) {
		_mementoHandler.markKeyFramesChange(node);
		markDirty();
		return true;
	}
	return false;
}

bool SceneManager::nodeUpdateTransform(scenegraph::SceneGraphNode &node, const glm::mat4 &localMatrix,
									   const glm::mat4 *deltaMatrix, scenegraph::KeyFrameIndex keyFrameIdx) {
	glm::vec3 translation;
	glm::quat orientation;
	glm::vec3 scale;
	glm::vec3 skew;
	glm::vec4 perspective;
	scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
	scenegraph::SceneGraphTransform &transform = keyFrame.transform();
	glm::decompose(localMatrix, scale, orientation, translation, skew, perspective);
	transform.setLocalTranslation(translation);
	transform.setLocalOrientation(orientation);
	transform.setLocalScale(scale);
	transform.update(_sceneGraph, node, keyFrame.frameIdx);

	_mementoHandler.markNodeTransform(node, keyFrameIdx);
	markDirty();

	return true;
}

int SceneManager::nodeReference(int nodeId) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		if (node->isReference()) {
			return nodeReference(node->reference());
		}
		return nodeReference(*node);
	}
	return InvalidNodeId;
}

bool SceneManager::nodeMove(int sourceNodeId, int targetNodeId) {
	if (_sceneGraph.changeParent(sourceNodeId, targetNodeId)) {
		core_assert(sceneGraphNode(sourceNodeId) != nullptr);
		_mementoHandler.markNodeMoved(targetNodeId, sourceNodeId);
		markDirty();
		return true;
	}
	return false;
}

bool SceneManager::nodeSetProperty(int nodeId, const core::String &key, const core::String &value) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		if (node->setProperty(key, value)) {
			_mementoHandler.markNodePropertyChange(*node);
			return true;
		}
	}
	return false;
}

bool SceneManager::nodeRemoveProperty(int nodeId, const core::String &key) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		if (node->properties().remove(key)) {
			_mementoHandler.markNodePropertyChange(*node);
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

bool SceneManager::nodeRename(scenegraph::SceneGraphNode &node, const core::String &name) {
	node.setName(name);
	_mementoHandler.markNodeRenamed(node);
	markDirty();
	return true;
}

bool SceneManager::nodeSetVisible(int nodeId, bool visible) {
	if (scenegraph::SceneGraphNode *node = sceneGraphNode(nodeId)) {
		node->setVisible(visible);
		if (node->type() == scenegraph::SceneGraphNodeType::Group) {
			_sceneGraph.visitChildren(nodeId, true, [visible] (scenegraph::SceneGraphNode &node) {
				node.setVisible(visible);
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

void SceneManager::markDirty() {
	_sceneGraph.markMaxFramesDirty();
	_needAutoSave = true;
	_dirty = true;
}

bool SceneManager::nodeRemove(scenegraph::SceneGraphNode &node, bool recursive) {
	const int nodeId = node.id();
	const core::String name = node.name();
	Log::debug("Delete node %i with name %s", nodeId, name.c_str());
	core::Buffer<int> removeReferenceNodes;
	for (auto iter = _sceneGraph.begin(scenegraph::SceneGraphNodeType::ModelReference); iter != _sceneGraph.end(); ++iter) {
		if ((*iter).reference() == nodeId) {
			removeReferenceNodes.push_back((*iter).id());
		}
	}
	for (int nodeId : removeReferenceNodes) {
		nodeRemove(_sceneGraph.node(nodeId), recursive);
	}
	// TODO: memento and recursive... - we only record the one node in the memento state - not the children
	_mementoHandler.markNodeRemoved(node);
	if (!_sceneGraph.removeNode(nodeId, recursive)) {
		Log::error("Failed to remove node with id %i", nodeId);
		_mementoHandler.removeLast();
		return false;
	}
	_sceneRenderer->nodeRemove(nodeId);
	if (_sceneGraph.empty()) {
		const voxel::Region region(glm::ivec3(0), glm::ivec3(31));
		scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
		newNode.setVolume(new voxel::RawVolume(region), true);
		if (name.empty()) {
			newNode.setName("unnamed");
		} else {
			newNode.setName(name);
		}
		addNodeToSceneGraph(newNode);
	} else {
		markDirty();
	}
	return true;
}

void SceneManager::nodeDuplicate(const scenegraph::SceneGraphNode &node) {
	const int newNodeId = scenegraph::addNodeToSceneGraph(_sceneGraph, node, node.parent(), true);
	onNewNodeAdded(newNodeId, false);
}

int SceneManager::nodeReference(const scenegraph::SceneGraphNode &node) {
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
	scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	if (node.type() == scenegraph::SceneGraphNodeType::Camera) {
		video::Camera *camera = activeCamera();
		if (camera == nullptr) {
			return false;
		}
		const scenegraph::SceneGraphNodeCamera& cameraNode = scenegraph::toCameraNode(node);
		video::Camera nodeCamera = voxelrender::toCamera(camera->size(), cameraNode);
		camera->lerp(nodeCamera);
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
		const glm::ivec3 pivot = region.getLowerCorner() + glm::ivec3(node.pivot() * glm::vec3(region.getDimensionsInVoxels()));
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
