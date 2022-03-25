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

bool SceneGraphRenderer::empty(voxelformat::SceneGraphNode &node) {
	return _renderer.empty(node.id());
}

bool SceneGraphRenderer::extractRegion(voxelformat::SceneGraphNode &node, const voxel::Region& region) {
	return _renderer.extractRegion(node.id(), region);
}

void SceneGraphRenderer::translate(voxelformat::SceneGraphNode &node, const glm::ivec3 &v) {
	_renderer.translate(node.id(), v);
}

bool SceneGraphRenderer::toMesh(voxelformat::SceneGraphNode &node, voxel::Mesh* mesh) {
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

void SceneGraphRenderer::prepare(voxelformat::SceneGraph &sceneGraph, int frame, bool hideInactive, bool grayInactive) {
	// remove those volumes that are no longer part of the scene graph
	for (int i = 0; i < RawVolumeRenderer::MAX_VOLUMES; ++i) {
		if (!sceneGraph.hasNode(i)) {
			_renderer.setVolume(i, nullptr, true);
		}
	}

	const int activeNode = sceneGraph.activeNode();
	for (voxelformat::SceneGraphNode &node : sceneGraph) {
		voxel::RawVolume *v = _renderer.volume(node.id());
		if (v != node.volume()) {
			_renderer.setVolume(node.id(), node.volume(), true);
			_renderer.extractRegion(node.id(), node.region());
		}
		if (_sceneMode) {
			// TODO ik solver
			const voxelformat::SceneGraphTransform &transform = node.transformForFrame(frame);
			const glm::vec3 pivot = transform.pivot() * glm::vec3(node.region().getDimensionsInVoxels());
			_renderer.setModelMatrix(node.id(), transform.matrix(), pivot);
		} else {
			_renderer.setModelMatrix(node.id(), glm::mat4(1.0f), glm::vec3(0.0f));
		}
		if (hideInactive) {
			_renderer.hide(node.id(), node.id() != activeNode);
		} else {
			_renderer.hide(node.id(), !node.visible());
		}
		if (grayInactive) {
			_renderer.gray(node.id(), node.id() != activeNode);
		} else {
			_renderer.gray(node.id(), false);
		}
	}
}

void SceneGraphRenderer::render(const video::Camera& camera, bool shadow, bool waitPending) {
	if (waitPending) {
		while (_renderer.scheduleExtractions(100)) {
		}
		_renderer.waitForPendingExtractions();
		_renderer.update();
	}

	_renderer.render(camera, shadow);
}

}
