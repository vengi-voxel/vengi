/**
 * @file
 */

#include "ShapeTool.h"
#include "sauce/ShapeToolInjector.h"
#include "video/Shader.h"
#include "video/GLDebug.h"
#include "core/GLM.h"
#include "core/Color.h"
#include "ui/WorldParametersWindow.h"
#include "ui/TreeParametersWindow.h"
#include "frontend/Movement.h"

// tool for testing the world createXXX functions without starting the application
ShapeTool::ShapeTool(video::MeshPoolPtr meshPool, io::FilesystemPtr filesystem, core::EventBusPtr eventBus, voxel::WorldPtr world) :
		ui::UIApp(filesystem, eventBus), _meshPool(meshPool), _worldRenderer(world), _world(world), _worldShader(), _plantShader(), _waterShader(),
		_meshShader(), _colorShader(), _deferredDirLightShader() {
	init("engine", "shapetool");
	_world->setClientData(true);
}

ShapeTool::~ShapeTool() {
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
}

core::AppState ShapeTool::onInit() {
	core::AppState state = ui::UIApp::onInit();
	if (state != core::Running)
		return state;

	GLDebug::enable(GLDebug::Medium);

	if (!_worldShader.setup()) {
		return core::Cleanup;
	}
	if (!_plantShader.setup()) {
		return core::Cleanup;
	}
	if (!_waterShader.setup()) {
		return core::Cleanup;
	}
	if (!_meshShader.setup()) {
		return core::Cleanup;
	}
	if (!_colorShader.setup()) {
		return core::Cleanup;
	}
	if (!_deferredDirLightShader.setup()) {
		return core::Cleanup;
	}

	_speed = core::Var::get(cfg::ClientMouseSpeed, "0.1");
	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	_world->setSeed(1);
	_worldRenderer.onInit(_plantShader, _deferredDirLightShader, _width, _height);
	_camera.init(_width, _height);
	_camera.setAngles(-glm::half_pi<float>(), glm::pi<float>());
	_camera.setPosition(glm::vec3(0.0f, 100.0f, 0.0f));

	_clearColor = core::Color::LightBlue;

	_worldRenderer.onSpawn(_camera.position(), core::Var::get(cfg::ShapeToolExtractRadius, "1")->intVal());

	const char *meshName = "chr_fatkid";
	const video::MeshPtr& mesh = _meshPool->getMesh(meshName);
	if (!mesh) {
		Log::error("Failed to load the mesh '%s'", meshName);
		return core::Cleanup;
	}
	_entity = std::make_shared<frontend::ClientEntity>(1, -1, _now, _camera.position(), 0.0f, mesh);
	if (!_worldRenderer.addEntity(_entity)) {
		Log::error("Failed to create entity");
		return core::Cleanup;
	}

	glm::vec3 targetPos = _camera.position();
	targetPos.x += 1000.0f;
	targetPos.z += 1000.0f;
	_entity->lerpPosition(_now, targetPos, _entity->orientation());

	static const float verticesAxis[] = {
			  0.0f,   0.0f,   0.0f, 1.0f,
			 20.0f,   0.0f,   0.0f, 1.0f,
			  0.0f,   0.0f,   0.0f, 1.0f,
			  0.0f,  20.0f,   0.0f, 1.0f,
			  0.0f,   0.0f,   0.0f, 1.0f,
			  0.0f,   0.0f,  20.0f, 1.0f};

	static const float colorAxis[] = {
			1.0f, 0.0f, 0.0f, 1.0f,
			1.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 1.0f, 0.0f, 1.0f,
			0.0f, 1.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 1.0f, 1.0f,
			0.0f, 0.0f, 1.0f, 1.0f};

	const int32_t vIndex = _axisBuffer.create(verticesAxis, sizeof(verticesAxis));
	const int32_t cIndex = _axisBuffer.create(colorAxis, sizeof(colorAxis));
	core_assert(vIndex >= 0 && cIndex >= 0);
	_axisBuffer.addAttribute(_colorShader.getAttributeLocation("a_pos"), vIndex, 4);
	_axisBuffer.addAttribute(_colorShader.getAttributeLocation("a_color"), cIndex, 4);

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

	const bool left = _moveMask & MOVELEFT;
	const bool right = _moveMask & MOVERIGHT;
	const bool forward = _moveMask & MOVEFORWARD;
	const bool backward = _moveMask & MOVEBACKWARD;
	const float speed = _speed->floatVal();
	const float farPlane = _worldRenderer.getViewDistance();
	_camera.updatePosition(_deltaFrame, left, right, forward, backward, speed);
	_camera.perspective(45.0f, _aspect, 0.1f, farPlane);
	_camera.update();

	_worldRenderer.extractNewMeshes(_camera.position());
	_worldRenderer.onRunning(_deltaFrame);
	_vertices = 0;
	_drawCallsWorld = _worldRenderer.renderWorld(_worldShader, _plantShader, _waterShader, _deferredDirLightShader, _camera, &_vertices);
	_drawCallsEntities = _worldRenderer.renderEntities(_meshShader, _camera);
}

void ShapeTool::afterUI() {
	tb::TBStr drawCallsWorld;
	drawCallsWorld.SetFormatted("drawcalls world: %i (verts: %i)", _drawCallsWorld, _vertices);
	tb::TBStr drawCallsEntity;
	drawCallsEntity.SetFormatted("drawcalls entities: %i", _drawCallsEntities);
	tb::TBStr position;
	const glm::vec3& pos = _camera.position();
	position.SetFormatted("pos: %.2f:%.2f:%.2f", pos.x, pos.y, pos.z);
	tb::TBStr extractions;
	int meshes;
	int extracted;
	int pending;
	_worldRenderer.stats(meshes, extracted, pending);
	extractions.SetFormatted("pending: %i, meshes: %i, extracted: %i", pending, meshes, extracted);
	tb::TBFontFace *font = _root.GetFont();
	const tb::TBColor color(255, 255, 255);
	const int x = 5;
	int y = 20;
	const int lineHeight = font->GetHeight() + 2;
	font->DrawString(x, y, color, drawCallsEntity);
	y += lineHeight;
	font->DrawString(x, y, color, drawCallsWorld);
	y += lineHeight;
	font->DrawString(x, y, color, position);
	y += lineHeight;
	font->DrawString(x, y, color, extractions);
	ui::UIApp::afterUI();
}

core::AppState ShapeTool::onRunning() {
	core::AppState state = UIApp::onRunning();

	const glm::mat4& view = _camera.viewMatrix();

	_colorShader.activate();
	_colorShader.setUniformMatrix("u_view", view, false);
	_colorShader.setUniformMatrix("u_projection", _camera.projectionMatrix(), false);

	//glm::vec3 entPos = _entity->position();
	//entPos.y = _world->findFloor(entPos.x, entPos.z);
	_entity->update(_deltaFrame);
	// TODO: add x, y and z letters to the axis
	glDisable(GL_DEPTH_TEST);
	core_assert_always(_axisBuffer.bind());
	glLineWidth(4.0f);
	glDrawArrays(GL_LINES, 0, 6);
	glLineWidth(1.0f);
	_axisBuffer.unbind();

	_colorShader.deactivate();
	GL_checkError();

	return state;
}

core::AppState ShapeTool::onCleanup() {
	_meshPool->shutdown();
	_worldRenderer.shutdown();
	_worldShader.shutdown();
	_plantShader.shutdown();
	_waterShader.shutdown();
	_meshShader.shutdown();
	_colorShader.shutdown();
	_deferredDirLightShader.shutdown();
	_axisBuffer.shutdown();
	_entity = frontend::ClientEntityPtr();
	core::AppState state = UIApp::onCleanup();
	_world->shutdown();
	return state;
}

void ShapeTool::onWindowResize() {
	_camera.init(_width, _height);
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
	return UIApp::onKeyPress(key, modifier);
}

void ShapeTool::onMouseButtonPress(int32_t x, int32_t y, uint8_t button) {
	UIApp::onMouseButtonPress(x, y, button);

	if (button != SDL_BUTTON_LEFT) {
		return;
	}

	Log::debug("Click to %u:%u", x, y);
	const glm::vec2 mousePos(x / float(width()), y / float(height()));
	const video::Ray& ray = _camera.screenRay(mousePos);
	glm::ivec3 hit;
	voxel::Voxel voxel;
	if (_world->raycast(ray.origin, ray.direction, _worldRenderer.getViewDistance(), hit, voxel)) {
		_worldRenderer.setVoxel(hit, voxel::createVoxel(voxel::Air));
	} else {
		Log::warn("Raycast didn't hit anything");
	}
}

void ShapeTool::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	UIApp::onMouseMotion(x, y, relX, relY);
	const bool current = SDL_GetRelativeMouseMode();
	if (!current) {
		return;
	}
	_camera.onMotion(x, y, relX, relY, _rotationSpeed->floatVal());
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
	return getInjector()->get<ShapeTool>()->startMainLoop(argc, argv);
}
