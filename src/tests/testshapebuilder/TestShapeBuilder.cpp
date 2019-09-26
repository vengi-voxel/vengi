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
	setCameraMotion(false);
	setRenderPlane(true, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));
	setRenderAxis(true);
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
	ImGui::TooltipText("Applies rendering only scale");
	ImGui::Unindent();

	if (ImGui::CollapsingHeader("Sphere", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		ImGui::InputInt("slices", &_sphere.numSlices);
		ImGui::InputInt("stacks", &_sphere.numStacks);
		ImGui::InputFloat("radius", &_sphere.radius);
		if (ImGui::Button("Add sphere")) {
			_shapeBuilder.sphere(_sphere.numSlices, _sphere.numStacks, _sphere.radius);
			buildMesh = true;
		}
	}

	if (ImGui::CollapsingHeader("Cube", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		ImGui::InputFloat3("Mins", glm::value_ptr(_mins));
		ImGui::InputFloat3("Maxs", glm::value_ptr(_maxs));
		if (ImGui::Button("Add cube")) {
			_shapeBuilder.cube(_mins, _maxs);
			buildMesh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add unitcube")) {
			if (_meshCount < lengthof(_meshes)) {
				_meshes[_meshCount++] = _meshUnitCube;
			}
		}
		ImGui::TooltipText("Creates a cube of size 1 with the given scale values\napplied on rendering only.\nIgnores mins/maxs");
		ImGui::SameLine();
		if (ImGui::Button("Add AABB")) {
			_shapeBuilder.aabb(math::AABB<float>(_mins, _maxs));
			buildMesh = true;
		}
	}
	if (ImGui::CollapsingHeader("AABB grid", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		ImGui::InputFloat3("Mins", glm::value_ptr(_mins));
		ImGui::InputFloat3("Maxs", glm::value_ptr(_maxs));
		ImGui::Checkbox("Near plane", &_near);
		ImGui::InputFloat("Step width", &_stepWidth);
		if (ImGui::Button("Add AABB Grid XY")) {
			_shapeBuilder.aabbGridXY(math::AABB<float>(_mins, _maxs), _near, _stepWidth);
			buildMesh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add AABB Grid XZ")) {
			_shapeBuilder.aabbGridXZ(math::AABB<float>(_mins, _maxs), _near, _stepWidth);
			buildMesh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add AABB Grid YZ")) {
			_shapeBuilder.aabbGridYZ(math::AABB<float>(_mins, _maxs), _near, _stepWidth);
			buildMesh = true;
		}
	}

	if (ImGui::CollapsingHeader("Line", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		ImGui::InputFloat3("Start", glm::value_ptr(_line.start));
		ImGui::InputFloat3("End", glm::value_ptr(_line.end));
		if (ImGui::Button("Add Line")) {
			_shapeBuilder.line(_line.start, _line.end);
			buildMesh = true;
		}
	}
	if (ImGui::CollapsingHeader("Pyramid", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		if (ImGui::Button("Add Pyramid")) {
			_shapeBuilder.pyramid(scale);
			buildMesh = true;
		}
	}
	if (ImGui::CollapsingHeader("Cylinder", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		if (ImGui::Button("Add Cylinder")) {
			_shapeBuilder.cylinder(scale.x, scale.y);
			buildMesh = true;
		}
	}
	if (ImGui::CollapsingHeader("Cone", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		if (ImGui::Button("Add Cone")) {
			_shapeBuilder.cone(10.0f, 10.0f, 20.0f, -1, true, true);
			buildMesh = true;
		}
	}

	if (ImGui::CollapsingHeader("Axis", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		if (ImGui::Button("Add Axis")) {
			_shapeBuilder.axis(scale);
			buildMesh = true;
		}
	}

	if (buildMesh && _meshCount < lengthof(_meshes)) {
		_meshes[_meshCount] = _shapeRenderer.create(_shapeBuilder);
		if (_meshes[_meshCount] != -1) {
			++_meshCount;
			_position[_meshCount] = _position[_meshCount - 1];
			_scale[_meshCount] = _scale[_meshCount - 1];
		} else {
			Log::warn("Failed to create the mesh");
		}
	}

	ImGui::Separator();

	ImGui::Text("meshes: %i/%i", _meshCount, (int)lengthof(_meshes));

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
