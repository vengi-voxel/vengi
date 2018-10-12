/**
 * @file
 */
#include "TestShapeBuilder.h"
#include "io/Filesystem.h"
#include "core/Color.h"
#include "core/Array.h"
#include "math/AABB.h"
#include "core/GLM.h"
#include "ui/imgui/IMGUI.h"

TestShapeBuilder::TestShapeBuilder(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _color(core::Color::DarkGreen) {
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
	Super::onRenderUI();
	const int width = 95;
	bool buildMesh = false;
	_shapeBuilder.clear();
	_shapeBuilder.setColor(_color);
	glm::ivec3& pos = _position[_meshCount];
	glm::vec3& scale = _scale[_meshCount];
	_shapeBuilder.setPosition(pos);

	ImGui::Begin("Actions and Settings");

	ImGui::Text("General settings");
	ImGui::Indent();
	ImGui::ColorEdit4("color", glm::value_ptr(_color), false);
	ImGui::InputInt3("pos", glm::value_ptr(pos));
	ImGui::InputFloat3("scale", glm::value_ptr(scale));
	ImGui::Unindent();

	ImGui::Text("Sphere");
	ImGui::Indent();
	ImGui::InputInt("slices", &_sphere.numSlices);
	ImGui::InputInt("stacks", &_sphere.numStacks);
	ImGui::InputFloat("radius", &_sphere.radius);
	if (ImGui::Button("Sphere")) {
		_shapeBuilder.sphere(_sphere.numSlices, _sphere.numStacks, _sphere.radius);
		buildMesh = true;
	}
	ImGui::Unindent();

	ImGui::Text("Cube");
	ImGui::Indent();
	ImGui::InputFloat3("Mins", glm::value_ptr(_mins));
	ImGui::InputFloat3("Maxs", glm::value_ptr(_maxs));
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
	ImGui::Unindent();

	if (ImGui::CollapsingHeader("AABB grid", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		ImGui::Checkbox("Near plane", &_near);
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
	}

	if (ImGui::Button("Line")) {
		_shapeBuilder.line(_mins, _maxs);
		buildMesh = true;
	}

	if (ImGui::Button("Pyramid")) {
		_shapeBuilder.pyramid(scale);
		buildMesh = true;
	}

	if (ImGui::Button("Axis")) {
		_shapeBuilder.axis(scale);
		buildMesh = true;
	}

	if (buildMesh && _meshCount < lengthof(_meshes)) {
		_meshes[_meshCount] = _shapeRenderer.create(_shapeBuilder);
		if (_meshes[_meshCount] != -1) {
			++_meshCount;
			_position[_meshCount] = _position[_meshCount - 1];
			_scale[_meshCount] = _scale[_meshCount - 1];
		}
	}

	ImGui::Separator();

	ImGui::Text("meshes: %i", _meshCount);

	ImGui::Separator();

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

TEST_APP(TestShapeBuilder)
