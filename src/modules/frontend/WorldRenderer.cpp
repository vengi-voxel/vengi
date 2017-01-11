/**
 * @file
 */

#include "WorldRenderer.h"
#include "core/Color.h"
#include "video/GLFunc.h"
#include "voxel/Spiral.h"
#include "core/App.h"
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
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		chunkBuffer.opaque.shutdown();
		chunkBuffer.water.shutdown();
	}
	_chunkBuffers.clear();
	_entities.clear();
	_viewDistance = 1.0f;
	_now = 0l;
}

void WorldRenderer::shutdown() {
	_worldBuffer.shutdown();
	_worldInstancedBuffer.shutdown();
	_shadowMapDebugBuffer.shutdown();
	_shadowMapRenderShader.shutdown();
	_shadowMapInstancedShader.shutdown();
	_worldShader.shutdown();
	_worldInstancedShader.shutdown();
	_waterShader.shutdown();
	_meshShader.shutdown();
	_shadowMapShader.shutdown();
	_depthBuffer.shutdown();
	_materialBlock.shutdown();
	reset();
	_colorTexture.shutdown();
	_entities.clear();

	for (video::GLMeshData& meshData : _meshPlantList) {
		meshData.shutdown();
	}
	_meshPlantList.clear();
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
	for (const ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (!chunkBuffer.inuse) {
			continue;
		}
		if (chunkBuffer.opaque.instancedPositions.empty()) {
			continue;
		}
		std::vector<glm::vec3> p = chunkBuffer.opaque.instancedPositions;
		core::Random rnd(_world->seed() + chunkBuffer.translation().x + chunkBuffer.translation().y + chunkBuffer.translation().z);
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

void WorldRenderer::updateAABB(ChunkBuffer& meshData) const {
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
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (chunkBuffer.translation() != mesh.translation()) {
			continue;
		}
		chunkBuffer.inuse = true;
		chunkBuffer.voxelMeshes = std::move(mesh);
		updateVertexBuffer(chunkBuffer.voxelMeshes.opaqueMesh, chunkBuffer.opaque);
		updateVertexBuffer(chunkBuffer.voxelMeshes.waterMesh, chunkBuffer.water);
		updateAABB(chunkBuffer);
		distributePlants(_world, plantAmount, chunkBuffer.translation(), chunkBuffer.opaque.instancedPositions);
		fillPlantPositionsFromMeshes();
		return;
	}

	// then check if there are unused buffers
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (chunkBuffer.inuse) {
			continue;
		}
		chunkBuffer.inuse = true;
		chunkBuffer.voxelMeshes = std::move(mesh);
		updateVertexBuffer(chunkBuffer.voxelMeshes.opaqueMesh, chunkBuffer.opaque);
		updateVertexBuffer(chunkBuffer.voxelMeshes.waterMesh, chunkBuffer.water);
		updateAABB(chunkBuffer);

		distributePlants(_world, plantAmount, chunkBuffer.translation(), chunkBuffer.opaque.instancedPositions);
		fillPlantPositionsFromMeshes();
		return;
	}

	// create a new mesh
	ChunkBuffer meshData;
	if (createVertexBuffer(mesh, meshData)) {
		meshData.voxelMeshes = std::move(mesh);
		updateAABB(meshData);
		_chunkBuffers.push_back(meshData);
		Log::debug("Meshes so far: %i", (int)_chunkBuffers.size());
		distributePlants(_world, plantAmount, meshData.translation(), meshData.opaque.instancedPositions);
		fillPlantPositionsFromMeshes();
	}
}

bool WorldRenderer::checkShaders() const {
	const int loc1 = _worldShader.getLocationPos();
	const int loc2 = _worldInstancedShader.getLocationPos();
	const int loc3 = _waterShader.getLocationPos();
	const int loc4 = _shadowMapShader.getLocationPos();
	const bool same = loc1 == loc2 && loc2 == loc3 && loc3 == loc4;
	core_assert_msg(same, "attribute locations for a_pos differ: %i, %i, %i, %i", loc1, loc2, loc3, loc4);
	return same;
}

void WorldRenderer::cull(const video::Camera& camera) {
	_visible.clear();
	_visibleWater.clear();
	const float cullingThreshold = _world->getMeshSize();
	const int maxAllowedDistance = glm::pow(_viewDistance + cullingThreshold, 2);
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (!chunkBuffer.inuse) {
			continue;
		}
		const int distance = getDistanceSquare(chunkBuffer.translation());
		Log::trace("distance is: %i (%i)", distance, maxAllowedDistance);
		if (distance >= maxAllowedDistance) {
			_world->allowReExtraction(chunkBuffer.translation());
			chunkBuffer.inuse = false;
			Log::debug("Remove mesh from %i:%i", chunkBuffer.translation().x, chunkBuffer.translation().z);
			continue;
		}
		if (camera.isVisible(chunkBuffer.aabb)) {
			if (chunkBuffer.opaque.noOfIndices > 0) {
				_visible.push_back(&chunkBuffer.opaque);
			}
			if (chunkBuffer.water.noOfIndices > 0) {
				_visibleWater.push_back(&chunkBuffer.water);
			}
		}
	}
	Log::trace("%i meshes left after culling, %i meshes overall", (int)_visible.size(), (int)_chunkBuffers.size());
}

int WorldRenderer::renderWorldMeshes(video::Shader& shader, const RendererMeshVisibleList& meshes, int* vertices) {
	for (const video::GLMeshData* meshData : meshes) {
		shaderSetUniformIf(shader, setUniformMatrix, "u_model", meshData->model);
		core_assert(meshData->vertexArrayObject > 0);
		glBindVertexArray(meshData->vertexArrayObject);
		if (meshData->amount == 1) {
			glDrawElements(GL_TRIANGLES, meshData->noOfIndices, GLmap<voxel::IndexType>(), nullptr);
		} else {
			const int amount = (int)meshData->instancedPositions.size();
			glBindBuffer(GL_ARRAY_BUFFER, meshData->offsetBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * amount, &meshData->instancedPositions[0], GL_DYNAMIC_DRAW);
			glDrawElementsInstanced(GL_TRIANGLES, meshData->noOfIndices, GLmap<voxel::IndexType>(), nullptr, amount);
		}
		if (vertices != nullptr) {
			*vertices += meshData->noOfVertices;
		}
	}

	GL_checkError();
	return meshes.size();
}

int WorldRenderer::renderWorld(const video::Camera& camera, int* vertices) {
	handleMeshQueue();

	if (vertices != nullptr) {
		*vertices = 0;
	}
	if (_chunkBuffers.empty()) {
		return 0;
	}

	const bool shadowMap = _shadowMap->boolVal();

	{
		video::ScopedShader scoped(_worldShader);
		_worldShader.setMaterialblock(_materialBlock);
		_worldShader.setViewdistance(_viewDistance);
		_worldShader.setLightdir(_shadow.sunDirection());
		_worldShader.setFogcolor(_clearColor);
		_worldShader.setTexture(0);
		_worldShader.setDiffuseColor(_diffuseColor);
		_worldShader.setAmbientColor(_ambientColor);
		_worldShader.setFogrange(_fogRange);
		if (shadowMap) {
			_worldShader.setViewprojection(camera.viewProjectionMatrix());
			_worldShader.setShadowmap(1);
			_worldShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
		}
	}
	{
		video::ScopedShader scoped(_worldInstancedShader);
		_worldInstancedShader.setViewdistance(_viewDistance);
		_worldInstancedShader.setLightdir(_shadow.sunDirection());
		_worldInstancedShader.setMaterialblock(_materialBlock);
		_worldInstancedShader.setFogcolor(_clearColor);
		_worldInstancedShader.setTexture(0);
		_worldInstancedShader.setDiffuseColor(_diffuseColor);
		_worldInstancedShader.setAmbientColor(_ambientColor);
		_worldInstancedShader.setFogrange(_fogRange);
		if (shadowMap) {
			_worldInstancedShader.setViewprojection(camera.viewProjectionMatrix());
			_worldInstancedShader.setShadowmap(1);
			_worldInstancedShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
		}
	}
	{
		video::ScopedShader scoped(_waterShader);
		_waterShader.setViewdistance(_viewDistance);
		_waterShader.setLightdir(_shadow.sunDirection());
		_waterShader.setMaterialblock(_materialBlock);
		_waterShader.setFogcolor(_clearColor);
		_waterShader.setDiffuseColor(_diffuseColor);
		_waterShader.setAmbientColor(_ambientColor);
		_waterShader.setFogrange(_fogRange);
		_waterShader.setTime(float(_now));
		_waterShader.setTexture(0);
		if (shadowMap) {
			_waterShader.setViewprojection(camera.viewProjectionMatrix());
			_waterShader.setShadowmap(1);
			_waterShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
		}
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

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);

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
				_shadowMapShader.setLightviewprojection(cascades[i]);
				drawCallsWorld += renderWorldMeshes(_shadowMapShader, _visible, nullptr);
			}
			{
				video::ScopedShader scoped(_shadowMapInstancedShader);
				_shadowMapInstancedShader.setLightviewprojection(cascades[i]);
				drawCallsWorld += renderWorldMeshes(_shadowMapInstancedShader, _visiblePlant, nullptr);
			}
		}
		_depthBuffer.unbind();
		glCullFace(GL_BACK);
		glEnable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	_colorTexture.bind(0);

	glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (shadowMap) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(std::enum_value(_depthBuffer.textureType()), _depthBuffer.texture());
	}

	{
		video::ScopedShader scoped(_worldShader);
		if (shadowMap) {
			_worldShader.setCascades(cascades);
			_worldShader.setDistances(distances);
		}
		drawCallsWorld += renderWorldMeshes(_worldShader, _visible, vertices);
	}
	{
		video::ScopedShader scoped(_worldInstancedShader);
		if (shadowMap) {
			_worldInstancedShader.setCascades(cascades);
			_worldInstancedShader.setDistances(distances);
		}
		drawCallsWorld += renderWorldMeshes(_worldInstancedShader, _visiblePlant, vertices);
	}
	{
		video::ScopedShader scoped(_waterShader);
		if (shadowMap) {
			_waterShader.setCascades(cascades);
			_waterShader.setDistances(distances);
		}
		drawCallsWorld += renderWorldMeshes(_waterShader, _visibleWater, vertices);
	}

	if (shadowMap) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
	}

	_colorTexture.unbind();

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (shadowMap && _shadowMapDebug->boolVal()) {
		const int width = camera.width();
		const int height = camera.height();

		// activate shader
		video::ScopedShader scopedShader(_shadowMapRenderShader);
		_shadowMapRenderShader.setShadowmap(0);
		_shadowMapRenderShader.setFar(camera.farPlane());
		_shadowMapRenderShader.setNear(camera.nearPlane());

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
			_shadowMapRenderShader.setCascade(i);
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
	video::ScopedShader scoped(_meshShader);
	_meshShader.setView(camera.viewMatrix());
	_meshShader.setProjection(camera.projectionMatrix());
	_meshShader.setFogrange(_fogRange);
	_meshShader.setViewdistance(_viewDistance);
	_meshShader.setTexture(1);
	_meshShader.setDiffuseColor(_diffuseColor);
	_meshShader.setAmbientColor(_ambientColor);
	_meshShader.setFogcolor(_clearColor);
	_meshShader.setCascades(_shadow.cascades());
	_meshShader.setDistances(_shadow.distances());
	_meshShader.setLightdir(_shadow.sunDirection());

	const bool shadowMap = _shadowMap->boolVal();
	if (shadowMap) {
		_meshShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
		_meshShader.setViewprojection(camera.viewProjectionMatrix());
		_meshShader.setShadowmap(1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(std::enum_value(_depthBuffer.textureType()), _depthBuffer.texture());
	}
	for (const auto& e : _entities) {
		const frontend::ClientEntityPtr& ent = e.second;
		ent->update(_deltaFrame);
		if (!camera.isVisible(ent->position())) {
			continue;
		}
		const video::MeshPtr& mesh = ent->mesh();
		if (!mesh->initMesh(_meshShader)) {
			continue;
		}
		const glm::mat4& rotate = glm::rotate(glm::mat4(1.0f), ent->orientation(), glm::up);
		const glm::mat4& translate = glm::translate(rotate, ent->position());
		const glm::mat4& scale = glm::scale(translate, glm::vec3(ent->scale()));
		const glm::mat4& model = scale;
		_meshShader.setModel(model);
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

void WorldRenderer::updateVertexBuffer(const voxel::Mesh& mesh, video::GLMeshData& meshData) const {
	core_trace_gl_scoped(WorldRendererUpdateMesh);
	const voxel::IndexType* vecIndices = mesh.getRawIndexData();
	const uint32_t numIndices = mesh.getNoOfIndices();
	const voxel::VoxelVertex* vecVertices = mesh.getRawVertexData();
	const uint32_t numVertices = mesh.getNoOfVertices();

	core_assert(meshData.vertexBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(voxel::VoxelVertex), vecVertices, GL_DYNAMIC_DRAW);

	core_assert(meshData.indexBuffer > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(voxel::IndexType), vecIndices, GL_DYNAMIC_DRAW);

	meshData.noOfVertices = numVertices;
	meshData.noOfIndices = numIndices;
	meshData.model = glm::translate(glm::vec3(mesh.getOffset()));
}

bool WorldRenderer::createVertexBufferInternal(const video::Shader& shader, const voxel::Mesh &mesh, int buffers, video::GLMeshData& meshData) {
	core_trace_gl_scoped(WorldRendererCreateMesh);
	if (mesh.getNoOfIndices() == 0) {
		return false;
	}

	core_assert(meshData.vertexArrayObject == 0u);
	// Create the VAOs for the meshes
	glGenVertexArrays(1, &meshData.vertexArrayObject);

	core_assert(meshData.indexBuffer == 0u);
	core_assert(meshData.vertexBuffer == 0u);
	core_assert(meshData.offsetBuffer == 0u);

	// The GL_ARRAY_BUFFER will contain the list of vertex positions
	// and GL_ELEMENT_ARRAY_BUFFER will contain the indices
	// and GL_ARRAY_BUFFER will contain the offsets for instanced rendering
	core_assert(buffers == 2 || buffers == 3);
	glGenBuffers(buffers, &meshData.indexBuffer);
	core_assert(buffers == 2 || meshData.offsetBuffer > 0);

	glBindVertexArray(meshData.vertexArrayObject);

	updateVertexBuffer(mesh, meshData);

	const int posLoc = shader.enableVertexAttributeArray("a_pos");
	const video::VertexBuffer::Attribute& posAttrib = getPositionVertexAttribute(0, posLoc, shader.getAttributeComponents(posLoc));
	shader.setVertexAttributeInt(posLoc, posAttrib.size, posAttrib.type, posAttrib.stride, GL_OFFSET_CAST(posAttrib.offset));

	const int locationInfo = shader.enableVertexAttributeArray("a_info");
	const video::VertexBuffer::Attribute& infoAttrib = getInfoVertexAttribute(0, locationInfo, shader.getAttributeComponents(locationInfo));
	shader.setVertexAttributeInt(locationInfo, infoAttrib.size, infoAttrib.type, infoAttrib.stride, GL_OFFSET_CAST(infoAttrib.offset));
	GL_checkError();

	return true;
}

bool WorldRenderer::createVertexBuffer(const voxel::ChunkMeshData &mesh, ChunkBuffer& meshData) {
	if (!createVertexBufferInternal(_worldShader, mesh.opaqueMesh, 2, meshData.opaque)) {
		return false;
	}

	createVertexBufferInternal(_worldShader, mesh.waterMesh, 2, meshData.water);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
}

bool WorldRenderer::createInstancedVertexBuffer(const voxel::Mesh &mesh, int amount, video::GLMeshData& meshData) {
	if (!createVertexBufferInternal(_worldInstancedShader, mesh, 3, meshData)) {
		return false;
	}

	meshData.amount = amount;
	meshData.model = glm::scale(glm::vec3(0.4f));

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
	active = _chunkBuffers.size();
}

void WorldRenderer::onConstruct() {
	core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER);
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_shadowMapDebug = core::Var::get(cfg::ClientDebugShadowMap, "false");
	core::Var::get(cfg::ClientShadowMapSize, "512");
}

bool WorldRenderer::onInit(const glm::ivec2& position, const glm::ivec2& dimension) {
	core_trace_scoped(WorldRendererOnInit);
	_colorTexture.init();
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
	if (!_shadowMapRenderShader.setup()) {
		return false;
	}

	_worldIndexBufferIndex = _worldBuffer.create(nullptr, 0, video::VertexBufferType::IndexBuffer);
	if (_worldIndexBufferIndex == -1) {
		Log::error("Could not create the world vertex buffer object for the indices");
		return false;
	}

	_worldBufferIndex = _worldBuffer.create();
	if (_worldBufferIndex == -1) {
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

	const glm::ivec2& fullscreenQuadIndices = _shadowMapDebugBuffer.createFullscreenTexturedQuad(true);
	video::VertexBuffer::Attribute attributePos;
	attributePos.bufferIndex = fullscreenQuadIndices.x;
	attributePos.index = _shadowMapRenderShader.getLocationPos();
	attributePos.size = _shadowMapRenderShader.getComponentsPos();
	_shadowMapDebugBuffer.addAttribute(attributePos);

	video::VertexBuffer::Attribute attributeTexcoord;
	attributeTexcoord.bufferIndex = fullscreenQuadIndices.y;
	attributeTexcoord.index = _shadowMapRenderShader.getLocationTexcoord();
	attributeTexcoord.size = _shadowMapRenderShader.getComponentsTexcoord();
	_shadowMapDebugBuffer.addAttribute(attributeTexcoord);

	video::VertexBuffer::Attribute voxelAttributePos = getPositionVertexAttribute(_worldBufferIndex, _worldShader.getLocationPos(), _worldShader.getComponentsPos());
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
		if (createInstancedVertexBuffer(*mesh, 40, meshDataPlant)) {
			_meshPlantList.push_back(meshDataPlant);
		}
	}

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	if (!_depthBuffer.init(smSize, video::DepthBufferMode::DEPTH_CMP, maxDepthBuffers)) {
		return false;
	}

	const int shaderMaterialColorsArraySize = SDL_arraysize(shader::Materialblock::Data::materialcolor);
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::Materialblock::Data materialBlock;
	memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	_materialBlock.update(materialBlock);

	if (!_shadow.init()) {
		return false;
	}

	return true;
}

void WorldRenderer::onRunning(const video::Camera& camera, long dt) {
	core_trace_scoped(WorldRendererOnRunning);
	_now += dt;
	_deltaFrame = dt;
	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	const bool shadowMap = _shadowMap->boolVal();
	_shadow.calculateShadowData(camera, shadowMap, maxDepthBuffers, _depthBuffer.dimension());
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
