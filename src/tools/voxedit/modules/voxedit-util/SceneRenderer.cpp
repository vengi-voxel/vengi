/**
 * @file
 */

#include "SceneRenderer.h"
#include "app/App.h"
#include "app/I18N.h"
#include "color/ColorUtil.h"
#include "core/Log.h"
#include "core/TimeProvider.h"
#include "core/Trace.h"
#include "core/concurrent/Thread.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneUtil.h"
#include "ui/Style.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "voxedit-util/AxisUtil.h"
#include "voxedit-util/Config.h"
#include "voxel/RawVolume.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/SceneGraphRenderer.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

namespace voxedit {

SceneRenderer::SceneRenderer(const core::TimeProviderPtr &timeProvider)
	: _meshState(core::make_shared<voxel::MeshState>()), _sceneGraphRenderer(timeProvider) {
}

void SceneRenderer::construct() {
	_sceneGraphRenderer.construct();
	_meshState->construct();
}

bool SceneRenderer::init() {
	_showGrid = core::getVar(cfg::VoxEditShowgrid);
	_showLockedAxis = core::getVar(cfg::VoxEditShowlockedaxis);
	_showAABB = core::getVar(cfg::VoxEditShowaabb);
	_showBones = core::getVar(cfg::VoxEditShowBones);
	_renderShadow = core::getVar(cfg::VoxEditRendershadow);
	const core::VarDef voxEditShadingMode(cfg::VoxEditShadingMode, 1, 0, 2, N_("Shading mode"),
										  N_("Shading mode: 0=Unlit, 1=Lit, 2=Shadows"));
	_shadingMode = core::Var::registerVar(voxEditShadingMode);
	_gridSize = core::getVar(cfg::VoxEditGridsize);
	_grayInactive = core::getVar(cfg::VoxEditGrayInactive);
	_hideInactive = core::getVar(cfg::VoxEditHideInactive);
	const core::VarDef voxEditAmbientColor(cfg::VoxEditAmbientColor, "0.3 0.3 0.3", N_("Ambient color"),
										   N_("Ambient color for lit rendering in r g b format"));
	_ambientColor = core::Var::registerVar(voxEditAmbientColor);
	const core::VarDef voxEditDiffuseColor(cfg::VoxEditDiffuseColor, "0.7 0.7 0.7", N_("Diffuse color"),
										   N_("Diffuse color for lit rendering in r g b format"));
	_diffuseColor = core::Var::registerVar(voxEditDiffuseColor);
	const core::VarDef voxEditSunAngle(cfg::VoxEditSunAngle, "35.0 135.0 0.0", N_("Sun angle"),
									   N_("pitch, yaw and ignored roll in degrees"));
	_sunAngle = core::Var::registerVar(voxEditSunAngle);
	_planeSize = core::getVar(cfg::VoxEditPlaneSize);
	_showPlane = core::getVar(cfg::VoxEditShowPlane);

	if (!_meshState->init()) {
		Log::error("Failed to initialize the mesh state");
		return false;
	}
	if (!_sceneGraphRenderer.init(_meshState->hasNormals())) {
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

	for (int i = 0; i < lengthof(_indices.plane); ++i) {
		_indices.plane[i] = -1;
	}
	return true;
}

const voxel::Region &SceneRenderer::sliceRegion() const {
	return _sceneGraphRenderer.sliceRegion();
}

SceneRenderer::RendererStats SceneRenderer::rendererStats() const {
	checkMainThread();
	RendererStats stats;
	stats.pendingExtractions = _meshState->pendingExtractions();
	stats.pendingMeshes = _meshState->pendingMeshes();
	stats.culledVolumes = _sceneGraphRenderer.culledVolumeCount();
	stats.freeVolumeIndices = _sceneGraphRenderer.freeVolumeIndexCount();
	stats.nextVolumeIdx = _sceneGraphRenderer.nextVolumeIdx();
	stats.mappedNodes = _sceneGraphRenderer.mappedNodeCount();
	return stats;
}

void SceneRenderer::shutdown() {
	_sceneGraphRenderer.shutdown();
	// don't free the volumes here, they belong to the scene graph
	(void)_meshState->shutdown();

	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_gridRenderer.shutdown();

	_indices.sliceRegion = -1;
	_indices.aabb = -1;
	_indices.bone = -1;
	_indices.highlight = -1;
}

/**
 * @brief Return the real model node, not the reference
 */
static scenegraph::SceneGraphNode *sceneGraphModelNode(const scenegraph::SceneGraph &sceneGraph, int nodeId) {
	if (sceneGraph.hasNode(nodeId)) {
		scenegraph::SceneGraphNode *n = &sceneGraph.node(nodeId);
		if (n->reference() != InvalidNodeId) {
			if (sceneGraph.hasNode(n->reference())) {
				n = &sceneGraph.node(n->reference());
			}
		}
		return n;
	}
	return nullptr;
}

void SceneRenderer::doUpdateLockedPlane(math::Axis axis, const scenegraph::SceneGraph &sceneGraph) {
	if (axis == math::Axis::None) {
		return;
	}
	const int index = math::getIndexForAxis(axis);
	int32_t &meshIndex = _indices.plane[index];
	if ((_lockedAxis & axis) == math::Axis::None) {
		if (meshIndex != -1) {
			_shapeRenderer.deleteMesh(meshIndex);
			meshIndex = -1;
		}
		return;
	}

	const int activeNode = sceneGraph.activeNode();
	const scenegraph::SceneGraphNode &node = sceneGraph.node(activeNode);
	if (!node.isModelNode()) {
		if (meshIndex != -1) {
			_shapeRenderer.deleteMesh(meshIndex);
			meshIndex = -1;
		}
		return;
	}
	glm::vec4 color{0.0f};
	if (axis == math::Axis::X) {
		color = style::color(style::ColorAxisX);
	} else if (axis == math::Axis::Y) {
		color = style::color(style::ColorAxisY);
	} else if (axis == math::Axis::Z) {
		color = style::color(style::ColorAxisZ);
	}
	updateShapeBuilderForPlane(_shapeBuilder, node.region(), false, _lockedAxisPosition, axis,
							   color::alpha(color, 0.4f));
	_shapeRenderer.createOrUpdate(meshIndex, _shapeBuilder);
}

void SceneRenderer::doUpdateAABBMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph,
									 scenegraph::FrameIndex frameIdx) {
	if (!sceneMode || !_showAABB->boolVal()) {
		return;
	}
	const int activeNodeId = sceneGraph.activeNode();
	if (!_cache.aabbDirty && frameIdx == _cache.lastAABBFrame && activeNodeId == _cache.lastAABBActiveNode) {
		return;
	}
	core_trace_scoped(UpdateAABBMesh);
	_shapeBuilder.clear();
	const scenegraph::SceneGraphNode &activeNode = sceneGraph.node(sceneGraph.activeNode());
	const bool activeNodeLocked = activeNode.locked();
	int modelNodes = 0;
	for (auto entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (!node.isAnyModelNode()) {
			continue;
		}
		if (!node.visible()) {
			continue;
		}
		if (node.id() == sceneGraph.activeNode()) {
			continue;
		} else if (activeNodeLocked && node.locked()) {
			_shapeBuilder.setColor(style::color(style::ColorLockedNode));
		} else if (node.isReferenceNode()) {
			_shapeBuilder.setColor(style::color(style::ColorReferenceNode));
		} else {
			_shapeBuilder.setColor(style::color(style::ColorInactiveNode));
		}
		const voxel::Region &region = sceneGraph.resolveRegion(node);
		core_assert_msg(region.isValid(), "Region for node %s of type %i is invalid", node.name().c_str(),
						(int)node.type());
		const glm::vec3 pivot = node.pivot();
		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frameIdx);
		const math::OBBF &obb = scenegraph::toOBB(true, region, pivot, transform);
		_shapeBuilder.obb(obb);
		++modelNodes;
	}

	if (activeNode.isAnyModelNode() && activeNode.visible()) {
		_shapeBuilder.setColor(style::color(style::ColorActiveNode));
		const voxel::RawVolume *v = sceneGraph.resolveVolume(activeNode);
		core_assert(v != nullptr);
		const voxel::Region &region = v->region();
		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(activeNode, frameIdx);
		_shapeBuilder.obb(scenegraph::toOBB(sceneMode, region, activeNode.pivot(), transform));
	}

	if (modelNodes > 1 && !activeNode.children().empty()) {
		const math::AABB<float> &aabb = sceneGraph.calculateGroupAABB(activeNode, frameIdx);
		if (aabb.isValid()) {
			_shapeBuilder.setColor(style::color(style::ColorGroupNode));
			_shapeBuilder.aabb(aabb);
		}
	}

	_shapeRenderer.createOrUpdate(_indices.aabb, _shapeBuilder);
	_cache.aabbDirty = false;
	_cache.lastAABBFrame = frameIdx;
	_cache.lastAABBActiveNode = activeNodeId;
}

void SceneRenderer::doUpdateBoneMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph,
									 scenegraph::FrameIndex frameIdx) {
	if (!sceneMode) {
		return;
	}
	if (!_showBones->boolVal()) {
		return;
	}
	if (!_cache.boneDirty && frameIdx == _cache.lastBoneFrame) {
		return;
	}
	core_trace_scoped(UpdateBoneMesh);
	_shapeBuilder.clear();
	_shapeBuilder.setColor(style::color(style::ColorBone));

	const bool hideInactive = _hideInactive->boolVal();
	const int activeNodeId = sceneGraph.activeNode();
	for (auto entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (!node.isAnyModelNode()) {
			continue;
		}
		if (!node.visible()) {
			continue;
		}
		if (node.parent() == InvalidNodeId) {
			continue;
		}
		const bool isActiveNode = node.id() == activeNodeId;
		if (hideInactive && !isActiveNode) {
			continue;
		}

		const scenegraph::SceneGraphNode &pnode = sceneGraph.node(node.parent());
		if (!pnode.isAnyModelNode()) {
			continue;
		}
		if (!pnode.visible()) {
			continue;
		}

		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frameIdx);
		const scenegraph::FrameTransform &ptransform = sceneGraph.transformForFrame(pnode, frameIdx);
		const glm::vec3 &ptranslation = ptransform.worldTranslation();
		const glm::vec3 &translation = transform.worldTranslation();

		_shapeBuilder.bone(ptranslation, translation);
	}

	_shapeRenderer.createOrUpdate(_indices.bone, _shapeBuilder);
	_cache.boneDirty = false;
	_cache.lastBoneFrame = frameIdx;
}

const voxel::RawVolume *SceneRenderer::volumeForNode(const scenegraph::SceneGraphNode &node) {
	int idx = _sceneGraphRenderer.getVolumeIdx(node);
	const voxel::RawVolume *v = _meshState->volume(idx);
	if (v == nullptr) {
		v = node.volume();
	}
	return v;
}

bool SceneRenderer::isVisible(int nodeId, bool hideEmpty) const {
	checkMainThread();
	return _sceneGraphRenderer.isVisible(_meshState, nodeId, hideEmpty);
}

void SceneRenderer::checkMainThread() const {
	core_assert(app::App::getInstance()->isMainThread(core::getCurrentThreadId()));
}

void SceneRenderer::handleCommandBuffer() {
	core::DynamicArray<CommandEvent> cmds;
	{
		core::ScopedLock lock(_commandBufferMutex);
		cmds = core::move(_commandBuffer);
	}

	for (const CommandEvent &cmd : cmds) {
		switch (cmd.type) {
		case CommandType::NodeRegion: {
			const voxel::Region region(
				glm::ivec3(cmd.nodeRegion.regionMins[0], cmd.nodeRegion.regionMins[1], cmd.nodeRegion.regionMins[2]),
				glm::ivec3(cmd.nodeRegion.regionMaxs[0], cmd.nodeRegion.regionMaxs[1], cmd.nodeRegion.regionMaxs[2]));
			_sceneGraphRenderer.scheduleRegionExtraction(_meshState, cmd.nodeRegion.nodeId, region);
			const core::TimeProviderPtr &timeProvider = app::App::getInstance()->timeProvider();
			_highlightRegion = TimedRegion(region, timeProvider->tickNow(), cmd.nodeRegion.renderRegionMillis);
			_cache.aabbDirty = true;
			_cache.boneDirty = true;
			_cache.lockedAxisDirty = true;
			break;
		}
		case CommandType::GridRegion: {
			const voxel::Region region(
				glm::ivec3(cmd.gridRegion.regionMins[0], cmd.gridRegion.regionMins[1], cmd.gridRegion.regionMins[2]),
				glm::ivec3(cmd.gridRegion.regionMaxs[0], cmd.gridRegion.regionMaxs[1], cmd.gridRegion.regionMaxs[2]));
			_gridRegion = scenegraph::toAABB(region);
			_cache.lockedAxisDirty = true;
			break;
		}
		case CommandType::SliceRegion: {
			const voxel::Region region(
				glm::ivec3(cmd.sliceRegion.regionMins[0], cmd.sliceRegion.regionMins[1], cmd.sliceRegion.regionMins[2]),
				glm::ivec3(cmd.sliceRegion.regionMaxs[0], cmd.sliceRegion.regionMaxs[1],
						   cmd.sliceRegion.regionMaxs[2]));
			_sceneGraphRenderer.setSliceRegion(region);
			break;
		}
		case CommandType::RemoveNode: {
			_sceneGraphRenderer.nodeRemove(_meshState, cmd.node.nodeId);
			_cache.aabbDirty = true;
			_cache.boneDirty = true;
			_cache.lockedAxisDirty = true;
			break;
		}
		case CommandType::UnhideNode: {
			const int idx = _sceneGraphRenderer.getVolumeIdx(cmd.node.nodeId);
			if (idx >= 0) {
				_meshState->hide(idx, false);
			}
			break;
		}
		case CommandType::Clear: {
			_sceneGraphRenderer.clear(_meshState);
			_cache.aabbDirty = true;
			_cache.boneDirty = true;
			_cache.lockedAxisDirty = true;
			break;
		}
		case CommandType::MarkDirty: {
			_cache.aabbDirty = true;
			_cache.boneDirty = true;
			_cache.lockedAxisDirty = true;
			break;
		}

		}
	}
}

void SceneRenderer::update() {
	core_trace_scoped(SceneRendererUpdate);

	handleCommandBuffer();

	_gridRenderer.setRenderAABB(_showAABB->boolVal());
	_gridRenderer.setRenderGrid(_showGrid->boolVal());
	_gridRenderer.setGridResolution(_gridSize->intVal());
	_gridRenderer.setPlaneGridSize(_planeSize->intVal());
	_gridRenderer.setRenderPlane(_showPlane->boolVal());
	_gridRenderer.setColor(style::color(style::ColorGridBorder));
	_gridRenderer.update(_gridRegion);
	glm::vec3 val;
	_ambientColor->vec3Val(&val[0]);
	_sceneGraphRenderer.setAmbientColor(val);
	_diffuseColor->vec3Val(&val[0]);
	_sceneGraphRenderer.setDiffuseColor(val);
	_sunAngle->vec3Val(&val[0]);
	_sceneGraphRenderer.setSunAngle(val);
	_sceneGraphRenderer.update(_meshState);
}

void SceneRenderer::doUpdateSliceRegionMesh() {
	if (!isSliceModeActive()) {
		return;
	}
	core_trace_scoped(UpdateSliceRegionMesh);
	_shapeBuilder.clear();

	const math::AABB<float> &aabb = scenegraph::toAABB(sliceRegion());
	_shapeBuilder.setColor(style::color(style::ColorSliceRegion));
	_shapeBuilder.aabb(aabb);
	_shapeRenderer.createOrUpdate(_indices.sliceRegion, _shapeBuilder);
}

void SceneRenderer::renderScene(voxelrender::RenderContext &renderContext, const video::Camera &camera) {
	core_trace_scoped(RenderScene);
	checkMainThread();
	if (renderContext.sceneGraph == nullptr) {
		Log::error("No scenegraph given to render");
		return;
	}

	const bool hideInactiveNow = _hideInactive->boolVal();
	const bool grayInactiveNow = _grayInactive->boolVal();
	if (hideInactiveNow != _cache.lastHideInactive || grayInactiveNow != _cache.lastGrayInactive) {
		_cache.lastHideInactive = hideInactiveNow;
		_cache.lastGrayInactive = grayInactiveNow;
		markDirty();
	}
	renderContext.hideInactive = hideInactiveNow;
	renderContext.grayInactive = grayInactiveNow;

	video::ScopedState depthTest(video::State::DepthTest, true);
	_sceneGraphRenderer.render(_meshState, renderContext, camera, _renderShadow->boolVal(), false);
}

void SceneRenderer::renderUI(voxelrender::RenderContext &renderContext, const video::Camera &camera) {
	core_trace_scoped(RenderUI);
	checkMainThread();
	if (renderContext.sceneGraph == nullptr) {
		Log::error("No scenegraph given to render");
		return;
	}

	doUpdateAABBMesh(renderContext.isSceneMode(), *renderContext.sceneGraph, renderContext.frame);
	doUpdateBoneMesh(renderContext.isSceneMode(), *renderContext.sceneGraph, renderContext.frame);

	if (_cache.lockedAxisDirty) {
		doUpdateLockedPlane(math::Axis::X, *renderContext.sceneGraph);
		doUpdateLockedPlane(math::Axis::Y, *renderContext.sceneGraph);
		doUpdateLockedPlane(math::Axis::Z, *renderContext.sceneGraph);
		_cache.lockedAxisDirty = false;
	}

	// TODO: PERF: add dirty state checks here by comparing the old with the current value with values in struct Cache
	doUpdateSliceRegionMesh();

	video::ScopedState depthTest(video::State::DepthTest, true);
	video::ScopedState blend(video::State::Blend, true);
	const scenegraph::SceneGraph &sceneGraph = *renderContext.sceneGraph;
	scenegraph::SceneGraphNode *n = sceneGraphModelNode(sceneGraph, sceneGraph.activeNode());
	_gridRenderer.renderForwardArrow(camera);
	_gridRenderer.renderPlane(camera);
	if (renderContext.isSceneMode()) {
		if (_showAABB->boolVal()) {
			_shapeRenderer.render(_indices.aabb, camera);
		}
		if (_showBones->boolVal()) {
			video::ScopedState depthDepthTest(video::State::DepthTest, false);
			_shapeRenderer.render(_indices.bone, camera);
		}
		// TODO: allow to render a grid in scene mode - makes shifting a lot easier
		// TODO: render arrows for the distance of the region mins to the origin - to indicate a shifted region

		if (isSliceModeActive()) {
			// TODO: model matrix for the slice region
			_shapeRenderer.render(_indices.sliceRegion, camera);
		}
	} else if (n != nullptr) {
		const voxel::Region &region = n->region();
		const bool applyTransforms = region.isValid() && renderContext.applyTransforms();
		const glm::mat4 &model = sceneGraph.worldMatrix(*n, renderContext.frame, applyTransforms);
		_gridRenderer.render(camera, scenegraph::toAABB(region), model);

		if (_showLockedAxis->boolVal()) {
			for (int i = 0; i < lengthof(_indices.plane); ++i) {
				// TODO: fix z-fighting
				_shapeRenderer.render(_indices.plane[i], camera, model);
			}
		}

		if (isSliceModeActive()) {
			_shapeRenderer.render(_indices.sliceRegion, camera, model);
		}

		const core::TimeProviderPtr &timeProvider = app::App::getInstance()->timeProvider();
		const uint64_t highlightMillis = _highlightRegion.remaining(timeProvider->tickNow());
		if (highlightMillis > 0) {
			video::ScopedPolygonMode o(video::PolygonMode::Solid, glm::vec2(1.0f, 1.0f));
			_shapeBuilder.clear();
			_shapeBuilder.setColor(style::color(style::ColorHighlightArea));
			_shapeBuilder.cube(_highlightRegion.value().getLowerCornerf(),
							   _highlightRegion.value().getUpperCornerf() + 1.0f);
			_shapeRenderer.createOrUpdate(_indices.highlight, _shapeBuilder);
			_shapeRenderer.render(_indices.highlight, camera, model);
			video::polygonOffset(glm::vec2(0.0f));
		}
	}
}

} // namespace voxedit
