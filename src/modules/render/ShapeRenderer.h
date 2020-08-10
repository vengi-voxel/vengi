/**
 * @file
 */

#pragma once

#include "video/ShapeBuilder.h"
#include "video/Buffer.h"
#include "video/Types.h"
#include "video/Shader.h"
#include "core/IComponent.h"
#include "core/collection/DynamicArray.h"
#include "ColorShader.h"
#include "ColorInstancedShader.h"
#include "TextureShader.h"

namespace video {
class Video;
}

namespace render {

/**
 * @brief Renderer for the shapes that you can build with the ShapeBuilder.
 *
 * @see video::ShapeBuilder
 * @see video::Buffer
 */
class ShapeRenderer : public core::IComponent {
public:
	static constexpr int MAX_MESHES = 16;
private:
	video::Buffer _vbo[MAX_MESHES];
	int32_t _vertexIndex[MAX_MESHES];
	bool _hidden[MAX_MESHES] { false };
	bool _texcoords[MAX_MESHES] { false };
	video::TextureUnit _texunits[MAX_MESHES] {video::TextureUnit::Max};
	int32_t _indexIndex[MAX_MESHES];
	// for instancing
	int32_t _offsetIndex[MAX_MESHES];
	int32_t _amounts[MAX_MESHES];
	video::Primitive _primitives[MAX_MESHES];
	uint32_t _currentMeshIndex = 0u;
	shader::ColorShader& _colorShader;
	shader::ColorInstancedShader& _colorInstancedShader;
	shader::TextureShader& _textureShader;

	int renderAllInstanced(const video::Camera& camera, const glm::mat4& model) const;
	int renderAllTextured(const video::Camera& camera, const glm::mat4& model) const;
	int renderAllColored(const video::Camera& camera, const glm::mat4& model) const;

public:
	ShapeRenderer();
	~ShapeRenderer();

	bool init() override;

	bool deleteMesh(int32_t meshIndex);

	/**
	 * @param[in,out] meshIndex If this is -1 a new mesh is created. The mesh index is 'returned' here. If
	 * this is a valid mesh index, the mesh is updated with the new data from the @c ShapeBuilder
	 */
	void createOrUpdate(int32_t& meshIndex, const video::ShapeBuilder& shapeBuilder);

	int32_t create(const video::ShapeBuilder& shapeBuilder);

	void hide(int32_t meshIndex, bool hide);
	bool hiddenState(int32_t meshIndex) const;

	void setTextureUnit(uint32_t meshIndex, video::TextureUnit unit);

	void shutdown() override;

	void update(uint32_t meshIndex, const video::ShapeBuilder& shapeBuilder);

	/**
	 * @brief Updating the positions for a mesh means that you are doing instanced rendering
	 * @param[in] meshIndex The index returned by @c create() that should not get updated
	 * @param[in] positions The positions to render the mesh instances at
	 * @return @c true if the update was successful, @c false otherwise
	 */
	bool updatePositions(uint32_t meshIndex, const core::DynamicArray<glm::vec3>& positions);

	/**
	 * @brief Updating the positions for a mesh means that you are doing instanced rendering
	 * @param[in] meshIndex The index returned by @c create() that should not get updated
	 * @param[in] posBuf The raw buffer pointer
	 * @param[in] posBufLength The size of the raw buffer given via @c posBuf
	 * @note It's assumed that the positions are always defined by a 3-component float vector
	 * @return @c true if the update was successful, @c false otherwise
	 */
	bool updatePositions(uint32_t meshIndex, const float* posBuf, size_t posBufLength);

	bool render(uint32_t meshIndex, const video::Camera& camera, const glm::mat4& model = glm::mat4(1.0f)) const;

	int renderAll(const video::Camera& camera, const glm::mat4& model = glm::mat4(1.0f)) const;
};

}
