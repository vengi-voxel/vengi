/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include "core/collection/DynamicArray.h"
#include "video/Camera.h"
#include "video/Buffer.h"
#include "voxelfont/VoxelFont.h"
#include "RenderShaders.h"

namespace voxelrender {

class VoxelFontRenderer : public core::IComponent {
private:
	struct VertexData {
		struct AttributeData {
			glm::vec4 vertex;
			glm::vec3 color {core::Color::Red};
		};
		core::DynamicArray<AttributeData> vertices;

		inline void reserve(size_t amount) {
			vertices.resize(amount);
		}
	};

	voxel::VoxelFont _voxelFont;
	shader::ColorShader& _colorShader;
	video::Buffer _vertexBuffer;
	int32_t _vertexBufferId = -1;
	int32_t _vertexBufferIndexId = -1;
	glm::mat4 _viewProjectionMatrix { 1.0f };
	glm::mat4 _modelMatrix { 1.0f };
	voxel::IndexArray _indices;
	VertexData _data;
	const int _fontSize;
	const int _depth;
	const uint8_t _optionMask;
public:
	VoxelFontRenderer(int fontSize, int depth = 4, uint8_t optionMask = voxel::VoxelFont::OriginUpperLeft | voxel::VoxelFont::MergeQuads);

	bool init() override;
	void shutdown() override;

	void setViewProjectionMatrix(const glm::mat4& viewProjectionMatrix);
	void setModelMatrix(const glm::mat4& modelMatrix);

	/**
	 * @brief Add the indices and vertices data to the local buffers to render the given string
	 * @note Before rendering the buffers, you have to call @c swapBuffers()
	 */
	void text(const glm::ivec3& pos, const glm::vec4& color, CORE_FORMAT_STRING const char *string, ...) CORE_PRINTF_VARARG_FUNC(4);

	/**
	 * @brief Update the gpu buffers and reset local vertex and index buffers for the next usage
	 */
	void swapBuffers();

	/**
	 * @brief Render the prepared buffers
	 * @note You have to call @c swapBuffers() at least once before using this.
	 */
	void render();

	inline int stringWidth(const char *str, int length) const {
		return _voxelFont.stringWidth(str, length);
	}

	inline int lineHeight() const {
		return _voxelFont.lineHeight();
	}
};

inline void VoxelFontRenderer::setViewProjectionMatrix(const glm::mat4& viewProjectionMatrix) {
	_viewProjectionMatrix = viewProjectionMatrix;
}

inline void VoxelFontRenderer::setModelMatrix(const glm::mat4& modelMatrix) {
	_modelMatrix = modelMatrix;
}

}
