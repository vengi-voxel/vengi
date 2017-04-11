/**
 * @file
 */

#include "TreeGenerator.h"

#include <unordered_map>
#include <functional>

namespace voxel {
namespace tree {

AttractionPoint::AttractionPoint(const glm::vec3& position) :
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
	Log::debug("Generate tree at mins(%i:%i:%i), maxs(%i:%i:%i)", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	Log::debug(" - position: (%f:%f:%f)", _position.x, _position.y, _position.z);
	Log::debug(" - trunkHeight: %i", _trunkHeight);
	Log::debug(" - branchLength: %i", _branchLength);
	Log::debug(" - branchSize: %f", _branchSize);
	Log::debug(" - treeHeight: %i", _treeHeight);
	Log::debug(" - treeWidth: %i", _treeWidth);
	Log::debug(" - treeDepth: %i", _treeDepth);
	Log::debug(" - attractionPointCount: %i", _attractionPointCount);
	const int radiusSquare = radius * radius;
	const glm::vec3 center(_crown.getCenter());
	// randomly place attraction points within our rectangle
	for (int i = 0; i < _attractionPointCount; ++i) {
		const glm::vec3 location(
				_random.random(_crown.getLowerX(), _crown.getUpperX()),
				_random.random(_crown.getLowerY(), _crown.getUpperY()),
				_random.random(_crown.getLowerZ(), _crown.getUpperZ()));
		if (glm::distance2(location, center) < radiusSquare) {
			_attrationPoints.emplace_back(AttractionPoint(location));
		}
	}
}

void Tree::generateTrunk() {
	_root = new Branch(nullptr, _position, glm::down, _branchSize);
	_branches.insert(std::make_pair(_root->_position, _root));

	Branch* current = new Branch(_root,
			glm::vec3(_position.x, _position.y - _branchLength, _position.z),
			glm::down, _branchSize);
	_branches.insert(std::make_pair(current->_position, current));

	// grow until the trunk height is reached
	while (glm::length(_root->_position - current->_position) < _trunkHeight) {
		Branch *trunk = new Branch(current,
				glm::vec3(current->_position.x, current->_position.y - _branchLength, current->_position.z),
				glm::down, _branchSize);
		_branches.insert(std::make_pair(trunk->_position, trunk));
		current = trunk;
		_branchSize *= _trunkSizeFactor;
	}
}

Tree::Tree(const glm::ivec3& position, int trunkHeight, int branchLength,
	int treeWidth, int treeDepth, int treeHeight, float branchSize, int seed) :
		_position(position.x, position.y + trunkHeight, position.z), _treeWidth(treeWidth), _treeDepth(treeDepth), _treeHeight(treeHeight),
		_trunkHeight(trunkHeight), _branchLength(branchLength), _branchSize(branchSize), _random(seed),
		_crown(_position.x - _treeWidth / 2, _position.y, _position.z - _treeDepth / 2,
				_position.x + _treeWidth / 2, _position.y + _treeHeight, _position.z + _treeDepth / 2) {
	_attractionPointCount = _treeDepth * 10;
	generateCrown(_treeWidth / 2);
	generateTrunk();
	//generateRoots(_treeWidth / 2);
}

Tree::Tree(const core::AABB<int>& crownAABB, int trunkHeight, int branchLength, int seed) :
		_position(crownAABB.getLowerCenter()), _trunkHeight(trunkHeight), _branchLength(branchLength), _random(seed), _crown(crownAABB) {
	_treeWidth = crownAABB.getWidthX();
	_treeHeight = crownAABB.getWidthY();
	_treeDepth = crownAABB.getWidthZ();
	_attractionPointCount = _treeDepth * 10;
	generateCrown(_treeWidth / 2);
	generateTrunk();
}

Tree::~Tree() {
	for (auto &e : _branches) {
		delete e.second;
	}
	_root = nullptr;
	_branches.clear();
	_attrationPoints.clear();
}

bool Tree::grow() {
	if (_doneGrowing) {
		return false;
	}

	// If no attraction points left, we are done
	if (_attrationPoints.empty()) {
		_doneGrowing = true;
		return false;
	}

	// process the attraction points
	for (auto pi = _attrationPoints.begin(); pi != _attrationPoints.end();) {
		bool attractionPointRemoved = false;
		AttractionPoint& attractionPoint = *pi;

		attractionPoint._closestBranch = nullptr;

		// Find the nearest branch for this attraction point
		for (auto bi = _branches.begin(); bi != _branches.end(); ++bi) {
			Branch* branch = bi->second;
			// distance to branch from attraction point
			const float length = (float) glm::round(glm::length2(attractionPoint._position - branch->_position));

			// Min attraction point distance reached, we remove it
			if (length <= _minDistance2) {
				pi = _attrationPoints.erase(pi);
				attractionPointRemoved = true;
				break;
			} else if (length <= _maxDistance2) {
				// branch in range, determine if it is the nearest
				if (attractionPoint._closestBranch == nullptr) {
					attractionPoint._closestBranch = branch;
				} else if (glm::length(attractionPoint._position - attractionPoint._closestBranch->_position) > length) {
					attractionPoint._closestBranch = branch;
				}
			}
		}

		// if the attraction point was removed, skip
		if (attractionPointRemoved) {
			continue;
		}

		++pi;

		// Set the grow parameters on all the closest branches that are in range
		if (attractionPoint._closestBranch == nullptr) {
			continue;
		}
		const glm::vec3& dir = glm::normalize(attractionPoint._position - attractionPoint._closestBranch->_position);
		// add to grow direction of branch
		attractionPoint._closestBranch->_growDirection += dir;
		++attractionPoint._closestBranch->_growCount;
	}

	// Generate the new branches
	std::vector<Branch*> newBranches;
	for (auto& e : _branches) {
		Branch* branch = e.second;
		// if at least one attraction point is affecting the branch
		if (branch->_growCount <= 0) {
			continue;
		}
		const glm::vec3& avgDirection = glm::normalize(branch->_growDirection / (float)branch->_growCount);
		const glm::vec3& branchPos = branch->_position + avgDirection * (float)_branchLength;
		Branch *newBranch = new Branch(branch, branchPos, avgDirection, branch->_size * _branchSizeFactor);
		newBranches.push_back(newBranch);
		branch->reset();
	}

	if (newBranches.empty()) {
		return false;
	}

	// Add the new branches to the tree
	bool branchAdded = false;
	for (Branch* branch : newBranches) {
		// Check if branch already exists. These cases seem to
		// happen when attraction point is in specific areas
		auto i = _branches.find(branch->_position);
		if (i == _branches.end()) {
			_branches.insert(std::make_pair(branch->_position, branch));
			branchAdded = true;
		}
	}

	// if no branches were added - we are done
	// this handles issues where attraction points equal out each other,
	// making branches grow without ever reaching the attraction point
	if (!branchAdded) {
		_doneGrowing = true;
		return false;
	}

	return true;
}

}
}
