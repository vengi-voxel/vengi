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
#include <glm/vec3.hpp>
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
	s = glm::to_string(getCentre());
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
	Log::debug("%s: %s", ctx, region.toString().c_str());
}

int Region::voxels() const {
	return getWidthInVoxels() * getHeightInVoxels() * getDepthInVoxels();
}

glm::ivec3 Region::getRandomPosition(math::Random& random) const {
	const int x = random.random(m_iLowerX, m_iUpperX);
	const int y = random.random(m_iLowerY, m_iUpperY);
	const int z = random.random(m_iLowerZ, m_iUpperZ);
	return glm::ivec3(x, y, z);
}

/**
 * @param iX The 'x' component of the position to accumulate.
 * @param iY The 'y' component of the position to accumulate.
 * @param iZ The 'z' component of the position to accumulate.
 */
void Region::accumulate(int32_t iX, int32_t iY, int32_t iZ) {
	m_iLowerX = core_min(m_iLowerX, iX);
	m_iLowerY = core_min(m_iLowerY, iY);
	m_iLowerZ = core_min(m_iLowerZ, iZ);
	m_iUpperX = core_max(m_iUpperX, iX);
	m_iUpperY = core_max(m_iUpperY, iY);
	m_iUpperZ = core_max(m_iUpperZ, iZ);
}

Region Region::accumulateCopy(const Region& reg) const {
	if (!reg.isValid()) {
		// The result of accumulating an invalid region is not defined.
		core_assert_msg(false, "You cannot accumulate an invalid region.");
	}

	Region r(*this);
	r.m_iLowerX = core_min(r.m_iLowerX, reg.getLowerX());
	r.m_iLowerY = core_min(r.m_iLowerY, reg.getLowerY());
	r.m_iLowerZ = core_min(r.m_iLowerZ, reg.getLowerZ());
	r.m_iUpperX = core_max(r.m_iUpperX, reg.getUpperX());
	r.m_iUpperY = core_max(r.m_iUpperY, reg.getUpperY());
	r.m_iUpperZ = core_max(r.m_iUpperZ, reg.getUpperZ());
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

	m_iLowerX = core_min(m_iLowerX, reg.getLowerX());
	m_iLowerY = core_min(m_iLowerY, reg.getLowerY());
	m_iLowerZ = core_min(m_iLowerZ, reg.getLowerZ());
	m_iUpperX = core_max(m_iUpperX, reg.getUpperX());
	m_iUpperY = core_max(m_iUpperY, reg.getUpperY());
	m_iUpperZ = core_max(m_iUpperZ, reg.getUpperZ());
}

/**
 * After calling this functions, the extents of this Region are given by the intersection
 * of this Region and the one it was cropped to.
 * @param other The Region to crop to.
 */
void Region::cropTo(const Region& other) {
	m_iLowerX = core_max(m_iLowerX, other.m_iLowerX);
	m_iLowerY = core_max(m_iLowerY, other.m_iLowerY);
	m_iLowerZ = core_max(m_iLowerZ, other.m_iLowerZ);
	m_iUpperX = core_min(m_iUpperX, other.m_iUpperX);
	m_iUpperY = core_min(m_iUpperY, other.m_iUpperY);
	m_iUpperZ = core_min(m_iUpperZ, other.m_iUpperZ);
}

/**
 * The same amount of growth is applied in all directions. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param iAmount The amount to grow by.
 */
void Region::grow(int32_t iAmount) {
	m_iLowerX -= iAmount;
	m_iLowerY -= iAmount;
	m_iLowerZ -= iAmount;

	m_iUpperX += iAmount;
	m_iUpperY += iAmount;
	m_iUpperZ += iAmount;
}

/**
 * The amount can be specified separately for each direction. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param iAmountX The amount to grow by in 'x'.
 * @param iAmountY The amount to grow by in 'y'.
 * @param iAmountZ The amount to grow by in 'z'.
 */
void Region::grow(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ) {
	m_iLowerX -= iAmountX;
	m_iLowerY -= iAmountY;
	m_iLowerZ -= iAmountZ;

	m_iUpperX += iAmountX;
	m_iUpperY += iAmountY;
	m_iUpperZ += iAmountZ;
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
glm::ivec3 Region::getCentre() const {
	return glm::ivec3(getCentreX(), getCentreY(), getCentreZ());
}

glm::vec3 Region::getCentref() const {
	return glm::vec3(getCentreXf(), getCentreYf(), getCentreZf());
}

/**
 * @return The position of the lower corner.
 */
glm::ivec3 Region::getLowerCorner() const {
	return glm::ivec3(m_iLowerX, m_iLowerY, m_iLowerZ);
}

/**
 * @return The position of the upper corner.
 */
glm::ivec3 Region::getUpperCorner() const {
	return glm::ivec3(m_iUpperX, m_iUpperY, m_iUpperZ);
}

glm::vec3 Region::getLowerCornerf() const {
	return glm::vec3(m_iLowerX, m_iLowerY, m_iLowerZ);
}

glm::vec3 Region::getUpperCornerf() const {
	return glm::vec3(m_iUpperX, m_iUpperY, m_iUpperZ);
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
	m_iLowerX = v3dLowerCorner.x;
	m_iLowerY = v3dLowerCorner.y;
	m_iLowerZ = v3dLowerCorner.z;
}

/**
 * @param v3dUpperCorner The new position of the upper corner.
 */
void Region::setUpperCorner(const glm::ivec3& v3dUpperCorner) {
	m_iUpperX = v3dUpperCorner.x;
	m_iUpperY = v3dUpperCorner.y;
	m_iUpperZ = v3dUpperCorner.z;
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
