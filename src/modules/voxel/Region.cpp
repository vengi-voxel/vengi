/**
 * @file
 */

#include "Region.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/Log.h"
#include <glm/vector_relational.hpp>
#include <glm/mat4x4.hpp>
#include <stdint.h>
#include <limits.h>

namespace voxel {

const Region Region::InvalidRegion = Region(0, -1);

static core::Buffer<Region> subtractRegion(const Region &box, const Region &sub) {
	core::Buffer<Region> result;
	result.reserve(6);

	Region clampedSub = sub;
	if (!clampedSub.cropTo(box)) {
		// No overlap, box remains unchanged
		result.push_back(box);
		return result;
	}

	// Top part (above the selected region)
	if (clampedSub.getUpperCorner().z < box.getUpperCorner().z) {
		result.emplace_back(
			glm::ivec3(box.getLowerCorner().x, box.getLowerCorner().y, clampedSub.getUpperCorner().z + 1),
			glm::ivec3(box.getUpperCorner().x, box.getUpperCorner().y, box.getUpperCorner().z));
	}

	// Bottom part (below the selected region)
	if (clampedSub.getLowerCorner().z > box.getLowerCorner().z) {
		result.emplace_back(
			glm::ivec3(box.getLowerCorner().x, box.getLowerCorner().y, box.getLowerCorner().z),
			glm::ivec3(box.getUpperCorner().x, box.getUpperCorner().y, clampedSub.getLowerCorner().z - 1));
	}

	// Front part (in front of the selected region)
	if (clampedSub.getUpperCorner().y < box.getUpperCorner().y) {
		result.emplace_back(
			glm::ivec3(box.getLowerCorner().x, clampedSub.getUpperCorner().y + 1, clampedSub.getLowerCorner().z),
			glm::ivec3(box.getUpperCorner().x, box.getUpperCorner().y, clampedSub.getUpperCorner().z));
	}

	// Back part (behind the selected region)
	if (clampedSub.getLowerCorner().y > box.getLowerCorner().y) {
		result.emplace_back(
			glm::ivec3(box.getLowerCorner().x, box.getLowerCorner().y, clampedSub.getLowerCorner().z),
			glm::ivec3(box.getUpperCorner().x, clampedSub.getLowerCorner().y - 1, clampedSub.getUpperCorner().z));
	}

	// Left part (left of the selected region)
	if (clampedSub.getLowerCorner().x > box.getLowerCorner().x) {
		result.emplace_back(
			glm::ivec3(box.getLowerCorner().x, clampedSub.getLowerCorner().y, clampedSub.getLowerCorner().z),
			glm::ivec3(clampedSub.getLowerCorner().x - 1, clampedSub.getUpperCorner().y,
					   clampedSub.getUpperCorner().z));
	}

	// Right part (right of the selected region)
	if (clampedSub.getUpperCorner().x < box.getUpperCorner().x) {
		result.emplace_back(
			glm::ivec3(clampedSub.getUpperCorner().x + 1, clampedSub.getLowerCorner().y, clampedSub.getLowerCorner().z),
			glm::ivec3(box.getUpperCorner().x, clampedSub.getUpperCorner().y, clampedSub.getUpperCorner().z));
	}

	return result;
}

core::Buffer<Region> Region::subtract(const Region& a, const core::Buffer<Region>& b) {
	core::Buffer<Region> remainingSelections;
	remainingSelections.reserve(b.size() * 6);
	remainingSelections.push_back(a);

	for (const Region &r : b) {
		core::Buffer<Region> newSelections;
		for (const Region &region : remainingSelections) {
			const core::Buffer<Region> &subtracted = subtractRegion(region, r);
			newSelections.insert(newSelections.end(), subtracted.begin(), subtracted.end());
		}
		remainingSelections = core::move(newSelections);
	}
	return remainingSelections;
}

void Region::update() {
	_width = _maxs - _mins;
	_center = _mins + _width / 2;
	_voxels = _width + 1;
	_stride = _voxels.x * _voxels.y;
}

core::String Region::toString(bool center) const {
	core::String regionStr("region[");
	if (center) {
		regionStr += "center(";
		regionStr += core::String::format("%i:%i:%i", _center.x, _center.y, _center.z);
		regionStr += "), ";
	}
	regionStr += "mins(";
	regionStr += core::String::format("%i:%i:%i", _mins.x, _mins.y, _mins.z);
	regionStr += "), ";
	regionStr += "maxs(";
	regionStr += core::String::format("%i:%i:%i", _maxs.x, _maxs.y, _maxs.z);
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
	if (!isValid()) {
		return 0;
	}
	return getWidthInVoxels() * getHeightInVoxels() * getDepthInVoxels();
}

/**
 * @param x The 'x' component of the position to accumulate.
 * @param y The 'y' component of the position to accumulate.
 * @param z The 'z' component of the position to accumulate.
 */
void Region::accumulate(int32_t x, int32_t y, int32_t z) {
	accumulate(glm::aligned_ivec4(x, y, z, 0));
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

	_mins = (glm::min)(_mins, reg._mins);
	_maxs = (glm::max)(_maxs, reg._maxs);
	update();
}

/**
 * After calling this functions, the extents of this Region are given by the intersection
 * of this Region and the one it was cropped to.
 * @param other The Region to crop to.
 */
bool Region::cropTo(const Region& other) {
	if (!intersects(*this, other)) {
		return false;
	}
	_mins = (glm::max)(_mins, other._mins);
	_maxs = (glm::min)(_maxs, other._maxs);
	update();
	return true;
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
	const glm::aligned_ivec4 a(amount.x, amount.y, amount.z, 0);
	_mins -= a;
	_maxs += a;
	update();
}

glm::ivec3 Region::getCenter() const {
	return _center;
}

glm::ivec3 Region::getLowerCenter() const {
	return {_center.x, _mins.y, _center.z};
}

glm::vec3 Region::calcCenterf() const {
	return glm::aligned_vec4(_mins) + glm::aligned_vec4(_voxels) / 2.0f;
}

glm::vec3 Region::calcCellCenterf() const {
	return glm::aligned_vec4(_mins) + glm::aligned_vec4(_width) / 2.0f;
}

/**
 * @return The position of the lower corner.
 */
const glm::ivec3& Region::getLowerCorner() const {
	return *(const glm::ivec3*)&_mins.x;
}

/**
 * @return The position of the upper corner.
 */
const glm::ivec3& Region::getUpperCorner() const {
	return *(const glm::ivec3*)&_maxs.x;
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
	return *(const glm::ivec3*)&_voxels.x;
}

/**
 * @return The dimensions of the region measured in cells.
 * @sa getDimensionsInVoxels()
 */
const glm::ivec3& Region::getDimensionsInCells() const {
	return *(const glm::ivec3*)&_width.x;
}

/**
 * @param mins The new position of the lower corner.
 */
void Region::setLowerCorner(const glm::ivec3& mins) {
	_mins.x = mins.x;
	_mins.y = mins.y;
	_mins.z = mins.z;
	update();
}

/**
 * @param maxs The new position of the upper corner.
 */
void Region::setUpperCorner(const glm::ivec3& maxs) {
	_maxs.x = maxs.x;
	_maxs.y = maxs.y;
	_maxs.z = maxs.z;
	update();
}

/**
 * @param pos The position to accumulate.
 */
void Region::accumulate(const glm::ivec3& pos) {
	accumulate({pos.x, pos.y, pos.z, 0});
}

/**
 * @param pos The position to accumulate.
 */
 void Region::accumulate(const glm::aligned_ivec4& pos) {
	_mins = (glm::min)(_mins, pos);
	_maxs = (glm::max)(_maxs, pos);
	update();
}

/**
 * @see math::transform()
 */
Region Region::rotate(const glm::mat4 &mat, const glm::vec3 &pivot) const {
	const glm::vec4 mins((float)_mins.x - 0.5f - pivot.x, (float)_mins.y - 0.5f - pivot.y, (float)_mins.z - 0.5f - pivot.z, 1.0f);
	const glm::vec4 maxs((float)_maxs.x + 0.5f - pivot.x, (float)_maxs.y + 0.5f - pivot.y, (float)_maxs.z + 0.5f - pivot.z, 1.0f);
	const glm::vec4 vertices[]{
		mins,
		glm::vec4(maxs.x, mins.y, mins.z, 1.0f),
		glm::vec4(mins.x, maxs.y, mins.z, 1.0f),
		glm::vec4(maxs.x, maxs.y, mins.z, 1.0f),
		glm::vec4(mins.x, mins.y, maxs.z, 1.0f),
		glm::vec4(maxs.x, mins.y, maxs.z, 1.0f),
		glm::vec4(mins.x, maxs.y, maxs.z, 1.0f),
		maxs
	};
	glm::aligned_ivec4 newMins(INT_MAX);
	glm::aligned_ivec4 newMaxs(INT_MIN);
	for (int i = 0; i < 8; ++i) {
		const glm::vec3 target(mat * vertices[i]);
		const glm::aligned_ivec4 corrected(glm::round(target + 0.5f + pivot), 0);
		newMins = glm::min(corrected, newMins);
		newMaxs = glm::max(corrected, newMaxs);
	}
	return {newMins.x, newMins.y, newMins.z, newMaxs.x - 1, newMaxs.y - 1, newMaxs.z - 1};
}

Region Region::transform(const glm::mat4 &mat) const {
	const glm::vec3 &rmins = getLowerCornerf();
	const glm::vec3 &rmaxs = getUpperCornerf();
	const glm::vec3 transformedRMaxs = mat * glm::vec4(rmaxs, 1.0f);
	const glm::vec3 transformedRMins = mat * glm::vec4(rmins, 1.0f);
	return Region(
		glm::ivec3(glm::floor(glm::min(transformedRMins.x, transformedRMaxs.x)),
				   glm::floor(glm::min(transformedRMins.y, transformedRMaxs.y)),
				   glm::floor(glm::min(transformedRMins.z, transformedRMaxs.z))),
		glm::ivec3(glm::ceil(glm::max(transformedRMins.x, transformedRMaxs.x)),
				   glm::ceil(glm::max(transformedRMins.y, transformedRMaxs.y)),
				   glm::ceil(glm::max(transformedRMins.z, transformedRMaxs.z))));
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

bool Region::containsPoint(const glm::aligned_ivec4 &pos) const {
	// TODO: glm misses simd for relational operators
	return (pos.x <= _maxs.x) && (pos.y <= _maxs.y) && (pos.z <= _maxs.z) && (pos.x >= _mins.x) && (pos.y >= _mins.y) &&
		   (pos.z >= _mins.z);}

/**
 * @param amount The amount to move the Region by.
 */
void Region::shift(const glm::ivec3& amount) {
	shiftLowerCorner(amount);
	shiftUpperCorner(amount);
}

bool Region::isOnBorder(const glm::ivec3 &pos) const {
	return glm::any(glm::equal(pos, getUpperCorner())) || glm::any(glm::equal(pos, getLowerCorner()));
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
