/**
 * @file
 */

#include "SceneRenderer.h"
#include "SceneUtil.h"
#include "app/App.h"
#include "core/TimeProvider.h"
#include "core/Log.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "voxedit-util/AxisUtil.h"
#include "voxedit-util/Config.h"
#include "voxel/RawVolume.h"

namespace voxedit {

SceneRenderer::SceneRenderer() {
}

void SceneRenderer::construct() {
	_volumeRenderer.construct();
}

bool SceneRenderer::init() {
	_showGrid = core::Var::getSafe(cfg::VoxEditShowgrid);
	_showLockedAxis = core::Var::getSafe(cfg::VoxEditShowlockedaxis);
	_showAABB = core::Var::getSafe(cfg::VoxEditShowaabb);
	_renderShadow = core::Var::getSafe(cfg::VoxEditRendershadow);
	_gridSize = core::Var::getSafe(cfg::VoxEditGridsize);
	_grayInactive = core::Var::getSafe(cfg::VoxEditGrayInactive);
	_hideInactive = core::Var::getSafe(cfg::VoxEditHideInactive);
	_ambientColor = core::Var::get(cfg::VoxEditAmbientColor, "1.0 1.0 1.0");
	_diffuseColor = core::Var::get(cfg::VoxEditDiffuseColor, "0.0 0.0 0.0");

	if (!_volumeRenderer.init()) {
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

	for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
		_planeMeshIndex[i] = -1;
	}
	return true;
}

void SceneRenderer::clear() {
	_volumeRenderer.clear();
}

void SceneRenderer::shutdown() {
	_volumeRenderer.shutdown();
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_gridRenderer.shutdown();

	_aabbMeshIndex = -1;
	_highlightMeshIndex = -1;
}

void SceneRenderer::updateGridRegion(const voxel::Region &region) {
	if (region.isValid()) {
		_gridRenderer.update(toAABB(region));
	}
}

void SceneRenderer::updateNodeRegion(int nodeId, const voxel::Region &region, uint64_t renderRegionMillis) {
	bool addNew = true;
	for (const auto &r : _extractRegions) {
		if (r.nodeId != nodeId) {
			continue;
		}
		if (r.region.containsRegion(region)) {
			addNew = false;
			break;
		}
	}
	if (addNew) {
		_extractRegions.push_back({region, nodeId});
	}
	const core::TimeProviderPtr &timeProvider = app::App::getInstance()->timeProvider();
	_highlightRegion = TimedRegion(region, timeProvider->tickNow(), renderRegionMillis);
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

bool SceneRenderer::extractVolume(const scenegraph::SceneGraph &sceneGraph) {
	core_trace_scoped(SceneManagerExtract);
	const size_t n = _extractRegions.size();
	if (n <= 0) {
		return false;
	}
	Log::debug("Extract the meshes for %i regions", (int)n);
	for (size_t i = 0; i < n; ++i) {
		const voxel::Region &region = _extractRegions[i].region;
		if (scenegraph::SceneGraphNode *node = sceneGraphModelNode(sceneGraph, _extractRegions[i].nodeId)) {
			if (!_volumeRenderer.extractRegion(*node, region)) {
				Log::error("Failed to extract the model mesh");
			}
			Log::debug("Extract node %i", _extractRegions[i].nodeId);
			voxel::logRegion("Extraction", region);
		}
	}
	_extractRegions.clear();
	return true;
}

void SceneRenderer::updateLockedPlanes(math::Axis lockedAxis, const scenegraph::SceneGraph &sceneGraph,
									   const glm::ivec3 &cursorPosition) {
	updateLockedPlane(lockedAxis, math::Axis::X, sceneGraph, cursorPosition);
	updateLockedPlane(lockedAxis, math::Axis::Y, sceneGraph, cursorPosition);
	updateLockedPlane(lockedAxis, math::Axis::Z, sceneGraph, cursorPosition);
}

void SceneRenderer::updateLockedPlane(math::Axis lockedAxis, math::Axis axis, const scenegraph::SceneGraph &sceneGraph,
									  const glm::ivec3 &cursorPosition) {
	if (axis == math::Axis::None) {
		return;
	}
	const int index = math::getIndexForAxis(axis);
	int32_t &meshIndex = _planeMeshIndex[index];
	if ((lockedAxis & axis) == math::Axis::None) {
		if (meshIndex != -1) {
			_shapeRenderer.deleteMesh(meshIndex);
			meshIndex = -1;
		}
		return;
	}

	const glm::vec4 colors[] = {core::Color::LightRed, core::Color::LightGreen, core::Color::LightBlue};
	updateShapeBuilderForPlane(_shapeBuilder, sceneGraph.region(), false, cursorPosition, axis,
							   core::Color::alpha(colors[index], 0.4f));
	_shapeRenderer.createOrUpdate(meshIndex, _shapeBuilder);
}

void SceneRenderer::updateAABBMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph,
								   scenegraph::FrameIndex frame) {
	_shapeBuilder.clear();
	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (!node.visible()) {
			continue;
		}
		if (node.id() == sceneGraph.activeNode()) {
			_shapeBuilder.setColor(core::Color::White);
		} else if (node.isReference()) {
			_shapeBuilder.setColor(core::Color::Yellow);
		} else {
			_shapeBuilder.setColor(core::Color::Gray);
		}
		const voxel::RawVolume *v = sceneGraph.resolveVolume(node);
		core_assert(v != nullptr);
		const voxel::Region &region = v->region();
		_shapeBuilder.obb(toOBB(sceneMode, region, node.pivot(), node.transformForFrame(frame)));
	}
	_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
}

void SceneRenderer::nodeRemove(int nodeId) {
	_volumeRenderer.nodeRemove(nodeId);
}

void SceneRenderer::update() {
	_gridRenderer.setRenderAABB(_showAABB->boolVal());
	_gridRenderer.setRenderGrid(_showGrid->boolVal());
	_gridRenderer.setGridResolution(_gridSize->intVal());
	glm::vec3 val;
	_ambientColor->vec3Val(&val[0]);
	_volumeRenderer.setAmbientColor(val);
	_diffuseColor->vec3Val(&val[0]);
	_volumeRenderer.setDiffuseColor(val);
	_volumeRenderer.update();
}

void SceneRenderer::renderScene(voxelrender::RenderContext &renderContext, const video::Camera &camera,
								const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx) {
	video::ScopedState depthTest(video::State::DepthTest, true);
	_volumeRenderer.setSceneMode(renderContext.sceneMode);
	updateAABBMesh(renderContext.sceneMode, sceneGraph, frameIdx);
	_volumeRenderer.prepare(sceneGraph, frameIdx, _hideInactive->boolVal(), _grayInactive->boolVal());
	_volumeRenderer.render(renderContext, camera, _renderShadow->boolVal(), false);
	extractVolume(sceneGraph);
}

void SceneRenderer::renderUI(voxelrender::RenderContext &renderContext, const video::Camera &camera,
							 const scenegraph::SceneGraph &sceneGraph) {
	video::ScopedState depthTest(video::State::DepthTest, true);
	video::ScopedState blend(video::State::Blend, true);
	if (renderContext.sceneMode) {
		if (_showAABB->boolVal()) {
			_shapeRenderer.render(_aabbMeshIndex, camera);
		}
	} else if (scenegraph::SceneGraphNode *n = sceneGraphModelNode(sceneGraph, sceneGraph.activeNode())) {
		const voxel::Region &region = n->region();
		_gridRenderer.render(camera, toAABB(region));

		if (_showLockedAxis->boolVal()) {
			for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
				// TODO: fix z-fighting
				_shapeRenderer.render(_planeMeshIndex[i], camera);
			}
		}
	}

	const core::TimeProviderPtr &timeProvider = app::App::getInstance()->timeProvider();
	const uint64_t highlightMillis = _highlightRegion.remaining(timeProvider->tickNow());
	if (highlightMillis > 0) {
		video::ScopedPolygonMode o(video::PolygonMode::Solid, glm::vec2(1.0f, 1.0f));
		_shapeBuilder.clear();
		_shapeBuilder.setColor(core::Color::alpha(core::Color::Green, 0.2f));
		_shapeBuilder.cube(_highlightRegion.value().getLowerCornerf(),
						   _highlightRegion.value().getUpperCornerf() + 1.0f);
		_shapeRenderer.createOrUpdate(_highlightMeshIndex, _shapeBuilder);
		_shapeRenderer.render(_highlightMeshIndex, camera);
		video::polygonOffset(glm::vec2(0.0f));
	}
}

} // namespace voxedit
