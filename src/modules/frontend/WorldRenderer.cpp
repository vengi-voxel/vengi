#include "WorldRenderer.h"
#include "video/GLFunc.h"
#include "video/Color.h"
#include "voxel/Spiral.h"
#include "core/App.h"
#include "noise/SimplexNoise.h"
#include <PolyVox/CubicSurfaceExtractor.h>
#include <PolyVox/RawVolume.h>
#include <SDL.h>

namespace frontend {

// TODO: destroy the gl buffers

WorldRenderer::WorldRenderer(const voxel::WorldPtr& world) :
		_world(world) {
}

ClientEntityPtr WorldRenderer::getEntity(ClientEntityId id) {
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

int WorldRenderer::renderWorld(video::Shader& shader, const glm::mat4& view, float aspect) {
	int drawCallsWorld = 0;
	voxel::DecodedMeshData mesh;
	if (_world->pop(mesh)) {
		// Now add the mesh to the list of meshes to render.
		_meshData.push_back(createMesh(shader, mesh.mesh, mesh.translation, 1.0f));
	}

	// TODO: use polyvox VolumeResampler to create a minimap of your volume
	// RawVolume<uint8_t> volDataLowLOD(PolyVox::Region(Vector3DInt32(0, 0, 0), Vector3DInt32(15, 31, 31)));
	// VolumeResampler< RawVolume<uint8_t>, RawVolume<uint8_t> > volumeResampler(&volData, PolyVox::Region(Vector3DInt32(0, 0, 0), Vector3DInt32(31, 63, 63)), &volDataLowLOD, volDataLowLOD.getEnclosingRegion());
	// volumeResampler.execute();
	// auto meshLowLOD = extractMarchingCubesMesh(&volDataLowLOD, volDataLowLOD.getEnclosingRegion());
	// auto decodedMeshLowLOD = decodeMesh(meshLowLOD);

	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);

	GL_checkError();

	const glm::mat4& projection = glm::perspective(45.0f, aspect, 0.1f, 1000.0f);

	static const glm::vec4 materialColors[] = {
		video::Color::LightBlue,	// air
		video::Color::Lime,			// dirt
		video::Color::Brown,		// grass
		// leaves
		video::Color::DarkGreen,	// clouds
		video::Color::Green,		// water
		video::Color::Purple,		// leaves
		video::Color::Cyan,			// trunk
		video::Color::Olive,		// clouds
		video::Color::Orange,
		video::Color::Red,
		video::Color::Yellow,
		video::Color::LightRed,
		video::Color::Blue,
		// leaves end
		video::Color::DarkGray,
		video::Color::White,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink
	};
	static_assert(SDL_arraysize(materialColors) == 32, "amount of colors doesn't match shader uniform");

	shader.activate();
	shader.setUniformMatrix("u_view", view, false);
	shader.setUniformMatrix("u_projection", projection, false);
	shader.setUniformf("u_fogrange", _fogRange);
	shader.setUniformf("u_viewdistance", _viewDistance);
	shader.setUniformi("u_texture", 0);
	shader.setUniformVec3("u_lightpos", _lightPos);
	shader.setUniformVec4v("u_materialcolor[0]", materialColors, SDL_arraysize(materialColors));
	_colorTexture->bind();
	for (auto i = _meshData.begin(); i != _meshData.end();) {
		const video::GLMeshData& meshData = *i;
		if (isCulled(meshData.translation)) {
			_world->allowReExtraction(meshData.translation);
			glDeleteBuffers(1, &meshData.vertexBuffer);
			glDeleteBuffers(1, &meshData.indexBuffer);
			glDeleteVertexArrays(1, &meshData.vertexArrayObject);
			i = _meshData.erase(i);
			continue;
		}
		const glm::mat4& model = glm::translate(glm::mat4(1.0f), glm::vec3(meshData.translation.x, 0, meshData.translation.y));
		shader.setUniformMatrix("u_model", model, false);
		glBindVertexArray(meshData.vertexArrayObject);
		glDrawElements(GL_TRIANGLES, meshData.noOfIndices, meshData.indexType, 0);
		GL_checkError();
		++drawCallsWorld;
		++i;
	}
	GL_checkError();
	shader.deactivate();
	GL_checkError();

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);

	GL_checkError();
	return drawCallsWorld;
}

// TODO: generate bigger buffers and use glBufferSubData
video::GLMeshData WorldRenderer::createMesh(video::Shader& shader, voxel::DecodedMesh& surfaceMesh, const glm::ivec2& translation, float scale) {
	// Convenient access to the vertices and indices
	const uint32_t* vecIndices = surfaceMesh.getRawIndexData();
	const uint32_t numIndices = surfaceMesh.getNoOfIndices();
	const voxel::VoxelVertexDecoded* vecVertices = surfaceMesh.getRawVertexData();
	const uint32_t numVertices = surfaceMesh.getNoOfVertices();

	// This struct holds the OpenGL properties (buffer handles, etc) which will be used
	// to render our mesh. We copy the data from the PolyVox mesh into this structure.
	video::GLMeshData meshData;

	// Create the VAO for the mesh
	glGenVertexArrays(1, &meshData.vertexArrayObject);
	core_assert(meshData.vertexArrayObject > 0);
	glBindVertexArray(meshData.vertexArrayObject);

	// The GL_ARRAY_BUFFER will contain the list of vertex positions
	glGenBuffers(1, &meshData.vertexBuffer);
	core_assert(meshData.vertexBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(voxel::VoxelVertexDecoded), vecVertices, GL_STATIC_DRAW);

	// and GL_ELEMENT_ARRAY_BUFFER will contain the indices
	glGenBuffers(1, &meshData.indexBuffer);
	core_assert(meshData.indexBuffer > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(typename voxel::DecodedMesh::IndexType), vecIndices, GL_STATIC_DRAW);

	const int posLoc = shader.enableVertexAttribute("a_pos");
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(voxel::VoxelVertexDecoded),
			GL_OFFSET(offsetof(voxel::VoxelVertexDecoded, position)));

	const int matLoc = shader.enableVertexAttribute("a_materialdensity");
	// our material and density is encoded as 8 bits material and 8 bits density
	core_assert(sizeof(voxel::Voxel) == sizeof(uint16_t));
	glVertexAttribIPointer(matLoc, sizeof(voxel::Voxel), GL_UNSIGNED_BYTE, sizeof(voxel::VoxelVertexDecoded),
			GL_OFFSET(offsetof(voxel::VoxelVertexDecoded, data)));

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Log::trace("mesh information:\n- mesh indices: %i, vertices: %i\n- position: %i:%i", numIndices, numVertices, translation.x,
			translation.y);

	meshData.noOfIndices = numIndices;
	meshData.translation = translation;
	meshData.scale = scale;
	meshData.indexType = GL_UNSIGNED_INT;
	return meshData;
}

void WorldRenderer::onSpawn(const glm::vec3& pos) {
	_viewDistance = _world->getChunkSize() * 3;
	_fogRange = _viewDistance;
	_lastCameraPosition = _world->getGridPos(pos);
	extractMeshAroundCamera(1000);
}

int WorldRenderer::renderEntities(const video::ShaderPtr& shader, const glm::mat4& view, float aspect) {
	if (_entities.empty())
		return 0;

	int drawCallsEntities = 0;
	const glm::mat4& projection = glm::perspective(45.0f, aspect, 0.1f, 1000.0f);

	shader->activate();
	shader->setUniformMatrix("u_view", view, false);
	shader->setUniformMatrix("u_projection", projection, false);
	shader->setUniformVec3("u_lightpos", _lightPos);
	shader->setUniformf("u_fogrange", _fogRange);
	shader->setUniformf("u_viewdistance", _viewDistance);
	shader->setUniformi("u_texture", 0);
	for (const auto& e : _entities) {
		const frontend::ClientEntityPtr& ent = e.second;
		ent->update(_now);
		const video::MeshPtr& mesh = ent->mesh();
		if (!mesh->initMesh(shader))
			continue;
		const glm::mat4& translate = glm::translate(glm::mat4(1.0f), ent->position());
		const glm::mat4& scale = glm::scale(translate, glm::vec3(0.01f));
		const glm::mat4& model = glm::rotate(scale, ent->orientation(), glm::vec3(0.0, 1.0, 0.0));
		shader->setUniformMatrix("u_model", model, false);
		mesh->render();
		GL_checkError();
		++drawCallsEntities;
	}
	shader->deactivate();
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_checkError();
	return drawCallsEntities;
}

void WorldRenderer::extractNewMeshes(const glm::vec3& position) {
	const glm::ivec2& camXZ = _world->getGridPos(position);
	const glm::vec2 diff = _lastCameraPosition - camXZ;
	if (glm::length(diff.x) >= 1 || glm::length(diff.y) >= 1) {
		_lastCameraPosition = camXZ;
		extractMeshAroundCamera(40);
	}
}

void WorldRenderer::extractMeshAroundCamera(int amount) {
	const int size = _world->getChunkSize();
	const glm::ivec2& cameraPos = _lastCameraPosition;
	glm::ivec2 pos = cameraPos;
	voxel::Spiral o;
	for (int i = 0; i < amount; ++i) {
		if (!isCulled(pos)) {
			_world->scheduleMeshExtraction(pos);
		}
		o.next();
		pos.x = cameraPos.x + o.x() * size;
		pos.y = cameraPos.y + o.y() * size;
	}
}

void WorldRenderer::onInit() {
	_noiseFuture.push_back(core::App::getInstance()->threadPool().enqueue([] () {
		const int ColorTextureSize = 256;
		const int ColorTextureDepth = 3;
		uint8_t *colorTexture = new uint8_t[ColorTextureSize * ColorTextureSize * ColorTextureDepth];
		noise::Simplex::SeamlessNoise2DRGB(colorTexture, ColorTextureSize, ColorTextureDepth, 0.3f, 0.7f);
		return NoiseGenerationTask(colorTexture, ColorTextureSize, ColorTextureSize, ColorTextureDepth);
	}));
	_colorTexture = video::createTexture("**colortexture**");
}

void WorldRenderer::onRunning(long now) {
	_now = now;
	if (!_noiseFuture.empty()) {
		NoiseFuture& future = _noiseFuture.back();
		if (future.valid()) {
			NoiseGenerationTask c = future.get();
			Log::info("Noise texture ready - upload it");
			_colorTexture->upload(c.buffer, c.width, c.height, c.depth);
			_colorTexture->unbind();
			delete[] c.buffer;
			_noiseFuture.erase(_noiseFuture.end());
		}
	}

	// TODO: properly lerp this
	if (_viewDistance < 500) {
		const int advance = _world->getChunkSize() / 16;
		_viewDistance += advance;
		_fogRange += advance / 2;
	}
}

bool WorldRenderer::isCulled(const glm::ivec2& pos) const {
	const glm::ivec2 dist = pos - _lastCameraPosition;
	const int distance = glm::sqrt(dist.x * dist.x + dist.y * dist.y);
	const float cullingThreshold = 10.0f;
	const int maxAllowedDistance = _viewDistance + cullingThreshold;
	if (distance >= maxAllowedDistance) {
		return true;
	}
	return false;
}

}
