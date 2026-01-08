/**
 * @file
 */

#include "SceneGraphRenderer.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "render/CameraRenderer.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "video/Camera.h"
#include "voxel/RawVolume.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/RenderUtil.h"
#include <limits>

namespace voxelrender {

SceneGraphRenderer::SceneGraphRenderer() {
}

void SceneGraphRenderer::construct() {
	_volumeRenderer.construct();
}

bool SceneGraphRenderer::init(bool normals) {
	if (!_cameraRenderer.init(0)) {
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
	if (sliceViewActiveForNode(nodeId)) {
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
	if (_sliceRegion != region) {
		_sliceVolumeDirty = true;
	}
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
												   const scenegraph::SceneGraphNode &node, int idx) const {
	core_trace_scoped(PrepareMeshStateTransform);
	const voxel::Region &region = sceneGraph.resolveRegion(node);
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

bool SceneGraphRenderer::sliceViewActiveForNode(int nodeId) const {
	if (!sliceViewActive()) {
		return false;
	}
	return _sliceVolumeNodeId == nodeId;
}

bool SceneGraphRenderer::sliceViewActive() const {
	return _sliceRegion.isValid();
}

void SceneGraphRenderer::handleSliceView(const voxel::MeshStatePtr &meshState, scenegraph::SceneGraphNode &node) {
	core_trace_scoped(HandleSliceView);
	// check several things to re-create the slice volume
	// * a new activated node
	// * the region changed
	// * we don't yet have a sliced volume view but requested one
	const int idx = getVolumeIdx(node);
	if (idx >= voxel::MAX_VOLUMES) {
		return;
	}

	if (_sliceVolumeDirty || _sliceVolumeNodeId != node.id() || !_sliceVolume || _sliceVolume->region() != _sliceRegion) {
		const voxel::RawVolume *nodeVolume = node.volume();
		// this enforces the lock on the volume renderer if the volume is currently extracted
		core::SharedPtr<voxel::RawVolume> newVolume(core::make_shared<voxel::RawVolume>(nodeVolume, _sliceRegion));
		const bool meshDelete = !_sliceVolumeDirty;
		const voxel::RawVolume *oldV = _volumeRenderer.setVolume(meshState, idx, newVolume.get(), &node.palette(),
																 &node.normalPalette(), meshDelete);
		if (_sliceVolumeDirty || oldV != nullptr) {
			_volumeRenderer.scheduleRegionExtraction(meshState, idx, newVolume->region());
		}
		_sliceVolume = newVolume;
		_sliceVolumeNodeId = node.id();

		const voxel::Region &region = _sliceVolume->region();
		meshState->setModelMatrix(idx, glm::mat4(1.0f), region.getLowerCorner(), region.getUpperCorner());
	}
	_sliceVolumeDirty = false;
}

void SceneGraphRenderer::updateNodeState(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext,
										 const scenegraph::SceneGraphNode &activeNode,
										 const scenegraph::SceneGraphNode &node, int idx) const {
	core_trace_scoped(UpdateNodeState);
	bool hideNode = false;
	if (renderContext.hideInactive) {
		if (activeNode.isGroupNode() || activeNode.isRootNode()) {
			if (node.parent() != activeNode.id()) {
				hideNode = true;
			}
		} else {
			hideNode = node.id() != activeNode.id();
		}
	} else {
		hideNode = !node.visible();
	}
	meshState->hide(idx, hideNode);

	if (renderContext.grayInactive) {
		meshState->gray(idx, node.id() != activeNode.id());
	} else {
		meshState->gray(idx, false);
	}
}

void SceneGraphRenderer::prepareReferenceNodes(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext) const {
	core_trace_scoped(PrepareReferenceNodes);
	meshState->resetReferences();
	if (!renderContext.isSceneMode()) {
		return;
	}
	const scenegraph::SceneGraph &sceneGraph = *renderContext.sceneGraph;
	const int activeNodeId = sceneGraph.activeNode();
	const scenegraph::SceneGraphNode &activeNode = sceneGraph.node(activeNodeId);
	for (auto entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (!node.isReferenceNode()) {
			continue;
		}

		const int idx = getVolumeIdx(node);
		if (idx >= voxel::MAX_VOLUMES) {
			continue;
		}
		updateNodeState(meshState, renderContext, activeNode, node, idx);
		if (meshState->hidden(idx)) {
			continue;
		}
		const int referencedIdx = getVolumeIdx(node.reference());
		meshState->setReference(idx, referencedIdx);
		prepareMeshStateTransform(meshState, sceneGraph, renderContext.frame, node, idx);
	}
}

void SceneGraphRenderer::prepareCameraNodes(const RenderContext &renderContext) {
	core_trace_scoped(PrepareCameraNodes);
	_cameras.clear();
	if (renderContext.onlyModels) {
		return;
	}

	const scenegraph::SceneGraph &sceneGraph = *renderContext.sceneGraph;
	for (auto entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->value;
		if (!node.isCameraNode()) {
			continue;
		}
		const scenegraph::SceneGraphNodeCamera &cameraNode = scenegraph::toCameraNode(node);
		if (!cameraNode.visible()) {
			continue;
		}
		const glm::ivec2 size(cameraNode.width(), cameraNode.height());
		_cameras.emplace_back(cameraNode.id(), toCamera(size, sceneGraph, cameraNode, renderContext.frame), cameraNode.color());
	}
}

void SceneGraphRenderer::resetVolumes(const voxel::MeshStatePtr &meshState, const scenegraph::SceneGraph &sceneGraph) {
	core_trace_scoped(ResetVolumes);
	for (int i = 0; i < voxel::MAX_VOLUMES; ++i) {
		const int nodeId = getNodeId(i);
		if (sceneGraph.hasNode(nodeId)) {
			continue;
		}
		// ignore the return value because the volume is owned by the node
		(void)_volumeRenderer.resetVolume(meshState, nodeId);
	}
}

void SceneGraphRenderer::applyTransform(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext,
										const scenegraph::SceneGraph &sceneGraph,
										const scenegraph::SceneGraphNode &node, int idx) {
	if (renderContext.applyTransforms()) {
		prepareMeshStateTransform(meshState, sceneGraph, renderContext.frame, node, idx);
	} else {
		meshState->setCullFace(idx, video::Face::Back);
		const voxel::Region &region = node.region();
		meshState->setModelMatrix(idx, glm::mat4(1.0f), region.getLowerCorner(), region.getUpperCorner());
	}
}

void SceneGraphRenderer::prepareModelNodes(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext) {
	core_trace_scoped(PrepareModelNodes);
	const scenegraph::SceneGraph &sceneGraph = *renderContext.sceneGraph;
	const int activeNodeId = sceneGraph.activeNode();
	if (sliceViewActive()) {
		scenegraph::SceneGraphNode &sliceNode = sceneGraph.node(activeNodeId);
		handleSliceView(meshState, sliceNode);
	} else {
		_sliceVolume = nullptr;
		_sliceVolumeDirty = false;
		_sliceVolumeNodeId = -1;
	}
	const scenegraph::SceneGraphNode &activeNode = sceneGraph.node(activeNodeId);
	sceneGraph.nodes().for_parallel([&](int nodeId, const scenegraph::SceneGraphNode &node) {
		if (!node.isModelNode()) {
			return;
		}
		const int idx = getVolumeIdx(nodeId);
		updateNodeState(meshState, renderContext, activeNode, node, idx);

		if (meshState->hidden(idx)) {
			return;
		}

		// also check the volume here on the first run, as they are added after this step for the first time
		if (meshState->volume(idx) == nullptr) {
			return;
		}

		applyTransform(meshState, renderContext, sceneGraph, node, idx);
	});
	for (auto entry : sceneGraph.nodes()) {
		scenegraph::SceneGraphNode &node = entry->value;
		if (!node.isModelNode()) {
			continue;
		}

		const int idx = getVolumeIdx(node);
		if (meshState->hidden(idx)) {
			continue;
		}

		if (sliceViewActiveForNode(node.id())) {
			continue;
		}
		const voxel::RawVolume *v = meshState->volume(idx);
		const voxel::RawVolume *nodeVolume = sceneGraph.resolveVolume(node);
		_volumeRenderer.setVolume(meshState, idx, node, true);
		if (v != nodeVolume) {
			_volumeRenderer.scheduleRegionExtraction(meshState, idx, node.region());
			if (v == nullptr) {
				// This is needed to setup the model matrix correctly for the first time - otherwise thumbnails wouldn't
				// work, as they only have one render call
				applyTransform(meshState, renderContext, sceneGraph, node, idx);
			}
		}
	}
}

void SceneGraphRenderer::prepare(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext) {
	core_trace_scoped(Prepare);
	core_assert_always(renderContext.sceneGraph != nullptr);
	const scenegraph::SceneGraph &sceneGraph = *renderContext.sceneGraph;
	resetVolumes(meshState, sceneGraph);
	prepareCameraNodes(renderContext);
	prepareModelNodes(meshState, renderContext);
	prepareReferenceNodes(meshState, renderContext);
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

	_volumeRenderer.render(meshState, renderContext, camera, shadow, true);
	if (renderContext.showCameras()) {
		for (render::CameraRenderer::Node &cameraNode : _cameras) {
			if (cameraNode.camera == camera) {
				continue;
			}
			cameraNode.camera.setSize(camera.size());
			cameraNode.camera.update(0.0);
			_cameraRenderer.render(camera, cameraNode);
		}
	}
}

} // namespace voxelrender
