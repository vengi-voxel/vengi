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
#include "frontend/ShaderAttribute.h"

constexpr int MinCullingDistance = 500;
constexpr int MinExtractionCullingDistance = 1000;

namespace frontend {

const std::string MaxDepthBufferUniformName = "u_cascades";

// TODO convert to VertexBuffer
// TODO: merge buffers into one big buffer (and if max vertex/index size exceeds, render in chunks)
//       all available buffers should be in there. we should just assemble a list of drawcall parameters
//       for glMultiDrawElementsIndirect as shown at
//       https://www.khronos.org/opengl/wiki/GLAPI/glMultiDrawElementsIndirect
WorldRenderer::WorldRenderer(const voxel::WorldPtr& world) :
		_clearColor(core::Color::LightBlue), _world(world) {
}

WorldRenderer::~WorldRenderer() {
}

void WorldRenderer::reset() {
	for (RendererChunkMeshData& meshData : _meshChunkList) {
		meshData.opaque.shutdown();
		meshData.water.shutdown();
	}
	_meshChunkList.clear();
	_entities.clear();
	_viewDistance = 1.0f;
	_now = 0l;
}

void WorldRenderer::shutdown() {
	_gbuffer.shutdown();
	_worldBuffer.shutdown();
	_worldInstancedBuffer.shutdown();
	_fullscreenQuad.shutdown();
	_shadowMapDebugBuffer.shutdown();
	_shadowMapDebugShader.shutdown();
	_shadowMapInstancedShader.shutdown();
	_worldShader.shutdown();
	_worldInstancedShader.shutdown();
	_waterShader.shutdown();
	_meshShader.shutdown();
	_shadowMapShader.shutdown();
	_deferredDirLightShader.shutdown();
	_depthBuffer.shutdown();
	reset();
	_colorTexture->shutdown();
	_colorTexture = video::TexturePtr();
	_entities.clear();

	for (video::GLMeshData& meshData : _meshPlantList) {
		meshData.shutdown();
	}
	_meshPlantList.clear();
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

bool WorldRenderer::removeEntity(ClientEntityId id) {
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		return false;
	}
	_entities.erase(i);
	return true;
}

void WorldRenderer::fillPlantPositionsFromMeshes() {
	const int plantMeshAmount = _meshPlantList.size();
	if (plantMeshAmount == 0) {
		return;
	}
	for (video::GLMeshData& mp : _meshPlantList) {
		mp.instancedPositions.clear();
	}
	for (const RendererChunkMeshData& data : _meshChunkList) {
		if (!data.inuse) {
			continue;
		}
		if (data.opaque.instancedPositions.empty()) {
			continue;
		}
		std::vector<glm::vec3> p = data.opaque.instancedPositions;
		core::Random rnd(_world->seed() + data.opaque.translation.x + data.opaque.translation.y + data.opaque.translation.z);
		rnd.shuffle(p.begin(), p.end());
		const int plantMeshes = p.size() / plantMeshAmount;
		int delta = p.size() - plantMeshes * plantMeshAmount;
		for (video::GLMeshData& mp : _meshPlantList) {
			auto it = std::next(p.begin(), plantMeshes + delta);
			std::move(p.begin(), it, std::back_inserter(mp.instancedPositions));
			p.erase(p.begin(), it);
			delta = 0;
		}
	}
}

void WorldRenderer::updateAABB(RendererChunkMeshData& meshData) const {
	glm::ivec3 mins(std::numeric_limits<int>::max());
	glm::ivec3 maxs(std::numeric_limits<int>::min());

	const glm::ivec3& positionOffset = meshData.translation();
	for (auto& v : meshData.voxelMeshes.opaqueMesh.getVertexVector()) {
		const glm::ivec3 p = v.position + positionOffset;
		mins = glm::min(mins, p);
		maxs = glm::max(maxs, p);
	}
	for (auto& v : meshData.voxelMeshes.waterMesh.getVertexVector()) {
		const glm::ivec3 p = v.position + positionOffset;
		mins = glm::min(mins, p);
		maxs = glm::max(maxs, p);
	}

	meshData.aabb = core::AABB<float>(mins, maxs);
}

void WorldRenderer::handleMeshQueue() {
	voxel::ChunkMeshData mesh(0, 0, 0, 0);
	if (!_world->pop(mesh)) {
		return;
	}
	// Now add the mesh to the list of meshes to render.
	core_trace_gl_scoped(WorldRendererHandleMeshQueue);

	const int plantAmount = 100;
	// first check whether we update an existing one
	for (RendererChunkMeshData& meshData : _meshChunkList) {
		if (meshData.translation() != mesh.translation()) {
			continue;
		}
		meshData.inuse = true;
		meshData.voxelMeshes = std::move(mesh);
		updateMesh(meshData.voxelMeshes.opaqueMesh, meshData.opaque);
		updateMesh(meshData.voxelMeshes.waterMesh, meshData.water);
		updateAABB(meshData);
		distributePlants(_world, plantAmount, meshData.translation(), meshData.opaque.instancedPositions);
		fillPlantPositionsFromMeshes();
		return;
	}

	// then check if there are unused buffers
	for (RendererChunkMeshData& meshData : _meshChunkList) {
		if (meshData.inuse) {
			continue;
		}
		meshData.inuse = true;
		meshData.voxelMeshes = std::move(mesh);
		updateMesh(meshData.voxelMeshes.opaqueMesh, meshData.opaque);
		updateMesh(meshData.voxelMeshes.waterMesh, meshData.water);
		updateAABB(meshData);

		distributePlants(_world, plantAmount, meshData.translation(), meshData.opaque.instancedPositions);
		fillPlantPositionsFromMeshes();
		return;
	}

	// create a new mesh
	RendererChunkMeshData meshData;
	if (createMesh(mesh, meshData)) {
		meshData.voxelMeshes = std::move(mesh);
		updateAABB(meshData);
		_meshChunkList.push_back(meshData);
		Log::info("Meshes so far: %i", (int)_meshChunkList.size());
		distributePlants(_world, plantAmount, meshData.translation(), meshData.opaque.instancedPositions);
		fillPlantPositionsFromMeshes();
	}
}

bool WorldRenderer::checkShaders() const {
	const int loc1 = _worldShader.getLocationPos();
	const int loc2 = _worldInstancedShader.getLocationPos();
	const int loc3 = _waterShader.getLocationPos();
	const int loc4 = _deferredDirLightShader.getLocationPos();
	const int loc5 = _shadowMapShader.getLocationPos();
	const bool same = loc1 == loc2 && loc2 == loc3 && loc3 == loc4 && loc4 == loc5;
	core_assert_msg(same, "attribute locations for a_pos differ: %i, %i, %i, %i, %i", loc1, loc2, loc3, loc4, loc5);
	return same;
}

void WorldRenderer::cull(const video::Camera& camera) {
	_visible.clear();
	_visibleWater.clear();
	const float cullingThreshold = _world->getMeshSize();
	const int maxAllowedDistance = glm::pow(_viewDistance + cullingThreshold, 2);
	for (RendererChunkMeshData& meshData : _meshChunkList) {
		if (!meshData.inuse) {
			continue;
		}
		const int distance = getDistanceSquare(meshData.translation());
		Log::trace("distance is: %i (%i)", distance, maxAllowedDistance);
		if (distance >= maxAllowedDistance) {
			_world->allowReExtraction(meshData.translation());
			meshData.inuse = false;
			Log::info("Remove mesh from %i:%i", meshData.translation().x, meshData.translation().z);
			continue;
		}
		if (camera.isVisible(meshData.aabb)) {
			_visible.push_back(&meshData.opaque);
			_visibleWater.push_back(&meshData.water);
		}
	}
	Log::trace("%i meshes left after culling, %i meshes overall", (int)_visible.size(), (int)_meshChunkList.size());
}

void WorldRenderer::setUniforms(video::Shader& shader, const video::Camera& camera) {
	shaderSetUniformIf(shader, setUniformMatrix, "u_viewprojection", camera.viewProjectionMatrix());
	shaderSetUniformIf(shader, setUniformMatrix, "u_view", camera.viewMatrix());
	shaderSetUniformIf(shader, setUniformMatrix, "u_projection", camera.projectionMatrix());
	shaderSetUniformIf(shader, setUniformf, "u_viewdistance", _viewDistance);
	shaderSetUniformIf(shader, setUniformVec3, "u_lightdir", _shadow.sunDirection());
	shaderSetUniformIf(shader, setUniformf, "u_screensize", glm::vec2(camera.dimension()));
}

int WorldRenderer::renderWorldMeshes(video::Shader& shader, const RendererMeshVisibleList& meshes, int* vertices) {
	for (const video::GLMeshData* meshData : meshes) {
		shaderSetUniformIf(shader, setUniformMatrix, "u_model", meshData->model);
		meshData->bindVAO();
		meshData->draw();
		if (vertices != nullptr) {
			*vertices += meshData->noOfVertices;
		}
	}

	GL_checkError();
	return meshes.size();
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
	handleMeshQueue();

	if (vertices != nullptr) {
		*vertices = 0;
	}
	if (_meshChunkList.empty()) {
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

	cull(camera);
	_visiblePlant.clear();
	for (auto i = _meshPlantList.begin(); i != _meshPlantList.end(); ++i) {
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
				drawCallsWorld += renderWorldMeshes(_shadowMapShader, _visible, nullptr);
			}
			{
				video::ScopedShader scoped(_shadowMapInstancedShader);
				setUniforms(_shadowMapInstancedShader, camera);
				_shadowMapInstancedShader.setLightviewprojection(cascades[i]);
				drawCallsWorld += renderWorldMeshes(_shadowMapInstancedShader, _visiblePlant, nullptr);
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

	if (shadowMap) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(std::enum_value(_depthBuffer.textureType()), _depthBuffer.texture());
	}

	{
		video::ScopedShader scoped(_worldShader);
		setUniforms(_worldShader, camera);
		_worldShader.setCascades(cascades);
		_worldShader.setDistances(distances);
		_worldShader.setShadowmap(1);
		drawCallsWorld += renderWorldMeshes(_worldShader, _visible, vertices);
	}
	{
		video::ScopedShader scoped(_worldInstancedShader);
		setUniforms(_worldInstancedShader, camera);
		_worldInstancedShader.setCascades(cascades);
		_worldInstancedShader.setDistances(distances);
		_worldInstancedShader.setShadowmap(1);
		drawCallsWorld += renderWorldMeshes(_worldInstancedShader, _visiblePlant, vertices);
	}
	{
		video::ScopedShader scoped(_waterShader);
		setUniforms(_waterShader, camera);
		shaderSetUniformIf(_waterShader, setUniformMatrixv, "u_cascades", &cascades.front(), maxDepthBuffers);
		shaderSetUniformIf(_waterShader, setUniformfv, "u_distances", &distances.front(), maxDepthBuffers, maxDepthBuffers);
		_waterShader.setShadowmap(1);
		drawCallsWorld += renderWorldMeshes(_waterShader, _visibleWater, vertices);
	}

	if (shadowMap) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
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
	if (shadowMap) {
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

void WorldRenderer::updateMesh(const voxel::Mesh& mesh, video::GLMeshData& meshData) const {
	core_trace_gl_scoped(WorldRendererUpdateMesh);
	const voxel::IndexType* vecIndices = mesh.getRawIndexData();
	const uint32_t numIndices = mesh.getNoOfIndices();
	const voxel::VoxelVertex* vecVertices = mesh.getRawVertexData();
	const uint32_t numVertices = mesh.getNoOfVertices();

	core_assert(meshData.vertexBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(voxel::VoxelVertex), vecVertices, GL_STATIC_DRAW);

	core_assert(meshData.indexBuffer > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(voxel::IndexType), vecIndices, GL_STATIC_DRAW);

	meshData.noOfVertices = numVertices;
	meshData.noOfIndices = numIndices;
	static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
	meshData.indexType = GL_UNSIGNED_INT;

	meshData.scale = glm::vec3(1.0f);
	meshData.translation = mesh.getOffset();
	const glm::mat4& translate = glm::translate(glm::mat4(1.0f), glm::vec3(meshData.translation));
	meshData.model = glm::scale(translate, meshData.scale);
}

bool WorldRenderer::createMeshInternal(const video::Shader& shader, const voxel::Mesh &mesh, int buffers, video::GLMeshData& meshData) {
	core_trace_gl_scoped(WorldRendererCreateMesh);
	if (mesh.getNoOfIndices() == 0) {
		return false;
	}

	meshData.create(buffers);
	meshData.bindVAO();

	updateMesh(mesh, meshData);

	const int posLoc = shader.enableVertexAttributeArray("a_pos");
	const video::VertexBuffer::Attribute& posAttrib = getPositionVertexAttribute(0, posLoc, shader.getAttributeComponents(posLoc));
	shader.setVertexAttributeInt(posLoc, posAttrib.size, posAttrib.type, posAttrib.stride, GL_OFFSET_CAST(posAttrib.offset));

	const int locationInfo = shader.enableVertexAttributeArray("a_info");
	const video::VertexBuffer::Attribute& infoAttrib = getInfoVertexAttribute(0, locationInfo, shader.getAttributeComponents(locationInfo));
	shader.setVertexAttributeInt(locationInfo, infoAttrib.size, infoAttrib.type, infoAttrib.stride, GL_OFFSET_CAST(infoAttrib.offset));
	GL_checkError();

	return true;
}

bool WorldRenderer::createMesh(const voxel::ChunkMeshData &mesh, RendererChunkMeshData& meshData) {
	if (!createMeshInternal(_worldShader, mesh.opaqueMesh, 2, meshData.opaque)) {
		return false;
	}

	createMeshInternal(_worldShader, mesh.waterMesh, 2, meshData.water);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
}

bool WorldRenderer::createInstancedMesh(const voxel::Mesh &mesh, int amount, video::GLMeshData& meshData) {
	if (!createMeshInternal(_worldInstancedShader, mesh, 3, meshData)) {
		return false;
	}

	meshData.amount = amount;

	core_assert(meshData.offsetBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.offsetBuffer);

	_worldInstancedShader.initOffset();
	_worldInstancedShader.setOffsetDivisor(1);
	GL_checkError();

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
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
	active = _meshChunkList.size();
}

void WorldRenderer::onConstruct() {
	core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER);
	_deferred = core::Var::getSafe(cfg::ClientDeferred);
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_deferredDebug = core::Var::get(cfg::ClientDebugDeferred, "false");
	_shadowMapDebug = core::Var::get(cfg::ClientDebugShadowMap, "false");
	core::Var::get(cfg::ClientShadowMapSize, "512");
}

bool WorldRenderer::onInit(const glm::ivec2& position, const glm::ivec2& dimension) {
	core_trace_scoped(WorldRendererOnInit);

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
	if (!_worldInstancedShader.setup()) {
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

	_worldIndexBufferIndex = _worldBuffer.create(nullptr, 0, video::VertexBufferType::IndexBuffer);
	if (_worldIndexBufferIndex == -1) {
		Log::error("Could not create the world vertex buffer object for the indices");
		return false;
	}

	_worldIndexBuffer = _worldBuffer.create();
	if (_worldIndexBuffer == -1) {
		Log::error("Could not create the world vertex buffer object");
		return false;
	}

	_worldInstancedIndexBufferIndex = _worldInstancedBuffer.create(nullptr, 0, video::VertexBufferType::IndexBuffer);
	if (_worldInstancedIndexBufferIndex == -1) {
		Log::error("Could not create the instanced world vertex buffer object for the indices");
		return false;
	}

	_worldInstancedBufferIndex = _worldInstancedBuffer.create();
	if (_worldInstancedBufferIndex == -1) {
		Log::error("Could not create the instanced world vertex buffer object");
		return false;
	}

	_worldInstancedOffsetBufferIndex = _worldInstancedBuffer.create();
	if (_worldInstancedOffsetBufferIndex == -1) {
		Log::error("Could not create the instanced world vertex buffer object for the offsets");
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

	video::VertexBuffer::Attribute voxelAttributePos = getPositionVertexAttribute(_worldIndexBuffer, _worldShader.getLocationPos(), _worldShader.getComponentsPos());
	_worldBuffer.addAttribute(voxelAttributePos);

	video::VertexBuffer::Attribute voxelAttributeInfo = getInfoVertexAttribute(voxelAttributePos.bufferIndex, _worldShader.getLocationInfo(), _worldShader.getComponentsInfo());
	_worldBuffer.addAttribute(voxelAttributeInfo);

	voxelAttributePos.bufferIndex = _worldInstancedBufferIndex;
	_worldInstancedBuffer.addAttribute(voxelAttributePos);

	voxelAttributeInfo.bufferIndex = voxelAttributePos.bufferIndex;
	_worldInstancedBuffer.addAttribute(voxelAttributeInfo);

	video::VertexBuffer::Attribute voxelAttributeOffsets = getOffsetVertexAttribute(_worldInstancedOffsetBufferIndex,
					_worldShader.getLocationOffset(),
					_worldShader.getComponentsOffset());
	_worldInstancedBuffer.addAttribute(voxelAttributeOffsets);

	for (int i = 0; i < (int)voxel::PlantType::MaxPlantTypes; ++i) {
		const voxel::Mesh* mesh = _plantGenerator.getMesh((voxel::PlantType)i);
		video::GLMeshData meshDataPlant;
		if (createInstancedMesh(*mesh, 40, meshDataPlant)) {
			meshDataPlant.scale = glm::vec3(0.4f);
			_meshPlantList.push_back(meshDataPlant);
		}
	}

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	if (!_depthBuffer.init(smSize, video::DepthBufferMode::DEPTH_CMP, maxDepthBuffers)) {
		return false;
	}

	const int shaderMaterialColorsArraySize = SDL_arraysize(shader::WorldShader::Materialblock::materialcolor);
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}
	{
		shader::WorldShader::Materialblock materialBlock;
		memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
		_worldShader.updateMaterialblock(materialBlock);
		video::ScopedShader scoped(_worldShader);
		_worldShader.setMaterialblock();
		_worldShader.setFogcolor(_clearColor);
		_worldShader.setTexture(0);
		_worldShader.setDiffuseColor(_diffuseColor);
		_worldShader.setAmbientColor(_ambientColor);
		_worldShader.setFogrange(_fogRange);
		_worldShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
	}
	{
		shader::WorldInstancedShader::Materialblock materialBlock;
		memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
		_worldInstancedShader.updateMaterialblock(materialBlock);
		video::ScopedShader scoped(_worldInstancedShader);
		_worldInstancedShader.setMaterialblock();
		_worldInstancedShader.setFogcolor(_clearColor);
		_worldInstancedShader.setTexture(0);
		_worldInstancedShader.setDiffuseColor(_diffuseColor);
		_worldInstancedShader.setAmbientColor(_ambientColor);
		_worldInstancedShader.setFogrange(_fogRange);
		_worldInstancedShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
	}

	if (!_shadow.init()) {
		return false;
	}

	if (!_gbuffer.init(dimension)) {
		return false;
	}

	// TODO: use limits from windowed app
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
			Log::trace("Noise texture ready - upload it");
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
