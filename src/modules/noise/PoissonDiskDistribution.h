/**
 * @file
 * @brief 2D Poisson Disk Distribution
 */

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

#pragma once

#include <functional>
#include <vector>
#include "core/Rect.h"
#include "core/AABB.h"

namespace noise {

/**
 * returns a set of poisson disk samples inside a rectangular \a area,
 * with a minimum \a separation and with a packing determined by how
 * high \a k is. The higher \a k is the higher the algorithm will be slow.
 * If no \a initialSet of points is provided the area center will be used
 * as the initial point.
 */
std::vector<glm::vec2> poissonDiskDistribution(float separation, const core::Rect<int> &area, const std::vector<glm::vec2> &initialSet =
		std::vector<glm::vec2>(), int k = 30);


/**
 * returns a set of poisson disk samples inside an axis aligned bounding box,
 * with a minimum \a separation and with a packing determined by how
 * high \a k is. The higher \a k is the higher the algorithm will be slow.
 * If no \a initialSet of points is provided the area center will be used
 * as the initial point.
 */
std::vector<glm::vec3> poissonDiskDistribution(float separation, const core::AABB<int> &area, const std::vector<glm::vec3> &initialSet =
		std::vector<glm::vec3>(), int k = 30);

/**
 * returns a set of poisson disk samples inside a rectangular \a area,
 * with a minimum separation defined by what \a distFunction returns and
 * with a packing determined by how high \a k is. The higher \a k is the
 * higher the algorithm will be slow. If no \a initialSet of points is
 * provided the area center will be used as the initial point.
 */
std::vector<glm::vec2> poissonDiskDistribution(const std::function<float(const glm::vec2&)> &distFunction, const core::Rect<int> &area,
		const std::vector<glm::vec2> &initialSet = std::vector<glm::vec2>(), int k = 30);
/**
 * returns a set of poisson disk samples within bounds defined by both \a
 * boundsFunction and a rectangular \a area, with a minimum separation defined
 * by what \a distFunction returns and with a packing determined by how high
 * \a k is. The higher \a k is the higher the algorithm will be slow. If no
 * \a initialSet of points is provided the area center will be used as the initial
 * point.
 *
 * @todo: remove Rect<int> area arguments and compute bounds inside the function
 */
std::vector<glm::vec2> poissonDiskDistribution(const std::function<float(const glm::vec2&)> &distFunction,
		const std::function<bool(const glm::vec2&)> &boundsFunction, const core::Rect<int> &area,
		const std::vector<glm::vec2> &initialSet = std::vector<glm::vec2>(), int k = 30);

}
