/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "video/Camera.h"
#include "video/Buffer.h"
#include "voxelfont/VoxelFont.h"
#include "RenderShaders.h"
#include <limits>

namespace voxelrender {

class VoxelFontRenderer : public core::IComponent {
private:
	struct VertexData {
		struct AttributeData {
			glm::vec4 vertex;
			glm::vec3 color {core::Color::Red};
		};
		std::vector<AttributeData> vertices;

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
	std::vector<uint32_t> _indices;
	VertexData _data;
	const int _fontSize;
	const int _depth;
public:
	VoxelFontRenderer(int fontSize, int depth = 4);

	bool init() override;
	void shutdown() override;

	void setViewProjectionMatrix(const glm::mat4& viewProjectionMatrix);
	void setModelMatrix(const glm::mat4& modelMatrix);

	void text(const char *string, const glm::ivec3& pos, const glm::vec4& color);

	void render();

	inline int stringWidth(const char *str, int length = std::numeric_limits<int>::max()) const {
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
