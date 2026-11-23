/**
 * @file
 */
#include "TestShapeBuilder.h"
#include "IMGUIEx.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "math/AABB.h"
#include "testcore/TestAppMain.h"
#include "video/ScopedState.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Mat4Ops.h"
#include "math/Scalar.h"

TestShapeBuilder::TestShapeBuilder(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider), _color(color::DarkGreen()) {
	init(ORGANISATION, "testshapebuilder");
	setCameraMotion(false);
	setRenderPlane(true, math::Vec4f(1.0f, 1.0f, 1.0f, 0.8f));
	setRenderAxis(true);
}

app::AppState TestShapeBuilder::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!_shapeRenderer.init()) {
		Log::error("Failed to init the shape renderer");
		return app::AppState::InitFailure;
	}

	for (int i = 0; i < lengthof(_position); ++i) {
		_scale[i] = 1.0f;
		_position[i] = math::Vec3i(0);
	}

	_shapeBuilder.clear();
	_shapeBuilder.setPosition(math::Vec3f(0.0f));
	_shapeBuilder.setColor(color::Red());
	_shapeBuilder.cube(math::Vec3f(-0.5f), math::Vec3f(0.5f));
	_meshUnitCube = _shapeRenderer.create(_shapeBuilder);
	_shapeBuilder.clear();

	return state;
}

void TestShapeBuilder::doRender() {
	video::ScopedState scoped(video::State::CullFace, true);
	for (int i = 0; i < _meshCount; ++i) {
		const math::Mat4f &model = math::scale(math::translate(math::Mat4f(1.0f), math::Vec3f(_position[i])), _scale[i]);
		_shapeRenderer.render(_meshes[i], camera(), model);
	}
}

void TestShapeBuilder::onRenderUI() {
	Super::onRenderUI();
	bool buildMesh = false;
	_shapeBuilder.clear();
	_shapeBuilder.setColor(_color);
	math::Vec3i &pos = _position[_meshCount];
	math::Vec3f &scale = _scale[_meshCount];
	math::Vec3f &rotation = _rotation[_meshCount];
	_shapeBuilder.setPosition(pos);
	const math::Vec3f radians = math::radians(rotation);
	_shapeBuilder.setRotation(math::eulerAngleXYZ(radians.x, radians.y, radians.z));

	ImGui::Begin("Actions and Settings");

	ImGui::Text("General settings");
	ImGui::Indent();
	ImGui::ColorEdit4("color", math::value_ptr(_color));
	ImGui::InputVec3("pos", pos);
	ImGui::InputVec3("scale", scale);
	if (ImGui::InputVec3("rotation (degree)", rotation)) {
		rotation = math::clamp(rotation, 0.0f, 360.0f);
	}
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

	if (ImGui::CollapsingHeader("Bone", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		ImGui::InputFloat("Length", &_bone.length);
		ImGui::InputFloat("Size", &_bone.posSize);
		ImGui::InputFloat("BoneSize", &_bone.boneSize);
		if (ImGui::Button("Add bone")) {
			_shapeBuilder.bone(_bone.length, _bone.posSize, _bone.boneSize);
			buildMesh = true;
		}
	}

	if (ImGui::CollapsingHeader("Cube", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		ImGui::InputVec3("Mins", _mins);
		ImGui::InputVec3("Maxs", _maxs);
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
		ImGui::TooltipText(
			"Creates a cube of size 1 with the given scale values\napplied on rendering only.\nIgnores mins/maxs");
		ImGui::SameLine();
		if (ImGui::Button("Add AABB")) {
			_shapeBuilder.aabb(math::AABB<float>(_mins, _maxs));
			buildMesh = true;
		}
	}
	if (ImGui::CollapsingHeader("AABB grid", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		ImGui::InputVec3("Mins", _mins);
		ImGui::InputVec3("Maxs", _maxs);
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
		ImGui::InputVec3("Start", _line.start);
		ImGui::InputVec3("End", _line.end);
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
		static int slides = 4;
		static float baseRadius = 20.0f;
		static float length = 100.0f;
		ImGui::InputInt("Slides", &slides);
		ImGui::InputFloat("Radius", &baseRadius);
		ImGui::InputFloat("length", &length);
		if (ImGui::Button("Add Cylinder")) {
			_shapeBuilder.cylinder(baseRadius, length, slides);
			buildMesh = true;
		}
	}
	if (ImGui::CollapsingHeader("Cone", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		static int slides = 4;
		static float baseRadius = 20.0f;
		static float length = 100.0f;
		ImGui::InputInt("Slides", &slides);
		ImGui::InputFloat("Radius", &baseRadius);
		ImGui::InputFloat("length", &length);
		if (ImGui::Button("Add Cone")) {
			_shapeBuilder.cone(baseRadius, length, slides);
			buildMesh = true;
		}
	}

	if (ImGui::CollapsingHeader("Axis", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding)) {
		if (ImGui::Button("Add Axis")) {
			_shapeBuilder.axis(scale);
			buildMesh = true;
		}
	}

	if (buildMesh && _meshCount < lengthof(_meshes) - 1) {
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

app::AppState TestShapeBuilder::onCleanup() {
	_shapeRenderer.shutdown();
	return Super::onCleanup();
}

TEST_APP(TestShapeBuilder)
