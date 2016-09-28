#include "TestVoxelFont.h"

TestVoxelFont::TestVoxelFont(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus) :
		Super(filesystem, eventBus), _colorShader(shader::ColorShader::getInstance()) {
	setCameraMotion(true);
}

core::AppState TestVoxelFont::onInit() {
	core::AppState state = Super::onInit();
	if (!changeFontSize(0)) {
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

	_camera.setFarPlane(4000.0f);

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

bool TestVoxelFont::changeFontSize(int delta) {
	_voxelFont.shutdown();
	_fontSize += delta;
	if (_fontSize < 2) {
		_fontSize = 2;
	} else if (_fontSize > 250) {
		_fontSize = 250;
	}
	return _voxelFont.init("font.ttf", _fontSize, 4, false, " Helowrd!");
}

void TestVoxelFont::onMouseWheel(int32_t x, int32_t y) {
	const SDL_Keymod mods = SDL_GetModState();
	if (mods & KMOD_SHIFT) {
		changeFontSize(y);
		return;
	}

	Super::onMouseWheel(x, y);
}

bool TestVoxelFont::onKeyPress(int32_t key, int16_t modifier) {
	const bool retVal = Super::onKeyPress(key, modifier);
	if (modifier & KMOD_SHIFT) {
		int delta = 0;
		if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
			delta = -1;
		} else if (key == SDLK_PLUS || key == SDLK_KP_PLUS) {
			delta = 1;
		}

		if (delta != 0) {
			changeFontSize(delta);
			return true;
		}
	}

	return retVal;
}

void TestVoxelFont::doRender() {
	std::vector<glm::vec3> colors;
	std::vector<glm::vec4> positions;
	std::vector<uint32_t> indices;

	const char* str = "Hello world!";
	const int renderedChars = _voxelFont.render(str, positions, indices);
	if ((int)strlen(str) != renderedChars) {
		Log::error("Failed to render string '%s' (chars: %i)", str, renderedChars);
		requestQuit();
		return;
	}

	if (indices.empty() || positions.empty()) {
		Log::error("Failed to render voxel font");
		requestQuit();
		return;
	}
	colors.resize(positions.size());
	class ColorGenerator {
	private:
		glm::vec3 _colors[2];
		int _index = 0;
		int _amount = 0;
	public:
		ColorGenerator (const glm::vec4& color1, const glm::vec4& color2) {
			_colors[0] = color1.xyz();
			_colors[1] = color2.xyz();
		}

		inline const glm::vec3& operator()() {
			++_amount;
			if (_amount >= 4) {
				_index = _index + 1;
				_index %= SDL_arraysize(_colors);
				_amount = 0;
			}
			return _colors[_index];
		}
	};
	ColorGenerator g(core::Color::LightGreen, core::Color::Red);
	std::generate(colors.begin(), colors.end(), g);

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
