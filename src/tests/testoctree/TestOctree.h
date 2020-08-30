/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "math/Octree.h"
#include "math/Random.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"

class TestOctree: public TestApp {
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

	struct Listener : public Tree::IOctreeListener {
		void onNodeCreated(const Node& parent, const Node& child) const override;
	};

	Listener _listener;

	Tree _octree { math::AABB<int>(glm::ivec3(-1024, 0, -1024), glm::ivec3(1024, 2048, 1024)), 10 };
	mutable video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	math::Random _random;
	bool _dirty = false;
	bool _renderItems = true;
	bool _renderAABBs = true;
	int _nodes = 0;
	int32_t _aabbMeshes = -1;
	int32_t _itemMeshes = -1;
	int32_t _queryMeshes = -1;

	std::vector<math::AABB<int> > _itemVector;
	int _itemIndex = -1;
	Tree::Contents _results;

	math::AABB<int> _queryAABB;
	glm::ivec3 _queryMins;
	glm::ivec3 _queryMaxs;

	void handleDirtyState();
	void clear();
	void insert();
	void doRender() override;
public:
	TestOctree(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual bool onKeyPress(int32_t key, int16_t modifier) override;

	virtual app::AppState onInit() override;
	virtual void onRenderUI() override;
	virtual app::AppState onCleanup() override;
};
