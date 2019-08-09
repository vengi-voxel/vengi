/*
 Copyright (c) 2015 Simon Geilfus

 Algorithm from Fast Poisson Disk Sampling in Arbitrary Dimensions by Robert Bridson
 http://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf
 as explained in this article: http://devmag.org.za/2009/05/03/poisson-disk-sampling/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include "PoissonDiskDistribution.h"
#include "core/Log.h"
#include "math/Random.h"
#include "core/GLM.h"

namespace noise {

namespace {
class Grid {
public:
	using Cell = std::vector<glm::vec2>;

	Grid(const math::Rect<int> &bounds, uint32_t k) {
		resize(bounds, k);
	}

	bool add(const glm::vec2 &position) {
		const uint32_t px = (uint32_t)position.x;
		const uint32_t py = (uint32_t)position.y;
		const int x = (px - _offset.x) >> _k;
		const int y = (py - _offset.y) >> _k;
		const int j = x + _numCells.x * y;

		if (j >= 0 && j < (int)_grid.size()) {
			_grid[j].push_back(position);
			return true;
		}
		return false;
	}

	bool hasNeighbors(const glm::vec2 &p, float radius) {
		if (radius <= 0.0f) {
			return false;
		}
		const float sqRadius = radius * radius;
		const glm::ivec2 radiusVec(radius);
		const glm::ivec2 ip(p);
		const glm::ivec2 bmaxs(_bounds.maxs() - 1);
		const glm::ivec2 min = glm::clamp(ip - radiusVec, _bounds.mins(), bmaxs);
		const glm::ivec2 max = glm::clamp(ip + radiusVec, _bounds.mins(), bmaxs);

		const glm::ivec2 minCell((min.x - _offset.x) >> _k, (min.y - _offset.y) >> _k);
		const glm::ivec2 maxCell = (glm::min)(1 + glm::ivec2((max.x - _offset.x) >> _k, (max.y - _offset.y) >> _k), _numCells);
		for (int y = minCell.y; y < maxCell.y; ++y) {
			for (int x = minCell.x; x < maxCell.x; ++x) {
				for (const glm::vec2& cell : _grid[x + _numCells.x * y]) {
					if (glm::length2(p - cell) < sqRadius) {
						return true;
					}
				}
			}
		}
		return false;
	}

	void resize(const math::Rect<int> &bounds, uint32_t k) {
		_bounds = bounds;
		_size = _bounds.size();
		resize(k);
	}

	void resize(uint32_t k) {
		_k = k;
		_cellSize = 1 << k;
		_offset = _bounds.mins();
		_numCells = glm::ceil(glm::vec2(_size) / (float) _cellSize);
		_grid.clear();
		_grid.resize(_numCells.x * _numCells.y);
	}

	inline const math::Rect<int>& bounds() const {
		return _bounds;
	}
protected:
	std::vector<Cell> _grid;
	glm::ivec2 _numCells;
	glm::ivec2 _offset;
	math::Rect<int> _bounds;
	glm::ivec2 _size;
	uint32_t _k;
	uint32_t _cellSize;
};

class Grid3D {
public:
	using Cell = std::vector<glm::vec3>;

	Grid3D(const math::AABB<int> &aabb, uint32_t k) : _aabb(aabb) {
		resize(aabb, k);
	}

	bool add(const glm::vec3 &position) {
		const uint32_t px = (uint32_t)position.x;
		const uint32_t py = (uint32_t)position.y;
		const uint32_t pz = (uint32_t)position.z;
		const int x = (px - _offset.x) >> _k;
		const int y = (py - _offset.y) >> _k;
		const int z = (pz - _offset.z) >> _k;
		const int j = x + _numCells.x * z + _numCells.x * _numCells.z * y;

		if (j >= 0 && j < (int)_grid.size()) {
			_grid[j].push_back(position);
			return true;
		}
		return false;
	}

	bool hasNeighbors(const glm::vec3 &p, float radius) {
		if (radius <= 0.0f) {
			return false;
		}
		const float sqRadius = radius * radius;
		const glm::ivec3 radiusVec(radius);
		const glm::ivec3 ip(p);
		const glm::ivec3 bmaxs(_aabb.maxs() - 1);
		const glm::ivec3 min = glm::clamp(ip - radiusVec, _aabb.mins(), bmaxs);
		const glm::ivec3 max = glm::clamp(ip + radiusVec, _aabb.mins(), bmaxs);

		const glm::ivec3 minCell((min.x - _offset.x) >> _k, (min.y - _offset.y) >> _k, (min.z - _offset.z) >> _k);
		const glm::ivec3 maxCell = (glm::min)(1 + glm::ivec3((max.x - _offset.x) >> _k, (max.y - _offset.y) >> _k, (max.z - _offset.z) >> _k), _numCells);
		for (int z = minCell.z; z < maxCell.z; ++z) {
			for (int y = minCell.y; y < maxCell.y; ++y) {
				for (int x = minCell.x; x < maxCell.x; ++x) {
					for (const glm::vec3& cell : _grid[x + _numCells.x * z + _numCells.x * _numCells.z * y]) {
						if (glm::length2(p - cell) < sqRadius) {
							return true;
						}
					}
				}
			}
		}
		return false;
	}

	void resize(const math::AABB<int> &aabb, uint32_t k) {
		_aabb = aabb;
		_size = _aabb.getWidth();
		resize(k);
	}

	void resize(uint32_t k) {
		_k = k;
		_cellSize = 1 << k;
		_offset = _aabb.mins();
		_numCells = glm::ceil(glm::vec3(_size) / (float) _cellSize);
		_grid.clear();
		_grid.resize(_numCells.x * _numCells.y * _numCells.z);
	}

	inline const math::AABB<int>& aabb() const {
		return _aabb;
	}
protected:
	std::vector<Cell> _grid;
	glm::ivec3 _numCells;
	glm::ivec3 _offset;
	math::AABB<int> _aabb;
	glm::ivec3 _size;
	uint32_t _k;
	uint32_t _cellSize;
};

}

static void prepare(Grid3D& grid, std::vector<glm::vec3>& processingList, std::vector<glm::vec3>& outputList, const std::vector<glm::vec3> &initialSet) {
	processingList.reserve(initialSet.size());
	outputList.reserve(initialSet.size());
	// add the initial points
	for (const glm::vec3& p : initialSet) {
		processingList.push_back(p);
		if (grid.add(p)) {
			outputList.push_back(p);
		}
	}

	// if there's no initial points add the center point
	if (processingList.empty()) {
		const glm::vec3 center(grid.aabb().getCenter());
		processingList.push_back(center);
		if (grid.add(center)) {
			outputList.push_back(center);
		}
	}
}

static void prepare(Grid& grid, std::vector<glm::vec2>& processingList, std::vector<glm::vec2>& outputList, const std::vector<glm::vec2> &initialSet) {
	processingList.reserve(initialSet.size());
	outputList.reserve(initialSet.size());
	// add the initial points
	for (const glm::vec2& p : initialSet) {
		processingList.push_back(p);
		if (grid.add(p)) {
			outputList.push_back(p);
		}
	}

	// if there's no initial points add the center point
	if (processingList.empty()) {
		const glm::vec2& center = grid.bounds().centerf();
		processingList.push_back(center);
		if (grid.add(center)) {
			outputList.push_back(center);
		}
	}
}

std::vector<glm::vec3> poissonDiskDistribution(float separation, const math::AABB<int> &aabb, const std::vector<glm::vec3> &initialSet, int k) {
	// prepare working structures
	std::vector<glm::vec3> processingList;
	std::vector<glm::vec3> outputList;
	math::Random rnd(aabb.getCenterX() + aabb.getCenterY() + aabb.getCenterZ());

	// create grid
	Grid3D grid(aabb, 3);

	prepare(grid, processingList, outputList, initialSet);

	// while there's points in the processing list
	while (!processingList.empty()) {
		// pick a random point in the processing list
		const int randPoint = rnd.random(0, processingList.size() - 1);
		const glm::vec3 center = processingList[randPoint];

		// remove it
		processingList.erase(processingList.begin() + randPoint);

		// spawn k points in an anulus around that point
		// the higher k is, the higher the packing will be and slower the algorithm
		for (int i = 0; i < k; ++i) {
			const float randRadius = separation * (1.0f + rnd.randomf());
			const float randAngle1 = rnd.randomf() * glm::two_pi<float>();
			const float randAngle2 = rnd.randomf() * glm::two_pi<float>();
			const float x = center.x + cos(randAngle1) * sin(randAngle2) * randRadius;
			const float y = center.y + sin(randAngle1) * sin(randAngle2) * randRadius;
			const float z = center.z + cos(randAngle2) * randRadius;
			const glm::vec3 newPoint(x, y, z);

			// check if the new random point is in the window bounds
			// and if it has no neighbors that are too close to them
			if (aabb.containsPoint(newPoint) && !grid.hasNeighbors(newPoint, separation)) {
				// if the point has no close neighbors add it to the processing list, output list and grid
				processingList.push_back(newPoint);
				if (grid.add(newPoint)) {
					outputList.push_back(newPoint);
				}
			}
		}
	}

	return outputList;
}

std::vector<glm::vec2> poissonDiskDistribution(float separation, const math::Rect<int> &bounds, const std::vector<glm::vec2> &initialSet, int k) {
	// prepare working structures
	std::vector<glm::vec2> processingList;
	std::vector<glm::vec2> outputList;
	math::Random rnd(bounds.getMinX());

	// create grid
	Grid grid(bounds, 3);

	prepare(grid, processingList, outputList, initialSet);

	// while there's points in the processing list
	while (!processingList.empty()) {
		// pick a random point in the processing list
		const int randPoint = rnd.random(0, processingList.size() - 1);
		const glm::vec2 center = processingList[randPoint];

		// remove it
		processingList.erase(processingList.begin() + randPoint);

		// spawn k points in an anulus around that point
		// the higher k is, the higher the packing will be and slower the algorithm
		for (int i = 0; i < k; ++i) {
			const float randRadius = separation * (1.0f + rnd.randomf());
			const float randAngle = rnd.randomf() * glm::two_pi<float>();
			const glm::vec2 newPoint = center + glm::vec2(cos(randAngle), sin(randAngle)) * randRadius;

			// check if the new random point is in the window bounds
			// and if it has no neighbors that are too close to them
			if (bounds.containsf(newPoint) && !grid.hasNeighbors(newPoint, separation)) {
				// if the point has no close neighbors add it to the processing list, output list and grid
				processingList.push_back(newPoint);
				if (grid.add(newPoint)) {
					outputList.push_back(newPoint);
				}
			}
		}
	}

	return outputList;
}

std::vector<glm::vec2> poissonDiskDistribution(const std::function<float(const glm::vec2&)> &distFunction, const math::Rect<int> &bounds,
		const std::vector<glm::vec2> &initialSet, int k) {
	// prepare working structures
	std::vector<glm::vec2> processingList;
	std::vector<glm::vec2> outputList;
	math::Random rnd(bounds.getMinX());

	// create grid
	Grid grid(bounds, 3);

	prepare(grid, processingList, outputList, initialSet);

	// while there's points in the processing list
	while (!processingList.empty()) {
		// pick a random point in the processing list
		const int randPoint = rnd.random(0, processingList.size() - 1);
		const glm::vec2 center = processingList[randPoint];

		// remove it
		processingList.erase(processingList.begin() + randPoint);

		// get the current min distance
		const float dist = distFunction(center);

		// spawn k points in an anulus around that point
		// the higher k is, the higher the packing will be and slower the algorithm
		for (int i = 0; i < k; ++i) {
			const float randRadius = dist * (1.0f + rnd.randomf());
			const float randAngle = rnd.randomf() * glm::two_pi<float>();
			const glm::vec2 newPoint = center + glm::vec2(cos(randAngle), sin(randAngle)) * randRadius;

			// check if the new random point is in the window bounds
			// and if it has no neighbors that are too close to them
			if (bounds.contains(newPoint) && !grid.hasNeighbors(newPoint, dist)) {
				// if the point has no close neighbors add it to the processing list, output list and grid
				processingList.push_back(newPoint);
				if (dist > 0.0f && grid.add(newPoint)) {
					outputList.push_back(newPoint);
				}
			}
		}
	}

	return outputList;
}

std::vector<glm::vec2> poissonDiskDistribution(const std::function<float(const glm::vec2&)> &distFunction,
		const std::function<bool(const glm::vec2&)> &boundsFunction, const math::Rect<int> &bounds, const std::vector<glm::vec2> &initialSet, int k) {
	// prepare working structures
	std::vector<glm::vec2> processingList;
	std::vector<glm::vec2> outputList;
	math::Random rnd(bounds.getMinX());

	// create grid
	Grid grid(bounds, 3);

	prepare(grid, processingList, outputList, initialSet);

	// while there's points in the processing list
	while (!processingList.empty()) {
		// pick a random point in the processing list
		const int randPoint = rnd.random(0, processingList.size() - 1);
		const glm::vec2 center = processingList[randPoint];

		// remove it
		processingList.erase(processingList.begin() + randPoint);

		// get the current min distance
		const float dist = distFunction(center);

		// spawn k points in an anulus around that point
		// the higher k is, the higher the packing will be and slower the algorithm
		for (int i = 0; i < k; i++) {
			const float randRadius = dist * (1.0f + rnd.randomf());
			const float randAngle = rnd.randomf() * glm::two_pi<float>();
			const glm::vec2 newPoint = center + glm::vec2(cos(randAngle), sin(randAngle)) * randRadius;

			// check if the new random point is in the window bounds
			// and if it has no neighbors that are too close to them
			if (bounds.contains(newPoint) && boundsFunction(newPoint) && !grid.hasNeighbors(newPoint, dist)) {
				// if the point has no close neighbors add it to the processing list, output list and grid
				processingList.push_back(newPoint);
				if (dist > 0.0f && grid.add(newPoint)) {
					outputList.push_back(newPoint);
				}
			}
		}
	}

	return outputList;
}

}
