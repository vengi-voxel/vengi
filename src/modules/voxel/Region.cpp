/**
 * @file
 */

#include "Region.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/Log.h"
#include <stdint.h>
#include <limits.h>
#include "math/AlignedVec4f.h"
#include "math/AlignedVec4i.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Conversions.h"

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
			math::Vec3i(box.getLowerCorner().x, box.getLowerCorner().y, clampedSub.getUpperCorner().z + 1),
			math::Vec3i(box.getUpperCorner().x, box.getUpperCorner().y, box.getUpperCorner().z));
	}

	// Bottom part (below the selected region)
	if (clampedSub.getLowerCorner().z > box.getLowerCorner().z) {
		result.emplace_back(
			math::Vec3i(box.getLowerCorner().x, box.getLowerCorner().y, box.getLowerCorner().z),
			math::Vec3i(box.getUpperCorner().x, box.getUpperCorner().y, clampedSub.getLowerCorner().z - 1));
	}

	// Front part (in front of the selected region)
	if (clampedSub.getUpperCorner().y < box.getUpperCorner().y) {
		result.emplace_back(
			math::Vec3i(box.getLowerCorner().x, clampedSub.getUpperCorner().y + 1, clampedSub.getLowerCorner().z),
			math::Vec3i(box.getUpperCorner().x, box.getUpperCorner().y, clampedSub.getUpperCorner().z));
	}

	// Back part (behind the selected region)
	if (clampedSub.getLowerCorner().y > box.getLowerCorner().y) {
		result.emplace_back(
			math::Vec3i(box.getLowerCorner().x, box.getLowerCorner().y, clampedSub.getLowerCorner().z),
			math::Vec3i(box.getUpperCorner().x, clampedSub.getLowerCorner().y - 1, clampedSub.getUpperCorner().z));
	}

	// Left part (left of the selected region)
	if (clampedSub.getLowerCorner().x > box.getLowerCorner().x) {
		result.emplace_back(
			math::Vec3i(box.getLowerCorner().x, clampedSub.getLowerCorner().y, clampedSub.getLowerCorner().z),
			math::Vec3i(clampedSub.getLowerCorner().x - 1, clampedSub.getUpperCorner().y,
					   clampedSub.getUpperCorner().z));
	}

	// Right part (right of the selected region)
	if (clampedSub.getUpperCorner().x < box.getUpperCorner().x) {
		result.emplace_back(
			math::Vec3i(clampedSub.getUpperCorner().x + 1, clampedSub.getLowerCorner().y, clampedSub.getLowerCorner().z),
			math::Vec3i(box.getUpperCorner().x, clampedSub.getUpperCorner().y, clampedSub.getUpperCorner().z));
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
	_stride = (int64_t)_voxels.x * (int64_t)_voxels.y;
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

math::Vec3i Region::moveInto(int32_t x, int32_t y, int32_t z) const {
	const math::Vec3i& size = getDimensionsInVoxels();
	const math::Vec3i& mins = getLowerCorner();
	const math::Vec3i& maxs = getUpperCorner();
	const int32_t ox = (x < 0 ? maxs.x : mins.x) + (x % size.x);
	const int32_t oy = (y < 0 ? maxs.y : mins.y) + (y % size.y);
	const int32_t oz = (z < 0 ? maxs.z : mins.z) + (z % size.z);
	core_assert_msg(containsPoint(ox, oy, oz),
			"shifted(%i:%i:%i) is outside the valid region for pos(%i:%i:%i), size(%i:%i:%i), mins(%i:%i:%i)",
			ox, oy, oz, x, y, z, size.x, size.y, size.z, getLowerX(), getLowerY(), getLowerZ());
	return math::Vec3i(ox, oy, oz);
}

void logRegion(const char *ctx, const voxel::Region& region) {
	const math::Vec3i& mins = region.getLowerCorner();
	const math::Vec3i& maxs = region.getUpperCorner();
	Log::debug("%s: region[mins(%i,%i,%i), maxs(%i,%i,%i)]", ctx, mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
}

int64_t Region::voxels() const {
	if (!isValid()) {
		return 0;
	}
	return (int64_t)getWidthInVoxels() * (int64_t)getHeightInVoxels() * (int64_t)getDepthInVoxels();
}

/**
 * @param x The 'x' component of the position to accumulate.
 * @param y The 'y' component of the position to accumulate.
 * @param z The 'z' component of the position to accumulate.
 */
void Region::accumulate(int32_t x, int32_t y, int32_t z) {
	accumulate(math::AlignedVec4i(x, y, z, 0));
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

	_mins = (math::min)(_mins, reg._mins);
	_maxs = (math::max)(_maxs, reg._maxs);
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
	_mins = (math::max)(_mins, other._mins);
	_maxs = (math::min)(_maxs, other._maxs);
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
void Region::grow(const math::Vec3i& amount) {
	const math::AlignedVec4i a(amount.x, amount.y, amount.z, 0);
	_mins -= a;
	_maxs += a;
	update();
}

math::Vec3i Region::getCenter() const {
	return _center;
}

math::Vec3i Region::getLowerCenter() const {
	return {_center.x, _mins.y, _center.z};
}

math::Vec3f Region::calcCenterf() const {
	return math::AlignedVec4f(_mins) + math::AlignedVec4f(_voxels) / 2.0f;
}

math::Vec3f Region::calcCellCenterf() const {
	return math::AlignedVec4f(_mins) + math::AlignedVec4f(_width) / 2.0f;
}

/**
 * @return The position of the lower corner.
 */
const math::Vec3i& Region::getLowerCorner() const {
	return *(const math::Vec3i*)&_mins.x;
}

/**
 * @return The position of the upper corner.
 */
const math::Vec3i& Region::getUpperCorner() const {
	return *(const math::Vec3i*)&_maxs.x;
}

math::Vec3f Region::getLowerCornerf() const {
	return math::Vec3f((float)_mins.x, (float)_mins.y, (float)_mins.z);
}

math::Vec3f Region::getUpperCornerf() const {
	return math::Vec3f((float)_maxs.x, (float)_maxs.y, (float)_maxs.z);
}

/**
 * @return The dimensions of the region measured in voxels.
 * @sa getDimensionsInCells()
 */
const math::Vec3i& Region::getDimensionsInVoxels() const {
	return *(const math::Vec3i*)&_voxels.x;
}

/**
 * @return The dimensions of the region measured in cells.
 * @sa getDimensionsInVoxels()
 */
const math::Vec3i& Region::getDimensionsInCells() const {
	return *(const math::Vec3i*)&_width.x;
}

/**
 * @param mins The new position of the lower corner.
 */
void Region::setLowerCorner(const math::Vec3i& mins) {
	_mins.x = mins.x;
	_mins.y = mins.y;
	_mins.z = mins.z;
	update();
}

/**
 * @param maxs The new position of the upper corner.
 */
void Region::setUpperCorner(const math::Vec3i& maxs) {
	_maxs.x = maxs.x;
	_maxs.y = maxs.y;
	_maxs.z = maxs.z;
	update();
}

/**
 * @param pos The position to accumulate.
 */
void Region::accumulate(const math::Vec3i& pos) {
	accumulate({pos.x, pos.y, pos.z, 0});
}

/**
 * @param pos The position to accumulate.
 */
 void Region::accumulate(const math::AlignedVec4i& pos) {
	_mins = (math::min)(_mins, pos);
	_maxs = (math::max)(_maxs, pos);
	update();
}

/**
 * @see math::transform()
 */
Region Region::rotate(const math::Mat4f &mat, const math::Vec3f &pivot) const {
	const math::Vec4f mins((float)_mins.x - 0.5f - pivot.x, (float)_mins.y - 0.5f - pivot.y, (float)_mins.z - 0.5f - pivot.z, 1.0f);
	const math::Vec4f maxs((float)_maxs.x + 0.5f - pivot.x, (float)_maxs.y + 0.5f - pivot.y, (float)_maxs.z + 0.5f - pivot.z, 1.0f);
	const math::Vec4f vertices[]{
		mins,
		math::Vec4f(maxs.x, mins.y, mins.z, 1.0f),
		math::Vec4f(mins.x, maxs.y, mins.z, 1.0f),
		math::Vec4f(maxs.x, maxs.y, mins.z, 1.0f),
		math::Vec4f(mins.x, mins.y, maxs.z, 1.0f),
		math::Vec4f(maxs.x, mins.y, maxs.z, 1.0f),
		math::Vec4f(mins.x, maxs.y, maxs.z, 1.0f),
		maxs
	};
	math::AlignedVec4i newMins(INT_MAX);
	math::AlignedVec4i newMaxs(INT_MIN);
	for (int i = 0; i < 8; ++i) {
		const math::Vec3f target(mat * vertices[i]);
		const math::AlignedVec4i corrected(math::round(target + 0.5f + pivot), 0);
		newMins = math::min(corrected, newMins);
		newMaxs = math::max(corrected, newMaxs);
	}
	return {newMins.x, newMins.y, newMins.z, newMaxs.x - 1, newMaxs.y - 1, newMaxs.z - 1};
}

void Region::transformArvo(const math::Mat4f &worldMatrix, math::Vec3f &transformedMins, math::Vec3f &transformedMaxs) const {
	// see "Transforming Axis-Aligned Bounding Boxes" by James Arvo
	const math::Vec3f &mins = getLowerCornerf();
	const math::Vec3f &maxs = getUpperCornerf();

	const math::Vec3f col0(worldMatrix[0]);
	const math::Vec3f col1(worldMatrix[1]);
	const math::Vec3f col2(worldMatrix[2]);
	const math::Vec3f tra(worldMatrix[3]);

	const math::Vec3f xa = col0 * mins.x;
	const math::Vec3f xb = col0 * maxs.x;
	const math::Vec3f ya = col1 * mins.y;
	const math::Vec3f yb = col1 * maxs.y;
	const math::Vec3f za = col2 * mins.z;
	const math::Vec3f zb = col2 * maxs.z;

	transformedMins = math::min(xa, xb) + math::min(ya, yb) + math::min(za, zb) + tra;
	transformedMaxs = math::max(xa, xb) + math::max(ya, yb) + math::max(za, zb) + tra;
}

Region Region::transform(const math::Mat4f &mat) const {
	const math::Vec3f &rmins = getLowerCornerf();
	const math::Vec3f &rmaxs = getUpperCornerf();
	const math::Vec3f transformedRMaxs = mat * math::Vec4f(rmaxs, 1.0f);
	const math::Vec3f transformedRMins = mat * math::Vec4f(rmins, 1.0f);
	return Region(
		math::Vec3i(math::floor(math::min(transformedRMins.x, transformedRMaxs.x)),
				   math::floor(math::min(transformedRMins.y, transformedRMaxs.y)),
				   math::floor(math::min(transformedRMins.z, transformedRMaxs.z))),
		math::Vec3i(math::ceil(math::max(transformedRMins.x, transformedRMaxs.x)),
				   math::ceil(math::max(transformedRMins.y, transformedRMaxs.y)),
				   math::ceil(math::max(transformedRMins.z, transformedRMaxs.z))));
}

/**
 * Constructs a Region and sets the lower and upper corners to the specified values.
 * @param mins The desired lower corner of the Region.
 * @param maxs The desired upper corner of the Region.
 */
Region::Region(const math::Vec3i& mins, const math::Vec3i& maxs) :
		Region(mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z) {
}

Region& Region::operator+=(const math::Vec3i& amount) {
	shift(amount);
	return *this;
}

Region operator+(const Region& region, const math::Vec3i& amount) {
	Region copy(region);
	copy.shift(amount);
	return copy;
}

/**
 * The test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 */
bool Region::containsPoint(const math::Vec3f& pos) const {
	return containsPoint(pos.x, pos.y, pos.z);
}

/**
 * The test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 */
bool Region::containsPoint(const math::Vec3i& pos) const {
	return containsPoint(pos.x, pos.y, pos.z);
}

bool Region::containsPoint(const math::AlignedVec4i &pos) const {
	return (pos.x <= _maxs.x) && (pos.y <= _maxs.y) && (pos.z <= _maxs.z) && (pos.x >= _mins.x) && (pos.y >= _mins.y) &&
		   (pos.z >= _mins.z);}

/**
 * @param amount The amount to move the Region by.
 */
void Region::shift(const math::Vec3i& amount) {
	shiftLowerCorner(amount);
	shiftUpperCorner(amount);
}

bool Region::isOnBorder(const math::Vec3i &pos) const {
	return math::any(math::equal(pos, getUpperCorner())) || math::any(math::equal(pos, getLowerCorner()));
}

/**
 * @param amount The amount to move the lower corner by.
 */
void Region::shiftLowerCorner(const math::Vec3i& amount) {
	shiftLowerCorner(amount.x, amount.y, amount.z);
}

/**
 * @param amount The amount to move the upper corner by.
 */
void Region::shiftUpperCorner(const math::Vec3i& amount) {
	shiftUpperCorner(amount.x, amount.y, amount.z);
}

/**
 * The amount can be specified separately for each direction. Negative shrinkage
 * is possible but you should prefer the grow() function for clarity.
 * @param amount The amount to shrink by (one component for each direction).
 */
void Region::shrink(const math::Vec3i& amount) {
	shrink(amount.x, amount.y, amount.z);
}

}
