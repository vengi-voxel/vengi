/**
 * @file
 */

#include "SpaceColonization.h"
#include "core/Common.h"
#include "core/Log.h"

#include <functional>

namespace voxelgenerator {
namespace tree {

AttractionPoint::AttractionPoint(const glm::vec3& position) :
		_position(position) {
}

Branch::Branch(Branch* parent, const glm::vec3& position, const glm::vec3& growDirection, float size) :
		_parent(parent), _children(32), _position(position), _growDirection(growDirection), _originalGrowDirection(growDirection), _size(size) {
	if (_parent) {
		_parent->_children.push_back(this);
	}
}

void Branch::reset() {
	_attractionPointInfluence = 0;
	_growDirection = _originalGrowDirection;
}

SpaceColonization::SpaceColonization(const glm::ivec3& position, int branchLength,
	int attractionPointWidth, int attractionPointHeight, int attractionPointDepth, float branchSize,
	unsigned int seed, int minDistance, int maxDistance, int attractionPointCount) :
		_position(position), _attractionPointCount(attractionPointCount), _attractionPointWidth(attractionPointWidth),
		_attractionPointDepth(attractionPointDepth), _attractionPointHeight(attractionPointHeight),
		_minDistance2(minDistance * minDistance), _maxDistance2(maxDistance * maxDistance),
		_branchLength(branchLength), _branchSize(branchSize), _random(seed) {
	_root = new Branch(nullptr, _position, glm::up, _branchSize);
	_branches.put(_root->_position, _root);

	fillAttractionPoints();
}

SpaceColonization::~SpaceColonization() {
	for (auto e : _branches) {
		delete e->value;
	}
	_root = nullptr;
	_branches.clear();
	_attractionPoints.clear();
}

void SpaceColonization::fillAttractionPoints() {
	const float radius = core_max(_attractionPointHeight, core_max(_attractionPointDepth, _attractionPointWidth)) / 2.0f;
	const glm::ivec3 mins(_position.x - (_attractionPointWidth / 2), _position.y, _position.z - (_attractionPointDepth / 2));
	const glm::ivec3 maxs(mins.x + _attractionPointWidth, mins.y + _attractionPointHeight, mins.z + _attractionPointDepth);
	const float radiusSquare = radius * radius;
	int failed = 0;
	const glm::vec3 center((mins + maxs) / 2);
	for (int i = 0; i < _attractionPointCount;) {
		const glm::vec3 location(
				_random.random(mins.x, maxs.x),
				_random.random(mins.y, maxs.y),
				_random.random(mins.z, maxs.z));
		if (glm::distance2(location, center) < radiusSquare) {
			_attractionPoints.emplace_back(AttractionPoint(location));
			++i;
		} else {
			++failed;
			if (failed > _attractionPointCount) {
				break;
			}
		}
	}
}

void SpaceColonization::grow() {
	int n = 100;
	while (step() && --n > 0) {
	}
	if (n <= 0) {
		Log::warn("Could not finish space colonization growing");
	}
}

bool SpaceColonization::step() {
	if (_doneGrowing) {
		return false;
	}

	// If no attraction points left, we are done
	if (_attractionPoints.empty()) {
		_doneGrowing = true;
		return false;
	}

	// process the attraction points
	for (auto pi = _attractionPoints.begin(); pi != _attractionPoints.end();) {
		bool attractionPointRemoved = false;
		AttractionPoint& attractionPoint = *pi;

		attractionPoint._closestBranch = nullptr;

		// Find the nearest branch for this attraction point
		for (auto bi = _branches.begin(); bi != _branches.end(); ++bi) {
			Branch* branch = bi->second;
			const float length2 = (float) glm::round(glm::distance2(branch->_position, attractionPoint._position));

			// Min attraction point distance reached, we remove it
			if (attractionPoint._closestBranch == nullptr) {
				attractionPoint._closestBranch = branch;
			} else if (length2 <= _minDistance2) {
				pi = _attractionPoints.erase(pi);
				attractionPointRemoved = true;
				break;
			} else if (length2 <= _maxDistance2) {
				// branch in range, determine if it is the nearest
				if (glm::distance2(attractionPoint._closestBranch->_position, attractionPoint._position) > length2) {
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
		++attractionPoint._closestBranch->_attractionPointInfluence;
	}

	// Generate the new branches
	core::DynamicArray<Branch*> newBranches;
	newBranches.reserve(_branches.size());
	for (auto e : _branches) {
		Branch* branch = e->value;
		// if at least one attraction point is affecting the branch
		if (branch->_attractionPointInfluence <= 0) {
			continue;
		}
		const glm::vec3& avgDirection = branch->_growDirection / (float)branch->_attractionPointInfluence;
		const glm::vec3& branchPos = branch->_position + avgDirection * (float)_branchLength;
		Branch *newBranch = new Branch(branch, branchPos, avgDirection, branch->_size * _branchSizeFactor);
		newBranches.push_back(newBranch);
		branch->reset();
	}

	if (newBranches.empty()) {
		_doneGrowing = true;
		return false;
	}

	// Add the new branches to the tree
	bool branchAdded = false;
	for (Branch* branch : newBranches) {
		// Check if branch already exists. These cases seem to
		// happen when attraction point is in specific areas
		auto i = _branches.find(branch->_position);
		if (i != _branches.end()) {
			auto& c = branch->_parent->_children;
			for (size_t i = 0; i < c.size(); ++i) {
				if (c[i] == branch) {
					c.erase(i);
					break;
				}
			}

			delete branch;
			continue;
		}
		_branches.put(branch->_position, branch);
		branchAdded = true;
	}
	newBranches.clear();

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
