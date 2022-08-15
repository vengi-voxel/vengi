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

bool SceneGraphRenderer::init(const glm::ivec2 &size) {
	return _renderer.init(size);
}

void SceneGraphRenderer::update() {
	_renderer.update();
}

static inline int volumeId(const voxelformat::SceneGraphNode &node) {
	// TODO: using the node id here is not good as they are increasing when you modify the scene graph
	return node.id();
}

static inline int nodeId(int volumeIdx) {
	// TODO: using the node id here is not good as they are increasing when you modify the scene graph
	return volumeIdx;
}

bool SceneGraphRenderer::empty(voxelformat::SceneGraphNode &node) {
	return _renderer.empty(volumeId(node));
}

bool SceneGraphRenderer::extractRegion(voxelformat::SceneGraphNode &node, const voxel::Region& region) {
	return _renderer.extractRegion(volumeId(node), region);
}

void SceneGraphRenderer::translate(voxelformat::SceneGraphNode &node, const glm::ivec3 &v) {
	_renderer.translate(volumeId(node), v);
}

bool SceneGraphRenderer::toMesh(voxelformat::SceneGraphNode &node, voxel::Mesh* mesh) {
	return _renderer.toMesh(volumeId(node), mesh);
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
		const int nId = nodeId(i);
		if (_renderer.setVolume(nId, nullptr, nullptr, true) != nullptr) {
			_renderer.updateBufferForVolume(nId);
		}
	}
}

void SceneGraphRenderer::prepare(voxelformat::SceneGraph &sceneGraph, uint32_t frame, bool hideInactive, bool grayInactive) {
	// remove those volumes that are no longer part of the scene graph
	for (int i = 0; i < RawVolumeRenderer::MAX_VOLUMES; ++i) {
		const int nId = nodeId(i);
		if (!sceneGraph.hasNode(nId)) {
			_renderer.setVolume(nId, nullptr, nullptr, true);
		}
	}

	const int activeNode = sceneGraph.activeNode();
	for (voxelformat::SceneGraphNode &node : sceneGraph) {
		const int id = volumeId(node);
		if (id >= RawVolumeRenderer::MAX_VOLUMES) {
			continue;
		}
		voxel::RawVolume *v = _renderer.volume(id);
		if (v != node.volume()) {
			_renderer.setVolume(id, node, true);
			_renderer.extractRegion(id, node.region());
		}
		if (_sceneMode) {
			// TODO ik solver
			const voxelformat::SceneGraphTransform &transform = node.transformForFrame(frame);
			const glm::vec3 pivot = transform.pivot() * glm::vec3(node.region().getDimensionsInVoxels());
			_renderer.setModelMatrix(id, transform.matrix(), pivot);
		} else {
			_renderer.setModelMatrix(id, glm::mat4(1.0f), glm::vec3(0.0f));
		}
		if (hideInactive) {
			_renderer.hide(id, id != activeNode);
		} else {
			_renderer.hide(id, !node.visible());
		}
		if (grayInactive) {
			_renderer.gray(id, id != activeNode);
		} else {
			_renderer.gray(id, false);
		}
	}
}

void SceneGraphRenderer::extractAll() {
	while (_renderer.scheduleExtractions(100)) {
	}
}

void SceneGraphRenderer::render(const video::Camera& camera, bool shadow, bool waitPending) {
	if (waitPending) {
		extractAll();
		_renderer.waitForPendingExtractions();
		_renderer.update();
	}

	_renderer.render(camera, shadow);
}

}
