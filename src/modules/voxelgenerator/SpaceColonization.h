/**
 * @file
 */

#pragma once

#include "math/Random.h"
#include "ShapeGenerator.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "core/collection/Map.h"
#include "core/collection/DynamicArray.h"
#include <glm/gtc/epsilon.hpp>

namespace voxelgenerator {
namespace tree {

struct Branch;

struct AttractionPoint {
	glm::vec3 _position;
	Branch* _closestBranch = nullptr;

	AttractionPoint(const glm::vec3& position);
};

struct Branch {
	Branch *_parent;
	core::DynamicArray<Branch*> _children;
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

	struct EqualCompare {
		inline bool operator() (const glm::vec3& lhs, const glm::vec3& rhs) const {
			return glm::all(glm::epsilonEqual(lhs, rhs, 0.001f));
		}
	};

	using Branches = core::Map<glm::vec3, Branch*, 64, glm::hash<glm::vec3>, EqualCompare>;
	Branches _branches;
	math::Random _random;

	/**
	 * Generate the attraction points for the crown
	 */
	void fillAttractionPoints();

	template<class Volume, class Voxel, class Size>
	void generateLeaves_r(Volume& volume, const Voxel& voxel, Branch* branch, const Size& size) const {
		if (!branch) {
			return;
		}
		if (branch->_children.empty()) {
			const glm::ivec3& s = size;
			shape::createEllipse(volume, branch->_position, s.x, s.y, s.z, voxel);
			return;
		}
		for (Branch* b : branch->_children) {
			generateLeaves_r(volume, voxel, b, size);
		}
	}

public:
	SpaceColonization(const glm::ivec3& position, int branchLength,
		int width, int height, int depth, float branchSize = 4.0f, unsigned int seed = 0U,
		int minDistance = 6, int maxDistance = 10, int attractionPointCount = 400);
	~SpaceColonization();

	bool step();

	void grow();

	template<class Volume>
	void generateAttractionPoints(Volume& volume, const voxel::Voxel& voxel) const {
		if (_root) {
			const voxel::Voxel& root = voxel::createVoxel(voxel::VoxelType::Flower, 0);
			volume.setVoxel(_root->_position, root);
		}
		for (const AttractionPoint& p : _attractionPoints) {
			volume.setVoxel(p._position, voxel);
		}
	}

	class RandomSize {
	private:
		math::Random& _random;
		const glm::ivec3 _mins;
		const glm::ivec3 _maxs;
	public:
		RandomSize(math::Random& random, int mins, int maxs) :
				_random(random), _mins(mins), _maxs(maxs) {
		}

		RandomSize(math::Random& random, int size) :
				_random(random), _mins(size - size / 2), _maxs(size + size / 2) {
		}

		RandomSize(math::Random& random, const glm::ivec3& mins, const glm::ivec3& maxs) :
				_random(random), _mins(mins), _maxs(maxs) {
		}

		RandomSize(math::Random& random) :
				_random(random), _mins(16), _maxs(80, 25, 80) {
		}

		operator glm::ivec3() const {
			const int x = _random.random(_mins.x, _maxs.x);
			const int y = _random.random(_mins.y, _maxs.y);
			const int z = _random.random(_mins.z, _maxs.z);
			return glm::ivec3(x, y, z);
		}
	};

	template<class Volume, class Size>
	void generateLeaves(Volume& volume, const voxel::Voxel& voxel, const Size& size) const {
		if (_root == nullptr) {
			return;
		}
		generateLeaves_r(volume, voxel, _root, size);
	}

	template<class Volume>
	void generate(Volume& volume, const voxel::Voxel& voxel) const {
		Log::debug("Generate for %i attraction points and %i branches", (int)_attractionPoints.size(), (int)_branches.size());
		for (const auto& e : _branches) {
			Branch* b = e->value;
			if (b->_parent == nullptr) {
				continue;
			}
			const glm::ivec3& start = b->_position;
			const glm::ivec3& end = b->_parent->_position;
			shape::createLine(volume, start, end, voxel, core_max(1, (int)(b->_size + 0.5f)));
		}
	}
};

}
}
