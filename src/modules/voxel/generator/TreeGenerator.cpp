/**
 * @file
 */

#include "TreeGenerator.h"

#include <unordered_map>
#include <functional>

namespace voxel {
namespace tree {

Leaf::Leaf(const glm::vec3& position) :
		_position(position) {
}

Branch::Branch(Branch* parent, const glm::vec3& position, const glm::vec3& growDirection, float size) :
		_parent(parent), _position(position), _growDirection(growDirection), _originalGrowDirection(growDirection), _size(size) {
}

void Branch::reset() {
	_growCount = 0;
	_growDirection = _originalGrowDirection;
}

void Tree::generateCrown(int radius) {
	const glm::ivec3& mins = _crown.mins();
	const glm::ivec3& maxs = _crown.maxs();
	Log::info("Generate tree at mins(%i:%i:%i), maxs(%i:%i:%i)", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	Log::info(" - position: (%f:%f:%f)", _position.x, _position.y, _position.z);
	Log::info(" - trunkHeight: %i", _trunkHeight);
	Log::info(" - branchLength: %i", _branchLength);
	Log::info(" - branchSize: %f", _branchSize);
	Log::info(" - treeHeight: %i", _treeHeight);
	Log::info(" - treeWidth: %i", _treeWidth);
	Log::info(" - treeDepth: %i", _treeDepth);
	Log::info(" - leafCount: %i", _leafCount);
	const int radiusSquare = radius * radius;
	const glm::vec3 center(_crown.getCenter());
	// randomly place leaves within our rectangle
	for (int i = 0; i < _leafCount; ++i) {
		const glm::vec3 location(
				_random.random(_crown.getLowerX(), _crown.getUpperX()),
				_random.random(_crown.getLowerY(), _crown.getUpperY()),
				_random.random(_crown.getLowerZ(), _crown.getUpperZ()));
		if (glm::distance2(location, center) < radiusSquare) {
			_leaves.emplace_back(Leaf(location));
		}
	}
}

void Tree::generateTrunk() {
	_root = new Branch(nullptr, _position, glm::up, _branchSize);
	_branches.insert(std::make_pair(_root->_position, _root));

	Branch* current = new Branch(_root,
			glm::vec3(_position.x, _position.y - _branchLength, _position.z),
			glm::up, _branchSize);
	_branches.insert(std::make_pair(current->_position, current));

	// grow until the trunk height is reached
	while (glm::length(_root->_position - current->_position) < _trunkHeight) {
		Branch *trunk = new Branch(current,
				glm::vec3(current->_position.x, current->_position.y - _branchLength, current->_position.z),
				glm::up, _branchSize);
		_branches.insert(std::make_pair(trunk->_position, trunk));
		current = trunk;
		_branchSize *= _trunkSizeFactor;
	}
}

Tree::Tree(const glm::ivec3& position, int trunkHeight, int branchLength,
	int treeWidth, int treeDepth, int treeHeight, float branchSize, int seed) :
		_position(position.x, position.y + trunkHeight, position.z), _treeWidth(treeWidth), _treeDepth(treeDepth), _treeHeight(treeHeight),
		_trunkHeight(trunkHeight), _branchLength(branchLength), _branchSize(branchSize), _random(seed),
		_crown(_position.x - _treeWidth / 2, _position.y + _trunkHeight, _position.z - _treeDepth / 2,
				_position.x + _treeWidth / 2, _position.y + _treeHeight + _trunkHeight, _position.z + _treeDepth / 2) {
	_leafCount = _treeDepth * 10;
	generateCrown(_treeWidth / 2);
	generateTrunk();
}

Tree::Tree(const core::AABB<int>& crownAABB, int trunkHeight, int branchLength, int seed) :
		_position(crownAABB.getLowerCenter()), _trunkHeight(trunkHeight), _branchLength(branchLength), _random(seed), _crown(crownAABB) {
	_treeWidth = crownAABB.getWidthX();
	_treeHeight = crownAABB.getWidthY();
	_treeDepth = crownAABB.getWidthZ();
	_leafCount = _treeDepth * 10;
	generateCrown(_treeWidth / 2);
	generateTrunk();
}

Tree::~Tree() {
	delete _root;
	for (auto &e : _branches) {
		delete e.second;
	}
	_branches.clear();
}

bool Tree::grow() {
	if (_doneGrowing) {
		return false;
	}

	// If no leaves left, we are done
	if (_leaves.empty()) {
		_doneGrowing = true;
		return false;
	}

	// process the leaves
	for (auto leavesiter = _leaves.begin(); leavesiter != _leaves.end();) {
		bool leafRemoved = false;
		Leaf& leaf = *leavesiter;

		leaf._closestBranch = nullptr;

		// Find the nearest branch for this leaf
		for (auto iter = _branches.begin(); iter != _branches.end(); ++iter) {
			Branch* b = iter->second;
			// direction to branch from leaf
			glm::vec3 direction = leaf._position - b->_position;
			// distance to branch from leaf
			const float distance = (float) glm::round(glm::length(direction));
			direction = glm::normalize(direction);

			// Min leaf distance reached, we remove it
			if (distance <= _minDistance) {
				leavesiter = _leaves.erase(leavesiter);
				leafRemoved = true;
				break;
			} else if (distance <= _maxDistance) {
				// branch in range, determine if it is the nearest
				if (leaf._closestBranch == nullptr) {
					leaf._closestBranch = b;
				} else if (glm::length(leaf._position - leaf._closestBranch->_position) > distance) {
					leaf._closestBranch = b;
				}
			}
		}

		// if the leaf was removed, skip
		if (leafRemoved) {
			continue;
		}

		++leavesiter;

		// Set the grow parameters on all the closest branches that are in range
		if (leaf._closestBranch == nullptr) {
			continue;
		}
		const glm::vec3& dir = glm::normalize(leaf._position - leaf._closestBranch->_position);
		// add to grow direction of branch
		leaf._closestBranch->_growDirection += dir;
		leaf._closestBranch->_growCount++;
	}

	// Generate the new branches
	std::vector<Branch*> newBranches;
	for (auto& e : _branches) {
		Branch* b = e.second;
		// if at least one leaf is affecting the branch
		if (b->_growCount <= 0) {
			continue;
		}
		const glm::vec3& avgDirection = glm::normalize(b->_growDirection / (float)b->_growCount);
		const glm::vec3& branchPos = b->_position + avgDirection * (float)_branchLength;
		Branch *newBranch = new Branch(b, branchPos, avgDirection, b->_size * _branchSizeFactor);
		newBranches.push_back(newBranch);
		b->reset();
	}

	if (newBranches.empty()) {
		return false;
	}

	// Add the new branches to the tree
	bool branchAdded = false;
	for (Branch* b : newBranches) {
		// Check if branch already exists. These cases seem to
		// happen when leaf is in specific areas
		auto findIter = _branches.find(b->_position);
		if (findIter == _branches.end()) {
			_branches.insert(std::make_pair(b->_position, b));
			branchAdded = true;
		}
	}

	// if no branches were added - we are done
	// this handles issues where leaves equal out each other,
	// making branches grow without ever reaching the leaf
	if (!branchAdded) {
		_doneGrowing = true;
		return false;
	}

	return true;
}

}
}
