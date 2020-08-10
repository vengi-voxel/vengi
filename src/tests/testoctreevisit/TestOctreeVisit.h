/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "math/Octree.h"
#include "math/Random.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "core/collection/DynamicArray.h"

class TestOctreeVisit: public TestApp {
private:
	using Super = TestApp;
	class Wrapper {
	private:
		math::AABB<int> _aabb;
	public:
		Wrapper(const glm::ivec3& pos) :
				_aabb(pos, pos + 1) {
		}
		inline math::AABB<int> aabb() const {
			return _aabb;
		}
	};

	using Tree = math::Octree<Wrapper>;
	using Node = Tree::OctreeNode;

	Tree _octree { math::AABB<int>(glm::ivec3(-1024, 0, -1024), glm::ivec3(1024, 2048, 1024)), 10 };
	mutable video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	video::Camera _octreeCamera;
	int32_t _itemMesh = -1;
	int32_t _frustumMesh = -1;
	int32_t _aabbMesh = -1;
	core::DynamicArray<glm::vec3> _positions;
	glm::vec3 _pos;
	glm::vec3 _lookAt{10.0f, 70.0f, 10.0f};
	glm::vec3 _omega{0.0f, 0.1f, 0.0f};
	float _farPlane = 500.0f;
	float _nearPlane = 0.1f;
	bool _ortho = false;

	void updateCamera();
	void doRender() override;
	void onRenderUI() override;
public:
	TestOctreeVisit(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
};
