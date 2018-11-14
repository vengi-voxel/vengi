/**
 * @file
 */

#include "VoxelFontRenderer.h"

namespace voxelrender {

VoxelFontRenderer::VoxelFontRenderer(int fontSize, int depth) :
		_colorShader(shader::ColorShader::getInstance()), _fontSize(fontSize), _depth(depth) {
}

bool VoxelFontRenderer::init() {
	if (!_colorShader.setup()) {
		Log::error("Failed to init color shader");
		return false;
	}

	if (_fontSize <= 0) {
		Log::error("Invalid font size given: %i", _fontSize);
		return false;
	}

	if (_depth <= 0) {
		Log::error("Invalid depth given: %i", _depth);
		return false;
	}

	if (!_voxelFont.init("font.ttf", _fontSize, _depth, true, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^"
			"_`abcdefghijklmnopqrstuvwxyz{|}~€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸"
			"¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ")) {
		Log::error("Failed to init voxel font");
		return false;
	}

	_vertexBufferId = _vertexBuffer.create();
	if (_vertexBufferId < 0) {
		Log::error("Failed to create vertex buffer");
		return false;
	}
	_vertexBuffer.setMode(_vertexBufferId, video::BufferMode::Dynamic);
	_vertexBufferIndexId = _vertexBuffer.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (_vertexBufferIndexId < 0) {
		Log::error("Failed to create index buffer");
		return false;
	}

	video::Attribute attribPos;
	attribPos.bufferIndex = _vertexBufferId;
	attribPos.location = _colorShader.enableVertexAttributeArray("a_pos");
	attribPos.stride = sizeof(VertexData::AttributeData);
	attribPos.size = _colorShader.getAttributeComponents(attribPos.location);
	attribPos.type = video::mapType<decltype(VertexData::AttributeData::vertex)::value_type>();
	attribPos.offset = offsetof(VertexData::AttributeData, vertex);
	if (!_vertexBuffer.addAttribute(attribPos)) {
		Log::error("Failed to add position attribute");
		return false;
	}

	video::Attribute attribColor;
	attribColor.bufferIndex = _vertexBufferId;
	attribColor.location = _colorShader.enableVertexAttributeArray("a_color");
	attribColor.stride = sizeof(VertexData::AttributeData);
	attribColor.size = _colorShader.getAttributeComponents(attribColor.location);
	attribColor.type = video::mapType<decltype(VertexData::AttributeData::color)::value_type>();
	attribColor.offset = offsetof(VertexData::AttributeData, color);
	if (!_vertexBuffer.addAttribute(attribColor)) {
		Log::error("Failed to add color attribute");
		return false;
	}

	return true;
}

void VoxelFontRenderer::shutdown() {
	_colorShader.shutdown();
	_vertexBuffer.shutdown();
	_voxelFont.shutdown();

	_vertexBufferId = -1;
	_vertexBufferIndexId = -1;
}

void VoxelFontRenderer::text(const char *string, const glm::ivec3& pos, const glm::vec4& color) {
	_voxelFont.render(string, _data.vertices, _indices, [&] (const voxel::VoxelVertex& vertex, std::vector<VertexData::AttributeData>& data, int x, int y) {
		const VertexData::AttributeData vp{glm::vec4(vertex.position.x + x + pos.x, vertex.position.y + y + pos.y, vertex.position.z + pos.z, 1.0f), color};
		data.push_back(vp);
	});
}

void VoxelFontRenderer::render() {
	const size_t elements = _vertexBuffer.elements(_vertexBufferIndexId, 1, sizeof(uint32_t));
	if (elements <= 0u) {
		return;
	}

	if (!_indices.empty()) {
		// TODO: the vertices should only be uploaded once for the whole glyph set. only the ibo should be dynamic and re-uploaded
		_vertexBuffer.update(_vertexBufferId, _data.vertices);
		_vertexBuffer.update(_vertexBufferIndexId, _indices);
	}

	video::ScopedShader scoped(_colorShader);
	_colorShader.setViewprojection(_viewProjectionMatrix);
	_colorShader.setModel(_modelMatrix);

	video::ScopedBuffer scopedBuf(_vertexBuffer);
	video::drawElements<uint32_t>(video::Primitive::Triangles, elements);

	_indices.clear();
	_data.vertices.clear();
}

}
