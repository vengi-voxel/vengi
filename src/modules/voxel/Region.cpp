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

std::string Region::toString() const {
	const std::string& regionStr = "region["
		"center(" + glm::to_string(getCentre()) + "), "
		"mins(" + glm::to_string(getLowerCorner()) + "), "
		"maxs(" + glm::to_string(getUpperCorner()) + ")"
	"]";
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

math::Rect<int> Region::rect(int border) const {
	core_assert(getUpperX() - getLowerX() > 2 * border);
	core_assert(getUpperZ() - getLowerZ() > 2 * border);
	return math::Rect<int>(getLowerX() + border, getLowerZ() + border, getUpperX() - border, getUpperZ() - border);
}

math::Rect<float> Region::rectf(int border) const {
	core_assert(getUpperX() - getLowerX() > 2 * border);
	core_assert(getUpperZ() - getLowerZ() > 2 * border);
	return math::Rect<float>(getLowerX() + border, getLowerZ() + border, getUpperX() - border, getUpperZ() - border);
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

}
