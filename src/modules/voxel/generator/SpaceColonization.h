/**
 * @file
 */

#pragma once

#include "core/Random.h"
#include "ShapeGenerator.h"
#include "core/Log.h"

#include <unordered_map>

namespace voxel {

struct Branch;

struct AttractionPoint {
	glm::vec3 _position;
	Branch* _closestBranch = nullptr;

	AttractionPoint(const glm::vec3& position);
};

struct Branch {
	Branch *_parent;
	std::vector<Branch*> _children;
	glm::vec3 _position;
	glm::vec3 _growDirection;
	glm::vec3 _originalGrowDirection;
	int _attractionPointInfluence = 0;
	float _size;

	Branch(Branch* parent, const glm::vec3& position, const glm::vec3& growDirection, float size);

	void reset();
};

/**
 * @brief Space colonization algorithm
 *
 * http://www.jgallant.com/procedurally-generating-trees-with-space-colonization-algorithm-in-xna/
 */
class SpaceColonization {
protected:
	bool _doneGrowing = false;
	glm::vec3 _position;

	int _attractionPointCount;
	int _attractionPointWidth;
	int _attractionPointDepth;
	int _attractionPointHeight;
	int _minDistance2;
	int _maxDistance2;
	int _branchLength;
	float _branchSize;
	float _branchSizeFactor = 0.9f;

	Branch *_root;
	using AttractionPoints = std::vector<AttractionPoint>;
	AttractionPoints _attractionPoints;
	using Branches = std::unordered_map<glm::vec3, Branch*, std::hash<glm::vec3>>;
	Branches _branches;
	core::Random _random;

	/**
	 * Generate the attraction points for the crown
	 */
	void fillAttractionPoints();

	template<class Volume, class Voxel>
	void generateLeaves_r(Volume& volume, const Voxel& voxel, Branch* branch, const glm::ivec3& leafSize) const {
		if (branch->_children.empty()) {
			voxel::shape::createEllipse(volume, branch->_position, leafSize.x, leafSize.y, leafSize.z, voxel);
			return;
		}
		for (Branch* b : branch->_children) {
			generateLeaves_r(volume, voxel, b, leafSize);
		}
	}

public:
	SpaceColonization(const glm::ivec3& position, int branchLength = 6,
		int crownWidth = 40, int crownHeight = 60, int crownDepth = 40, float branchSize = 4.0f, int seed = 0);
	~SpaceColonization();

	bool step();

	void grow();

	template<class Volume, class Voxel>
	void generateAttractionPoints(Volume& volume, const Voxel& voxel) const {
		for (const AttractionPoint& p : _attractionPoints) {
			volume.setVoxel(p._position, voxel);
		}
	}

	template<class Volume, class Voxel>
	void generateLeaves(Volume& volume, const Voxel& voxel, const glm::ivec3& leafSize) const {
		if (_root == nullptr) {
			return;
		}
		generateLeaves_r(volume, voxel, _root, leafSize);
	}

	template<class Volume, class Voxel>
	void generate(Volume& volume, const Voxel& voxel) const {
		Log::debug("Generate for %i attraction points and %i branches", (int)_attractionPoints.size(), (int)_branches.size());
		for (const auto& e : _branches) {
			Branch* b = e.second;
			if (b->_parent == nullptr) {
				continue;
			}
			voxel::shape::createLine(volume, b->_position, b->_parent->_position, voxel, std::max(1, (int)(b->_size + 0.5f)));
		}
	}
};

}
