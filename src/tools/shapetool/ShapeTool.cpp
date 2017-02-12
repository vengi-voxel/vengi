/**
 * @file
 */

#include "ShapeTool.h"
#include "video/Shader.h"
#include "video/Renderer.h"
#include "core/GLM.h"
#include "core/Color.h"
#include "io/Filesystem.h"
#include "frontend/Movement.h"
#include "voxel/MaterialColor.h"

ShapeTool::ShapeTool(const video::MeshPoolPtr& meshPool, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider), _camera(), _meshPool(meshPool) {
	init(ORGANISATION, "shapetool");
}

ShapeTool::~ShapeTool() {
}

core::AppState ShapeTool::onConstruct() {
	core::AppState state = Super::onConstruct();

	_speed = core::Var::get(cfg::ClientMouseSpeed, "0.1");
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);

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

	return state;
}

core::AppState ShapeTool::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	video::enableDebug(video::DebugSeverity::High);

	if (!_axis.init()) {
		return core::AppState::Cleanup;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::Cleanup;
	}

	if (!_biomeManager.init(filesystem()->load("biomes.lua"))) {
		return core::AppState::Cleanup;
	}
	if (!_ctx.load(filesystem()->load("world.lua"))) {
		return core::AppState::Cleanup;
	}
	_volumeData = new voxel::PagedVolume(&_pager);
	_pager.init(_volumeData, &_biomeManager, &_ctx);

	const voxel::Region region(0, 0, 0, 255, 127, 255);
	if (!_worldRenderer.init(_volumeData, region, 32)) {
		return core::AppState::Cleanup;
	}
	_camera.init(glm::ivec2(), dimension());
	_camera.setFieldOfView(45.0f);
	_camera.setPosition(glm::vec3(50.0f, 100.0f, 50.0f));
	_camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

	_meshPool->init();

	const char *meshName = "chr_skelett2_bake";
	const video::MeshPtr& mesh = _meshPool->getMesh(meshName);
	if (!mesh) {
		Log::error("Failed to load the mesh '%s'", meshName);
		return core::AppState::Cleanup;
	}
	_entity = std::make_shared<frontend::ClientEntity>(1, network::EntityType::NONE, _camera.position(), 0.0f, mesh);

	glm::vec3 targetPos = _camera.position();
	targetPos.x += 1000.0f;
	targetPos.z += 1000.0f;
	_entity->lerpPosition(targetPos, _entity->orientation());

	_worldTimer.init();

	return state;
}

void ShapeTool::beforeUI() {
	ScopedProfiler<ProfilerCPU> but(_beforeUiTimer);
	_worldRenderer.update(_deltaFrame, _camera);

	const float speed = _speed->floatVal() * static_cast<float>(_deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, _moveMask);
	_camera.move(moveDelta);
	_camera.update(_deltaFrame);

	ScopedProfiler<video::ProfilerGPU> wt(_worldTimer);
	if (_lineModeRendering) {
		video::polygonMode(video::Face::FrontAndBack, video::PolygonMode::WireFrame);
	}
	_worldRenderer.render(_camera);
	if (_lineModeRendering) {
		video::polygonMode(video::Face::FrontAndBack, video::PolygonMode::Solid);
	}
}

void ShapeTool::afterRootWidget() {
	const glm::vec3& pos = _camera.position();
	const int x = 5;
	enqueueShowStr(x, core::Color::White, "%s: %f, max: %f", _frameTimer.name().c_str(), _frameTimer.avg(), _frameTimer.maximum());
	enqueueShowStr(x, core::Color::White, "%s: %f, max: %f", _beforeUiTimer.name().c_str(), _beforeUiTimer.avg(), _beforeUiTimer.maximum());
	enqueueShowStr(x, core::Color::White, "%s: %f, max: %f", _worldTimer.name().c_str(), _worldTimer.avg(), _worldTimer.maximum());
	enqueueShowStr(x, core::Color::White, "pos: %.2f:%.2f:%.2f", pos.x, pos.y, pos.z);

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

int main(int argc, char *argv[]) {
	const video::MeshPoolPtr meshPool = std::make_shared<video::MeshPool>();
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	ShapeTool app(meshPool, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
