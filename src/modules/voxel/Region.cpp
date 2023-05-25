/**
 * @file
 */

#include "Region.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/Log.h"
#include <glm/vector_relational.hpp>
#include "math/AABB.h"
#include "math/Rect.h"
#include "math/Random.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <stdint.h>
#include <limits>

namespace voxel {

const Region Region::InvalidRegion = Region(0, -1);

void Region::update() {
	_width = _maxs - _mins;
	_center = _mins + _width / 2;
	_voxels = _width + glm::ivec3(1);
	_stride = _voxels.x * _voxels.y;
}

core::String Region::toString(bool center) const {
	core::String regionStr("region[");
	std::string s;
	if (center) {
		s = glm::to_string(getCenter());
		regionStr += "center(";
		regionStr += s.c_str();
		regionStr += "), ";
	}
	s = glm::to_string(getLowerCorner());
	regionStr += "mins(";
	regionStr += s.c_str();
	regionStr += "), ";
	s = glm::to_string(getUpperCorner());
	regionStr += "maxs(";
	regionStr += s.c_str();
	regionStr += ")]";
	return regionStr;
}

glm::ivec3 Region::moveInto(int32_t x, int32_t y, int32_t z) const {
	const glm::ivec3& size = getDimensionsInVoxels();
	const glm::ivec3& mins = getLowerCorner();
	const glm::ivec3& maxs = getUpperCorner();
	const int32_t ox = (x < 0 ? maxs.x : mins.x) + (x % size.x);
	const int32_t oy = (y < 0 ? maxs.y : mins.y) + (y % size.y);
	const int32_t oz = (z < 0 ? maxs.z : mins.z) + (z % size.z);
	core_assert_msg(containsPoint(ox, oy, oz),
			"shifted(%i:%i:%i) is outside the valid region for pos(%i:%i:%i), size(%i:%i:%i), mins(%i:%i:%i)",
			ox, oy, oz, x, y, z, size.x, size.y, size.z, getLowerX(), getLowerY(), getLowerZ());
	return glm::ivec3(ox, oy, oz);
}

void logRegion(const char *ctx, const voxel::Region& region) {
	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();
	Log::debug("%s: region[mins(%i,%i,%i), maxs(%i,%i,%i)]", ctx, mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
}

int Region::voxels() const {
	return getWidthInVoxels() * getHeightInVoxels() * getDepthInVoxels();
}

glm::ivec3 Region::getRandomPosition(math::Random& random) const {
	const int x = random.random(_mins.x, _maxs.x);
	const int y = random.random(_mins.y, _maxs.y);
	const int z = random.random(_mins.z, _maxs.z);
	return glm::ivec3(x, y, z);
}

/**
 * @param x The 'x' component of the position to accumulate.
 * @param y The 'y' component of the position to accumulate.
 * @param z The 'z' component of the position to accumulate.
 */
void Region::accumulate(int32_t x, int32_t y, int32_t z) {
	_mins.x = core_min(_mins.x, x);
	_mins.y = core_min(_mins.y, y);
	_mins.z = core_min(_mins.z, z);
	_maxs.x = core_max(_maxs.x, x);
	_maxs.y = core_max(_maxs.y, y);
	_maxs.z = core_max(_maxs.z, z);
	update();
}

/**
 * Note that this is not the same as computing the union of two Regions (as the result of
 * such a union may not be a shape which can be exactly represented by a Region). Instead,
 * the result is simply big enough to contain both this Region and the one passed as a parameter.
 * @param reg The Region to accumulate. This must be valid as defined by the isValid() function.
 * @sa isValid()
 */
void Region::accumulate(const Region& reg) {
	if (!reg.isValid()) {
		// The result of accumulating an invalid region is not defined.
		core_assert_msg(false, "You cannot accumulate an invalid region.");
	}

	_mins = (glm::min)(_mins, reg.getLowerCorner());
	_maxs = (glm::max)(_maxs, reg.getUpperCorner());
	update();
}

/**
 * After calling this functions, the extents of this Region are given by the intersection
 * of this Region and the one it was cropped to.
 * @param other The Region to crop to.
 */
void Region::cropTo(const Region& other) {
	_mins = (glm::max)(_mins, other._mins);
	_maxs = (glm::min)(_maxs, other._maxs);
	update();
}

/**
 * The same amount of growth is applied in all directions. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param amount The amount to grow by.
 */
void Region::grow(int32_t amount) {
	_mins -= amount;
	_maxs += amount;
	update();
}

/**
 * The amount can be specified separately for each direction. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param x The amount to grow by in 'x'.
 * @param y The amount to grow by in 'y'.
 * @param z The amount to grow by in 'z'.
 */
void Region::grow(int32_t x, int32_t y, int32_t z) {
	_mins.x -= x;
	_mins.y -= y;
	_mins.z -= z;

	_maxs.x += x;
	_maxs.y += y;
	_maxs.z += z;
	update();
}

/**
 * The amount can be specified separately for each direction. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param amount The amount to grow by (one component for each direction).
 */
void Region::grow(const glm::ivec3& amount) {
	_mins -= amount;
	_maxs += amount;
	update();
}

/**
 * @return The position of the lower corner.
 */
const glm::ivec3& Region::getCenter() const {
	return _center;
}

glm::vec3 Region::calcCenterf() const {
	return glm::vec3(_mins + _width) / 2.0f;
}

const glm::vec3& Region::pivot() const {
	return _pivot;
}

glm::vec3 Region::worldPivot() const {
	return glm::vec3(_mins) + _pivot * glm::vec3(_voxels);
}

void Region::setPivot(const glm::vec3 &normalizedPivot) {
	_pivot = normalizedPivot;
}

/**
 * @return The position of the lower corner.
 */
const glm::ivec3& Region::getLowerCorner() const {
	return _mins;
}

/**
 * @return The position of the upper corner.
 */
const glm::ivec3& Region::getUpperCorner() const {
	return _maxs;
}

glm::vec3 Region::getLowerCornerf() const {
	return glm::vec3(_mins);
}

glm::vec3 Region::getUpperCornerf() const {
	return glm::vec3(_maxs);
}

/**
 * @return The dimensions of the region measured in voxels.
 * @sa getDimensionsInCells()
 */
const glm::ivec3& Region::getDimensionsInVoxels() const {
	return _voxels;
}

/**
 * @return The dimensions of the region measured in cells.
 * @sa getDimensionsInVoxels()
 */
const glm::ivec3& Region::getDimensionsInCells() const {
	return _width;
}

/**
 * @param mins The new position of the lower corner.
 */
void Region::setLowerCorner(const glm::ivec3& mins) {
	_mins = mins;
	update();
}

/**
 * @param maxs The new position of the upper corner.
 */
void Region::setUpperCorner(const glm::ivec3& maxs) {
	_maxs = maxs;
	update();
}

/**
 * @param pos The position to accumulate.
 */
void Region::accumulate(const glm::ivec3& pos) {
	accumulate(pos.x, pos.y, pos.z);
}

/**
 * @see math::transform()
 */
Region Region::rotate(const glm::mat4 &mat, const glm::vec3 &pivot) const {
	const glm::vec4 pivot4(pivot, 0.0f);
	const glm::vec4 vertices[]{
		glm::vec4((float)_mins.x + 0.5f, (float)_mins.y + 0.5f, (float)_mins.z + 0.5f, 1.0f) - pivot4,
		glm::vec4((float)_maxs.x + 0.5f, (float)_mins.y + 0.5f, (float)_mins.z + 0.5f, 1.0f) - pivot4,
		glm::vec4((float)_mins.x + 0.5f, (float)_maxs.y + 0.5f, (float)_mins.z + 0.5f, 1.0f) - pivot4,
		glm::vec4((float)_maxs.x + 0.5f, (float)_maxs.y + 0.5f, (float)_mins.z + 0.5f, 1.0f) - pivot4,
		glm::vec4((float)_mins.x + 0.5f, (float)_mins.y + 0.5f, (float)_maxs.z + 0.5f, 1.0f) - pivot4,
		glm::vec4((float)_maxs.x + 0.5f, (float)_mins.y + 0.5f, (float)_maxs.z + 0.5f, 1.0f) - pivot4,
		glm::vec4((float)_mins.x + 0.5f, (float)_maxs.y + 0.5f, (float)_maxs.z + 0.5f, 1.0f) - pivot4,
		glm::vec4((float)_maxs.x + 0.5f, (float)_maxs.y + 0.5f, (float)_maxs.z + 0.5f, 1.0f) - pivot4
	};
	glm::vec3 newMins(FLT_MAX);
	glm::vec3 newMaxs(-FLT_MAX);
	for (int i = 0; i < 8; ++i) {
		const glm::vec4 &target = mat * vertices[i];
		const glm::vec3 &corrected = glm::floor(glm::vec3(target) + pivot);
		newMins = glm::min(corrected, newMins);
		newMaxs = glm::max(corrected, newMaxs);
	}
	const voxel::Region region(newMins, newMaxs);
	return region;
}

/**
 * Constructs a Region and sets the lower and upper corners to the specified values.
 * @param mins The desired lower corner of the Region.
 * @param maxs The desired upper corner of the Region.
 */
Region::Region(const glm::ivec3& mins, const glm::ivec3& maxs) :
		Region(mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z) {
}

Region& Region::operator+=(const glm::ivec3& amount) {
	shift(amount);
	return *this;
}

Region operator+(const Region& region, const glm::ivec3& amount) {
	Region copy(region);
	copy.shift(amount);
	return copy;
}

/**
 * The test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 */
bool Region::containsPoint(const glm::vec3& pos) const {
	return containsPoint(pos.x, pos.y, pos.z);
}

/**
 * The test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 */
bool Region::containsPoint(const glm::ivec3& pos) const {
	return containsPoint(pos.x, pos.y, pos.z);
}

/**
 * @param amount The amount to move the Region by.
 */
void Region::shift(const glm::ivec3& amount) {
	shiftLowerCorner(amount);
	shiftUpperCorner(amount);
}

bool Region::isOnBorder(const glm::ivec3 &pos) const {
	return glm::any(glm::equal(pos, _maxs)) || glm::any(glm::equal(pos, _mins));
}

/**
 * @param amount The amount to move the lower corner by.
 */
void Region::shiftLowerCorner(const glm::ivec3& amount) {
	shiftLowerCorner(amount.x, amount.y, amount.z);
}

/**
 * @param amount The amount to move the upper corner by.
 */
void Region::shiftUpperCorner(const glm::ivec3& amount) {
	shiftUpperCorner(amount.x, amount.y, amount.z);
}

/**
 * The amount can be specified separately for each direction. Negative shrinkage
 * is possible but you should prefer the grow() function for clarity.
 * @param amount The amount to shrink by (one component for each direction).
 */
void Region::shrink(const glm::ivec3& amount) {
	shrink(amount.x, amount.y, amount.z);
}

}
