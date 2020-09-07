/**
 * @file
 */

#include "VoxelFontRenderer.h"
#include "core/collection/DynamicArray.h"

namespace voxelrender {

VoxelFontRenderer::VoxelFontRenderer(int fontSize, int depth, uint8_t optionMask) :
		_colorShader(shader::ColorShader::getInstance()), _fontSize(fontSize), _depth(depth), _optionMask(optionMask) {
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

	if (!_voxelFont.init("font.ttf", _fontSize, _depth, _optionMask, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^"
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

	const video::Attribute& attribPos = _colorShader.getPosAttribute(_vertexBufferId, &VertexData::AttributeData::vertex);
	if (!_vertexBuffer.addAttribute(attribPos)) {
		Log::error("Failed to add position attribute");
		return false;
	}

	const video::Attribute& attribColor = _colorShader.getColorAttribute(_vertexBufferId, &VertexData::AttributeData::color);
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

	_indices.clear();
	_data.vertices.clear();
	_modelMatrix = glm::mat4(1.0f);
	_viewProjectionMatrix = glm::mat4(1.0f);
}

void VoxelFontRenderer::text(const glm::ivec3& pos, const glm::vec4& color, const char *string, ...) {
	va_list ap;
	const size_t bufSize = 4096;
	char buf[bufSize];

	va_start(ap, string);
	SDL_vsnprintf(buf, bufSize, string, ap);
	buf[sizeof(buf) - 1] = '\0';
	va_end(ap);

	_voxelFont.render(buf, _data.vertices, _indices, [&] (const voxel::VoxelVertex& vertex, core::DynamicArray<VertexData::AttributeData>& data, int x, int y) {
		const VertexData::AttributeData vp{glm::vec4(vertex.position.x + x + pos.x, vertex.position.y + y + pos.y, vertex.position.z + pos.z, 1.0f), color};
		data.push_back(vp);
	});
}

void VoxelFontRenderer::swapBuffers() {
	// TODO: the vertices should only be uploaded once for the whole glyph set. only the ibo should be dynamic and re-uploaded
	_vertexBuffer.update(_vertexBufferId, &_data.vertices.front(), _data.vertices.size() * sizeof(voxel::VertexArray::value_type));
	_vertexBuffer.update(_vertexBufferIndexId, &_indices.front(), _indices.size() * sizeof(voxel::IndexArray::value_type));

	_indices.clear();
	_data.vertices.clear();
}

void VoxelFontRenderer::render() {
	const size_t elements = _vertexBuffer.elements(_vertexBufferIndexId, 1, sizeof(uint32_t));
	if (elements <= 0u) {
		return;
	}

	video::ScopedShader scoped(_colorShader);
	_colorShader.setViewprojection(_viewProjectionMatrix);
	_colorShader.setModel(_modelMatrix);

	video::ScopedBuffer scopedBuf(_vertexBuffer);
	video::drawElements<uint32_t>(video::Primitive::Triangles, elements);
}

}
