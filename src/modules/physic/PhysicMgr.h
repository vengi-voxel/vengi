/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "VoxTree.h"
#include "voxel/polyvox/Region.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

namespace physic {

class PhysicMgr : public core::IComponent {
private:
	btDynamicsWorld* _dynamicsWorld = nullptr;
	btDbvtBroadphase* _broadphase = nullptr;
	btDefaultCollisionConfiguration* _collisionConfiguration = nullptr;
	btCollisionDispatcher* _dispatcher = nullptr;
	btSequentialImpulseConstraintSolver* _solver = nullptr;

	void addStaticBox(const btVector3 &pos, const btVector3 &halfSize);
	int addVoxelNode(VoxTreeNode *node);
public:
	PhysicMgr();
	~PhysicMgr();

	void construct() override;
	bool init() override;
	void shutdown() override;

	template<class Volume>
	int addVoxelTree(const Volume* volume) {
		return addVoxelTree(volume, volume->region());
	}

	template<class Volume>
	int addVoxelTree(const Volume* volume, const voxel::Region& region) {
		const glm::ivec3 minCoord = region.getLowerCorner();
		const glm::ivec3 maxCoord = region.getUpperCorner();

		// This is the bit that does all the magic:
		VoxTree<Volume> tree(volume, minCoord, maxCoord);

		// This is the bit that makes it work in bullet:
		return addVoxelNode(tree.root());
	}
};

}
