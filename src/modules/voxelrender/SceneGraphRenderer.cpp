/**
 * @file
 */

#include "SceneGraphRenderer.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Set.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelutil/VolumeMerger.h"
#include "voxel/MaterialColor.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedPolygonMode.h"
#include "ShaderAttribute.h"
#include "video/Camera.h"
#include "video/Types.h"
#include "video/Renderer.h"
#include "video/ScopedState.h"
#include "video/Shader.h"
#include "core/Color.h"
#include "core/ArrayLength.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/Algorithm.h"
#include "core/StandardLib.h"
#include "VoxelShaderConstants.h"
#include <SDL.h>

namespace voxelrender {

void SceneGraphRenderer::construct() {
	_renderer.construct();
}

bool SceneGraphRenderer::init() {
	return _renderer.init();
}

void SceneGraphRenderer::update() {
	_renderer.update();
}

bool SceneGraphRenderer::empty(voxel::SceneGraphNode &node) {
	return _renderer.empty(node.id());
}

bool SceneGraphRenderer::extractRegion(voxel::SceneGraphNode &node, const voxel::Region& region) {
	return _renderer.extractRegion(node.id(), region);
}

void SceneGraphRenderer::translate(voxel::SceneGraphNode &node, const glm::ivec3 &v) {
	_renderer.translate(node.id(), v);
}

bool SceneGraphRenderer::toMesh(voxel::SceneGraphNode &node, voxel::Mesh* mesh) {
	return _renderer.toMesh(node.id(), mesh);
}

void SceneGraphRenderer::setAmbientColor(const glm::vec3& color) {
	_renderer.setAmbientColor(color);
}

void SceneGraphRenderer::setDiffuseColor(const glm::vec3& color) {
	_renderer.setDiffuseColor(color);
}

void SceneGraphRenderer::shutdown() {
	_renderer.shutdown();
}

void SceneGraphRenderer::clear() {
	_renderer.clearPendingExtractions();
	for (int i = 0; i < RawVolumeRenderer::MAX_VOLUMES; ++i) {
		if (_renderer.setVolume(i, nullptr, true) != nullptr) {
			_renderer.update(i);
		}
	}
}

void SceneGraphRenderer::render(voxel::SceneGraph &sceneGraph, bool waitPending, const video::Camera& camera, bool shadow, std::function<bool(int)> funcGray) {
	int models = 0;

	// collect those volumes that are still in use
	core::Set<const voxel::RawVolume *> vPtrs;
	for (voxel::SceneGraphNode &node : sceneGraph) {
		voxel::RawVolume *v = _renderer.volume(node.id());
		if (v == nullptr) {
			continue;
		}
		vPtrs.insert(v);
	}

	// remove those volumes that are no longer part of the scene graph
	for (int i = 0; i < RawVolumeRenderer::MAX_VOLUMES; ++i) {
		voxel::RawVolume *v = _renderer.volume(i);
		if (!vPtrs.has(v)) {
			_renderer.setVolume(i, nullptr, true);
		}
	}

	for (voxel::SceneGraphNode &node : sceneGraph) {
		voxel::RawVolume *v = _renderer.volume(node.id());
		if (v != node.volume()) {
			_renderer.setVolume(node.id(), node.volume(), true);
			_renderer.extractRegion(node.id(), node.region());
		}
		if (0 && _renderScene) {
			_renderer.setInstancingAmount(node.id(), 0);
		} else {
			_renderer.setModelMatrix(node.id(), node.matrix(), true);
		}
		_renderer.hide(node.id(), !node.visible());
		++models;
	}

	if (0 && _renderScene) {
		for (auto iter = sceneGraph.begin(voxel::SceneGraphNodeType::ModelReference); iter != sceneGraph.end(); ++iter) {
			const int referencedModelNodeId = (*iter).referencedNodeId();
			core_assert(referencedModelNodeId != -1);
			core_assert_always(_renderer.setModelMatrix(referencedModelNodeId, (*iter).matrix(), false));
		}
	}

	if (waitPending) {
		_renderer.waitForPendingExtractions();
	}

	_renderer.render(camera, shadow, core::move(funcGray));
}

}
