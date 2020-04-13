/**
 * @file
 */

#include "Region.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/Log.h"
#include "math/AABB.h"
#include "math/Rect.h"
#include "math/Random.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <stdint.h>
#include <limits>

namespace voxel {

const Region Region::MaxRegion = Region((std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::max)());
const Region Region::InvalidRegion = Region(0, -1);

core::String Region::toString() const {
	core::String regionStr("region[");
	std::string s;
	s = glm::to_string(getCenter());
	regionStr += "center(";
	regionStr += s.c_str();
	regionStr += "), ";
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
 * @param iX The 'x' component of the position to accumulate.
 * @param iY The 'y' component of the position to accumulate.
 * @param iZ The 'z' component of the position to accumulate.
 */
void Region::accumulate(int32_t iX, int32_t iY, int32_t iZ) {
	_mins.x = core_min(_mins.x, iX);
	_mins.y = core_min(_mins.y, iY);
	_mins.z = core_min(_mins.z, iZ);
	_maxs.x = core_max(_maxs.x, iX);
	_maxs.y = core_max(_maxs.y, iY);
	_maxs.z = core_max(_maxs.z, iZ);
}

Region Region::accumulateCopy(const Region& reg) const {
	if (!reg.isValid()) {
		// The result of accumulating an invalid region is not defined.
		core_assert_msg(false, "You cannot accumulate an invalid region.");
	}

	Region r(*this);
	r._mins.x = core_min(r._mins.x, reg.getLowerX());
	r._mins.y = core_min(r._mins.y, reg.getLowerY());
	r._mins.z = core_min(r._mins.z, reg.getLowerZ());
	r._maxs.x = core_max(r._maxs.x, reg.getUpperX());
	r._maxs.y = core_max(r._maxs.y, reg.getUpperY());
	r._maxs.z = core_max(r._maxs.z, reg.getUpperZ());
	return r;
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

	_mins.x = core_min(_mins.x, reg.getLowerX());
	_mins.y = core_min(_mins.y, reg.getLowerY());
	_mins.z = core_min(_mins.z, reg.getLowerZ());
	_maxs.x = core_max(_maxs.x, reg.getUpperX());
	_maxs.y = core_max(_maxs.y, reg.getUpperY());
	_maxs.z = core_max(_maxs.z, reg.getUpperZ());
}

/**
 * After calling this functions, the extents of this Region are given by the intersection
 * of this Region and the one it was cropped to.
 * @param other The Region to crop to.
 */
void Region::cropTo(const Region& other) {
	_mins.x = core_max(_mins.x, other._mins.x);
	_mins.y = core_max(_mins.y, other._mins.y);
	_mins.z = core_max(_mins.z, other._mins.z);
	_maxs.x = core_min(_maxs.x, other._maxs.x);
	_maxs.y = core_min(_maxs.y, other._maxs.y);
	_maxs.z = core_min(_maxs.z, other._maxs.z);
}

/**
 * The same amount of growth is applied in all directions. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param iAmount The amount to grow by.
 */
void Region::grow(int32_t iAmount) {
	_mins.x -= iAmount;
	_mins.y -= iAmount;
	_mins.z -= iAmount;

	_maxs.x += iAmount;
	_maxs.y += iAmount;
	_maxs.z += iAmount;
}

/**
 * The amount can be specified separately for each direction. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param iAmountX The amount to grow by in 'x'.
 * @param iAmountY The amount to grow by in 'y'.
 * @param iAmountZ The amount to grow by in 'z'.
 */
void Region::grow(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ) {
	_mins.x -= iAmountX;
	_mins.y -= iAmountY;
	_mins.z -= iAmountZ;

	_maxs.x += iAmountX;
	_maxs.y += iAmountY;
	_maxs.z += iAmountZ;
}

/**
 * The amount can be specified separately for each direction. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param v3dAmount The amount to grow by (one component for each direction).
 */
void Region::grow(const glm::ivec3& v3dAmount) {
	grow(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

/**
 * @return The position of the lower corner.
 */
glm::ivec3 Region::getCenter() const {
	return glm::ivec3(getCenterX(), getCenterY(), getCenterZ());
}

glm::vec3 Region::getCenterf() const {
	return glm::vec3(getCenterXf(), getCenterYf(), getCenterZf());
}

/**
 * @return The position of the lower corner.
 */
glm::ivec3 Region::getLowerCorner() const {
	return glm::ivec3(_mins.x, _mins.y, _mins.z);
}

/**
 * @return The position of the upper corner.
 */
glm::ivec3 Region::getUpperCorner() const {
	return glm::ivec3(_maxs.x, _maxs.y, _maxs.z);
}

glm::vec3 Region::getLowerCornerf() const {
	return glm::vec3(_mins.x, _mins.y, _mins.z);
}

glm::vec3 Region::getUpperCornerf() const {
	return glm::vec3(_maxs.x, _maxs.y, _maxs.z);
}

/**
 * @return The dimensions of the region measured in voxels.
 * @sa getDimensionsInCells()
 */
glm::ivec3 Region::getDimensionsInVoxels() const {
	return getDimensionsInCells() + glm::ivec3(1, 1, 1);
}

/**
 * @return The dimensions of the region measured in cells.
 * @sa getDimensionsInVoxels()
 */
glm::ivec3 Region::getDimensionsInCells() const {
	return glm::ivec3(getWidthInCells(), getHeightInCells(), getDepthInCells());
}

/**
 * @param v3dLowerCorner The new position of the lower corner.
 */
void Region::setLowerCorner(const glm::ivec3& v3dLowerCorner) {
	_mins.x = v3dLowerCorner.x;
	_mins.y = v3dLowerCorner.y;
	_mins.z = v3dLowerCorner.z;
}

/**
 * @param v3dUpperCorner The new position of the upper corner.
 */
void Region::setUpperCorner(const glm::ivec3& v3dUpperCorner) {
	_maxs.x = v3dUpperCorner.x;
	_maxs.y = v3dUpperCorner.y;
	_maxs.z = v3dUpperCorner.z;
}

/**
 * @param v3dPos The position to accumulate.
 */
void Region::accumulate(const glm::ivec3& v3dPos) {
	accumulate(v3dPos.x, v3dPos.y, v3dPos.z);
}

Region Region::accumulateCopy(const glm::ivec3& v3dPos) const {
	Region r(*this);
	r.accumulate(v3dPos.x, v3dPos.y, v3dPos.z);
	return r;
}
/**
 * Constructs a Region and sets the lower and upper corners to the specified values.
 * @param v3dLowerCorner The desired lower corner of the Region.
 * @param v3dUpperCorner The desired upper corner of the Region.
 */
Region::Region(const glm::ivec3& v3dLowerCorner, const glm::ivec3& v3dUpperCorner) :
		Region(v3dLowerCorner.x, v3dLowerCorner.y, v3dLowerCorner.z, v3dUpperCorner.x, v3dUpperCorner.y, v3dUpperCorner.z) {
}

Region& Region::operator+=(const glm::ivec3& v3dAmount) {
	shift(v3dAmount);
	return *this;
}

Region operator+(const Region& region, const glm::ivec3& v3dAmount) {
	Region copy(region);
	copy.shift(v3dAmount);
	return copy;
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in all directions. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
bool Region::containsPoint(const glm::vec3& pos, float boundary) const {
	return containsPoint(pos.x, pos.y, pos.z, boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in all directions. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
bool Region::containsPoint(const glm::ivec3& pos, uint8_t boundary) const {
	return containsPoint(pos.x, pos.y, pos.z, boundary);
}

/**
 * @param v3dAmount The amount to move the Region by.
 */
void Region::shift(const glm::ivec3& v3dAmount) {
	shiftLowerCorner(v3dAmount);
	shiftUpperCorner(v3dAmount);
}

/**
 * @param v3dAmount The amount to move the lower corner by.
 */
void Region::shiftLowerCorner(const glm::ivec3& v3dAmount) {
	shiftLowerCorner(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

/**
 * @param v3dAmount The amount to move the upper corner by.
 */
void Region::shiftUpperCorner(const glm::ivec3& v3dAmount) {
	shiftUpperCorner(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

/**
 * The amount can be specified seperatly for each direction. Negative shrinkage
 * is possible but you should prefer the grow() function for clarity.
 * @param v3dAmount The amount to shrink by (one component for each direction).
 */
void Region::shrink(const glm::ivec3& v3dAmount) {
	shrink(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

}
