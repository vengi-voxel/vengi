/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "core/Octree.h"
#include "core/Random.h"
#include "video/ShapeBuilder.h"
#include "frontend/ShapeRenderer.h"

class TestOctree: public TestApp {
private:
	using Super = TestApp;
	class Wrapper {
	private:
		core::AABB<int> _aabb;
	public:
		Wrapper(const glm::ivec3& pos) :
				_aabb(pos, pos + 1) {
		}
		inline core::AABB<int> aabb() const {
			return _aabb;
		}
	};

	using Tree = core::Octree<Wrapper>;
	using Node = Tree::OctreeNode;

	class Listener : public Tree::IOctreeListener {
	private:
		TestOctree* _app;
	public:
		Listener(TestOctree* app) :
				_app(app) {
		}
		void onNodeCreated(const Node& parent, const Node& child) const override;
	};

	Listener _listener;

	Tree _octree { core::AABB<int>(glm::ivec3(-1024, 0, -1024), glm::ivec3(1024, 2048, 1024)), 10 };
	mutable video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;
	core::Random _random;
	bool _dirty = false;
	int _nodes = 0;
	int32_t _aabbMeshes = -1;
	int32_t _itemMeshes = -1;
	int32_t _queryMeshes = -1;

	Tree::Contents _results;

	core::AABB<int> _queryAABB;
	glm::ivec3 _queryMins;
	glm::ivec3 _queryMaxs;

	void handleDirtyState();
	void clear();
	void insert();
	void doRender() override;
public:
	TestOctree(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual bool onKeyPress(int32_t key, int16_t modifier) override;

	virtual core::AppState onInit() override;
	virtual void onRenderUI() override;
	virtual core::AppState onCleanup() override;
};
