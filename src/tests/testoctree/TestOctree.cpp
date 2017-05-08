#include "TestOctree.h"
#include "io/Filesystem.h"
#include "imgui/IMGUI.h"
#include "core/Color.h"
#include <array>

TestOctree::TestOctree(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider), _listener(this) {
	init(ORGANISATION, "testoctree");
	setCameraMotion(true);
	//setRenderPlane(true);
	setRenderAxis(true);
	setCameraSpeed(0.5f);
}

bool TestOctree::onKeyPress(int32_t key, int16_t modifier) {
	if (Super::onKeyPress(key, modifier)) {
		return true;
	}
	const SDL_bool current = SDL_GetRelativeMouseMode();
	if (current) {
		if (key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_INSERT) {
			insert();
		} else if (key == SDLK_DELETE || key == SDLK_KP_CLEAR) {
			clear();
		}
	}
	return true;
}

void TestOctree::insert() {
	const core::AABB<int>& aabb = _octree.aabb();
	const glm::ivec3& mins = aabb.mins();
	const glm::ivec3& maxs = aabb.maxs();
	glm::ivec3 pos(glm::uninitialize);
	for (int i = 0; i < (int)decltype(pos)::length(); ++i) {
		pos[i] = _random.random(mins[i] + 1, maxs[i] - 1);
	}
	if (_octree.insert(Wrapper(pos))) {
		_dirty = true;
	} else {
		Log::info("Failed to add element for %i:%i:%i", pos.x, pos.y, pos.z);
	}
}

void TestOctree::clear() {
	_octree.clear();
	_results.clear();
	_dirty = true;
}

core::AppState TestOctree::onInit() {
	core::AppState state = Super::onInit();

	if (!_shapeRenderer.init()) {
		Log::error("Failed to init the shape renderer");
		return core::AppState::Cleanup;
	}

	const glm::ivec3& center = _octree.aabb().getCenter();
	_queryMins = center - 150;
	_queryMaxs = center + 150;

	_octree.setListener(&_listener);
	_camera.setFarPlane(4000.0f);

	return state;
}

void TestOctree::Listener::onNodeCreated(const Node& parent, const Node& child) const {
	Log::info("Created node");
}

void TestOctree::handleDirtyState() {
	if (!_dirty) {
		return;
	}
	_dirty = false;

	_nodes = 0;
	_octree.visit([&] (const Node& node) {
		++_nodes;
		Log::info("aabb for depth %i: %s", node.depth(), glm::to_string(node.aabb().getWidth()).c_str());
	});

	// build AABBs
	_octree.visit([this] (const Node& node) {
		static const std::array<glm::vec4, 5> colors {
			core::Color::Blue,
			core::Color::Red,
			core::Color::Green,
			core::Color::Yellow,
			core::Color::Cyan
		};
		_shapeBuilder.setColor(colors[node.depth() % colors.size()]);
		_shapeBuilder.aabb(node.aabb());
	});
	if (_aabbMeshes == -1) {
		_aabbMeshes = _shapeRenderer.createMesh(_shapeBuilder);
	} else {
		_shapeRenderer.update(_aabbMeshes, _shapeBuilder);
	}
	_shapeBuilder.clear();

	// build spheres
	_octree.visit([this] (const Node& node) {
		const Tree::Contents& contents = node.getContents();
		for (const Wrapper& wrapper : contents) {
			const core::AABB<int>& itemAABB = wrapper.aabb();
			_shapeBuilder.setPosition(itemAABB.getCenter());
			_shapeBuilder.sphere(10, 10, 5.0f);
		}
	});
	if (_itemMeshes == -1) {
		_itemMeshes = _shapeRenderer.createMesh(_shapeBuilder);
	} else {
		_shapeRenderer.update(_itemMeshes, _shapeBuilder);
	}

	_shapeBuilder.clear();
}

void TestOctree::onRenderUI() {
	handleDirtyState();
	ImGui::SetNextWindowSize(ImVec2(400, 120));
	ImGui::Begin("Keys and information");
	ImGui::BulletText("+/INSERT: Insert new element");
	ImGui::BulletText("DELETE: Remove all elements");
	Super::onRenderUI();
	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(500, 180));
	ImGui::Begin("Actions");
	if (ImGui::Button("Clear")) {
		clear();
	}
	ImGui::SameLine();
	if (ImGui::Button("Insert")) {
		// TODO: insert with position and size
		insert();
	}
	ImGui::Separator();

	const int width = 95;
	ImGui::PushItemWidth(width);
	ImGui::InputInt("mins.x", &_queryMins.x);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputInt("mins.y", &_queryMins.y);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputInt("mins.z", &_queryMins.z);

	ImGui::PushItemWidth(width);
	ImGui::InputInt("maxs.x", &_queryMaxs.x);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputInt("maxs.y", &_queryMaxs.y);
	ImGui::SameLine();
	ImGui::PushItemWidth(width);
	ImGui::InputInt("maxs.z", &_queryMaxs.z);

	if (ImGui::Button("Query")) {
		_results.clear();
		_queryAABB = core::AABB<int>(_queryMins, _queryMaxs);
		_octree.query(_queryAABB, _results);
		//_octree.query(_octree.aabb(), _results);
		Log::info("Query (%i:%i:%i) to (%i:%i:%i) (found: %i)",
				_queryMins.x, _queryMins.y, _queryMins.z, _queryMaxs.x, _queryMaxs.y, _queryMaxs.z, (int)_results.size());
	}
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Pink);
	_shapeBuilder.aabb(_queryAABB);
	if (_queryMeshes == -1) {
		_queryMeshes = _shapeRenderer.createMesh(_shapeBuilder);
	} else {
		_shapeRenderer.update(_queryMeshes, _shapeBuilder);
	}
	_shapeBuilder.clear();
	const glm::ivec3& mins = _octree.aabb().mins();
	const glm::ivec3& maxs = _octree.aabb().maxs();
	ImGui::Text("Tree size: mins(%i:%i:%i) maxs(%i:%i:%i)",
			mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	ImGui::BulletText("Nodes: %i", _nodes);
	ImGui::BulletText("Elements: %i", _octree.count());
	ImGui::BulletText("Results: %i", (int)_results.size());
	ImGui::End();

	ImGui::Begin("Elements");
	// TODO: visit tree with elements and show aabb - transfer aabb to query parameter on click
	ImGui::End();
}

void TestOctree::doRender() {
	_shapeRenderer.renderAll(_camera);
}

core::AppState TestOctree::onCleanup() {
	core::AppState state = Super::onCleanup();
	_shapeRenderer.shutdown();
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	TestOctree app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
