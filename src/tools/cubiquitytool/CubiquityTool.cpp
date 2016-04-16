#include "CubiquityTool.h"
#include "sauce/CubiquityToolInjector.h"
#include "video/Shader.h"
#include "video/Color.h"
#include "video/GLDebug.h"
#include "frontend/Movement.h"
// TODO: rmeove me
#include "cubiquity/CubiquityC.h"

CubiquityTool::CubiquityTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus, voxel::WorldPtr world) :
		ui::UIApp(filesystem, eventBus), _worldRenderer(world), _world(world), _currentShader(nullptr) {
	init("engine", "cubiquitytool");
}

CubiquityTool::~CubiquityTool() {
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
}

core::AppState CubiquityTool::onInit() {
	int rc;
	if (_argc == 2) {
		_currentShader = &_coloredCubesShader;
		rc = cuNewColoredCubesVolumeFromVDB("colored.vdb", CU_READONLY, 32, &_worldRenderer._volumeHandle);
	} else {
		_currentShader = &_terrainShader;
		rc = cuNewTerrainVolumeFromVDB("terrain.vdb", CU_READONLY, 32, &_worldRenderer._volumeHandle);
	}
	if (rc != CU_OK) {
		Log::error("%s : %s", cuGetErrorCodeAsString(rc), cuGetLastErrorMessage());
		return core::AppState::Cleanup;
	}

	core::AppState state = ui::UIApp::onInit();
	GLDebug::enable(GLDebug::Medium);

	if (!_terrainShader.init()) {
		return core::Cleanup;
	}

	if (!_coloredCubesShader.init()) {
		return core::Cleanup;
	}

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	_world->setSeed(1);
	_worldRenderer.onInit();
	_camera.init(_width, _height);
	_camera.setAngles(-M_PI_2, M_PI);
	_camera.setPosition(glm::vec3(0.0f, 100.0f, 0.0f));

	_clearColor = video::Color::LightBlue;

	return state;
}

void CubiquityTool::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	UIApp::onMouseMotion(x, y, relX, relY);
	_camera.onMotion(x, y, relX, relY);
}

void CubiquityTool::beforeUI() {
	_world->onFrame(_deltaFrame);

	const bool left = _moveMask & MOVELEFT;
	const bool right = _moveMask & MOVERIGHT;
	const bool forward = _moveMask & MOVEFORWARD;
	const bool backward = _moveMask & MOVEBACKWARD;
	_camera.updatePosition(_deltaFrame, left, right, forward, backward);
	_camera.updateViewMatrix();

	_worldRenderer.onRunning(_deltaFrame);

	const glm::mat4& view = _camera.getViewMatrix();
	_worldRenderer.renderOctree(*_currentShader, view, _aspect);
}

core::AppState CubiquityTool::onCleanup() {
	_worldRenderer.onCleanup();
	core::AppState state = UIApp::onCleanup();
	_world->destroy();
	return state;
}

int main(int argc, char *argv[]) {
	return getInjector()->get<CubiquityTool>()->startMainLoop(argc, argv);
}
