/**
 * @file
 */

#include "ShapeTool.h"
#include "video/Shader.h"
#include "video/GLDebug.h"
#include "core/GLM.h"
#include "core/Color.h"
#include "ui/WorldParametersWindow.h"
#include "ui/TreeParametersWindow.h"
#include "frontend/Movement.h"

// tool for testing the world createXXX functions without starting the application
ShapeTool::ShapeTool(video::MeshPoolPtr meshPool, io::FilesystemPtr filesystem, core::EventBusPtr eventBus, voxel::WorldPtr world) :
		Super(filesystem, eventBus), _camera(), _meshPool(meshPool), _worldRenderer(world), _world(world) {
	init("engine", "shapetool");
	_world->setClientData(true);
}

ShapeTool::~ShapeTool() {
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
	core::Command::unregisterCommand("+linemode");
	core::Command::unregisterCommand("camsync");
}

core::AppState ShapeTool::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::Running) {
		return state;
	}

	GLDebug::enable(GLDebug::Medium);

	if (!_axis.init()) {
		return core::Cleanup;
	}

	_speed = core::Var::get(cfg::ClientMouseSpeed, "0.1");
	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");

	core::Command::registerCommand("+linemode", [&] (const core::CmdArgs& args) { \
		if (args.empty()) {
			return;
		}
		_lineModeRendering = args[0] == "true";
	}).setHelp("Toggle line rendering mode");

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	core::Command::registerCommand("camsync", [&] (const core::CmdArgs& args) {
		const video::Camera& sunCamera = _worldRenderer.sunLight().camera();
		_camera.setQuaternion(sunCamera.quaternion());
	}).setHelp("Set the world camera rotation to be equal to the sun rotation");

	_world->setSeed(1);
	if (!_worldRenderer.onInit(_dimension)) {
		return core::Cleanup;
	}
	_camera.init(dimension());
	_camera.setPosition(glm::vec3(50.0f, 100.0f, 50.0f));
	_camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

	_worldRenderer.onSpawn(_camera.position(), core::Var::get(cfg::ShapeToolExtractRadius, "1")->intVal());

	_meshPool->init();

	const char *meshName = "chr_skelett2_bake";
	const video::MeshPtr& mesh = _meshPool->getMesh(meshName);
	if (!mesh) {
		Log::error("Failed to load the mesh '%s'", meshName);
		return core::Cleanup;
	}
	_entity = std::make_shared<frontend::ClientEntity>(1, network::EntityType::NONE, _camera.position(), 0.0f, mesh);
	if (!_worldRenderer.addEntity(_entity)) {
		Log::error("Failed to create entity");
		return core::Cleanup;
	}

	glm::vec3 targetPos = _camera.position();
	targetPos.x += 1000.0f;
	targetPos.z += 1000.0f;
	_entity->lerpPosition(targetPos, _entity->orientation());

	new WorldParametersWindow(this);
	new TreeParametersWindow(this);

	return state;
}

void ShapeTool::beforeUI() {
	_world->onFrame(_deltaFrame);

	if (_resetTriggered && !_world->isReset()) {
		_world->setContext(_ctx);
		_worldRenderer.onSpawn(_camera.position());
		_resetTriggered = false;
	}

	const float speed = _speed->floatVal() * static_cast<float>(_deltaFrame);
	glm::vec3 moveDelta = getMoveDelta(speed, _moveMask);;
	_camera.move(moveDelta);

	_camera.setFarPlane(_worldRenderer.getViewDistance());
	_camera.setFieldOfView(45.0f);
	_camera.setAspectRatio(_aspect);
	_camera.update(_deltaFrame);

	_worldRenderer.extractNewMeshes(_camera.position());
	_worldRenderer.onRunning(_deltaFrame);
	_vertices = 0;
	if (_lineModeRendering) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	_drawCallsWorld = _worldRenderer.renderWorld(_camera, &_vertices);
	_drawCallsEntities = _worldRenderer.renderEntities(_camera);
	if (_lineModeRendering) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void ShapeTool::afterUI() {
	const glm::vec3& pos = _camera.position();
	int meshes;
	int extracted;
	int pending;
	int active;
	_worldRenderer.stats(meshes, extracted, pending, active);
	const int x = 5;
	enqueueShowStr(x, core::Color::White, "drawcalls world: %i (verts: %i)", _drawCallsWorld, _vertices);
	enqueueShowStr(x, core::Color::White, "drawcalls entities: %i", _drawCallsEntities);
	enqueueShowStr(x, core::Color::White, "pos: %.2f:%.2f:%.2f", pos.x, pos.y, pos.z);
	enqueueShowStr(x, core::Color::White, "pending: %i, meshes: %i, extracted: %i, uploaded: %i", pending, meshes, extracted, active);

	enqueueShowStr(x, core::Color::Gray, "+/-: change move speed");
	enqueueShowStr(x, core::Color::Gray, "LMB: perform raycast to voxel under cursor");
	enqueueShowStr(x, core::Color::Gray, "l: line mode rendering");

	Super::afterUI();
}

core::AppState ShapeTool::onRunning() {
	core::AppState state = Super::onRunning();

	_axis.render(_camera);
	//glm::vec3 entPos = _entity->position();
	//entPos.y = _world->findFloor(entPos.x, entPos.z, voxel::isFloor);
	_entity->update(_deltaFrame);
	return state;
}

core::AppState ShapeTool::onCleanup() {
	_meshPool->shutdown();
	_worldRenderer.shutdown();
	_axis.shutdown();
	_entity = frontend::ClientEntityPtr();
	core::AppState state = Super::onCleanup();
	_world->shutdown();
	return state;
}

void ShapeTool::onWindowResize() {
	Super::onWindowResize();
	_camera.init(dimension());
}

bool ShapeTool::onKeyPress(int32_t key, int16_t modifier) {
	if (key == SDLK_ESCAPE) {
		const SDL_bool current = SDL_GetRelativeMouseMode();
		const SDL_bool mode = current ? SDL_FALSE : SDL_TRUE;
		SDL_SetRelativeMouseMode(mode);
		if (mode) {
			_root.SetVisibility(tb::WIDGET_VISIBILITY::WIDGET_VISIBILITY_INVISIBLE);
		} else {
			_root.SetVisibility(tb::WIDGET_VISIBILITY::WIDGET_VISIBILITY_VISIBLE);
		}
	} else if (key == SDLK_PLUS || key == SDLK_KP_PLUS) {
		const float speed = _speed->floatVal() + 0.1f;
		_speed->setVal(std::to_string(speed));
	} else if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
		const float speed = std::max(0.1f, _speed->floatVal() - 0.1f);
		_speed->setVal(std::to_string(speed));
	}
	return Super::onKeyPress(key, modifier);
}

void ShapeTool::onMouseButtonPress(int32_t x, int32_t y, uint8_t button) {
	Super::onMouseButtonPress(x, y, button);

	if (button != SDL_BUTTON_LEFT) {
		return;
	}

	Log::debug("Click to %u:%u", x, y);
	const glm::vec2 mousePos = glm::vec2(float(x), float(y)) / glm::vec2(dimension());
	const video::Ray& ray = _camera.screenRay(mousePos);
	glm::ivec3 hit;
	voxel::Voxel voxel;
	if (_world->raycast(ray.origin, ray.direction, _worldRenderer.getViewDistance(), hit, voxel)) {
		_worldRenderer.setVoxel(hit, voxel::createVoxel(voxel::Air));
		Log::info("Raycast hit %i:%i:%i", hit.x, hit.y, hit.z);
		// TODO: store to place trees at that position
	} else {
		Log::warn("Raycast didn't hit anything");
	}
}

void ShapeTool::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	Super::onMouseMotion(x, y, relX, relY);
	const bool current = SDL_GetRelativeMouseMode();
	if (!current) {
		return;
	}
	_camera.rotate(glm::vec3(relY, relX, 0.0f) * _rotationSpeed->floatVal());
}

void ShapeTool::placeTree(const voxel::TreeContext& ctx) {
	_world->placeTree(ctx);
	regenerate(ctx.pos);
	// TODO: might have affected more than one chunk
}

void ShapeTool::regenerate(const glm::ivec2& pos) {
	_worldRenderer.extractNewMeshes(glm::ivec3(pos.x, 0, pos.y), true);
}

void ShapeTool::reset(const voxel::WorldContext& ctx) {
	_ctx = ctx;
	_worldRenderer.reset();
	_world->reset();
	_resetTriggered = true;
}

int main(int argc, char *argv[]) {
	const video::MeshPoolPtr meshPool = std::make_shared<video::MeshPool>();
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const voxel::WorldPtr world = std::make_shared<voxel::World>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	ShapeTool app(meshPool, filesystem, eventBus, world);
	return app.startMainLoop(argc, argv);
}
