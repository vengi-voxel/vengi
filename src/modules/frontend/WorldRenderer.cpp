/**
 * @file
 */

#include "WorldRenderer.h"
#include "core/Color.h"
#include "video/GLFunc.h"
#include "voxel/Spiral.h"
#include "core/App.h"
#include "noise/SimplexNoise.h"
#include "core/Var.h"
#include "voxel/MaterialColor.h"
#include "frontend/PlantDistributor.h"
#include "video/ScopedViewPort.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedPolygonMode.h"

constexpr int MinCullingDistance = 500;
constexpr int MinExtractionCullingDistance = 1000;

namespace frontend {

const std::string MaxDepthBufferUniformName = "u_cascades";

WorldRenderer::WorldRenderer(const voxel::WorldPtr& world) :
		_clearColor(core::Color::LightBlue), _world(world) {
}

WorldRenderer::~WorldRenderer() {
}

void WorldRenderer::reset() {
	for (video::GLMeshData& meshData : _meshDataOpaque) {
		meshData.shutdown();
	}
	for (video::GLMeshData& meshData : _meshDataWater) {
		meshData.shutdown();
	}
	_meshDataOpaque.clear();
	_meshDataWater.clear();
	_entities.clear();
	_viewDistance = 1.0f;
	_now = 0l;
}

void WorldRenderer::shutdown() {
	_gbuffer.shutdown();
	_fullscreenQuad.shutdown();
	_shadowMapDebugBuffer.shutdown();
	_shadowMapDebugShader.shutdown();
	_shadowMapInstancedShader.shutdown();
	_worldShader.shutdown();
	_plantShader.shutdown();
	_waterShader.shutdown();
	_meshShader.shutdown();
	_shadowMapShader.shutdown();
	_deferredDirLightShader.shutdown();
	_depthBuffer.shutdown();
	_materialBuffer.shutdown();
	reset();
	_colorTexture->shutdown();
	_colorTexture = video::TexturePtr();
	_entities.clear();

	for (video::GLMeshData& meshData : _meshDataPlant) {
		meshData.shutdown();
	}
	_meshDataPlant.clear();
	_noiseFuture.clear();
	_plantGenerator.shutdown();
}

ClientEntityPtr WorldRenderer::getEntity(ClientEntityId id) const {
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		return ClientEntityPtr();
	}
	return i->second;
}

bool WorldRenderer::addEntity(const ClientEntityPtr& entity) {
	auto i = _entities.find(entity->id());
	if (i != _entities.end()) {
		return false;
	}
	_entities[entity->id()] = entity;
	return true;
}

void WorldRenderer::deleteMesh(const glm::ivec3& pos) {
	core_trace_gl_scoped(WorldRendererDeleteMesh);
	const glm::ivec3& p = _world->getMeshPos(pos);
	for (auto i = _meshDataOpaque.begin(); i != _meshDataOpaque.end(); ++i) {
		video::GLMeshData& meshData = *i;
		if (meshData.translation != p) {
			continue;
		}
		_meshDataOpaque.erase(i);
		meshData.shutdown();
		break;
	}
	for (auto i = _meshDataWater.begin(); i != _meshDataWater.end(); ++i) {
		video::GLMeshData& meshData = *i;
		if (meshData.translation != p) {
			continue;
		}
		_meshDataWater.erase(i);
		meshData.shutdown();
		break;
	}
}

bool WorldRenderer::removeEntity(ClientEntityId id) {
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		return false;
	}
	_entities.erase(i);
	return true;
}

// redistribute the plants on the meshes that are already extracted
void WorldRenderer::fillPlantPositionsFromMeshes() {
	const int plantMeshAmount = _meshDataPlant.size();
	if (plantMeshAmount == 0) {
		return;
	}
	for (video::GLMeshData& mp : _meshDataPlant) {
		mp.instancedPositions.clear();
	}
	for (const video::GLMeshData& data : _meshDataOpaque) {
		if (data.instancedPositions.empty()) {
			continue;
		}
		std::vector<glm::vec3> p = data.instancedPositions;
		core::Random rnd(_world->seed() + data.translation.x + data.translation.y + data.translation.z);
		rnd.shuffle(p.begin(), p.end());
		const int plantMeshes = p.size() / plantMeshAmount;
		int delta = p.size() - plantMeshes * plantMeshAmount;
		for (video::GLMeshData& mp : _meshDataPlant) {
			auto it = std::next(p.begin(), plantMeshes + delta);
			std::move(p.begin(), it, std::back_inserter(mp.instancedPositions));
			p.erase(p.begin(), it);
			delta = 0;
		}
	}
}

void WorldRenderer::handleMeshQueue(const video::Shader& shader) {
	voxel::ChunkMeshData mesh(0, 0);
	if (!_world->pop(mesh)) {
		return;
	}
	core_trace_gl_scoped(WorldRendererHandleMeshQueue);
	for (video::GLMeshData& m : _meshDataOpaque) {
		if (m.translation == mesh.opaqueMesh.getOffset()) {
			updateMesh(mesh.opaqueMesh, m);
			return;
		}
	}
	for (video::GLMeshData& m : _meshDataWater) {
		if (m.translation == mesh.waterMesh.getOffset()) {
			updateMesh(mesh.waterMesh, m);
			return;
		}
	}
	// Now add the mesh to the list of meshes to render.
	video::GLMeshData meshDataOpaque = createMesh(shader, mesh.opaqueMesh);
	if (meshDataOpaque.noOfIndices > 0) {
		distributePlants(_world, 100, meshDataOpaque.translation, meshDataOpaque.instancedPositions);
		_meshDataOpaque.push_back(meshDataOpaque);
		fillPlantPositionsFromMeshes();
	}
	const video::GLMeshData& meshDataWater = createMesh(shader, mesh.waterMesh);
	if (meshDataWater.noOfIndices > 0) {
		_meshDataWater.push_back(meshDataWater);
	}
}

bool WorldRenderer::checkShaders() const {
	static const std::string pos = "a_pos";
	const int loc1 = _worldShader.getAttributeLocation(pos);
	const int loc2 = _plantShader.getAttributeLocation(pos);
	const int loc3 = _waterShader.getAttributeLocation(pos);
	const int loc4 = _deferredDirLightShader.getAttributeLocation(pos);
	const int loc5 = _shadowMapShader.getAttributeLocation(pos);
	const bool same = loc1 == loc2 && loc2 == loc3 && loc3 == loc4 && loc4 == loc5;
	core_assert_msg(same, "attribute locations for %s differ: %i, %i, %i, %i, %i", pos.c_str(), loc1, loc2, loc3, loc4, loc5);
	return same;
}

void WorldRenderer::cull(GLMeshDatas& meshes, GLMeshesVisible& visible, const video::Camera& camera) const {
	visible.clear();
	int meshesCount = 0;
	int visibleCount = 0;
	const float cullingThreshold = _world->getMeshSize();
	const int maxAllowedDistance = glm::pow(_viewDistance + cullingThreshold, 2);
	for (auto i = meshes.begin(); i != meshes.end();) {
		video::GLMeshData& meshData = *i;
		const int distance = getDistanceSquare(meshData.translation);
		Log::trace("distance is: %i (%i)", distance, maxAllowedDistance);
		if (distance >= maxAllowedDistance) {
			_world->allowReExtraction(meshData.translation);
			meshData.shutdown();
			Log::info("Remove mesh from %i:%i", meshData.translation.x, meshData.translation.z);
			i = meshes.erase(i);
			continue;
		}
		if (camera.isVisible(meshData.aabb)) {
			visible.push_back(&meshData);
			++visibleCount;
		}
		++meshesCount;
		++i;
	}
	Log::trace("%i meshes left after culling, %i meshes overall", visibleCount, meshesCount);
}

void WorldRenderer::setUniforms(video::Shader& shader, const video::Camera& camera) {
	shaderSetUniformIf(shader, setUniformMatrix, "u_viewprojection", camera.viewProjectionMatrix());
	shaderSetUniformIf(shader, setUniformMatrix, "u_view", camera.viewMatrix());
	shaderSetUniformIf(shader, setUniformMatrix, "u_projection", camera.projectionMatrix());
	shaderSetUniformIf(shader, setUniformBuffer, "u_materialblock", _materialBuffer);
	shaderSetUniformIf(shader, setUniformi, "u_texture", 0);
	shaderSetUniformIf(shader, setUniformVec3, "u_fogcolor", _clearColor);
	shaderSetUniformIf(shader, setUniformf, "u_fogrange", _fogRange);
	shaderSetUniformIf(shader, setUniformf, "u_viewdistance", _viewDistance);
	shaderSetUniformIf(shader, setUniformVec3, "u_lightdir", _shadow.sunDirection());
	shaderSetUniformIf(shader, setUniformf, "u_depthsize", glm::vec2(_depthBuffer.dimension()));
	shaderSetUniformIf(shader, setUniformVec3, "u_diffuse_color", _diffuseColor);
	shaderSetUniformIf(shader, setUniformVec3, "u_ambient_color", _ambientColor);
	shaderSetUniformIf(shader, setUniformf, "u_debug_color", 1.0);
	shaderSetUniformIf(shader, setUniformf, "u_screensize", glm::vec2(camera.dimension()));
}

int WorldRenderer::renderWorldMeshes(video::Shader& shader, const GLMeshesVisible& meshes, int* vertices) {
	const bool deferred = _deferred->boolVal();
	const bool shadowMap = shader.hasUniform("u_shadowmap");
	int maxDepthBuffers = 0;
	if (shadowMap) {
		maxDepthBuffers = shader.getUniformArraySize(MaxDepthBufferUniformName);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(std::enum_value(_depthBuffer.textureType()), _depthBuffer.texture());
		shaderSetUniformIf(shader, setUniformi, "u_shadowmap", 1);
	}

	const bool debugGeometry = _debugGeometry->boolVal();
	int drawCallsWorld = 0;
	for (auto i = meshes.begin(); i != meshes.end();) {
		video::GLMeshData* meshData = *i;
		const glm::mat4& translate = glm::translate(glm::mat4(1.0f), glm::vec3(meshData->translation));
		const glm::mat4& model = glm::scale(translate, meshData->scale);
		shaderSetUniformIf(shader, setUniformMatrix, "u_model", model);
		meshData->bindVAO();

		if (debugGeometry && !deferred) {
			shaderSetUniformIf(shader, setUniformf, "u_debug_color", 1.0);
		}
		meshData->draw();
		if (vertices != nullptr) {
			*vertices += meshData->noOfVertices;
		}
		GL_checkError();

		if (debugGeometry && !deferred) {
			video::ScopedPolygonMode polygonMode(video::PolygonMode::WireFrame, glm::vec2(-2.0f));
			video::ScopedLineWidth lineWidth(_lineWidth, true);
			shaderSetUniformIf(shader, setUniformf, "u_debug_color", 0.0);
			glDrawElements(GL_TRIANGLES, meshData->noOfIndices, meshData->indexType, 0);
			GL_checkError();
		}

		++drawCallsWorld;
		++i;
	}

	if (shadowMap) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
	}

	return drawCallsWorld;
}

void WorldRenderer::renderWorldDeferred(const video::Camera& camera, const int width, const int height) {
	_gbuffer.bindForReading(false);
	shader::DeferredLightDirShader& deferredShader = _deferredDirLightShader;
	video::ScopedShader scoped(deferredShader);
	shaderSetUniformIf(deferredShader, setUniformVec3, "u_lightdir", _shadow.sunDirection());
	shaderSetUniformIf(deferredShader, setUniformVec3, "u_diffuse_color", _diffuseColor);
	shaderSetUniformIf(deferredShader, setUniformVec3, "u_ambient_color", _ambientColor);
	shaderSetUniformIf(deferredShader, setUniformi, "u_pos", video::GBuffer::GBUFFER_TEXTURE_TYPE_POSITION);
	shaderSetUniformIf(deferredShader, setUniformi, "u_color", video::GBuffer::GBUFFER_TEXTURE_TYPE_DIFFUSE);
	shaderSetUniformIf(deferredShader, setUniformi, "u_norm", video::GBuffer::GBUFFER_TEXTURE_TYPE_NORMAL);
	shaderSetUniformIf(deferredShader, setUniformVec2, "u_screensize", glm::vec2(width, height));
	core_assert_always(_fullscreenQuad.bind());
	glDrawArrays(GL_TRIANGLES, 0, _fullscreenQuad.elements(0));
	_fullscreenQuad.unbind();
	_gbuffer.unbind();
}

int WorldRenderer::renderWorld(const video::Camera& camera, int* vertices) {
	handleMeshQueue(_worldShader);

	if (_meshDataOpaque.empty()) {
		return 0;
	}

	core_assert_msg(checkShaders(), "Shader attributes don't have the same order");

	core_trace_gl_scoped(WorldRendererRenderWorld);
	int drawCallsWorld = 0;

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LEQUAL);
	// Cull triangles whose normal is not towards the camera
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	GL_checkError();

	cull(_meshDataOpaque, _visibleOpaque, camera);
	cull(_meshDataWater, _visibleWater, camera);
	_visiblePlant.clear();
	for (auto i = _meshDataPlant.begin(); i != _meshDataPlant.end(); ++i) {
		_visiblePlant.push_back(&*i);
	}

	const bool shadowMap = _shadowMap->boolVal();
	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);

	_shadow.calculateShadowData(camera, shadowMap, maxDepthBuffers, _depthBuffer.dimension());
	const std::vector<glm::mat4>& cascades = _shadow.cascades();
	const std::vector<float>& distances = _shadow.distances();
	if (shadowMap) {
		glDisable(GL_BLEND);
		// put shadow acne into the dark
		glCullFace(GL_FRONT);
		glEnable(GL_POLYGON_OFFSET_FILL);
		const float shadowBiasSlope = 2;
		const float shadowBias = 0.09f;
		const float shadowRangeZ = camera.farPlane() * 3.0f;
		glPolygonOffset(shadowBiasSlope, (shadowBias / shadowRangeZ) * (1 << 24));

		_depthBuffer.bind();
		for (int i = 0; i < maxDepthBuffers; ++i) {
			_depthBuffer.bindTexture(i);
			{
				video::ScopedShader scoped(_shadowMapShader);
				setUniforms(_shadowMapShader, camera);
				_shadowMapShader.setLightviewprojection(cascades[i]);
				drawCallsWorld += renderWorldMeshes(_shadowMapShader, _visibleOpaque, vertices);
			}
			{
				video::ScopedShader scoped(_shadowMapInstancedShader);
				setUniforms(_shadowMapInstancedShader, camera);
				_shadowMapInstancedShader.setLightviewprojection(cascades[i]);
				drawCallsWorld += renderWorldMeshes(_shadowMapInstancedShader, _visiblePlant, vertices);
			}
		}
		_depthBuffer.unbind();
		glCullFace(GL_BACK);
		glEnable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	_colorTexture->bind(0);

	const bool deferred = _deferred->boolVal();
	if (deferred) {
		_gbuffer.bindForWriting();
		glDisable(GL_BLEND);
	}

	glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	{
		video::ScopedShader scoped(_worldShader);
		setUniforms(_worldShader, camera);
		shaderSetUniformIf(_worldShader, setUniformMatrixv, "u_cascades", &cascades.front(), maxDepthBuffers);
		shaderSetUniformIf(_worldShader, setUniformfv, "u_distances", &distances.front(), maxDepthBuffers, maxDepthBuffers);
		drawCallsWorld += renderWorldMeshes(_worldShader, _visibleOpaque, vertices);
	}
	{
		video::ScopedShader scoped(_plantShader);
		setUniforms(_plantShader, camera);
		shaderSetUniformIf(_plantShader, setUniformMatrixv, "u_cascades", &cascades.front(), maxDepthBuffers);
		shaderSetUniformIf(_plantShader, setUniformfv, "u_distances", &distances.front(), maxDepthBuffers, maxDepthBuffers);
		drawCallsWorld += renderWorldMeshes(_plantShader, _visiblePlant, vertices);
	}
	{
		video::ScopedShader scoped(_waterShader);
		setUniforms(_waterShader, camera);
		shaderSetUniformIf(_waterShader, setUniformMatrixv, "u_cascades", &cascades.front(), maxDepthBuffers);
		shaderSetUniformIf(_waterShader, setUniformfv, "u_distances", &distances.front(), maxDepthBuffers, maxDepthBuffers);
		drawCallsWorld += renderWorldMeshes(_waterShader, _visibleWater, vertices);
	}

	_colorTexture->unbind();

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (deferred) {
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);

		const int width = camera.width();
		const int height = camera.height();
		if (_deferredDebug->boolVal()) {
			// show the gbuffer buffers
			_gbuffer.bindForReading(true);
			GL_checkError();

			const GLsizei halfWidth = (GLsizei) (width / 2.0f);
			const GLsizei halfHeight = (GLsizei) (height / 2.0f);

			_gbuffer.setReadBuffer(video::GBuffer::GBUFFER_TEXTURE_TYPE_POSITION);
			glBlitFramebuffer(0, 0, width, height, 0, 0, halfWidth, halfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			_gbuffer.setReadBuffer(video::GBuffer::GBUFFER_TEXTURE_TYPE_DIFFUSE);
			glBlitFramebuffer(0, 0, width, height, 0, halfHeight, halfWidth, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			_gbuffer.setReadBuffer(video::GBuffer::GBUFFER_TEXTURE_TYPE_NORMAL);
			glBlitFramebuffer(0, 0, width, height, halfWidth, halfHeight, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			video::ScopedViewPort scoped(halfWidth, 0, halfWidth, halfHeight);
			renderWorldDeferred(camera, halfWidth, halfHeight);
		} else {
			renderWorldDeferred(camera, width, height);
		}

		GL_checkError();
	}

	if (shadowMap && _shadowMapDebug->boolVal()) {
		const int width = camera.width();
		const int height = camera.height();

		// activate shader
		video::ScopedShader scopedShader(_shadowMapDebugShader);
		_shadowMapDebugShader.setShadowmap(0);
		_shadowMapDebugShader.setFar(camera.farPlane());
		_shadowMapDebugShader.setNear(camera.nearPlane());

		// bind buffers
		core_assert_always(_shadowMapDebugBuffer.bind());

		// configure shadow map texture
		glActiveTexture(GL_TEXTURE0);
		const GLenum glTextureType = std::enum_value(_depthBuffer.textureType());
		glBindTexture(glTextureType, _depthBuffer.texture());
		if (_depthBuffer.depthCompare()) {
			glTexParameteri(glTextureType, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		}

		// render shadow maps
		for (int i = 0; i < maxDepthBuffers; ++i) {
			const GLsizei halfWidth = (GLsizei) (width / 4.0f);
			const GLsizei halfHeight = (GLsizei) (height / 4.0f);
			video::ScopedViewPort scopedViewport(i * halfWidth, 0, halfWidth, halfHeight);
			_shadowMapDebugShader.setCascade(i);
			glDrawArrays(GL_TRIANGLES, 0, _shadowMapDebugBuffer.elements(0));
		}

		// restore texture
		if (_depthBuffer.depthCompare()) {
			glTexParameteri(glTextureType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		}
		glBindTexture(glTextureType, 0);

		// unbind buffer
		_shadowMapDebugBuffer.unbind();
	}

	GL_checkError();
	return drawCallsWorld;
}

int WorldRenderer::renderEntities(const video::Camera& camera) {
	if (_entities.empty()) {
		return 0;
	}
	core_trace_gl_scoped(WorldRendererRenderEntities);

	int drawCallsEntities = 0;

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	// TODO: deferred rendering
	shader::MeshShader& shader = _meshShader;
	video::ScopedShader scoped(shader);
	setUniforms(shader, camera);
	const bool shadowMap = shader.hasUniform("u_shadowmap");
	int maxDepthBuffers = 0;
	if (shadowMap) {
		maxDepthBuffers = shader.getUniformArraySize(MaxDepthBufferUniformName);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(std::enum_value(_depthBuffer.textureType()), _depthBuffer.texture());
		shaderSetUniformIf(shader, setUniformi, "u_shadowmap", 1);
	}
	for (const auto& e : _entities) {
		const frontend::ClientEntityPtr& ent = e.second;
		ent->update(_deltaFrame);
		if (!camera.isVisible(ent->position())) {
			continue;
		}
		const video::MeshPtr& mesh = ent->mesh();
		if (!mesh->initMesh(shader)) {
			continue;
		}
		const glm::mat4& rotate = glm::rotate(glm::mat4(1.0f), ent->orientation(), glm::up);
		const glm::mat4& translate = glm::translate(rotate, ent->position());
		const glm::mat4& scale = glm::scale(translate, glm::vec3(ent->scale()));
		const glm::mat4& model = scale;
		shader.setUniformMatrix("u_model", model);
		drawCallsEntities += mesh->render();
		GL_checkError();
	}

	if (shadowMap) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_checkError();
	return drawCallsEntities;
}

void WorldRenderer::updateMesh(const voxel::Mesh& surfaceMesh, video::GLMeshData& meshData) {
	core_trace_gl_scoped(WorldRendererUpdateMesh);
	const voxel::IndexType* vecIndices = surfaceMesh.getRawIndexData();
	const uint32_t numIndices = surfaceMesh.getNoOfIndices();
	const voxel::VoxelVertex* vecVertices = surfaceMesh.getRawVertexData();
	const uint32_t numVertices = surfaceMesh.getNoOfVertices();

	core_assert(meshData.vertexBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(voxel::VoxelVertex), vecVertices, GL_STATIC_DRAW);

	core_assert(meshData.indexBuffer > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(voxel::IndexType), vecIndices, GL_STATIC_DRAW);

	meshData.noOfVertices = numVertices;
	meshData.noOfIndices = numIndices;
}

video::GLMeshData WorldRenderer::createMeshInternal(const video::Shader& shader, const voxel::Mesh &mesh, int buffers) {
	core_trace_gl_scoped(WorldRendererCreateMesh);

	// This struct holds the OpenGL properties (buffer handles, etc) which will be used
	// to render our mesh. We copy the data from the PolyVox mesh into this structure.
	video::GLMeshData meshData;
	meshData.translation = mesh.getOffset();

	const float chunkSize = (float)_world->getMeshSize();
	const glm::vec3& mins = glm::vec3(meshData.translation);
	meshData.aabb = core::AABB<float>(mins, mins + chunkSize);
	meshData.scale = glm::vec3(1.0f);

	static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
	meshData.indexType = GL_UNSIGNED_INT;

	if (mesh.getNoOfIndices() == 0) {
		return meshData;
	}

	meshData.create(buffers);
	meshData.bindVAO();

	updateMesh(mesh, meshData);

	const int posLoc = shader.enableVertexAttributeArray("a_pos");
	const int components = sizeof(voxel::VoxelVertex::position) / sizeof(decltype(voxel::VoxelVertex::position)::value_type);
	shader.setVertexAttributeInt(posLoc, components, GL_UNSIGNED_BYTE, sizeof(voxel::VoxelVertex), GL_OFFSET_CAST(offsetof(voxel::VoxelVertex, position)));

	const int locationInfo = shader.enableVertexAttributeArray("a_info");
	// we are uploading two bytes at once here
	static_assert(sizeof(voxel::VoxelVertex::colorIndex) == sizeof(uint8_t), "Voxel color size doesn't match");
	static_assert(sizeof(voxel::VoxelVertex::ambientOcclusion) == sizeof(uint8_t), "AO type size doesn't match");
	static_assert(sizeof(voxel::VoxelVertex::material) == sizeof(uint8_t), "Material type size doesn't match");
	static_assert(offsetof(voxel::VoxelVertex, ambientOcclusion) < offsetof(voxel::VoxelVertex, colorIndex), "Layout change of VoxelVertex without change in upload");
	static_assert(offsetof(voxel::VoxelVertex, ambientOcclusion) < offsetof(voxel::VoxelVertex, material), "Layout change of VoxelVertex without change in upload");
	shader.setVertexAttributeInt(locationInfo, 3, GL_UNSIGNED_BYTE, sizeof(voxel::VoxelVertex), GL_OFFSET_CAST(offsetof(voxel::VoxelVertex, ambientOcclusion)));
	GL_checkError();

	return meshData;
}

// TODO: generate bigger buffers and use glBufferSubData
video::GLMeshData WorldRenderer::createMesh(const video::Shader& shader, const voxel::Mesh &mesh) {
	const video::GLMeshData& meshData = createMeshInternal(shader, mesh, 2);
	if (mesh.getNoOfIndices() == 0) {
		return meshData;
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return meshData;
}

video::GLMeshData WorldRenderer::createInstancedMesh(const video::Shader& shader, const voxel::Mesh &mesh, int amount) {
	video::GLMeshData meshData = createMeshInternal(shader, mesh, 3);
	if (mesh.getNoOfIndices() == 0) {
		return meshData;
	}

	meshData.amount = amount;

	core_assert(meshData.offsetBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.offsetBuffer);

	const int offsetLoc = shader.enableVertexAttributeArray("a_offset");
	const int components = sizeof(glm::vec3) / sizeof(glm::vec3::value_type);
	shader.setVertexAttribute(offsetLoc, components, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), GL_OFFSET_CAST(offsetof(glm::vec3, x)));
	glVertexAttribDivisor(offsetLoc, 1);
	GL_checkError();

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return meshData;
}

void WorldRenderer::onSpawn(const glm::vec3& pos, int initialExtractionRadius) {
	core_trace_scoped(WorldRendererOnSpawn);
	_viewDistance = 1.0f;
	extractMeshAroundCamera(_world->getMeshPos(pos), initialExtractionRadius);
}

bool WorldRenderer::extractNewMeshes(const glm::vec3& position, bool force) {
	core_trace_scoped(WorldRendererExtractNewMeshes);
	if (force) {
		_world->allowReExtraction(position);
		return _world->scheduleMeshExtraction(position);
	}
	const glm::ivec3& meshGridPos = _world->getMeshPos(position);
	const glm::ivec3& diff = _lastGridPosition - meshGridPos;
	if (glm::abs(diff.x) >= 1 || glm::abs(diff.y) >= 1 || glm::abs(diff.z) >= 1) {
		const int chunks = MinCullingDistance / _world->getMeshSize() + 1;
		extractMeshAroundCamera(meshGridPos, chunks);
		return true;
	}
	return false;
}

void WorldRenderer::extractMeshAroundCamera(const glm::ivec3& meshGridPos, int radius) {
	core_trace_scoped(WorldRendererExtractAroundCamera);
	const int sideLength = radius * 2 + 1;
	const int amount = sideLength * (sideLength - 1) + sideLength;
	const int meshSize = _world->getMeshSize();
	if (meshGridPos == _lastGridPosition) {
		return;
	}
	Log::debug("set last grid position to %i:%i", meshGridPos.x, meshGridPos.z);
	_lastGridPosition = meshGridPos;
	glm::ivec3 pos = meshGridPos;
	pos.y = 0;
	voxel::Spiral o;
	for (int i = 0; i < amount; ++i) {
		const float distance = getDistanceSquare(pos);
		if (distance <= glm::pow(MinExtractionCullingDistance, 2)) {
			_world->scheduleMeshExtraction(pos);
		}
		o.next();
		pos.x = meshGridPos.x + o.x() * meshSize;
		pos.z = meshGridPos.z + o.z() * meshSize;
	}
}

void WorldRenderer::stats(int& meshes, int& extracted, int& pending, int& active) const {
	_world->stats(meshes, extracted, pending);
	active = _meshDataOpaque.size();
}

bool WorldRenderer::onInit(const glm::ivec2& position, const glm::ivec2& dimension) {
	core_trace_scoped(WorldRendererOnInit);
	_debugGeometry = core::Var::get(cfg::ClientDebugGeometry, "false");
	core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER);
	_deferred = core::Var::getSafe(cfg::ClientDeferred);
	core_assert(_deferred);
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	core_assert(_shadowMap);
	_deferredDebug = core::Var::get(cfg::ClientDebugDeferred, "false");
	_shadowMapDebug = core::Var::get(cfg::ClientDebugShadowMap, "false");

	_noiseFuture.push_back(core::App::getInstance()->threadPool().enqueue([] () {
		const int ColorTextureSize = 256;
		const int ColorTextureOctaves = 2;
		const int ColorTextureDepth = 3;
		uint8_t *colorTexture = new uint8_t[ColorTextureSize * ColorTextureSize * ColorTextureDepth];
		const float persistence = 0.3f;
		const float frequency = 0.7f;
		const float amplitude = 1.0f;
		noise::Simplex::SeamlessNoise2DRGB(colorTexture, ColorTextureSize, ColorTextureOctaves, persistence, frequency, amplitude);
		return NoiseGenerationTask(colorTexture, ColorTextureSize, ColorTextureSize, ColorTextureDepth);
	}));
	_colorTexture = video::createEmptyTexture("**colortexture**");
	_plantGenerator.generateAll();

	if (!_worldShader.setup()) {
		return false;
	}
	if (!_plantShader.setup()) {
		return false;
	}
	if (!_shadowMapInstancedShader.setup()) {
		return false;
	}
	if (!_waterShader.setup()) {
		return false;
	}
	if (!_meshShader.setup()) {
		return false;
	}
	if (!_shadowMapShader.setup()) {
		return false;
	}
	if (!_deferredDirLightShader.setup()) {
		return false;
	}
	if (!_shadowMapDebugShader.setup()) {
		return false;
	}

	const int shaderMaterialColorsArraySize = _worldShader.getUniformArraySize("u_materialcolor");
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	video::VertexBuffer::Attribute attributePosLightDeferred;
	attributePosLightDeferred.bufferIndex = _fullscreenQuad.createFullscreenQuad();
	attributePosLightDeferred.index = _deferredDirLightShader.getLocationPos();
	attributePosLightDeferred.size = _deferredDirLightShader.getComponentsPos();
	_fullscreenQuad.addAttribute(attributePosLightDeferred);

	const glm::ivec2& fullscreenQuadIndices = _shadowMapDebugBuffer.createFullscreenTexturedQuad(true);
	video::VertexBuffer::Attribute attributePos;
	attributePos.bufferIndex = fullscreenQuadIndices.x;
	attributePos.index = _shadowMapDebugShader.getLocationPos();
	attributePos.size = _shadowMapDebugShader.getComponentsPos();
	_shadowMapDebugBuffer.addAttribute(attributePos);

	video::VertexBuffer::Attribute attributeTexcoord;
	attributeTexcoord.bufferIndex = fullscreenQuadIndices.y;
	attributeTexcoord.index = _shadowMapDebugShader.getLocationTexcoord();
	attributeTexcoord.size = _shadowMapDebugShader.getComponentsTexcoord();
	_shadowMapDebugBuffer.addAttribute(attributeTexcoord);

	for (int i = 0; i < voxel::MaxPlantTypes; ++i) {
		voxel::Mesh* mesh = _plantGenerator.getMesh((voxel::PlantType)i);
		video::GLMeshData meshDataPlant = createInstancedMesh(_plantShader, *mesh, 40);
		if (meshDataPlant.noOfIndices > 0) {
			meshDataPlant.scale = glm::vec3(0.4f);
			_meshDataPlant.push_back(meshDataPlant);
		}
	}

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	const glm::ivec2 smSize(core::Var::get(cfg::ClientShadowMapSize, "512")->intVal());
	if (!_depthBuffer.init(smSize, video::DepthBufferMode::DEPTH_CMP, maxDepthBuffers)) {
		return false;
	}

	if (!_shadow.init()) {
		return false;
	}

	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	_materialBuffer.create(materialColors.size() * sizeof(voxel::MaterialColorArray::value_type), &materialColors.front());

	if (!_gbuffer.init(dimension)) {
		return false;
	}

	GLdouble buf[2];
	glGetDoublev(GL_SMOOTH_LINE_WIDTH_RANGE, buf);
	_lineWidth = std::min((float)buf[1], _lineWidth);

	return true;
}

void WorldRenderer::onRunning(long dt) {
	core_trace_scoped(WorldRendererOnRunning);
	_now += dt;
	_deltaFrame = dt;
	if (!_noiseFuture.empty()) {
		NoiseFuture& future = _noiseFuture.back();
		if (future.valid()) {
			NoiseGenerationTask c = future.get();
			Log::info("Noise texture ready - upload it");
			video::TextureFormat format;
			if (c.depth == 4) {
				format = video::TextureFormat::RGBA;
			} else {
				format = video::TextureFormat::RGB;
			}
			_colorTexture->upload(format, c.width, c.height, c.buffer);
			delete[] c.buffer;
			_noiseFuture.pop_back();
		}
	}

	if (_viewDistance < MinCullingDistance) {
		const float advance = _world->getMeshSize() * (dt / 1000.0f);
		_viewDistance += advance;
	}
}

int WorldRenderer::getDistanceSquare(const glm::ivec3& pos) const {
	const glm::ivec3 dist = pos - _lastGridPosition;
	const int distance = dist.x * dist.x + dist.z * dist.z;
	return distance;
}

}
