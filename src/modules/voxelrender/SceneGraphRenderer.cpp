/**
 * @file
 */

#include "SceneGraphRenderer.h"
#include "ShaderAttribute.h"
#include "VoxelShaderConstants.h"
#include "core/Algorithm.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Set.h"
#include "video/Camera.h"
#include "video/Renderer.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "video/Shader.h"
#include "video/Types.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelutil/VolumeMerger.h"

namespace voxelrender {

void SceneGraphRenderer::construct() {
	_renderer.construct();
}

bool SceneGraphRenderer::init() {
	if (!_cameraRenderer.init(core::Color::White, 0)) {
		Log::warn("Failed to initialize camera renderer");
	}
	return _renderer.init();
}

void SceneGraphRenderer::update() {
	_renderer.update();
}

static inline int getVolumeId(const scenegraph::SceneGraphNode &node) {
	// TODO: using the node id here is not good as they are increasing when you modify the scene graph
	return node.id();
}

static inline int getNodeId(int volumeIdx) {
	// TODO: using the node id here is not good as they are increasing when you modify the scene graph
	return volumeIdx;
}

bool SceneGraphRenderer::empty(scenegraph::SceneGraphNode &node) {
	return _renderer.empty(getVolumeId(node));
}

bool SceneGraphRenderer::extractRegion(scenegraph::SceneGraphNode &node, const voxel::Region &region) {
	return _renderer.extractRegion(getVolumeId(node), region);
}

void SceneGraphRenderer::setAmbientColor(const glm::vec3 &color) {
	_renderer.setAmbientColor(color);
}

void SceneGraphRenderer::setDiffuseColor(const glm::vec3 &color) {
	_renderer.setDiffuseColor(color);
}

void SceneGraphRenderer::shutdown() {
	_renderer.shutdown();
	_cameraRenderer.shutdown();
}

void SceneGraphRenderer::clear() {
	_renderer.clearPendingExtractions();
	for (int i = 0; i < RawVolumeRenderer::MAX_VOLUMES; ++i) {
		const int nodeId = getNodeId(i);
		if (_renderer.setVolume(nodeId, nullptr, nullptr, true) != nullptr) {
			_renderer.updateBufferForVolume(nodeId);
		}
	}
}

scenegraph::SceneGraphNodeCamera toCameraNode(const video::Camera& camera) {
	scenegraph::SceneGraphNodeCamera node;
	scenegraph::SceneGraphTransform transform;
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	transform.setWorldMatrix(camera.viewMatrix());
	node.setAspectRatio(camera.aspect());
	node.setWidth(camera.size().x);
	node.setHeight(camera.size().y);
	node.setTransform(keyFrameIdx, transform);
	node.setFarPlane(camera.farPlane());
	node.setNearPlane(camera.nearPlane());
	if (camera.mode() == video::CameraMode::Orthogonal) {
		node.setOrthographic();
	} else {
		node.setPerspective();
	}
	node.setFieldOfView((int)camera.fieldOfView());
	node.setName("new camera");
	return node;
}

video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraphNodeCamera &cameraNode) {
	video::Camera camera;
	// width, heigth and aspect of the cameraNode are not taken into account here
	camera.setSize(glm::max(glm::ivec2(1, 1), size));
	if (cameraNode.isOrthographic()) {
		camera.setMode(video::CameraMode::Orthogonal);
	} else {
		camera.setMode(video::CameraMode::Perspective);
	}
	float fplane = cameraNode.farPlane();
	float nplane = cameraNode.nearPlane();
	if (fplane > nplane) {
		camera.setFarPlane(fplane);
		camera.setNearPlane(nplane);
	}
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = cameraNode.transform(keyFrameIdx);
	camera.setWorldPosition(transform.worldTranslation());
	camera.setOrientation(transform.worldOrientation());
	camera.setRotationType(video::CameraRotationType::Eye);
	const int fovDegree = cameraNode.fieldOfView();
	if (fovDegree > 0) {
		camera.setFieldOfView((float)fovDegree);
	}
	return camera;
}

void SceneGraphRenderer::prepare(const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frame, bool hideInactive,
								 bool grayInactive) {
	// remove those volumes that are no longer part of the scene graph
	for (int i = 0; i < RawVolumeRenderer::MAX_VOLUMES; ++i) {
		const int nodeId = getNodeId(i);
		if (!sceneGraph.hasNode(nodeId)) {
			if (_renderer.setVolume(nodeId, nullptr, nullptr, true) != nullptr) {
				Log::debug("%i is no longer part of the scene graph - remove from renderer", nodeId);
			}
		}
	}
	_cameras.clear();
	for (scenegraph::SceneGraph::iterator iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::Camera);
		 iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNodeCamera &cameraNode = scenegraph::toCameraNode(*iter);
		if (!cameraNode.visible()) {
			continue;
		}
		const glm::ivec2 size(cameraNode.width(), cameraNode.height());
		video::Camera camera = toCamera(size, cameraNode);
		_cameras.push_back(camera);
	}

	const int activeNode = sceneGraph.activeNode();
	for (scenegraph::SceneGraphNode &node : sceneGraph) {
		const int id = getVolumeId(node);
		if (id >= RawVolumeRenderer::MAX_VOLUMES) {
			continue;
		}
		voxel::RawVolume *v = _renderer.volume(id);
		_renderer.setVolume(id, node, true);
		if (v != node.volume()) {
			_renderer.extractRegion(id, node.region());
		}
		if (_sceneMode) {
			const scenegraph::SceneGraphTransform &transform = node.transformForFrame(frame);
			const glm::vec3 pivot = transform.worldScale() * transform.pivot() * glm::vec3(node.region().getDimensionsInVoxels());
			_renderer.setModelMatrix(id, transform.worldMatrix(), pivot);
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

void SceneGraphRenderer::render(RenderContext &renderContext, const video::Camera &camera, bool shadow, bool waitPending) {
	if (waitPending) {
		extractAll();
		_renderer.waitForPendingExtractions();
		_renderer.update();
	}

	_renderer.render(renderContext, camera, shadow);
	if (renderContext.sceneMode) {
		for (video::Camera &sceneCamera : _cameras) {
			sceneCamera.setSize(camera.size());
			sceneCamera.update(0.0);
			_cameraRenderer.render(camera, sceneCamera);
		}
	}
}

} // namespace voxelrender
