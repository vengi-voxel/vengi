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

static core::DynamicArray<Region> subtractRegion(const Region &box, const Region &sub) {
	core::DynamicArray<Region> result;
	result.reserve(6);

	// Ensure the subtraction region is inside the box
	if (sub.getLowerCorner().x > box.getUpperCorner().x || sub.getUpperCorner().x < box.getLowerCorner().x ||
		sub.getLowerCorner().y > box.getUpperCorner().y || sub.getUpperCorner().y < box.getLowerCorner().y ||
		sub.getLowerCorner().z > box.getUpperCorner().z || sub.getUpperCorner().z < box.getLowerCorner().z) {
		// No overlap, box remains unchanged
		result.push_back(box);
		return result;
	}

	// Top part (above the selected region)
	if (sub.getUpperCorner().z < box.getUpperCorner().z) {
		result.emplace_back(glm::ivec3(box.getLowerCorner().x, box.getLowerCorner().y, sub.getUpperCorner().z + 1),
							glm::ivec3(box.getUpperCorner().x, box.getUpperCorner().y, box.getUpperCorner().z));
	}

	// Bottom part (below the selected region)
	if (sub.getLowerCorner().z > box.getLowerCorner().z) {
		result.emplace_back(glm::ivec3(box.getLowerCorner().x, box.getLowerCorner().y, box.getLowerCorner().z),
							glm::ivec3(box.getUpperCorner().x, box.getUpperCorner().y, sub.getLowerCorner().z - 1));
	}

	// Front part (in front of the selected region)
	if (sub.getUpperCorner().y < box.getUpperCorner().y) {
		result.emplace_back(glm::ivec3(box.getLowerCorner().x, sub.getUpperCorner().y + 1, sub.getLowerCorner().z),
							glm::ivec3(box.getUpperCorner().x, box.getUpperCorner().y, sub.getUpperCorner().z));
	}

	// Back part (behind the selected region)
	if (sub.getLowerCorner().y > box.getLowerCorner().y) {
		result.emplace_back(glm::ivec3(box.getLowerCorner().x, box.getLowerCorner().y, sub.getLowerCorner().z),
							glm::ivec3(box.getUpperCorner().x, sub.getLowerCorner().y - 1, sub.getUpperCorner().z));
	}

	// Left part (left of the selected region)
	if (sub.getLowerCorner().x > box.getLowerCorner().x) {
		result.emplace_back(glm::ivec3(box.getLowerCorner().x, sub.getLowerCorner().y, sub.getLowerCorner().z),
							glm::ivec3(sub.getLowerCorner().x - 1, sub.getUpperCorner().y, sub.getUpperCorner().z));
	}

	// Right part (right of the selected region)
	if (sub.getUpperCorner().x < box.getUpperCorner().x) {
		result.emplace_back(glm::ivec3(sub.getUpperCorner().x + 1, sub.getLowerCorner().y, sub.getLowerCorner().z),
							glm::ivec3(box.getUpperCorner().x, sub.getUpperCorner().y, sub.getUpperCorner().z));
	}

	return result;
}

core::DynamicArray<Region> Region::subtract(const Region& a, const core::DynamicArray<Region>& b) {
	core::DynamicArray<Region> remainingSelections;
	remainingSelections.reserve(b.size() * 6);
	remainingSelections.push_back(a);

	for (const Region &r : b) {
		core::DynamicArray<Region> newSelections;
		for (const Region &region : remainingSelections) {
			const core::DynamicArray<Region> &subtracted = subtractRegion(region, r);
			newSelections.insert(newSelections.end(), subtracted.begin(), subtracted.end());
		}
		remainingSelections = core::move(newSelections);
	}
	return remainingSelections;
}

void Region::update() {
	_width = _maxs - _mins;
	_center = _mins + _width / 2;
	_voxels = _width + glm::ivec3(1);
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
	return getWidthInVoxels() * getHeightInVoxels() * getDepthInVoxels();
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
	_mins -= amount;
	_maxs += amount;
	update();
}

const glm::ivec3& Region::getCenter() const {
	return _center;
}

glm::ivec3 Region::getLowerCenter() const {
	glm::ivec3 c = _center;
	c.y = _mins.y;
	return c;
}

glm::vec3 Region::calcCenterf() const {
	return glm::vec3(_mins) + glm::vec3(_voxels) / 2.0f;
}

glm::vec3 Region::calcCellCenterf() const {
	return glm::vec3(_mins) + glm::vec3(_width) / 2.0f;
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
	const glm::vec4 vertices[]{
		glm::vec4((float)_mins.x - 0.5f - pivot.x, (float)_mins.y - 0.5f - pivot.y, (float)_mins.z - 0.5f - pivot.z, 1.0f),
		glm::vec4((float)_maxs.x + 0.5f - pivot.x, (float)_mins.y - 0.5f - pivot.y, (float)_mins.z - 0.5f - pivot.z, 1.0f),
		glm::vec4((float)_mins.x - 0.5f - pivot.x, (float)_maxs.y + 0.5f - pivot.y, (float)_mins.z - 0.5f - pivot.z, 1.0f),
		glm::vec4((float)_maxs.x + 0.5f - pivot.x, (float)_maxs.y + 0.5f - pivot.y, (float)_mins.z - 0.5f - pivot.z, 1.0f),
		glm::vec4((float)_mins.x - 0.5f - pivot.x, (float)_mins.y - 0.5f - pivot.y, (float)_maxs.z + 0.5f - pivot.z, 1.0f),
		glm::vec4((float)_maxs.x + 0.5f - pivot.x, (float)_mins.y - 0.5f - pivot.y, (float)_maxs.z + 0.5f - pivot.z, 1.0f),
		glm::vec4((float)_mins.x - 0.5f - pivot.x, (float)_maxs.y + 0.5f - pivot.y, (float)_maxs.z + 0.5f - pivot.z, 1.0f),
		glm::vec4((float)_maxs.x + 0.5f - pivot.x, (float)_maxs.y + 0.5f - pivot.y, (float)_maxs.z + 0.5f - pivot.z, 1.0f)
	};
	glm::ivec3 newMins(INT_MAX);
	glm::ivec3 newMaxs(INT_MIN);
	for (int i = 0; i < 8; ++i) {
		const glm::vec3 &target = mat * vertices[i];
		const glm::ivec3 &corrected = glm::round(target + 0.5f + pivot);
		newMins = glm::min(corrected, newMins);
		newMaxs = glm::max(corrected, newMaxs);
	}
	const voxel::Region region(newMins, newMaxs - 1);
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
