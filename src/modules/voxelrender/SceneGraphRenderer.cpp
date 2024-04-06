/**
 * @file
 */

#include "SceneGraphRenderer.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/collection/DynamicArray.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "voxel/RawVolume.h"
#include "voxelrender/RawVolumeRenderer.h"

namespace voxelrender {

scenegraph::SceneGraphNodeCamera toCameraNode(const video::Camera &camera) {
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

void configureCamera(video::Camera &camera, const voxel::Region &sceneRegion, SceneCameraMode mode, float farPlane,
					 const glm::vec3 &angles) {
	const glm::vec3 size(sceneRegion.getDimensionsInVoxels());
	const float maxDim = (float)glm::max(size.x, glm::max(size.y, size.z));
	const float distance = maxDim * 2.0f;
	const glm::vec3 &center = sceneRegion.calcCenterf();

	camera.resetZoom();
	camera.setRotationType(video::CameraRotationType::Target);
	camera.setAngles(angles[0], angles[1], angles[2]);
	camera.setTarget(center);
	camera.setTargetDistance(distance);
	camera.setFarPlane(farPlane);
	if (mode == SceneCameraMode::Free) {
		camera.setWorldPosition(glm::vec3(center.x - distance, (float)sceneRegion.getUpperY(), center.z - distance));
	} else if (mode == SceneCameraMode::Top) {
		camera.setWorldPosition(glm::vec3(center.x, center.y + size.y, center.z));
	} else if (mode == SceneCameraMode::Bottom) {
		camera.setWorldPosition(glm::vec3(center.x, center.y - size.y, center.z));
	} else if (mode == SceneCameraMode::Left) {
		camera.setWorldPosition(glm::vec3(center.x + size.x, center.y, center.z));
	} else if (mode == SceneCameraMode::Right) {
		camera.setWorldPosition(glm::vec3(center.x - size.x, center.y, center.z));
	} else if (mode == SceneCameraMode::Front) {
		camera.setWorldPosition(glm::vec3(center.x, center.y, center.z + size.z));
	} else if (mode == SceneCameraMode::Back) {
		camera.setWorldPosition(glm::vec3(center.x, center.y, center.z - size.z));
	}
	camera.lookAt(center);
}

video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraphNodeCamera &cameraNode) {
	video::Camera camera;
	// width, height and aspect of the cameraNode are not taken into account here
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
	camera.setRotationType(video::CameraRotationType::Target);
	const int fovDegree = cameraNode.fieldOfView();
	if (fovDegree > 0) {
		camera.setFieldOfView((float)fovDegree);
	}
	camera.update(0.0);
	return camera;
}

static inline int getVolumeId(int nodeId) {
	// TODO: using the node id here is not good as they are increasing when you modify the scene graph
	return nodeId;
}

static inline int getVolumeId(const scenegraph::SceneGraphNode &node) {
	return getVolumeId(node.id());
}

static inline int getNodeId(int volumeIdx) {
	// TODO: using the node id here is not good as they are increasing when you modify the scene graph
	return volumeIdx;
}

SceneGraphRenderer::SceneGraphRenderer(const voxel::MeshStatePtr &meshState) : _volumeRenderer(meshState) {
}

SceneGraphRenderer::SceneGraphRenderer() : voxelrender::SceneGraphRenderer(core::make_shared<voxel::MeshState>()) {
}

void SceneGraphRenderer::construct() {
	_volumeRenderer.construct();
}

bool SceneGraphRenderer::init() {
	if (!_cameraRenderer.init(core::Color::White(), 0)) {
		Log::warn("Failed to initialize camera renderer");
	}
	return _volumeRenderer.init();
}

void SceneGraphRenderer::update() {
	_volumeRenderer.update();
}

void SceneGraphRenderer::scheduleRegionExtraction(scenegraph::SceneGraphNode &node, const voxel::Region &region) {
	_volumeRenderer.scheduleRegionExtraction(getVolumeId(node), region);
}

void SceneGraphRenderer::setAmbientColor(const glm::vec3 &color) {
	_volumeRenderer.setAmbientColor(color);
}

void SceneGraphRenderer::setDiffuseColor(const glm::vec3 &color) {
	_volumeRenderer.setDiffuseColor(color);
}

void SceneGraphRenderer::shutdown() {
	_volumeRenderer.shutdown();
	_cameraRenderer.shutdown();
}

void SceneGraphRenderer::clear() {
	_volumeRenderer.clear();
}

void SceneGraphRenderer::nodeRemove(int nodeId) {
	const int id = getVolumeId(nodeId);
	if (id < 0 || id >= voxel::MAX_VOLUMES) {
		return;
	}
	_volumeRenderer.resetVolume(id);
}

bool SceneGraphRenderer::isVisible(int nodeId) const {
	const int id = getVolumeId(nodeId);
	if (id < 0 || id >= voxel::MAX_VOLUMES) {
		return false;
	}
	return _volumeRenderer.isVisible(id);
}

void SceneGraphRenderer::prepare(const RenderContext &renderContext) {
	core_assert_always(renderContext.sceneGraph != nullptr);
	const scenegraph::SceneGraph &sceneGraph = *renderContext.sceneGraph;
	const scenegraph::FrameIndex frame = renderContext.frame;
	const bool hideInactive = renderContext.hideInactive;
	const bool grayInactive = renderContext.grayInactive;
	const bool sceneMode = renderContext.sceneMode;
	// remove those volumes that are no longer part of the scene graph
	for (int i = 0; i < voxel::MAX_VOLUMES; ++i) {
		const int nodeId = getNodeId(i);
		if (!sceneGraph.hasNode(nodeId)) {
			_volumeRenderer.resetVolume(nodeId);
		}
	}
	_cameras.clear();

	const voxel::MeshStatePtr &meshState = _volumeRenderer.meshState();

	const int activeNode = sceneGraph.activeNode();
	for (auto entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (renderContext.onlyModels && node.isModelNode()) {
			continue;
		}

		if (node.type() == scenegraph::SceneGraphNodeType::Camera) {
			const scenegraph::SceneGraphNodeCamera &cameraNode = scenegraph::toCameraNode(node);
			if (!cameraNode.visible()) {
				continue;
			}
			const glm::ivec2 size(cameraNode.width(), cameraNode.height());
			video::Camera camera = toCamera(size, cameraNode);
			_cameras.push_back(camera);
			continue;
		} else if (node.type() != scenegraph::SceneGraphNodeType::Model) {
			continue;
		}

		const int id = getVolumeId(node);
		if (id >= voxel::MAX_VOLUMES) {
			continue;
		}
		voxel::RawVolume *v = meshState->volume(id);
		_volumeRenderer.setVolume(id, node, true);
		const voxel::Region &region = node.region();
		if (v != node.volume()) {
			_volumeRenderer.scheduleRegionExtraction(id, region);
		}
		if (sceneMode) {
			const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frame);
			const int negative = (int)std::signbit(transform.scale.x) + (int)std::signbit(transform.scale.y) +
								 (int)std::signbit(transform.scale.z);
			if (negative == 1 || negative == 3) {
				meshState->setCullFace(id, video::Face::Front);
			} else {
				meshState->setCullFace(id, video::Face::Back);
			}
			const glm::mat4 worldMatrix = transform.worldMatrix();
			const glm::vec3 maxs = worldMatrix * glm::vec4(region.getUpperCorner(), 1.0f);
			const glm::vec3 mins = worldMatrix * glm::vec4(region.getLowerCorner(), 1.0f);
			const glm::vec3 pivot = transform.scale * node.pivot() * glm::vec3(region.getDimensionsInVoxels());
			meshState->setModelMatrix(id, worldMatrix, pivot, mins, maxs);
		} else {
			meshState->setCullFace(id, video::Face::Back);
			meshState->setModelMatrix(id, glm::mat4(1.0f), glm::vec3(0.0f), region.getLowerCorner(),
												  region.getUpperCorner());
		}
		if (hideInactive) {
			meshState->hide(id, id != activeNode);
		} else {
			meshState->hide(id, !node.visible());
		}
		if (grayInactive) {
			meshState->gray(id, id != activeNode);
		} else {
			meshState->gray(id, false);
		}
	}

	meshState->resetReferences();
	if (sceneMode) {
		for (auto entry : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = entry->second;
			if (!node.isReference()) {
				continue;
			}
			const int id = getVolumeId(node);
			if (id >= voxel::MAX_VOLUMES) {
				continue;
			}
			const int referencedId = getVolumeId(node.reference());
			meshState->setReference(id, referencedId);
			const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frame);
			const voxel::Region region = sceneGraph.resolveRegion(node);
			const glm::mat4 worldMatrix = transform.worldMatrix();
			const glm::vec3 maxs = worldMatrix * glm::vec4(region.getUpperCorner(), 1.0f);
			const glm::vec3 mins = worldMatrix * glm::vec4(region.getLowerCorner(), 1.0f);
			const glm::vec3 pivot =
				transform.scale * node.pivot() * glm::vec3(region.getDimensionsInVoxels());
			meshState->setModelMatrix(id, worldMatrix, pivot, mins, maxs);
			if (hideInactive) {
				meshState->hide(id, id != activeNode);
			} else {
				meshState->hide(id, !node.visible());
			}
			if (grayInactive) {
				meshState->gray(id, id != activeNode);
			} else {
				meshState->gray(id, false);
			}
		}
	}
}

void SceneGraphRenderer::render(RenderContext &renderContext, const video::Camera &camera, bool shadow,
								bool waitPending) {
	prepare(renderContext);
	if (waitPending) {
		_volumeRenderer.meshState()->extractAllPending();
		_volumeRenderer.update();
	}

	_volumeRenderer.render(renderContext, camera, shadow);
	if (renderContext.sceneMode) {
		for (video::Camera &sceneCamera : _cameras) {
			sceneCamera.setSize(camera.size());
			sceneCamera.update(0.0);
			_cameraRenderer.render(camera, sceneCamera);
		}
	}
}

} // namespace voxelrender
