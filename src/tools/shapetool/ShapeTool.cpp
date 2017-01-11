/**
 * @file
 */

#include "ShapeTool.h"
#include "video/Shader.h"
#include "video/GLDebug.h"
#include "core/GLM.h"
#include "core/Color.h"
#include "ui/WorldParametersWindow.h"
#include "frontend/Movement.h"
#include "voxel/MaterialColor.h"

ShapeTool::ShapeTool(const video::MeshPoolPtr& meshPool, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const voxel::WorldPtr& world) :
		Super(filesystem, eventBus, timeProvider), _camera(), _meshPool(meshPool), _worldRenderer(world), _world(world) {
	init(ORGANISATION, "shapetool");
	_world->setClientData(true);
}

ShapeTool::~ShapeTool() {
}

core::AppState ShapeTool::onConstruct() {
	core::AppState state = Super::onConstruct();

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
	core::Var::get(cfg::VoxelMeshSize, "128", core::CV_READONLY);
	core::Var::get(cfg::ShapeToolExtractRadius, "1");

	core::Command::registerCommand("freelook", [this] (const core::CmdArgs& args) {
		this->_freelook ^= true;
	}).setHelp("Toggle free look");

	_worldRenderer.onConstruct();

	return state;
}

core::AppState ShapeTool::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	GLDebug::enable(GLDebug::Medium);

	if (!_axis.init()) {
		return core::AppState::Cleanup;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::Cleanup;
	}

	if (!_world->init(filesystem()->open("world.lua"))) {
		return core::AppState::Cleanup;
	}

	_world->setSeed(1);
	if (!_worldRenderer.onInit(glm::ivec2(), _dimension)) {
		return core::AppState::Cleanup;
	}
	_camera.init(glm::ivec2(), dimension());
	_camera.setFieldOfView(45.0f);
	_camera.setPosition(glm::vec3(50.0f, 100.0f, 50.0f));
	_camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

	_worldRenderer.onSpawn(_camera.position(), core::Var::getSafe(cfg::ShapeToolExtractRadius)->intVal());

	_meshPool->init();

	const char *meshName = "chr_skelett2_bake";
	const video::MeshPtr& mesh = _meshPool->getMesh(meshName);
	if (!mesh) {
		Log::error("Failed to load the mesh '%s'", meshName);
		return core::AppState::Cleanup;
	}
	_entity = std::make_shared<frontend::ClientEntity>(1, network::EntityType::NONE, _camera.position(), 0.0f, mesh);
	if (!_worldRenderer.addEntity(_entity)) {
		Log::error("Failed to create entity");
		return core::AppState::Cleanup;
	}

	glm::vec3 targetPos = _camera.position();
	targetPos.x += 1000.0f;
	targetPos.z += 1000.0f;
	_entity->lerpPosition(targetPos, _entity->orientation());

	_worldTimer.init();

	new WorldParametersWindow(this);

	return state;
}

void ShapeTool::beforeUI() {
	ScopedProfiler<ProfilerCPU> but(_beforeUiTimer);
	_world->onFrame(_deltaFrame);

	if (_resetTriggered && !_world->isReset()) {
		_world->setContext(_ctx);
		_worldRenderer.onSpawn(_camera.position());
		_resetTriggered = false;
	}

	const float speed = _speed->floatVal() * static_cast<float>(_deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, _moveMask);
	_camera.move(moveDelta);
	if (!_freelook) {
		const glm::vec3& position = _camera.position();
		const int y = _world->findFloor(position.x, position.z, [] (voxel::VoxelType type) {
			return voxel::isFloor(type);
		});
		_camera.setPosition(glm::vec3(position.x, y + 10, position.z));
	}
	_camera.setFarPlane(_worldRenderer.getViewDistance());
	_camera.update(_deltaFrame);

	_worldRenderer.extractNewMeshes(_camera.position());
	_worldRenderer.onRunning(_camera, _deltaFrame);
	ScopedProfiler<ProfilerGPU> wt(_worldTimer);
	if (_lineModeRendering) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	_drawCallsWorld = _worldRenderer.renderWorld(_camera, &_vertices);
	_drawCallsEntities = _worldRenderer.renderEntities(_camera);
	if (_lineModeRendering) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void ShapeTool::afterRootWidget() {
	const glm::vec3& pos = _camera.position();
	int meshes;
	int extracted;
	int pending;
	int active;
	_worldRenderer.stats(meshes, extracted, pending, active);
	const int x = 5;
	enqueueShowStr(x, core::Color::White, "%s: %f, max: %f", _frameTimer.name().c_str(), _frameTimer.avg(), _frameTimer.maximum());
	enqueueShowStr(x, core::Color::White, "%s: %f, max: %f", _beforeUiTimer.name().c_str(), _beforeUiTimer.avg(), _beforeUiTimer.maximum());
	enqueueShowStr(x, core::Color::White, "%s: %f, max: %f", _worldTimer.name().c_str(), _worldTimer.avg(), _worldTimer.maximum());
	enqueueShowStr(x, core::Color::White, "drawcalls world: %i (verts: %i)", _drawCallsWorld, _vertices);
	enqueueShowStr(x, core::Color::White, "drawcalls entities: %i", _drawCallsEntities);
	enqueueShowStr(x, core::Color::White, "pos: %.2f:%.2f:%.2f", pos.x, pos.y, pos.z);
	enqueueShowStr(x, core::Color::White, "pending: %i, meshes: %i, extracted: %i, uploaded: %i", pending, meshes, extracted, active);

	enqueueShowStr(x, core::Color::Gray, "+/-: change move speed");
	enqueueShowStr(x, core::Color::Gray, "l: line mode rendering");

	Super::afterRootWidget();
}

core::AppState ShapeTool::onRunning() {
	ScopedProfiler<ProfilerCPU> wt(_frameTimer);
	const core::AppState state = Super::onRunning();

	_axis.render(_camera);
	//glm::vec3 entPos = _entity->position();
	//entPos.y = _world->findFloor(entPos.x, entPos.z, voxel::isFloor);
	_entity->update(_deltaFrame);
	return state;
}

core::AppState ShapeTool::onCleanup() {
	_meshPool->shutdown();
	_worldRenderer.shutdown();
	_worldTimer.shutdown();
	_axis.shutdown();
	_entity = frontend::ClientEntityPtr();
	const core::AppState state = Super::onCleanup();
	_world->shutdown();
	return state;
}

void ShapeTool::onWindowResize() {
	Super::onWindowResize();
	_camera.init(glm::ivec2(), dimension());
}

bool ShapeTool::onKeyPress(int32_t key, int16_t modifier) {
	if (key == SDLK_ESCAPE) {
		toggleRelativeMouseMode();
		if (isRelativeMouseMode()) {
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

void ShapeTool::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	Super::onMouseMotion(x, y, relX, relY);
	const bool current = isRelativeMouseMode();
	if (!current) {
		return;
	}
	_camera.rotate(glm::vec3(relY, relX, 0.0f) * _rotationSpeed->floatVal());
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
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	ShapeTool app(meshPool, filesystem, eventBus, timeProvider, world);
	return app.startMainLoop(argc, argv);
}
