/**
 * @file
 */

#pragma once

#include "core/Random.h"
#include "voxel/Spiral.h"
#include "voxel/BiomeManager.h"
#include "voxel/MaterialColor.h"
#include "voxel/TreeContext.h"
#include "ShapeGenerator.h"
#include "CactusGenerator.h"
#include "LSystemGenerator.h"
#include "core/AABB.h"
#include "core/Log.h"

#include <unordered_map>
#include <functional>

namespace voxel {
namespace tree {

class Branch;

struct Leaf {
	glm::vec3 _position;
	Branch* _closestBranch = nullptr;

	Leaf(const glm::vec3& position) :
			_position(position) {
	}

	template<class Volume, class Voxel>
	void generate(Volume& volume, const Voxel& voxel) const {
		const int size = 4;
		voxel::shape::createEllipse(volume, _position, size, size, size, voxel);
	}
};

class Branch {
public:
	Branch *_parent;
	glm::vec3 _position;
	glm::vec3 _growDirection;
	glm::vec3 _originalGrowDirection;
	int _growCount = 0;
	float _size;

	Branch(Branch* parent, const glm::vec3& position, const glm::vec3& growDirection, float size) :
			_parent(parent), _position(position), _growDirection(growDirection), _originalGrowDirection(growDirection), _size(size) {
	}

	void reset() {
		_growCount = 0;
		_growDirection = _originalGrowDirection;
	}

	template<class Volume, class Voxel>
	void generate(Volume& volume, Voxel& trunkVoxel) const {
		if (_parent == nullptr) {
			return;
		}
		voxel::shape::createLine(volume, _position, _parent->_position, trunkVoxel, std::max(1, (int)(_size + 0.5f)));
	}
};

/**
 * http://www.jgallant.com/procedurally-generating-trees-with-space-colonization-algorithm-in-xna/
 */
class Tree {
private:
	bool _doneGrowing = false;
	glm::vec3 _position;

	int _leafCount = 400;
	int _treeWidth = 80;
	int _treeDepth = 80;
	int _treeHeight = 150;
	int _trunkHeight = 40;
	int _minDistance = 6;
	int _maxDistance = 10;
	int _branchLength = 2;
	float _branchSize = 4.0f;
	float _trunkSizeFactor = 0.8f;
	float _branchSizeFactor = 0.6f;

	Branch *_root;
	std::vector<Leaf> _leaves;
	std::unordered_map<glm::vec3, Branch*, Vec3Hash, Vec3Hash> _branches;
	core::Random _random;

	core::AABB<int> _crown;

	void generateCrown(int radius) {
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

	void generateTrunk() {
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

public:
	Tree(const glm::ivec3& position, int trunkHeight, int branchLength,
		int treeWidth = 80, int treeDepth = 80, int treeHeight = 150, float branchSize = 4.0f, int seed = 0) :
			_position(position), _treeWidth(treeWidth), _treeDepth(treeDepth), _treeHeight(treeHeight),
			_trunkHeight(trunkHeight), _branchLength(branchLength), _branchSize(branchSize), _random(seed),
			_crown(_position.x - _treeWidth / 2,
					_position.y - _treeHeight - _trunkHeight,
					_position.z - _treeDepth / 2, _treeWidth, _treeHeight,
					_treeDepth) {
		generateCrown(_treeWidth / 2);
		generateTrunk();
	}

	Tree(const core::AABB<int>& crownAABB, int trunkHeight, int branchLength, int seed = 0) :
			_position(crownAABB.getLowerCenter()), _trunkHeight(trunkHeight), _branchLength(branchLength), _random(seed), _crown(crownAABB) {
		_treeWidth = crownAABB.getWidthX();
		_treeHeight = crownAABB.getWidthY();
		_treeDepth = crownAABB.getWidthZ();
		_leafCount = _treeDepth * 10;
		generateCrown(_treeWidth / 2);
		generateTrunk();
	}

	~Tree() {
		delete _root;
		for (auto &e : _branches) {
			delete e.second;
		}
		_branches.clear();
	}

	bool grow() {
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

	template<class Volume>
	void generate(Volume& volume) const {
		Log::debug("Generate for %i leaves and %i branches", (int)_leaves.size(), (int)_branches.size());
		const voxel::RandomVoxel leavesVoxel(voxel::VoxelType::Leaf, _random);
		for (const Leaf& l : _leaves) {
			l.generate(volume, leavesVoxel);
		}

		const voxel::RandomVoxel woodRandomVoxel(voxel::VoxelType::Wood, _random);
		for (const auto& e : _branches) {
			e.second->generate(volume, woodRandomVoxel);
		}
	}
};

/**
 * @brief Looks for a suitable height level for placing a tree
 * @return @c -1 if no suitable floor for placing a tree was found
 */
template<class Volume>
static int findFloor(const Volume& volume, int x, int z) {
	glm::ivec3 start(x, MAX_TERRAIN_HEIGHT - 1, z);
	glm::ivec3 end(x, 0, z);
	int y = -1;
	voxel::raycastWithEndpoints(&volume, start, end, [&y] (const typename Volume::Sampler& sampler) {
		const Voxel& voxel = sampler.getVoxel();
		const VoxelType material = voxel.getMaterial();
		if (isLeaves(material)) {
			return false;
		}
		if (!isRock(material) && (isFloor(material) || isWood(material))) {
			y = sampler.getPosition().y + 1;
			return false;
		}
		return true;
	});
	return y;
}

/**
 * @brief Creates an ellipsis tree with side branches and smaller ellipsis on top of those branches
 * @sa createTreeEllipsis()
 */
template<class Volume>
void createTreeBranchEllipsis(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const int top = ctx.treeTop();
	const RandomVoxel trunkVoxel(VoxelType::Wood, random);
	shape::createCubeNoCenter(volume, ctx.pos - glm::ivec3(1), ctx.trunkWidth + 2, 1, ctx.trunkWidth + 2, trunkVoxel);
	shape::createCubeNoCenter(volume, ctx.pos, ctx.trunkWidth, ctx.trunkHeight, ctx.trunkWidth, trunkVoxel);
	if (ctx.trunkHeight <= 8) {
		return;
	}
	const RandomVoxel leavesVoxel(VoxelType::Leaf, random);
	std::vector<int> branches = {1, 2, 3, 4};
	random.shuffle(branches.begin(), branches.end());
	const int n = random.random(1, 4);
	for (int i = 0; i < n; ++i) {
		const int thickness = std::max(2, ctx.trunkWidth / 2);
		const int branchHeight = ctx.trunkHeight / 2;
		const int branchSize = random.random(thickness * 2, std::max(thickness * 2, ctx.trunkWidth));

		glm::ivec3 branch = ctx.pos;
		branch.y = random.random(ctx.pos.y + 2, top - 2);

		const int delta = (ctx.trunkWidth - thickness) / 2;
		glm::ivec3 leavesPos;
		switch (branches[i]) {
		case 1:
			branch.x += delta;
			leavesPos = shape::createL(volume, branch, 0, branchSize, branchHeight, thickness, trunkVoxel);
			break;
		case 2:
			branch.x += delta;
			leavesPos = shape::createL(volume, branch, 0, -branchSize, branchHeight, thickness, trunkVoxel);
			break;
		case 3:
			branch.z += delta;
			leavesPos = shape::createL(volume, branch, branchSize, 0, branchHeight, thickness, trunkVoxel);
			break;
		case 4:
			branch.z += delta;
			leavesPos = shape::createL(volume, branch, -branchSize, 0, branchHeight, thickness, trunkVoxel);
			break;
		}
		leavesPos.y += branchHeight / 2;
		shape::createEllipse(volume, leavesPos, branchHeight, branchHeight, branchHeight, leavesVoxel);
	}
	const glm::ivec3 leafesPos(ctx.pos.x + ctx.trunkWidth / 2, top + ctx.leavesHeight / 2, ctx.pos.z + ctx.trunkWidth / 2);
	shape::createEllipse(volume, leafesPos, ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
}

/**
 * @brief Creates the trunk for a tree - the full height of the tree is taken
 */
template<class Volume>
static void createTrunk(Volume& volume, const TreeContext& ctx, const Voxel& voxel) {
	const int top = ctx.treeTop();
	int trunkWidthBottomOffset = 2;
	for (int y = ctx.treeBottom(); y < top; ++y) {
		const int trunkWidth = std::max(0, trunkWidthBottomOffset);
		--trunkWidthBottomOffset;
		const int startX = ctx.pos.x - ctx.trunkWidth / 2 - trunkWidth;
		const int endX = startX + ctx.trunkWidth + trunkWidth * 2;
		for (int x = startX; x < endX; ++x) {
			const int startZ = ctx.pos.z - ctx.trunkWidth / 2 - trunkWidth;
			const int endZ = startZ + ctx.trunkWidth + trunkWidth * 2;
			for (int z = startZ; z < endZ; ++z) {
				glm::ivec3 finalPos(x, y, z);
				if (y == ctx.treeBottom()) {
					finalPos.y = findFloor(volume, x, z);
					if (finalPos.y < 0) {
						continue;
					}
					for (int i = finalPos.y + 1; i <= y; ++i) {
						volume.setVoxel(finalPos.x, i, finalPos.z, voxel);
					}
				}

				volume.setVoxel(finalPos, voxel);
			}
		}
	}
}

/**
 * @return The end of the trunk to start the leaves from
 */
template<class Volume>
static glm::ivec3 createBezierTrunk(Volume& volume, const TreeContext& ctx, const Voxel& voxel) {
	voxel::Spiral o;
	const int amount = 4;
	const glm::ivec3& trunkTop = ctx.trunkTopV();
	const int shiftX = ctx.trunkWidth;
	const int shiftZ = ctx.trunkWidth;
	glm::ivec3 end = trunkTop;
	for (int i = 0; i < amount; ++i) {
		const glm::ivec3 start(ctx.pos.x + o.x(), ctx.pos.y, ctx.pos.z + o.z());
		end.x = trunkTop.x + shiftX + o.x();
		end.z = trunkTop.z + shiftZ + o.z();
		const glm::ivec3 control(start.x, start.y + 10, start.z);
		shape::createBezierFunc(volume, start, end, control, voxel,
			[] (Volume& volume, const glm::ivec3& last, const glm::ivec3& pos, const Voxel& voxel) {
				shape::createLine(volume, pos, last, voxel);
			},
		ctx.trunkHeight);
		o.next();
	}
	end.y -= 1;
	return end;
}

template<class Volume>
void createTreePalm(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const RandomVoxel trunkVoxel(VoxelType::Wood, random);
	const glm::ivec3& start = createBezierTrunk(volume, ctx, trunkVoxel);
	const RandomVoxel leavesVoxel(VoxelType::Leaf, random);
	const int branches = 6;
	const float stepWidth = glm::radians(360.0f / (float)branches);
	float angle = random.randomf(0.0f, glm::two_pi<float>());
	const float w = ctx.leavesWidth;
	for (int b = 0; b < branches; ++b) {
		const float x = glm::cos(angle);
		const float z = glm::sin(angle);
		const int randomLength = random.random(ctx.leavesHeight - 3, ctx.leavesHeight);
		const glm::ivec3 control(start.x - x * (w / 2.0f), start.y + 10, start.z - z * (w / 2.0f));
		const glm::ivec3 end(start.x - x * w, start.y - randomLength, start.z - z * w);
		shape::createBezierFunc(volume, start, end, control, leavesVoxel,
			[=] (Volume& volume, const glm::ivec3& last, const glm::ivec3& pos, const Voxel& voxel) {
				shape::createLine(volume, pos, last, voxel);
				const float x1 = glm::cos(angle + glm::radians(15.0f));
				const float z1 = glm::sin(angle + glm::radians(15.0f));
				const float x2 = glm::cos(angle - glm::radians(15.0f));
				const float z2 = glm::sin(angle - glm::radians(15.0f));
				const float length = 4.0f;
				const glm::ivec3 end1(pos.x + x1 * length, pos.y - length, pos.z + z1 * length);
				shape::createBezier(volume, pos, end1, control, voxel, length);
				const glm::ivec3 end2(pos.x + x2 * length, pos.y - length, pos.z + z2 * length);
				shape::createBezier(volume, pos, end2, control, voxel, length);
				volume.setVoxel(pos, voxel::createVoxel(VoxelType::Roof, 0));
			},
		ctx.leavesHeight / 4);
		angle += stepWidth;
	}
}

template<class Volume>
void createTreeEllipsis(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const RandomVoxel trunkVoxel(VoxelType::Wood, random);
	createTrunk(volume, ctx, trunkVoxel);
	const RandomVoxel leavesVoxel(VoxelType::Leaf, random);
	shape::createEllipse(volume, ctx.leavesCenterV(), ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
}

template<class Volume>
void createTreeCone(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const RandomVoxel trunkVoxel(VoxelType::Wood, random);
	createTrunk(volume, ctx, trunkVoxel);
	const RandomVoxel leavesVoxel(VoxelType::LeafFir, random);
	shape::createCone(volume, ctx.leavesCenterV(), ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
}

/**
 * @brief Creates a fir with several branches based on lines falling down from the top of the tree
 */
template<class Volume>
void createTreeFir(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const RandomVoxel leavesVoxel(VoxelType::LeafFir, random);
	const RandomVoxel trunkVoxel(VoxelType::Wood, random);
	createTrunk(volume, ctx, trunkVoxel);

	const int branches = 12;
	const float stepWidth = glm::radians(360.0f / branches);
	float angle = random.randomf(0.0f, glm::two_pi<float>());
	float w = 1.3f;
	const int amount = 3;
	const int stepHeight = 10;
	glm::ivec3 leafesPos = ctx.leavesTopV();

	const int halfHeight = ((amount - 1) * stepHeight) / 2;
	glm::ivec3 center(ctx.pos.x, ctx.treeTop() - halfHeight, ctx.pos.z);
	shape::createCube(volume, center, ctx.trunkWidth, halfHeight * 2, ctx.trunkWidth, leavesVoxel);

	for (int n = 0; n < amount; ++n) {
		for (int b = 0; b < branches; ++b) {
			glm::ivec3 start = leafesPos;
			glm::ivec3 end = start;
			const float x = glm::cos(angle);
			const float z = glm::sin(angle);
			const int randomZ = random.random(4, 8);
			end.y -= randomZ;
			end.x -= x * w;
			end.z -= z * w;
			shape::createLine(volume, start, end, leavesVoxel);
			glm::ivec3 end2 = end;
			end2.y -= 4;
			end2.x -= x * w * 1.8;
			end2.z -= z * w * 1.8;
			shape::createLine(volume, end, end2, leavesVoxel);
			angle += stepWidth;
			w += 1.0 / (double)(b + 1);
		}
		leafesPos.y -= stepHeight;
	}
}

template<class Volume>
void createTreePine(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const RandomVoxel trunkVoxel(VoxelType::Wood, random);
	createTrunk(volume, ctx, trunkVoxel);
	const int singleLeaveHeight = 2;
	const int singleStepDelta = 1;
	const int singleStepHeight = singleLeaveHeight + singleStepDelta;
	const int steps = std::max(1, ctx.leavesHeight / singleStepHeight);
	const int stepWidth = ctx.leavesWidth / steps;
	const int stepDepth = ctx.leavesDepth / steps;
	int currentWidth = 2;
	int currentDepth = 2;
	const int top = ctx.treeTop();
	glm::ivec3 leavesPos(ctx.pos.x, top, ctx.pos.z);
	const RandomVoxel leavesVoxel(VoxelType::LeafPine, random);
	for (int i = 0; i < steps; ++i) {
		shape::createDome(volume, leavesPos, currentWidth, singleLeaveHeight, currentDepth, leavesVoxel);
		leavesPos.y -= singleStepDelta;
		shape::createDome(volume, leavesPos, currentWidth + 1, singleLeaveHeight, currentDepth + 1, leavesVoxel);
		currentDepth += stepDepth;
		currentWidth += stepWidth;
		leavesPos.y -= singleLeaveHeight;
	}
}

/**
 * @sa createTreeDomeHangingLeaves()
 */
template<class Volume>
void createTreeDome(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const RandomVoxel trunkVoxel(VoxelType::Wood, random);
	createTrunk(volume, ctx, trunkVoxel);
	const RandomVoxel leavesVoxel(VoxelType::Leaf, random);
	shape::createDome(volume, ctx.leavesCenterV(), ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
}

/**
 * @brief Creates a dome based tree with leaves hanging down from the dome.
 * @sa createTreeDome()
 */
template<class Volume>
void createTreeDomeHangingLeaves(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const RandomVoxel trunkVoxel(VoxelType::Wood, random);
	createTrunk(volume, ctx, trunkVoxel);
	const RandomVoxel leavesVoxel(VoxelType::Leaf, random);
	shape::createDome(volume, ctx.leavesCenterV(), ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
	const int branches = 6;
	const float stepWidth = glm::radians(360.0f / (float)branches);
	float angle = random.randomf(0.0f, glm::two_pi<float>());
	// leaves falling down
	const int y = ctx.leavesBottom() + 1;
	for (int b = 0; b < branches; ++b) {
		const float x = glm::cos(angle);
		const float z = glm::sin(angle);
		const int randomLength = random.random(4, 8);

		const glm::ivec3 start(ctx.pos.x - x * (ctx.leavesWidth - 1) / 2, y, ctx.pos.z - z * (ctx.leavesDepth - 1) / 2);
		const glm::ivec3 end(start.x, start.y - randomLength, start.z);
		shape::createLine(volume, start, end, leavesVoxel);

		angle += stepWidth;
	}
}

/**
 * @sa createTreeCubeSideCubes()
 */
template<class Volume>
void createTreeCube(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const RandomVoxel leavesVoxel(VoxelType::Leaf, random);
	const RandomVoxel trunkVoxel(VoxelType::Wood, random);
	createTrunk(volume, ctx, trunkVoxel);

	const glm::ivec3& leafesPos = ctx.leavesCenterV();
	shape::createCube(volume, leafesPos, ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
	// TODO: use CreatePlane
	shape::createCube(volume, leafesPos, ctx.leavesWidth + 2, ctx.leavesHeight - 2, ctx.leavesDepth - 2, leavesVoxel);
	shape::createCube(volume, leafesPos, ctx.leavesWidth - 2, ctx.leavesHeight + 2, ctx.leavesDepth - 2, leavesVoxel);
	shape::createCube(volume, leafesPos, ctx.leavesWidth - 2, ctx.leavesHeight - 2, ctx.leavesDepth + 2, leavesVoxel);
}

/**
 * @brief Creates a cube based tree with small cubes on the four side faces
 * @sa createTreeCube()
 */
template<class Volume>
void createTreeCubeSideCubes(Volume& volume, const TreeContext& ctx, core::Random& random) {
	const RandomVoxel leavesVoxel(VoxelType::Leaf, random);
	const RandomVoxel trunkVoxel(VoxelType::Wood, random);
	createTrunk(volume, ctx, trunkVoxel);

	const glm::ivec3& leafesPos = ctx.leavesCenterV();
	shape::createCube(volume, leafesPos, ctx.leavesWidth, ctx.leavesHeight, ctx.leavesDepth, leavesVoxel);
	// TODO: use CreatePlane
	shape::createCube(volume, leafesPos, ctx.leavesWidth + 2, ctx.leavesHeight - 2, ctx.leavesDepth - 2, leavesVoxel);
	shape::createCube(volume, leafesPos, ctx.leavesWidth - 2, ctx.leavesHeight + 2, ctx.leavesDepth - 2, leavesVoxel);
	shape::createCube(volume, leafesPos, ctx.leavesWidth - 2, ctx.leavesHeight - 2, ctx.leavesDepth + 2, leavesVoxel);

	Spiral o;
	o.next();
	const int halfWidth = ctx.leavesWidth / 2;
	const int halfHeight = ctx.leavesHeight / 2;
	const int halfDepth = ctx.leavesDepth / 2;
	for (int i = 0; i < 4; ++i) {
		glm::ivec3 leafesPos2 = leafesPos;
		leafesPos2.x += o.x() * halfWidth;
		leafesPos2.z += o.z() * halfDepth;
		shape::createEllipse(volume, leafesPos2, halfWidth, halfHeight, halfDepth, leavesVoxel);
		o.next(2);
	}
}

/**
 * @brief Delegates to the corresponding create method for the given TreeType in the TreeContext
 */
template<class Volume>
void createTree(Volume& volume, const TreeContext& ctx, core::Random& random) {
	if (ctx.type == TreeType::BranchesEllipsis) {
		createTreeBranchEllipsis(volume, ctx, random);
	} else if (ctx.type == TreeType::Ellipsis) {
		createTreeEllipsis(volume, ctx, random);
	} else if (ctx.type == TreeType::Palm) {
		createTreePalm(volume, ctx, random);
	} else if (ctx.type == TreeType::Cone) {
		createTreeCone(volume, ctx, random);
	} else if (ctx.type == TreeType::Fir) {
		createTreeFir(volume, ctx, random);
	} else if (ctx.type == TreeType::Pine) {
		createTreePine(volume, ctx, random);
	} else if (ctx.type == TreeType::Dome) {
		createTreeDome(volume, ctx, random);
	} else if (ctx.type == TreeType::DomeHangingLeaves) {
		createTreeDomeHangingLeaves(volume, ctx, random);
	} else if (ctx.type == TreeType::Cube) {
		createTreeCube(volume, ctx, random);
	} else if (ctx.type == TreeType::CubeSideCubes) {
		createTreeCubeSideCubes(volume, ctx, random);
	} else if (ctx.type == TreeType::SpaceColonization) {
		Tree tree(ctx.pos, ctx.trunkHeight / 2, 6, ctx.leavesWidth, ctx.leavesDepth, ctx.leavesHeight, ctx.pos.x);
		while (tree.grow()) {
		}
		tree.generate(volume);
	}
}

/**
 * @brief Fill a world with trees based on the configured bioms
 */
template<class Volume>
void createTrees(Volume& volume, const Region& region, const BiomeManager& biomManager, core::Random& random) {
	const int maxSize = 18;
	std::vector<glm::vec2> positions;
	biomManager.getTreePositions(region, positions, random, maxSize);
	TreeContext ctx;
	for (const glm::vec2& position : positions) {
		const int y = findFloor(volume, position.x, position.y);
		if (y == -1) {
			continue;
		}
		ctx.pos = glm::ivec3(position.x, y, position.y);
		ctx.trunkWidth = 3;
		int size = random.random(12, maxSize);
		ctx.leavesWidth = size;
		ctx.leavesDepth = size;
		ctx.type = (TreeType)random.random(0, int(TreeType::Max) - 1);
		switch (ctx.type) {
		case TreeType::Fir:
			ctx.leavesHeight = random.random(20, 28);
			ctx.trunkHeight = ctx.leavesHeight * 2;
			break;
		case TreeType::Pine:
		case TreeType::Cone:
		case TreeType::Dome:
		case TreeType::DomeHangingLeaves:
			ctx.leavesHeight = random.random(20, 28);
			ctx.trunkHeight = ctx.leavesHeight + random.random(5, 9);
			break;
		case TreeType::BranchesEllipsis:
			ctx.leavesHeight = random.random(10, 14);
			ctx.trunkHeight = ctx.leavesHeight + random.random(6, 10);
			ctx.trunkWidth = 3;
			break;
		default:
			ctx.leavesHeight = random.random(10, 14);
			ctx.trunkHeight = ctx.leavesHeight + random.random(5, 9);
			break;
		}
		createTree(volume, ctx, random);
	}
}

}
}
