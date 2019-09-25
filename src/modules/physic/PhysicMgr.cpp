/**
 * @file
 */

#include "PhysicMgr.h"

namespace physic {

PhysicMgr::PhysicMgr() {
}

PhysicMgr::~PhysicMgr() {
	shutdown();
}

void PhysicMgr::construct() {
}

bool PhysicMgr::init() {
	_broadphase = new btDbvtBroadphase();

	_collisionConfiguration = new btDefaultCollisionConfiguration();
	_dispatcher = new btCollisionDispatcher(_collisionConfiguration);

	_solver = new btSequentialImpulseConstraintSolver();

	_dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _broadphase, _solver,
			_collisionConfiguration);

	_dynamicsWorld->setGravity(btVector3(0, -900, 0));
	return true;
}

void PhysicMgr::shutdown() {
	delete _dynamicsWorld;
	_dynamicsWorld = nullptr;
}

int PhysicMgr::addVoxelNode(VoxTreeNode *node) {
	switch (node->_value) {
	case VoxTreeNode::nodeMixed: {
		int n = 0;
		n += addVoxelNode(node->_children[0]);
		n += addVoxelNode(node->_children[1]);
		return n;
	}
	case VoxTreeNode::nodeFull: {
		btVector3 low(btScalar(node->_mins.x), btScalar(node->_mins.y), btScalar(node->_mins.z));
		low -= btVector3(0.5f, 0.5f, 0.5f);
		btVector3 high(btScalar(node->_maxs.x), btScalar(node->_maxs.y), btScalar(node->_maxs.z));
		high += btVector3(0.5f, 0.5f, 0.5f);
		addStaticBox(0.5f * (low + high), 0.5f * (high - low));
		return 1;
	}
	default:
		break;
	}
	return 0;
}

void PhysicMgr::addStaticBox(const btVector3 &pos, const btVector3 &halfSize) {
	btCollisionShape *colShape = new btBoxShape(halfSize);
	const btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, nullptr, colShape);
	btRigidBody *body = new btRigidBody(rbInfo);
	btTransform trans;
	trans.setIdentity();
	trans.setOrigin(pos);
	body->setWorldTransform(trans);
	_dynamicsWorld->addRigidBody(body);
}
}
