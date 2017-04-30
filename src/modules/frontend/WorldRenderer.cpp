/**
 * @file
 */

#include "WorldRenderer.h"
#include "core/Color.h"
#include "video/Renderer.h"
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

// TODO: use octree for all the visibility/extraction stuff - this class should only render
// TODO: respect max vertex/index size of the one-big-vbo/ibo
WorldRenderer::WorldRenderer(const voxel::WorldPtr& world) :
		_world(world) {
}

WorldRenderer::~WorldRenderer() {
}

void WorldRenderer::reset() {
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		chunkBuffer.inuse = false;
	}
	_activeChunkBuffers = 0;
	_entities.clear();
	_viewDistance = 1.0f;
	_now = 0l;
}

void WorldRenderer::shutdown() {
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
	_opaqueBuffer.shutdown();
	_waterBuffer.shutdown();

	for (PlantBuffer& vbo : _meshPlantList) {
		vbo.shutdown();
	}
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
	const int plantMeshAmount = SDL_arraysize(_meshPlantList);
	for (PlantBuffer& vbo : _meshPlantList) {
		vbo.instancedPositions.clear();
	}
	for (const ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (!chunkBuffer.inuse) {
			continue;
		}
		if (chunkBuffer.instancedPositions.empty()) {
			continue;
		}
		std::vector<glm::vec3> p = chunkBuffer.instancedPositions;
		core::Random rnd(_world->seed() + chunkBuffer.translation().x + chunkBuffer.translation().y + chunkBuffer.translation().z);
		rnd.shuffle(p.begin(), p.end());
		const int plantMeshes = p.size() / plantMeshAmount;
		int delta = p.size() - plantMeshes * plantMeshAmount;
		for (PlantBuffer& vbo : _meshPlantList) {
			auto it = std::next(p.begin(), plantMeshes + delta);
			std::move(p.begin(), it, std::back_inserter(vbo.instancedPositions));
			p.erase(p.begin(), it);
			delta = 0;
		}
	}
	for (PlantBuffer& vbo : _meshPlantList) {
		const std::vector<glm::vec3>& positions = vbo.instancedPositions;
		vbo.vb.update(vbo.offsetBuffer, positions);
	}
}

void WorldRenderer::updateAABB(ChunkBuffer& chunkBuffer) const {
	glm::ivec3 mins(std::numeric_limits<int>::max());
	glm::ivec3 maxs(std::numeric_limits<int>::min());

	for (auto& v : chunkBuffer.meshes.opaqueMesh.getVertexVector()) {
		mins = glm::min(mins, v.position);
		maxs = glm::max(maxs, v.position);
	}
	for (auto& v : chunkBuffer.meshes.waterMesh.getVertexVector()) {
		mins = glm::min(mins, v.position);
		maxs = glm::max(maxs, v.position);
	}

	chunkBuffer.aabb = core::AABB<float>(mins, maxs);
}

void WorldRenderer::handleMeshQueue() {
	voxel::ChunkMeshes meshes(0, 0, 0, 0);
	if (!_world->pop(meshes)) {
		return;
	}
	// Now add the mesh to the list of meshes to render.
	core_trace_gl_scoped(WorldRendererHandleMeshQueue);

	ChunkBuffer* freeChunkBuffer = nullptr;
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (freeChunkBuffer == nullptr && !chunkBuffer.inuse) {
			freeChunkBuffer = &chunkBuffer;
		}
		// check whether we update an existing one
		if (chunkBuffer.translation() == meshes.translation()) {
			freeChunkBuffer = &chunkBuffer;
			break;
		}
	}

	if (freeChunkBuffer == nullptr) {
		Log::warn("Could not find free chunk buffer slot");
		return;
	}
	if (!freeChunkBuffer->inuse) {
		freeChunkBuffer->inuse = true;
		++_activeChunkBuffers;
	}
	freeChunkBuffer->meshes = std::move(meshes);
	updateAABB(*freeChunkBuffer);
	distributePlants(_world, freeChunkBuffer->translation(), freeChunkBuffer->instancedPositions);
	fillPlantPositionsFromMeshes();
}

WorldRenderer::ChunkBuffer* WorldRenderer::findFreeChunkBuffer() {
	for (int i = 0; i < (int)SDL_arraysize(_chunkBuffers); ++i) {
		if (!_chunkBuffers[i].inuse) {
			return &_chunkBuffers[i];
		}
	}
	return nullptr;
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

static size_t transform(size_t indexOffset, const voxel::Mesh& mesh, std::vector<voxel::VoxelVertex>& verts, std::vector<voxel::IndexType>& idxs) {
	const std::vector<voxel::IndexType>& indices = mesh.getIndexVector();
	std::transform(indices.begin(), indices.end(),
		std::back_inserter(idxs),
		[=] (voxel::IndexType index) {
			return index + indexOffset;
		}
	);

	const std::vector<voxel::VoxelVertex>& vertices = mesh.getVertexVector();
	verts.insert(verts.end(), vertices.begin(), vertices.end());
	return vertices.size();
}

int WorldRenderer::cull(const video::Camera& camera) {
	_opaqueIndices.clear();
	_opaqueVertices.clear();
	_waterIndices.clear();
	_waterVertices.clear();
	size_t opaqueIndexOffset = 0;
	size_t waterIndexOffset = 0;
	int visibleChunks = 0;
	const float cullingThreshold = _world->getMeshSize();
	const int maxAllowedDistance = glm::pow(_viewDistance + cullingThreshold, 2);
	// TODO: move this buffer into an octree.
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (!chunkBuffer.inuse) {
			continue;
		}
		const int distance = getDistanceSquare(chunkBuffer.translation());
		Log::trace("distance is: %i (%i)", distance, maxAllowedDistance);
		if (distance >= maxAllowedDistance) {
			_world->allowReExtraction(chunkBuffer.translation());
			chunkBuffer.inuse = false;
			--_activeChunkBuffers;
			Log::debug("Remove mesh from %i:%i", chunkBuffer.translation().x, chunkBuffer.translation().z);
			continue;
		}
		if (!camera.isVisible(chunkBuffer.aabb)) {
			continue;
		}
		opaqueIndexOffset += transform(opaqueIndexOffset, chunkBuffer.meshes.opaqueMesh, _opaqueVertices, _opaqueIndices);
		waterIndexOffset += transform(waterIndexOffset, chunkBuffer.meshes.waterMesh, _waterVertices, _waterIndices);
		++visibleChunks;
	}
	return visibleChunks;
}

bool WorldRenderer::renderOpaqueBuffers() {
	const uint32_t numIndices = _opaqueBuffer.elements(_opaqueIbo, 1, sizeof(voxel::IndexType));
	if (numIndices == 0u) {
		return false;
	}
	_opaqueBuffer.bind();
	video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
	_opaqueBuffer.unbind();
	return true;
}

bool WorldRenderer::renderWaterBuffers() {
	const uint32_t numIndices = _waterBuffer.elements(_waterIbo, 1, sizeof(voxel::IndexType));
	if (numIndices == 0u) {
		return false;
	}
	_waterBuffer.bind();
	video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
	_waterBuffer.unbind();
	return true;
}

int WorldRenderer::renderPlants(const std::list<PlantBuffer*>& vbos, int* vertices) {
	for (PlantBuffer* vbo : vbos) {
		const uint32_t numIndices = vbo->vb.elements(vbo->indexBuffer, 1, sizeof(voxel::IndexType));
		if (numIndices == 0u) {
			continue;
		}

		vbo->vb.bind();
		if (vbo->amount == 1) {
			video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
		} else {
			const std::vector<glm::vec3>& positions = vbo->instancedPositions;
			video::drawElementsInstanced<voxel::IndexType>(video::Primitive::Triangles, numIndices, positions.size());
		}
		if (vertices != nullptr) {
			*vertices += vbo->vb.elements(vbo->vertexBuffer, 1, sizeof(voxel::VoxelVertex));
		}
	}

	return vbos.size();
}

int WorldRenderer::renderWorld(const video::Camera& camera, int* vertices) {
	handleMeshQueue();

	if (vertices != nullptr) {
		*vertices = 0;
	}

	_visibleChunks = cull(camera);
	if (_visibleChunks == 0) {
		return 0;
	}
	if (_opaqueIndices.empty() && _waterIndices.empty()) {
		return 0;
	}

	core_assert_always(_opaqueBuffer.update(_opaqueVbo, _opaqueVertices));
	core_assert_always(_opaqueBuffer.update(_opaqueIbo, _opaqueIndices));
	core_assert_always(_waterBuffer.update(_waterVbo, _waterVertices));
	core_assert_always(_waterBuffer.update(_waterIbo, _waterIndices));

	const bool shadowMap = _shadowMap->boolVal();

	{
		video::ScopedShader scoped(_worldShader);
		_worldShader.setMaterialblock(_materialBlock);
		_worldShader.setViewdistance(_viewDistance);
		_worldShader.setLightdir(_shadow.sunDirection());
		_worldShader.setFogcolor(_clearColor);
		_worldShader.setTexture(video::TextureUnit::Zero);
		_worldShader.setDiffuseColor(_diffuseColor);
		_worldShader.setAmbientColor(_ambientColor);
		_worldShader.setFogrange(_fogRange);
		if (shadowMap) {
			_worldShader.setViewprojection(camera.viewProjectionMatrix());
			_worldShader.setShadowmap(video::TextureUnit::One);
			_worldShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
		}
	}
	{
		video::ScopedShader scoped(_worldInstancedShader);
		_worldInstancedShader.setViewdistance(_viewDistance);
		_worldInstancedShader.setLightdir(_shadow.sunDirection());
		_worldInstancedShader.setMaterialblock(_materialBlock);
		_worldInstancedShader.setFogcolor(_clearColor);
		_worldInstancedShader.setTexture(video::TextureUnit::Zero);
		_worldInstancedShader.setDiffuseColor(_diffuseColor);
		_worldInstancedShader.setAmbientColor(_ambientColor);
		_worldInstancedShader.setFogrange(_fogRange);
		if (shadowMap) {
			_worldInstancedShader.setViewprojection(camera.viewProjectionMatrix());
			_worldInstancedShader.setShadowmap(video::TextureUnit::One);
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
		_waterShader.setTexture(video::TextureUnit::Zero);
		if (shadowMap) {
			_waterShader.setViewprojection(camera.viewProjectionMatrix());
			_waterShader.setShadowmap(video::TextureUnit::One);
			_waterShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
		}
	}

	core_assert_msg(checkShaders(), "Shader attributes don't have the same order");

	core_trace_gl_scoped(WorldRendererRenderWorld);
	int drawCallsWorld = 0;

	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);

	const std::vector<glm::mat4>& cascades = _shadow.cascades();
	const std::vector<float>& distances = _shadow.distances();
	if (shadowMap) {
		video::disable(video::State::Blend);
		// put shadow acne into the dark
		video::cullFace(video::Face::Front);
		const float shadowBiasSlope = 2;
		const float shadowBias = 0.09f;
		const float shadowRangeZ = camera.farPlane() * 3.0f;
		const glm::vec2 offset(shadowBiasSlope, (shadowBias / shadowRangeZ) * (1 << 24));
		const video::ScopedPolygonMode scopedPolygonMode(video::PolygonMode::Solid, offset);

		_depthBuffer.bind();
		for (int i = 0; i < maxDepthBuffers; ++i) {
			_depthBuffer.bindTexture(i);
			{
				video::ScopedShader scoped(_shadowMapShader);
				_shadowMapShader.setLightviewprojection(cascades[i]);
				_shadowMapShader.setModel(glm::mat4());
				renderOpaqueBuffers();
				++drawCallsWorld;
			}
			{
				video::ScopedShader scoped(_shadowMapInstancedShader);
				_shadowMapInstancedShader.setLightviewprojection(cascades[i]);
				_shadowMapInstancedShader.setModel(glm::scale(glm::vec3(0.4f)));
				drawCallsWorld += renderPlants(_visiblePlant, nullptr);
			}
		}
		_depthBuffer.unbind();
		video::cullFace(video::Face::Back);
		video::enable(video::State::Blend);
	}

	_colorTexture.bind(video::TextureUnit::Zero);

	video::clearColor(_clearColor);
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);

	if (shadowMap) {
		video::bindTexture(video::TextureUnit::One, _depthBuffer);
	}

	{
		video::ScopedShader scoped(_worldShader);
		_worldShader.setModel(glm::mat4());
		if (shadowMap) {
			_worldShader.setCascades(cascades);
			_worldShader.setDistances(distances);
		}
		if (renderOpaqueBuffers()) {
			++drawCallsWorld;
		}
	}
	{
		video::ScopedShader scoped(_worldInstancedShader);
		_worldInstancedShader.setModel(glm::scale(glm::vec3(0.4f)));
		if (shadowMap) {
			_worldInstancedShader.setCascades(cascades);
			_worldInstancedShader.setDistances(distances);
		}
		drawCallsWorld += renderPlants(_visiblePlant, vertices);
	}
	{
		video::ScopedShader scoped(_waterShader);
		_waterShader.setModel(glm::mat4());
		if (shadowMap) {
			_waterShader.setCascades(cascades);
			_waterShader.setDistances(distances);
		}
		if (renderWaterBuffers()) {
			++drawCallsWorld;
		}
	}

	video::bindVertexArray(video::InvalidId);

	_colorTexture.unbind();

	if (shadowMap && _shadowMapShow->boolVal()) {
		const int width = camera.width();
		const int height = camera.height();

		// activate shader
		video::ScopedShader scopedShader(_shadowMapRenderShader);
		_shadowMapRenderShader.setShadowmap(video::TextureUnit::Zero);
		_shadowMapRenderShader.setFar(camera.farPlane());
		_shadowMapRenderShader.setNear(camera.nearPlane());

		// bind buffers
		core_assert_always(_shadowMapDebugBuffer.bind());

		// configure shadow map texture
		video::bindTexture(video::TextureUnit::Zero, _depthBuffer);
		if (_depthBuffer.depthCompare()) {
			video::disableDepthCompareTexture(video::TextureUnit::Zero, _depthBuffer.textureType(), _depthBuffer.texture());
		}

		// render shadow maps
		for (int i = 0; i < maxDepthBuffers; ++i) {
			const int halfWidth = (int) (width / 4.0f);
			const int halfHeight = (int) (height / 4.0f);
			video::ScopedViewPort scopedViewport(i * halfWidth, 0, halfWidth, halfHeight);
			_shadowMapRenderShader.setCascade(i);
			video::drawArrays(video::Primitive::Triangles, _shadowMapDebugBuffer.elements(0));
		}

		// restore texture
		if (_depthBuffer.depthCompare()) {
			video::setupDepthCompareTexture(video::TextureUnit::Zero, _depthBuffer.textureType(), _depthBuffer.texture());
		}

		// unbind buffer
		_shadowMapDebugBuffer.unbind();
	}

	return drawCallsWorld;
}

int WorldRenderer::renderEntities(const video::Camera& camera) {
	if (_entities.empty()) {
		return 0;
	}
	core_trace_gl_scoped(WorldRendererRenderEntities);

	int drawCallsEntities = 0;

	video::enable(video::State::DepthTest);
	video::enable(video::State::DepthMask);
	video::ScopedShader scoped(_meshShader);
	_meshShader.setFogrange(_fogRange);
	_meshShader.setViewdistance(_viewDistance);
	_meshShader.setTexture(video::TextureUnit::Zero);
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
		_meshShader.setShadowmap(video::TextureUnit::One);
		video::bindTexture(video::TextureUnit::One, _depthBuffer);
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
	}
	return drawCallsEntities;
}

bool WorldRenderer::createInstancedVertexBuffer(const voxel::Mesh &mesh, int amount, PlantBuffer& vbo) {
	if (mesh.getNoOfIndices() == 0) {
		return false;
	}

	core_trace_gl_scoped(WorldRendererCreateMesh);
	vbo.vb.clearAttributes();
	vbo.vertexBuffer = vbo.vb.create(mesh.getVertexVector());
	if (vbo.vertexBuffer == -1) {
		Log::error("Failed to create vertex buffer");
		return false;
	}
	vbo.indexBuffer = vbo.vb.create(mesh.getIndexVector(), video::VertexBufferType::IndexBuffer);
	if (vbo.indexBuffer == -1) {
		Log::error("Failed to create index buffer");
		return false;
	}
	vbo.offsetBuffer = vbo.vb.create();
	if (vbo.offsetBuffer == -1) {
		Log::error("Failed to create offset buffer");
		return false;
	}

	const int locationPos = _worldInstancedShader.getLocationPos();
	_worldInstancedShader.enableVertexAttributeArray(locationPos);
	const video::Attribute& posAttrib = getPositionVertexAttribute(vbo.vertexBuffer, locationPos, _worldInstancedShader.getAttributeComponents(locationPos));
	if (!vbo.vb.addAttribute(posAttrib)) {
		Log::error("Failed to add position attribute");
		return false;
	}

	const int locationInfo = _worldInstancedShader.getLocationInfo();
	_worldInstancedShader.enableVertexAttributeArray(locationInfo);
	const video::Attribute& infoAttrib = getInfoVertexAttribute(vbo.vertexBuffer, locationInfo, _worldInstancedShader.getAttributeComponents(locationInfo));
	if (!vbo.vb.addAttribute(infoAttrib)) {
		Log::error("Failed to add info attribute");
		return false;
	}

	const int locationOffset = _worldInstancedShader.getLocationOffset();
	_worldInstancedShader.enableVertexAttributeArray(locationOffset);
	const video::Attribute& offsetAttrib = getOffsetVertexAttribute(vbo.offsetBuffer, locationOffset, _worldInstancedShader.getAttributeComponents(locationOffset));
	if (!vbo.vb.addAttribute(offsetAttrib)) {
		Log::error("Failed to add offset attribute");
		return false;
	}

	vbo.amount = amount;

	return true;
}

void WorldRenderer::onSpawn(const glm::vec3& pos, int initialExtractionRadius) {
	core_trace_scoped(WorldRendererOnSpawn);
	_viewDistance = 1.0f;
	// TODO: move into World class
	extractMeshAroundCamera(_world->getMeshPos(pos), initialExtractionRadius);
}

// TODO: move into World class
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

// TODO: move into World class
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

void WorldRenderer::stats(int& meshes, int& extracted, int& pending, int& active, int& visible) const {
	_world->stats(meshes, extracted, pending);
	active = _activeChunkBuffers;
	visible = _visibleChunks;
}

void WorldRenderer::onConstruct() {
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_shadowMapShow = core::Var::get(cfg::ClientShadowMapShow, "false");
}

bool WorldRenderer::initOpaqueBuffer() {
	_opaqueVbo = _opaqueBuffer.create();
	if (_opaqueVbo == -1) {
		Log::error("Failed to create vertex buffer");
		return false;
	}
	_opaqueIbo = _opaqueBuffer.create(nullptr, 0, video::VertexBufferType::IndexBuffer);
	if (_opaqueIbo == -1) {
		Log::error("Failed to create index buffer");
		return false;
	}

	const int locationPos = _worldShader.getLocationPos();
	_worldShader.enableVertexAttributeArray(locationPos);
	const video::Attribute& posAttrib = getPositionVertexAttribute(_opaqueVbo, locationPos, _worldShader.getAttributeComponents(locationPos));
	if (!_opaqueBuffer.addAttribute(posAttrib)) {
		Log::error("Failed to add position attribute");
		return false;
	}

	const int locationInfo = _worldShader.getLocationInfo();
	_worldShader.enableVertexAttributeArray(locationInfo);
	const video::Attribute& infoAttrib = getInfoVertexAttribute(_opaqueVbo, locationInfo, _worldShader.getAttributeComponents(locationInfo));
	if (!_opaqueBuffer.addAttribute(infoAttrib)) {
		Log::error("Failed to add info attribute");
		return false;
	}

	return true;
}

bool WorldRenderer::initWaterBuffer() {
	_waterVbo = _waterBuffer.create();
	if (_waterVbo == -1) {
		Log::error("Failed to create water vertex buffer");
		return false;
	}
	_waterIbo = _waterBuffer.create(nullptr, 0, video::VertexBufferType::IndexBuffer);
	if (_waterIbo == -1) {
		Log::error("Failed to create water index buffer");
		return false;
	}

	const int locationPos = _waterShader.getLocationPos();
	_waterShader.enableVertexAttributeArray(locationPos);
	const video::Attribute& posAttrib = getPositionVertexAttribute(_waterVbo, locationPos, _waterShader.getAttributeComponents(locationPos));
	if (!_waterBuffer.addAttribute(posAttrib)) {
		Log::error("Failed to add water position attribute");
		return false;
	}

	const int locationInfo = _waterShader.getLocationInfo();
	_waterShader.enableVertexAttributeArray(locationInfo);
	const video::Attribute& infoAttrib = getInfoVertexAttribute(_waterVbo, locationInfo, _waterShader.getAttributeComponents(locationInfo));
	if (!_waterBuffer.addAttribute(infoAttrib)) {
		Log::error("Failed to add water info attribute");
		return false;
	}

	return true;
}

bool WorldRenderer::init(const glm::ivec2& position, const glm::ivec2& dimension) {
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

	const glm::ivec2& fullscreenQuadIndices = _shadowMapDebugBuffer.createFullscreenTexturedQuad(true);
	video::Attribute attributePos;
	attributePos.bufferIndex = fullscreenQuadIndices.x;
	attributePos.index = _shadowMapRenderShader.getLocationPos();
	attributePos.size = _shadowMapRenderShader.getComponentsPos();
	_shadowMapDebugBuffer.addAttribute(attributePos);

	video::Attribute attributeTexcoord;
	attributeTexcoord.bufferIndex = fullscreenQuadIndices.y;
	attributeTexcoord.index = _shadowMapRenderShader.getLocationTexcoord();
	attributeTexcoord.size = _shadowMapRenderShader.getComponentsTexcoord();
	_shadowMapDebugBuffer.addAttribute(attributeTexcoord);

	_visiblePlant.clear();
	for (int i = 0; i < (int)voxel::PlantType::MaxPlantTypes; ++i) {
		const voxel::Mesh* mesh = _plantGenerator.getMesh((voxel::PlantType)i);
		createInstancedVertexBuffer(*mesh, 40, _meshPlantList[i]);
		_visiblePlant.push_back(&_meshPlantList[i]);
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
	_materialBlock.create(materialBlock);

	if (!initOpaqueBuffer()) {
		return false;
	}

	if (!initWaterBuffer()) {
		return false;
	}

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
