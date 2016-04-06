#include "ShapeTool.h"
#include "sauce/ShapeToolInjector.h"
#include "core/App.h"
#include "core/Process.h"
#include "voxel/Spiral.h"
#include "video/Shader.h"
#include "video/Color.h"
#include "video/GLDebug.h"

// tool for testing the world createXXX functions without starting the application
ShapeTool::ShapeTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus, voxel::WorldPtr world) :
		ui::UIApp(filesystem, eventBus), _worldRenderer(world), _world(world) {
	init("engine", "shapetool");
}

ShapeTool::~ShapeTool() {
}

core::AppState ShapeTool::onInit() {
	GLDebug::enable(GLDebug::Medium);

	if (!_worldShader.init()) {
		return core::Cleanup;
	}

	_world->setSeed(1);
	_worldRenderer.onInit();
	_camera.init(_width, _height);
	_camera.setAngles(-M_PI_2, M_PI);
	_camera.setPosition(glm::vec3(0.0f, 100.0f, 0.0f));

	_clearColor = video::Color::LightBlue;

	// TODO: replace this with a scripting interface for the World::create* functions
	voxel::Spiral o;
	const int chunkSize = _world->getChunkSize();
	glm::ivec2 pos(chunkSize / 2, chunkSize / 2);
	_world->scheduleMeshExtraction(pos);
	for (int i = 0; i < 8; ++i) {
		o.next();
		glm::ivec2 opos(pos.x + o.x() * chunkSize, pos.x + o.y() * chunkSize);
		_world->scheduleMeshExtraction(opos);
	}

	return ui::UIApp::onInit();
}

core::AppState ShapeTool::onRunning() {
	core::AppState state = UIApp::onRunning();

	_world->onFrame(_deltaFrame);

	_camera.updatePosition(_deltaFrame, false, false, false, false);
	_camera.updateViewMatrix();

	_worldRenderer.onRunning(_now);

	const glm::mat4& view = _camera.getViewMatrix();
	_worldRenderer.renderWorld(_worldShader, view, _aspect);

	return state;
}

core::AppState ShapeTool::onCleanup() {
	core::AppState state = UIApp::onCleanup();
	_world->destroy();
	return state;
}

void ShapeTool::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	UIApp::onMouseMotion(x, y, relX, relY);
	_camera.onMotion(x, y, relX, relY);
}

int main(int argc, char *argv[]) {
	return getInjector()->get<ShapeTool>()->startMainLoop(argc, argv);
}
