#include "TestVoxelFont.h"

TestVoxelFont::TestVoxelFont(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
}

core::AppState TestVoxelFont::onInit() {
	core::AppState state = Super::onInit();
	if (!_voxelFont.init("font.ttf", 14, " Helowrd!")) {
		Log::error("Failed to start voxel font test application - could not load the given font file");
		return core::AppState::Cleanup;
	}

	if (!_colorShader.setup()) {
		Log::error("Failed to initialize the color shader");
		return core::AppState::Cleanup;
	}

	_vertexBufferIndex = _vertexBuffer.create();
	if (_vertexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object");
		return core::AppState::Cleanup;
	}

	_indexBufferIndex = _vertexBuffer.create(nullptr, 0, GL_ELEMENT_ARRAY_BUFFER);
	if (_indexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object for the indices");
		return core::AppState::Cleanup;
	}

	_colorBufferIndex = _vertexBuffer.create();
	if (_colorBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object for the colors");
		return core::AppState::Cleanup;
	}

	// configure shader attributes
	core_assert_always(_vertexBuffer.addAttribute(_colorShader.getLocationPos(), _vertexBufferIndex, _colorShader.getComponentsPos()));
	core_assert_always(_vertexBuffer.addAttribute(_colorShader.getLocationColor(), _colorBufferIndex, _colorShader.getComponentsColor()));

	return state;
}

core::AppState TestVoxelFont::onCleanup() {
	core::AppState state = Super::onCleanup();
	_voxelFont.shutdown();
	_vertexBuffer.shutdown();
	_colorShader.shutdown();
	_vertexBufferIndex = -1;
	_indexBufferIndex = -1;
	_colorBufferIndex = -1;
	return state;
}

void TestVoxelFont::doRender() {
	std::vector<glm::vec3> colors;
	std::vector<glm::vec4> positions;
	std::vector<uint32_t> indices;

	const char* str = "Hello world!";
	const int renderedChars = _voxelFont.render(str, positions, indices);
	if (strlen(str) != renderedChars) {
		Log::error("Failed to render string '%s' (chars: %i)", str, renderedChars);
		requestQuit();
		return;
	}

	if (indices.empty() || positions.empty()) {
		Log::error("Failed to render voxel font");
		requestQuit();
		return;
	}
	colors.assign(positions.size(), core::Color::LightGreen.xyz());

	if (!_vertexBuffer.update(_vertexBufferIndex, positions)) {
		Log::error("Failed to update the vertex buffer");
		requestQuit();
		return;
	}
	if (!_vertexBuffer.update(_indexBufferIndex, indices)) {
		Log::error("Failed to update the index buffer");
		requestQuit();
		return;
	}
	if (!_vertexBuffer.update(_colorBufferIndex, colors)) {
		Log::error("Failed to update the color buffer");
		requestQuit();
		return;
	}

	video::ScopedShader scoped(_colorShader);
	core_assert_always(_colorShader.setView(_camera.viewMatrix()));
	core_assert_always(_colorShader.setProjection(_camera.projectionMatrix()));

	core_assert_always(_vertexBuffer.bind());
	const GLuint nIndices = _vertexBuffer.elements(_indexBufferIndex, 1, sizeof(uint32_t));
	core_assert(nIndices > 0);
	glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, nullptr);
	_vertexBuffer.unbind();
	GL_checkError();
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	TestVoxelFont app(filesystem, eventBus);
	return app.startMainLoop(argc, argv);
}
