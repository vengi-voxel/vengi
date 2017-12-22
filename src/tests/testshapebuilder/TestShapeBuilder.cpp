#include "TestShapeBuilder.h"
#include "io/Filesystem.h"
#include "core/Color.h"
#include "core/Array.h"
#include "math/AABB.h"
#include "core/GLM.h"
#include "imgui/IMGUI.h"

TestShapeBuilder::TestShapeBuilder(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider), _color(core::Color::DarkGreen) {
	init(ORGANISATION, "testshapebuilder");
	setCameraMotion(true);
	setRenderPlane(true);
	setRenderAxis(true);
	setCameraSpeed(0.5f);
}

core::AppState TestShapeBuilder::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!_shapeRenderer.init()) {
		Log::error("Failed to init the shape renderer");
		return core::AppState::InitFailure;
	}

	for (int i = 0; i < lengthof(_position); ++i) {
		_scale[i] = glm::one<glm::vec3>();
		_position[i] = glm::zero<glm::ivec3>();
	}

	_shapeBuilder.clear();
	_shapeBuilder.setPosition(glm::vec3(0.0f));
	_shapeBuilder.setColor(core::Color::Red);
	_shapeBuilder.cube(glm::vec3(-0.5f), glm::vec3(0.5f));
	_meshUnitCube = _shapeRenderer.create(_shapeBuilder);
	_shapeBuilder.clear();

	return state;
}

void TestShapeBuilder::doRender() {
	for (int i = 0; i < _meshCount; ++i) {
		const glm::mat4& model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(_position[i])), _scale[i]);
		_shapeRenderer.render(_meshes[i], _camera, model);
	}
}

void TestShapeBuilder::onRenderUI() {
	const int width = 95;
	bool buildMesh = false;
	_shapeBuilder.clear();
	_shapeBuilder.setColor(_color);
	glm::ivec3& pos = _position[_meshCount];
	glm::vec3& scale = _scale[_meshCount];
	_shapeBuilder.setPosition(pos);

	ImGui::SetNextWindowSize(ImVec2(400, 120));
	ImGui::Begin("Keys and information");
	Super::onRenderUI();
	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(540, 300));
	ImGui::Begin("Actions and Settings");

	ImGui::ColorEdit4("color", glm::value_ptr(_color), false);
	ImGui::PushItemWidth(width);
	ImGui::InputInt("x", &pos.x);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputInt("y", &pos.y);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputInt("z", &pos.z);

	ImGui::PushItemWidth(width);
	ImGui::InputFloat("sx", &scale.x);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputFloat("sy", &scale.y);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputFloat("sz", &scale.z);

	int numSlices = 5;
	int numStacks = 4;
	float radius = 20.0f;
	ImGui::PushItemWidth(width);
	ImGui::InputInt("slices", &numSlices);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputInt("stacks", &numStacks);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputFloat("radius", &radius);
	ImGui::SameLine();
	if (ImGui::Button("Sphere")) {
		_shapeBuilder.sphere(numSlices, numStacks, radius);
		buildMesh = true;
	}
	ImGui::Separator();

	ImGui::PushItemWidth(width);
	ImGui::InputFloat("mins.x", &_mins.x);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputFloat("mins.y", &_mins.y);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputFloat("mins.z", &_mins.z);

	ImGui::PushItemWidth(width);
	ImGui::InputFloat("maxs.x", &_maxs.x);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputFloat("maxs.y", &_maxs.y);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputFloat("maxs.z", &_maxs.z);

	if (ImGui::Button("Cube")) {
		_shapeBuilder.cube(_mins, _maxs);
		buildMesh = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("UnitCube")) {
		if (_meshCount < lengthof(_meshes)) {
			_meshes[_meshCount++] = _meshUnitCube;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("AABB")) {
		_shapeBuilder.aabb(math::AABB<float>(_mins, _maxs));
		buildMesh = true;
	}

	ImGui::Checkbox("Near plane", &_near);
	ImGui::SameLine();
	ImGui::InputFloat("Step width", &_stepWidth);
	if (ImGui::Button("AABB Grid XY")) {
		_shapeBuilder.aabbGridXY(math::AABB<float>(_mins, _maxs), _near, _stepWidth);
		buildMesh = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("AABB Grid XZ")) {
		_shapeBuilder.aabbGridXZ(math::AABB<float>(_mins, _maxs), _near, _stepWidth);
		buildMesh = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("AABB Grid YZ")) {
		_shapeBuilder.aabbGridYZ(math::AABB<float>(_mins, _maxs), _near, _stepWidth);
		buildMesh = true;
	}
	ImGui::Separator();

	if (buildMesh && _meshCount < lengthof(_meshes)) {
		_meshes[_meshCount] = _shapeRenderer.create(_shapeBuilder);
		if (_meshes[_meshCount] != -1) {
			++_meshCount;
		}
	}

	ImGui::Separator();

	ImGui::Text("meshes: %i", _meshCount);

	ImGui::Separator();

	ImGui::Checkbox("Render Axis", &_renderAxis);
	ImGui::Checkbox("Render Plane", &_renderPlane);

	if (ImGui::Button("Clear")) {
		for (int i = 0; i < _meshCount; ++i) {
			_shapeRenderer.deleteMesh(_meshes[i]);
			_meshes[i] = -1;
		}
		_meshCount = 0;
	}
	ImGui::End();
}

core::AppState TestShapeBuilder::onCleanup() {
	core::AppState state = Super::onCleanup();
	_shapeRenderer.shutdown();
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	TestShapeBuilder app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
