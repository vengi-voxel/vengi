/**
 * @file
 */

#include "SceneGraphRenderer.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "voxel/RawVolume.h"
#include "voxelrender/RawVolumeRenderer.h"
#include <limits>

namespace voxelrender {

scenegraph::SceneGraphNodeCamera toCameraNode(const video::Camera &camera) {
	scenegraph::SceneGraphNodeCamera cameraNode;
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	scenegraph::SceneGraphTransform transform;
	transform.setWorldTranslation(camera.eye());
	transform.setWorldOrientation(camera.orientation());
	cameraNode.setTransform(keyFrameIdx, transform);

	cameraNode.setAspectRatio(camera.aspect());
	cameraNode.setWidth(camera.size().x);
	cameraNode.setHeight(camera.size().y);
	cameraNode.setFarPlane(camera.farPlane());
	cameraNode.setNearPlane(camera.nearPlane());
	if (camera.mode() == video::CameraMode::Orthogonal) {
		cameraNode.setOrthographic();
	} else {
		cameraNode.setPerspective();
	}
	cameraNode.setFieldOfView((int)camera.fieldOfView());
	cameraNode.setName("new camera");
	return cameraNode;
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
	if (mode != SceneCameraMode::Free) {
		camera.setOmega({0.0f, 0.0f, 0.0f});
	}
	if (mode == SceneCameraMode::Free) {
		camera.setWorldPosition(glm::vec3(center.x - distance, (float)sceneRegion.getUpperY(), center.z - distance));
	} else if (mode == SceneCameraMode::Top) {
		camera.setWorldPosition(glm::vec3(center.x, center.y + size.y, center.z));
	} else if (mode == SceneCameraMode::Bottom) {
		camera.setWorldPosition(glm::vec3(center.x, center.y - size.y, center.z));
	} else if (mode == SceneCameraMode::Right) {
		camera.setWorldPosition(glm::vec3(center.x + size.x, center.y, center.z));
	} else if (mode == SceneCameraMode::Left) {
		camera.setWorldPosition(glm::vec3(center.x - size.x, center.y, center.z));
	} else if (mode == SceneCameraMode::Back) {
		camera.setWorldPosition(glm::vec3(center.x, center.y, center.z + size.z));
	} else if (mode == SceneCameraMode::Front) {
		camera.setWorldPosition(glm::vec3(center.x, center.y, center.z - size.z));
	}
	camera.lookAt(center);
}

static video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraphNodeCamera &cameraNode,
							  const glm::vec3 &worldPos, const glm::quat &orientation) {
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
	camera.setWorldPosition(worldPos);
	camera.setOrientation(orientation);
	const int fovDegree = cameraNode.fieldOfView();
	if (fovDegree > 0) {
		camera.setFieldOfView((float)fovDegree);
	}
	camera.update(0.0);
	return camera;
}

video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNodeCamera &cameraNode, scenegraph::FrameIndex frameIdx) {
	const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(cameraNode, frameIdx);
	return toCamera(size, cameraNode, transform.worldTranslation(), glm::quat(transform.worldMatrix()));
}

video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraphNodeCamera &cameraNode) {
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = cameraNode.transform(keyFrameIdx);
	return toCamera(size, cameraNode, transform.worldTranslation(), transform.worldOrientation());
}

SceneGraphRenderer::SceneGraphRenderer() {
}

void SceneGraphRenderer::construct() {
	_volumeRenderer.construct();
}

bool SceneGraphRenderer::init(bool normals) {
	if (!_cameraRenderer.init(color::White(), 0)) {
		Log::warn("Failed to initialize camera renderer");
	}
	return _volumeRenderer.init(normals);
}

void SceneGraphRenderer::update(const voxel::MeshStatePtr &meshState) {
	core_trace_scoped(SceneGraphRendererUpdate);
	_volumeRenderer.update(meshState);
}

void SceneGraphRenderer::scheduleRegionExtraction(const voxel::MeshStatePtr &meshState, int nodeId, const voxel::Region &region) {
	const int idx = getVolumeIdx(nodeId);
	if (_sliceVolume && _sliceVolume.get() == meshState->volume(idx)) {
		_sliceVolumeDirty = true;
		return;
	}
	_volumeRenderer.scheduleRegionExtraction(meshState, idx, region);
}

void SceneGraphRenderer::setAmbientColor(const glm::vec3 &color) {
	_volumeRenderer.setAmbientColor(color);
}

void SceneGraphRenderer::setDiffuseColor(const glm::vec3 &color) {
	_volumeRenderer.setDiffuseColor(color);
}

void SceneGraphRenderer::setSunAngle(const glm::vec3 &angle) {
	_volumeRenderer.setSunAngle(angle);
}

void SceneGraphRenderer::shutdown() {
	_volumeRenderer.shutdown();
	_cameraRenderer.shutdown();
}

void SceneGraphRenderer::clear(const voxel::MeshStatePtr &meshState) {
	_volumeRenderer.clear(meshState);
	_sliceRegion = voxel::Region::InvalidRegion;
}

const voxel::Region &SceneGraphRenderer::sliceRegion() const {
	return _sliceRegion;
}

void SceneGraphRenderer::setSliceRegion(const voxel::Region &region) {
	_sliceRegion = region;
}

bool SceneGraphRenderer::isSliceModeActive() const {
	return _sliceRegion.isValid();
}

void SceneGraphRenderer::nodeRemove(const voxel::MeshStatePtr &meshState, int nodeId) {
	const int idx = getVolumeIdx(nodeId);
	if (idx < 0 || idx >= voxel::MAX_VOLUMES) {
		return;
	}
	// ignore the return value because the volume is owned by the node
	(void)_volumeRenderer.resetVolume(meshState, idx);
}

bool SceneGraphRenderer::isVisible(const voxel::MeshStatePtr &meshState, int nodeId, bool hideEmpty) const {
	const int idx = getVolumeIdx(nodeId);
	if (idx < 0 || idx >= voxel::MAX_VOLUMES) {
		return false;
	}
	return _volumeRenderer.isVisible(meshState, idx, hideEmpty);
}

/**
 * @sa scenegraph::SceneGraph::worldMatrix()
 */
void SceneGraphRenderer::prepareMeshStateTransform(const voxel::MeshStatePtr &meshState,
												   const scenegraph::SceneGraph &sceneGraph,
												   const scenegraph::FrameIndex &frame,
												   const scenegraph::SceneGraphNode &node, int idx,
												   const voxel::Region &region) const {
	core_trace_scoped(PrepareMeshStateTransform);
	const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frame);
	const glm::vec3 &scale = transform.worldScale();
	const int negative = (int)std::signbit(scale.x) + (int)std::signbit(scale.y) + (int)std::signbit(scale.z);
	if (negative == 1 || negative == 3) {
		meshState->setCullFace(idx, video::Face::Front);
	} else {
		meshState->setCullFace(idx, video::Face::Back);
	}
	const glm::vec3 &pivot = node.pivot();
	const glm::mat4 &worldMatrix = transform.calculateWorldMatrix(pivot, region.getDimensionsInVoxels());
	const glm::vec3 &mins = region.getLowerCornerf();
	const glm::vec3 &maxs = region.getUpperCornerf();
	const glm::vec3 corners[] = {mins,
								 glm::vec3(maxs.x, mins.y, mins.z),
								 glm::vec3(mins.x, maxs.y, mins.z),
								 glm::vec3(maxs.x, maxs.y, mins.z),
								 glm::vec3(mins.x, mins.y, maxs.z),
								 glm::vec3(maxs.x, mins.y, maxs.z),
								 glm::vec3(mins.x, maxs.y, maxs.z),
								 maxs};

	glm::vec3 transformedMins(std::numeric_limits<float>::max());
	glm::vec3 transformedMaxs(std::numeric_limits<float>::lowest());

	for (int i = 0; i < lengthof(corners); ++i) {
		const glm::vec3 transformed(worldMatrix * glm::vec4(corners[i], 1.0f));
		transformedMins = glm::min(transformedMins, transformed);
		transformedMaxs = glm::max(transformedMaxs, transformed);
	}
	meshState->setModelMatrix(idx, worldMatrix, transformedMins, transformedMaxs);
}

bool SceneGraphRenderer::handleSliceView(const voxel::MeshStatePtr &meshState, int activeNodeId,
										 scenegraph::SceneGraphNode &node, int idx,
										 const voxel::RawVolume *nodeVolume) {
	if (node.id() != activeNodeId) {
		return false;
	}
	if (!_sliceRegion.isValid()) {
		_sliceVolume = nullptr;
		_sliceVolumeDirty = false;
		_sliceVolumeNodeId = -1;
		return false;
	}

	// check several things to re-create the slice volume
	// * a new activated node
	// * the region changed
	// * we don't yet have a sliced volume view but requested one
	if (_sliceVolumeDirty || _sliceVolumeNodeId != activeNodeId || !_sliceVolume ||
		_sliceVolume->region() != _sliceRegion) {
		// this enforces the lock on the volume renderer if the volume is currently extracted
		core::SharedPtr<voxel::RawVolume> newVolume(core::make_shared<voxel::RawVolume>(nodeVolume, _sliceRegion));
		const voxel::RawVolume *oldV = _volumeRenderer.setVolume(meshState, idx, newVolume.get(), &node.palette(),
																 &node.normalPalette(), !_sliceVolumeDirty);
		if (_sliceVolumeDirty || oldV != nullptr) {
			_volumeRenderer.scheduleRegionExtraction(meshState, idx, newVolume->region());
		}
		_sliceVolume = newVolume;
		_sliceVolumeNodeId = activeNodeId;
	}
	_sliceVolumeDirty = false;
	return true;
}

void SceneGraphRenderer::prepare(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext) {
	core_trace_scoped(Prepare);
	core_assert_always(renderContext.sceneGraph != nullptr);
	const scenegraph::SceneGraph &sceneGraph = *renderContext.sceneGraph;
	const scenegraph::FrameIndex frame = renderContext.frame;
	const bool hideInactive = renderContext.hideInactive;
	const bool grayInactive = renderContext.grayInactive;
	// remove those volumes that are no longer part of the scene graph
	for (int i = 0; i < voxel::MAX_VOLUMES; ++i) {
		const int nodeId = getNodeId(i);
		if (!sceneGraph.hasNode(nodeId)) {
			// ignore the return value because the volume is owned by the node
			(void)_volumeRenderer.resetVolume(meshState, nodeId);
		}
	}
	_cameras.clear();

	const int activeNodeId = sceneGraph.activeNode();
	const scenegraph::SceneGraphNode &activeNode = sceneGraph.node(activeNodeId);
	// TODO: PERF: remove direct access to gpu buffers and use for_parallel here
	// TODO: for this we have to handle the slice region outside of this loop
	for (auto entry : sceneGraph.nodes()) {
		scenegraph::SceneGraphNode &node = entry->value;
		if (renderContext.onlyModels && !node.isModelNode()) {
			continue;
		}

		if (node.isCameraNode()) {
			const scenegraph::SceneGraphNodeCamera &cameraNode = scenegraph::toCameraNode(node);
			if (!cameraNode.visible()) {
				continue;
			}
			const glm::ivec2 size(cameraNode.width(), cameraNode.height());
			// TODO: we need the node id here, to colorize the camera frustum rendering according to the node color
			_cameras.emplace_back(toCamera(size, sceneGraph, cameraNode, frame));
			// TODO: see https://github.com/vengi-voxel/vengi/issues/254 - if the node is the current active node, we
			// should lerp to the camera
			continue;
		} else if (!node.isModelNode()) {
			continue;
		}

		const int idx = getVolumeIdx(node);
		if (idx >= voxel::MAX_VOLUMES) {
			continue;
		}
		const voxel::RawVolume *v = meshState->volume(idx);
		const bool sliceView = handleSliceView(meshState, activeNodeId, node, idx, v);

		voxel::Region region;
		if (sliceView) {
			region = _sliceVolume->region();
		} else {
			const voxel::RawVolume *nodeVolume = sceneGraph.resolveVolume(node);
			_volumeRenderer.setVolume(meshState, idx, node, true);
			region = node.region();
			if (v != nodeVolume) {
				_volumeRenderer.scheduleRegionExtraction(meshState, idx, region);
			}
		}

		if (renderContext.applyTransforms()) {
			prepareMeshStateTransform(meshState, sceneGraph, frame, node, idx, region);
		} else {
			meshState->setCullFace(idx, video::Face::Back);
			meshState->setModelMatrix(idx, glm::mat4(1.0f), region.getLowerCorner(), region.getUpperCorner());
		}

		bool hideNode = false;
		if (hideInactive) {
			if (activeNode.isGroupNode() || activeNode.isRootNode()) {
				if (node.parent() != activeNode.id()) {
					hideNode = true;
				}
			} else {
				hideNode = node.id() != activeNodeId;
			}
		} else {
			hideNode = !node.visible();
		}
		meshState->hide(idx, hideNode);

		if (grayInactive) {
			meshState->gray(idx, node.id() != activeNodeId);
		} else {
			meshState->gray(idx, false);
		}
	}

	meshState->resetReferences();
	if (renderContext.isSceneMode()) {
		for (auto entry : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = entry->second;
			if (!node.isReferenceNode()) {
				continue;
			}
			const int idx = getVolumeIdx(node);
			if (idx >= voxel::MAX_VOLUMES) {
				continue;
			}
			const int referencedIdx = getVolumeIdx(node.reference());
			meshState->setReference(idx, referencedIdx);
			prepareMeshStateTransform(meshState, sceneGraph, frame, node, idx, sceneGraph.resolveRegion(node));

			bool hideNode = false;
			if (hideInactive) {
				if (activeNode.isGroupNode() || activeNode.isRootNode()) {
					if (node.parent() != activeNode.id()) {
						hideNode = true;
					}
				} else {
					hideNode = node.id() != activeNodeId;
				}
			} else {
				hideNode = !node.visible();
			}
			meshState->hide(idx, hideNode);

			if (grayInactive) {
				meshState->gray(idx, node.id() != activeNodeId);
			} else {
				meshState->gray(idx, false);
			}
		}
	}
}

void SceneGraphRenderer::render(const voxel::MeshStatePtr &meshState, RenderContext &renderContext, const video::Camera &camera, bool shadow,
								bool waitPending) {
	core_trace_scoped(SceneGraphRenderer);
	prepare(meshState, renderContext);
	if (waitPending) {
		core_trace_scoped(SceneGraphRendererWaitPending);
		meshState->extractAllPending();
		_volumeRenderer.update(meshState);
	}

	_volumeRenderer.render(meshState, renderContext, camera, shadow);
	if (renderContext.showCameras()) {
		for (video::Camera &sceneCamera : _cameras) {
			sceneCamera.setSize(camera.size());
			sceneCamera.update(0.0);
			// TODO: node id to resolv the color _cameraRenderer.setColor(style::color(style::ColorCamera));
			_cameraRenderer.render(camera, sceneCamera);
		}
	}
}

} // namespace voxelrender
