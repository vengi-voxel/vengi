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
#include "core/Random.h"
#include "core/GLM.h"

namespace noise {

namespace {
class Grid {
public:
	using Cell = std::vector<glm::vec2>;

	Grid(const core::RectFloat &bounds, uint32_t k);

	void add(const glm::vec2 &position);
	bool hasNeighbors(const glm::vec2 &p, float radius);

	void resize(const core::RectFloat &bounds, uint32_t k);
	void resize(uint32_t k);

protected:
	std::vector<Cell> _grid;
	glm::ivec2 _numCells;
	glm::ivec2 _offset;
	core::RectFloat _bounds;
	uint32_t _k;
	uint32_t _cellSize;
};

Grid::Grid(const core::RectFloat &bounds, uint32_t k) {
	resize(bounds, k);
}

void Grid::add(const glm::vec2 &position) {
	const int x = ((uint32_t) position.x + _offset.x) >> _k;
	const int y = ((uint32_t) position.y + _offset.y) >> _k;
	const int j = x + _numCells.x * y;

	if (j >= 0 && j < (int)_grid.size()) {
		_grid[j].push_back(position);
	} else {
		Log::error("Out of bounds: j: %i, size: %i, x: %i, y: %i", j, (int)_grid.size(), x, y);
	}
}

bool Grid::hasNeighbors(const glm::vec2 &p, float radius) {
	const float sqRadius = radius * radius;
	const glm::ivec2 radiusVec = glm::ivec2(radius);
	const glm::ivec2 min = glm::max(glm::min(glm::ivec2(p) - radiusVec, glm::ivec2(_bounds.maxs()) - glm::ivec2(1)), glm::ivec2(_bounds.mins()));
	const glm::ivec2 max = glm::max(glm::min(glm::ivec2(p) + radiusVec, glm::ivec2(_bounds.maxs()) - glm::ivec2(1)), glm::ivec2(_bounds.mins()));

	const glm::ivec2 minCell((min.x + _offset.x) >> _k, (min.y + _offset.y) >> _k);
	const glm::ivec2 maxCell = glm::min(1 + glm::ivec2((max.x + _offset.x) >> _k, (max.y + _offset.y) >> _k), _numCells);
	for (int y = minCell.y; y < maxCell.y; ++y) {
		for (int x = minCell.x; x < maxCell.x; ++x) {
			for (auto cell : _grid[x + _numCells.x * y]) {
				if (glm::length2(p - cell) < sqRadius) {
					return true;
				}
			}
		}
	}
	return false;
}

void Grid::resize(const core::RectFloat &bounds, uint32_t k) {
	_bounds = bounds;
	resize(k);
}

void Grid::resize(uint32_t k) {
	_k = k;
	_cellSize = 1 << k;
	_offset = glm::abs(_bounds.mins());
	_numCells = glm::ceil(glm::vec2(_bounds.size()) / (float) _cellSize);
	_grid.clear();
	_grid.resize(_numCells.x * _numCells.y);
}

}

std::vector<glm::vec2> poissonDiskDistribution(float separation, const core::RectFloat &bounds, const std::vector<glm::vec2> &initialSet, int k) {
	// prepare working structures
	std::vector<glm::vec2> processingList;
	std::vector<glm::vec2> outputList;
	core::Random rnd;

	// create grid
	Grid grid(bounds, 3);

	processingList.reserve(initialSet.size());
	outputList.reserve(initialSet.size());
	// add the initial points
	for (auto p : initialSet) {
		processingList.push_back(p);
		outputList.push_back(p);
		grid.add(p);
	}

	// if there's no initial points add the center point
	if (processingList.empty()) {
		const glm::vec2& center = bounds.center();
		processingList.push_back(center);
		outputList.push_back(center);
		grid.add(center);
	}

	// while there's points in the processing list
	while (!processingList.empty()) {
		// pick a random point in the processing list
		const int randPoint = rnd.random(0, processingList.size() - 1);
		const glm::vec2& center = processingList[randPoint];

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
			if (bounds.contains(newPoint) && !grid.hasNeighbors(newPoint, separation)) {
				// if the point has no close neighbors add it to the processing list, output list and grid
				processingList.push_back(newPoint);
				outputList.push_back(newPoint);
				grid.add(newPoint);
			}
		}
	}

	return outputList;
}

std::vector<glm::vec2> poissonDiskDistribution(const std::function<float(const glm::vec2&)> &distFunction, const core::RectFloat &bounds,
		const std::vector<glm::vec2> &initialSet, int k) {
	// prepare working structures
	std::vector<glm::vec2> processingList;
	std::vector<glm::vec2> outputList;
	core::Random rnd;

	// create grid
	Grid grid(bounds, 3);

	processingList.reserve(initialSet.size());
	outputList.reserve(initialSet.size());
	// add the initial points
	for (auto p : initialSet) {
		processingList.push_back(p);
		outputList.push_back(p);
		grid.add(p);
	}

	// if there's no initial points add the center point
	if (processingList.empty()) {
		processingList.push_back(bounds.center());
		outputList.push_back(bounds.center());
		grid.add(bounds.center());
	}

	// while there's points in the processing list
	while (!processingList.empty()) {
		// pick a random point in the processing list
		const int randPoint = rnd.random(0, processingList.size() - 1);
		const glm::vec2& center = processingList[randPoint];

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
				outputList.push_back(newPoint);
				grid.add(newPoint);
			}
		}
	}

	return outputList;
}

std::vector<glm::vec2> poissonDiskDistribution(const std::function<float(const glm::vec2&)> &distFunction,
		const std::function<bool(const glm::vec2&)> &boundsFunction, const core::RectFloat &bounds, const std::vector<glm::vec2> &initialSet, int k) {
	// prepare working structures
	std::vector<glm::vec2> processingList;
	std::vector<glm::vec2> outputList;
	core::Random rnd;

	// create grid
	Grid grid(bounds, 3);

	processingList.reserve(initialSet.size());
	outputList.reserve(initialSet.size());
	// add the initial points
	for (auto p : initialSet) {
		processingList.push_back(p);
		outputList.push_back(p);
		grid.add(p);
	}

	// if there's no initial points add the center point
	if (processingList.empty()) {
		processingList.push_back(bounds.center());
		outputList.push_back(bounds.center());
		grid.add(bounds.center());
	}

	// while there's points in the processing list
	while (!processingList.empty()) {
		// pick a random point in the processing list
		const int randPoint = rnd.random(0, processingList.size() - 1);
		const glm::vec2& center = processingList[randPoint];

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
				outputList.push_back(newPoint);
				grid.add(newPoint);
			}
		}
	}

	return outputList;
}

}
