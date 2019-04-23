/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "math/AABB.h"
#include "math/Rect.h"
#include "math/Random.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace voxel {

/** Represents a part of a Volume.
 *
 *  Many operations in PolyVox are constrained to only part of a volume. For example, when running the surface extractors
 *  it is unlikely that you will want to run it on the whole volume at once, as this will give a very large mesh which may
 *  be too much to render. Instead you will probably want to run a surface extractor a number of times on different parts
 *  of the volume, there by giving a number of meshes which can be culled and rendered separately.
 *
 *  The Region class is used to define these parts (regions) of the volume. Essentially it consists of an upper and lower
 *  bound which specify the range of voxels positions considered to be part of the region. Note that these bounds are
 *  <em>inclusive</em>.
 *
 *  As well as the expected set of getters and setters, this class also provide utility functions for increasing and decreasing
 *  the size of the Region, shifting the Region in 3D space, testing whether it contains a given position, enlarging it so that
 *  it does contain a given position, cropping it to another Region, and various other utility functions.
 *
 *  @note The dimensions of a region can be measured either in voxels or in cells. See the manual for more information
 *  about these definitions.
 *
 */
class Region {
public:
	/// Constructor
	Region();
	/// Constructor
	Region(const glm::ivec3& v3dLowerCorner, const glm::ivec3& v3dUpperCorner);
	/// Constructor
	Region(int32_t iLowerX, int32_t iLowerY, int32_t iLowerZ, int32_t iUpperX, int32_t iUpperY, int32_t iUpperZ);
	Region(int mins, int maxs);
	Region(const Region& region);

	/// A Region with the lower corner set as low as possible and the upper corner set as high as possible.
	static const Region MaxRegion;
	static const Region InvalidRegion;

	/// Equality Operator.
	bool operator==(const Region& rhs) const;
	/// Inequality Operator.
	bool operator!=(const Region& rhs) const;

	/// Moves the Region by the amount specified.
	Region& operator+=(const glm::ivec3& v3dAmount);

	/// Gets the 'x' position of the centre.
	int32_t getCentreX() const;
	/// Gets the 'y' position of the centre.
	int32_t getCentreY() const;
	/// Gets the 'z' position of the centre.
	int32_t getCentreZ() const;
	/// Gets the 'x' position of the lower corner.
	int32_t getLowerX() const;
	/// Gets the 'y' position of the lower corner.
	int32_t getLowerY() const;
	/// Gets the 'z' position of the lower corner.
	int32_t getLowerZ() const;
	/// Gets the 'x' position of the upper corner.
	int32_t getUpperX() const;
	/// Gets the 'y' position of the upper corner.
	int32_t getUpperY() const;
	/// Gets the 'z' position of the upper corner.
	int32_t getUpperZ() const;

	/// Gets the centre of the region
	glm::ivec3 getCentre() const;
	/// Gets the position of the lower corner.
	glm::ivec3 getLowerCorner() const;
	/// Gets the position of the upper corner.
	glm::ivec3 getUpperCorner() const;

	glm::ivec3 getRandomPosition(math::Random& random) const;

	/// Gets the width of the region measured in voxels.
	int32_t getWidthInVoxels() const;
	/// Gets the height of the region measured in voxels.
	int32_t getHeightInVoxels() const;
	/// Gets the depth of the region measured in voxels.
	int32_t getDepthInVoxels() const;
	/// Gets the dimensions of the region measured in voxels.
	glm::ivec3 getDimensionsInVoxels() const;

	/// Gets the width of the region measured in cells.
	int32_t getWidthInCells() const;
	/// Gets the height of the region measured in cells.
	int32_t getHeightInCells() const;
	/// Gets the depth of the region measured in cells.
	int32_t getDepthInCells() const;
	/// Gets the dimensions of the region measured in cells.
	glm::ivec3 getDimensionsInCells() const;

	/// Sets the 'x' position of the lower corner.
	void setLowerX(int32_t iX);
	/// Sets the 'y' position of the lower corner.
	void setLowerY(int32_t iY);
	/// Sets the 'z' position of the lower corner.
	void setLowerZ(int32_t iZ);
	/// Sets the 'x' position of the upper corner.
	void setUpperX(int32_t iX);
	/// Sets the 'y' position of the upper corner.
	void setUpperY(int32_t iY);
	/// Sets the 'z' position of the upper corner.
	void setUpperZ(int32_t iZ);

	glm::ivec3 moveInto(int32_t x, int32_t y, int32_t z) const;

	/// Sets the position of the lower corner.
	void setLowerCorner(const glm::ivec3& v3dLowerCorner);
	/// Sets the position of the upper corner.
	void setUpperCorner(const glm::ivec3& v3dUpperCorner);

	/// Tests whether the given point is contained in this Region.
	bool containsPoint(float fX, float fY, float fZ, float boundary = 0.0f) const;
	/// Tests whether the given point is contained in this Region.
	bool containsPoint(const glm::vec3& pos, float boundary = 0.0f) const;
	/// Tests whether the given point is contained in this Region.
	bool containsPoint(int32_t iX, int32_t iY, int32_t iZ, uint8_t boundary = 0) const;
	/// Tests whether the given point is contained in this Region.
	bool containsPoint(const glm::ivec3& pos, uint8_t boundary = 0) const;
	/// Tests whether the given position is contained in the 'x' range of this Region.
	bool containsPointInX(float pos, float boundary = 0.0f) const;
	/// Tests whether the given position is contained in the 'x' range of this Region.
	bool containsPointInX(int32_t pos, uint8_t boundary = 0) const;
	/// Tests whether the given position is contained in the 'y' range of this Region.
	bool containsPointInY(float pos, float boundary = 0.0f) const;
	/// Tests whether the given position is contained in the 'y' range of this Region.
	bool containsPointInY(int32_t pos, uint8_t boundary = 0) const;
	/// Tests whether the given position is contained in the 'z' range of this Region.
	bool containsPointInZ(float pos, float boundary = 0.0f) const;
	/// Tests whether the given position is contained in the 'z' range of this Region.
	bool containsPointInZ(int32_t pos, uint8_t boundary = 0) const;

	/// Tests whether the given Region is contained in this Region.
	bool containsRegion(const Region& reg, uint8_t boundary = 0) const;

	/// Enlarges the Region so that it contains the specified position.
	void accumulate(int32_t iX, int32_t iY, int32_t iZ);
	/// Enlarges the Region so that it contains the specified position.
	void accumulate(const glm::ivec3& v3dPos);
	/// Enlarges the Region so that it contains the specified Region.
	void accumulate(const Region& reg);

	/// Crops the extents of this Region according to another Region.
	void cropTo(const Region& other);

	/// Grows this region by the amount specified.
	void grow(int32_t iAmount);
	/// Grows this region by the amounts specified.
	void grow(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ);
	/// Grows this region by the amounts specified.
	void grow(const glm::ivec3& v3dAmount);

	/// Tests whether all components of the upper corner are at least
	/// as great as the corresponding components of the lower corner.
	bool isValid() const;

	/// Moves the Region by the amount specified.
	void shift(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ);
	/// Moves the Region by the amount specified.
	void shift(const glm::ivec3& v3dAmount);
	/// Moves the lower corner of the Region by the amount specified.
	void shiftLowerCorner(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ);
	/// Moves the lower corner of the Region by the amount specified.
	void shiftLowerCorner(const glm::ivec3& v3dAmount);
	/// Moves the upper corner of the Region by the amount specified.
	void shiftUpperCorner(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ);
	/// Moves the upper corner of the Region by the amount specified.
	void shiftUpperCorner(const glm::ivec3& v3dAmount);

	/// Shrinks this region by the amount specified.
	void shrink(int32_t iAmount);
	/// Shrinks this region by the amounts specified.
	void shrink(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ);
	/// Shrinks this region by the amounts specified.
	void shrink(const glm::ivec3& v3dAmount);

	math::AABB<int> aabb() const;
	/// Returns a rect of the x and z area this region covers
	math::Rect<int> rect(int border = 0) const;
	math::Rect<float> rectf(int border = 0) const;

private:
	int32_t m_iLowerX;
	int32_t m_iLowerY;
	int32_t m_iLowerZ;
	int32_t m_iUpperX;
	int32_t m_iUpperY;
	int32_t m_iUpperZ;
};

inline math::AABB<int> Region::aabb() const {
	return math::AABB<int>(getLowerCorner(), getUpperCorner() + 1);
}

inline math::Rect<int> Region::rect(int border) const {
	core_assert(getUpperX() - getLowerX() > 2 * border);
	core_assert(getUpperZ() - getLowerZ() > 2 * border);
	return math::Rect<int>(getLowerX() + border, getLowerZ() + border, getUpperX() - border, getUpperZ() - border);
}

inline math::Rect<float> Region::rectf(int border) const {
	core_assert(getUpperX() - getLowerX() > 2 * border);
	core_assert(getUpperZ() - getLowerZ() > 2 * border);
	return math::Rect<float>(getLowerX() + border, getLowerZ() + border, getUpperX() - border, getUpperZ() - border);
}

/**
 * @return The 'x' position of the centre.
 */
inline int32_t Region::getCentreX() const {
	return (m_iLowerX + m_iUpperX) / 2;
}

/**
 * @return The 'y' position of the centre.
 */
inline int32_t Region::getCentreY() const {
	return (m_iLowerY + m_iUpperY) / 2;
}

/**
 * @return The 'z' position of the centre.
 */
inline int32_t Region::getCentreZ() const {
	return (m_iLowerZ + m_iUpperZ) / 2;
}

/**
 * @return The 'x' position of the lower corner.
 */
inline int32_t Region::getLowerX() const {
	return m_iLowerX;
}

/**
 * @return The 'y' position of the lower corner.
 */
inline int32_t Region::getLowerY() const {
	return m_iLowerY;
}

/**
 * @return The 'z' position of the lower corner.
 */
inline int32_t Region::getLowerZ() const {
	return m_iLowerZ;
}

/**
 * @return The 'x' position of the upper corner.
 */
inline int32_t Region::getUpperX() const {
	return m_iUpperX;
}

/**
 * @return The 'y' position of the upper corner.
 */
inline int32_t Region::getUpperY() const {
	return m_iUpperY;
}

/**
 * @return The 'z' position of the upper corner.
 */
inline int32_t Region::getUpperZ() const {
	return m_iUpperZ;
}

/**
 * @return The position of the lower corner.
 */
inline glm::ivec3 Region::getCentre() const {
	return glm::ivec3(getCentreX(), getCentreY(), getCentreZ());
}

/**
 * @return The position of the lower corner.
 */
inline glm::ivec3 Region::getLowerCorner() const {
	return glm::ivec3(m_iLowerX, m_iLowerY, m_iLowerZ);
}

/**
 * @return The position of the upper corner.
 */
inline glm::ivec3 Region::getUpperCorner() const {
	return glm::ivec3(m_iUpperX, m_iUpperY, m_iUpperZ);
}

inline glm::ivec3 Region::getRandomPosition(math::Random& random) const {
	const int x = random.random(m_iLowerX, m_iUpperX);
	const int y = random.random(m_iLowerY, m_iUpperY);
	const int z = random.random(m_iLowerZ, m_iUpperZ);
	return glm::ivec3(x, y, z);
}

/**
 * @return The width of the region measured in voxels.
 * @sa getWidthInCells()
 */
inline int32_t Region::getWidthInVoxels() const {
	return getWidthInCells() + 1;
}

/**
 * @return The height of the region measured in voxels.
 * @sa getHeightInCells()
 */
inline int32_t Region::getHeightInVoxels() const {
	return getHeightInCells() + 1;
}

/**
 * @return The depth of the region measured in voxels.
 * @sa getDepthInCells()
 */
inline int32_t Region::getDepthInVoxels() const {
	return getDepthInCells() + 1;
}

/**
 * @return The dimensions of the region measured in voxels.
 * @sa getDimensionsInCells()
 */
inline glm::ivec3 Region::getDimensionsInVoxels() const {
	return getDimensionsInCells() + glm::ivec3(1, 1, 1);
}

/**
 * @return The width of the region measured in cells.
 * @sa getWidthInVoxels()
 */
inline int32_t Region::getWidthInCells() const {
	return m_iUpperX - m_iLowerX;
}

/**
 * @return The height of the region measured in cells.
 * @sa getHeightInVoxels()
 */
inline int32_t Region::getHeightInCells() const {
	return m_iUpperY - m_iLowerY;
}

/**
 * @return The depth of the region measured in cells.
 * @sa getDepthInVoxels()
 */
inline int32_t Region::getDepthInCells() const {
	return m_iUpperZ - m_iLowerZ;
}

/**
 * @return The dimensions of the region measured in cells.
 * @sa getDimensionsInVoxels()
 */
inline glm::ivec3 Region::getDimensionsInCells() const {
	return glm::ivec3(getWidthInCells(), getHeightInCells(), getDepthInCells());
}

/**
 * @param iX The new 'x' position of the lower corner.
 */
inline void Region::setLowerX(int32_t iX) {
	m_iLowerX = iX;
}

/**
 * @param iY The new 'y' position of the lower corner.
 */
inline void Region::setLowerY(int32_t iY) {
	m_iLowerY = iY;
}

/**
 * @param iZ The new 'z' position of the lower corner.
 */
inline void Region::setLowerZ(int32_t iZ) {
	m_iLowerZ = iZ;
}

/**
 * @param iX The new 'x' position of the upper corner.
 */
inline void Region::setUpperX(int32_t iX) {
	m_iUpperX = iX;
}

/**
 * @param iY The new 'y' position of the upper corner.
 */
inline void Region::setUpperY(int32_t iY) {
	m_iUpperY = iY;
}

/**
 * @param iZ The new 'z' position of the upper corner.
 */
inline void Region::setUpperZ(int32_t iZ) {
	m_iUpperZ = iZ;
}

/**
 * @param v3dLowerCorner The new position of the lower corner.
 */
inline void Region::setLowerCorner(const glm::ivec3& v3dLowerCorner) {
	m_iLowerX = v3dLowerCorner.x;
	m_iLowerY = v3dLowerCorner.y;
	m_iLowerZ = v3dLowerCorner.z;
}

/**
 * @param v3dUpperCorner The new position of the upper corner.
 */
inline void Region::setUpperCorner(const glm::ivec3& v3dUpperCorner) {
	m_iUpperX = v3dUpperCorner.x;
	m_iUpperY = v3dUpperCorner.y;
	m_iUpperZ = v3dUpperCorner.z;
}

/**
 * @param iX The 'x' component of the position to accumulate.
 * @param iY The 'y' component of the position to accumulate.
 * @param iZ The 'z' component of the position to accumulate.
 */
inline void Region::accumulate(int32_t iX, int32_t iY, int32_t iZ) {
	m_iLowerX = glm::min(m_iLowerX, iX);
	m_iLowerY = glm::min(m_iLowerY, iY);
	m_iLowerZ = glm::min(m_iLowerZ, iZ);
	m_iUpperX = glm::max(m_iUpperX, iX);
	m_iUpperY = glm::max(m_iUpperY, iY);
	m_iUpperZ = glm::max(m_iUpperZ, iZ);
}

/**
 * @param v3dPos The position to accumulate.
 */
inline void Region::accumulate(const glm::ivec3& v3dPos) {
	accumulate(v3dPos.x, v3dPos.y, v3dPos.z);
}

/**
 * Note that this is not the same as computing the union of two Regions (as the result of
 * such a union may not be a shape which can be exactly represented by a Region). Instead,
 * the result is simply big enough to contain both this Region and the one passed as a parameter.
 * @param reg The Region to accumulate. This must be valid as defined by the isValid() function.
 * @sa isValid()
 */
inline void Region::accumulate(const Region& reg) {
	if (!reg.isValid()) {
		// The result of accumulating an invalid region is not defined.
		core_assert_msg(false, "You cannot accumulate an invalid region.");
	}

	m_iLowerX = glm::min(m_iLowerX, reg.getLowerX());
	m_iLowerY = glm::min(m_iLowerY, reg.getLowerY());
	m_iLowerZ = glm::min(m_iLowerZ, reg.getLowerZ());
	m_iUpperX = glm::max(m_iUpperX, reg.getUpperX());
	m_iUpperY = glm::max(m_iUpperY, reg.getUpperY());
	m_iUpperZ = glm::max(m_iUpperZ, reg.getUpperZ());
}

inline Region::Region(const Region& region) :
		m_iLowerX(region.m_iLowerX), m_iLowerY(region.m_iLowerY), m_iLowerZ(
				region.m_iLowerZ), m_iUpperX(region.m_iUpperX), m_iUpperY(
				region.m_iUpperY), m_iUpperZ(region.m_iUpperZ) {
}

inline Region::Region(int mins, int maxs) :
		Region(mins, mins, mins, maxs, maxs, maxs) {
}

/**
 * Constructs a Region and clears all extents to zero.
 */
inline Region::Region() :
		Region(0, 0, 0, 0, 0, 0) {
}

/**
 * Constructs a Region and sets the lower and upper corners to the specified values.
 * @param v3dLowerCorner The desired lower corner of the Region.
 * @param v3dUpperCorner The desired upper corner of the Region.
 */
inline Region::Region(const glm::ivec3& v3dLowerCorner, const glm::ivec3& v3dUpperCorner) :
		Region(v3dLowerCorner.x, v3dLowerCorner.y, v3dLowerCorner.z, v3dUpperCorner.x, v3dUpperCorner.y, v3dUpperCorner.z) {
}

/**
 * Constructs a Region and sets the extents to the specified values.
 * @param iLowerX The desired lower 'x' extent of the Region.
 * @param iLowerY The desired lower 'y' extent of the Region.
 * @param iLowerZ The desired lower 'z' extent of the Region.
 * @param iUpperX The desired upper 'x' extent of the Region.
 * @param iUpperY The desired upper 'y' extent of the Region.
 * @param iUpperZ The desired upper 'z' extent of the Region.
 */
inline Region::Region(int32_t iLowerX, int32_t iLowerY, int32_t iLowerZ, int32_t iUpperX, int32_t iUpperY, int32_t iUpperZ) :
		m_iLowerX(iLowerX), m_iLowerY(iLowerY), m_iLowerZ(iLowerZ), m_iUpperX(iUpperX), m_iUpperY(iUpperY), m_iUpperZ(iUpperZ) {
}

/**
 * Two regions are considered equal if all their extents match.
 * @param rhs The Region to compare to.
 * @return true if the Regions match.
 * @sa operator!=
 */
inline bool Region::operator==(const Region& rhs) const {
	return ((m_iLowerX == rhs.m_iLowerX) && (m_iLowerY == rhs.m_iLowerY) && (m_iLowerZ == rhs.m_iLowerZ) && (m_iUpperX == rhs.m_iUpperX) && (m_iUpperY == rhs.m_iUpperY)
			&& (m_iUpperZ == rhs.m_iUpperZ));
}

/**
 * Two regions are considered different if any of their extents differ.
 * @param rhs The Region to compare to.
 * @return true if the Regions are different.
 * @sa operator==
 */
inline bool Region::operator!=(const Region& rhs) const {
	return !(*this == rhs);
}

inline Region& Region::operator+=(const glm::ivec3& v3dAmount) {
	shift(v3dAmount);
	return *this;
}

inline Region operator+(const Region& region, const glm::ivec3& v3dAmount) {
	Region copy(region);
	copy.shift(v3dAmount);
	return copy;
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in all directions. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param fX The 'x' position of the point to test.
 * @param fY The 'y' position of the point to test.
 * @param fZ The 'z' position of the point to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsPoint(float fX, float fY, float fZ, float boundary) const {
	return (fX <= m_iUpperX - boundary) && (fY <= m_iUpperY - boundary) && (fZ <= m_iUpperZ - boundary) && (fX >= m_iLowerX + boundary) && (fY >= m_iLowerY + boundary)
			&& (fZ >= m_iLowerZ + boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in all directions. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsPoint(const glm::vec3& pos, float boundary) const {
	return containsPoint(pos.x, pos.y, pos.z, boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in all directions. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param iX The 'x' position of the point to test.
 * @param iY The 'y' position of the point to test.
 * @param iZ The 'z' position of the point to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsPoint(int32_t iX, int32_t iY, int32_t iZ, uint8_t boundary) const {
	return (iX <= m_iUpperX - boundary) && (iY <= m_iUpperY - boundary) && (iZ <= m_iUpperZ - boundary) && (iX >= m_iLowerX + boundary) && (iY >= m_iLowerY + boundary)
			&& (iZ >= m_iLowerZ + boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in all directions. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsPoint(const glm::ivec3& pos, uint8_t boundary) const {
	return containsPoint(pos.x, pos.y, pos.z, boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in the 'x' direction. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsPointInX(float pos, float boundary) const {
	return (pos <= m_iUpperX - boundary) && (pos >= m_iLowerX + boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in the 'x' direction. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsPointInX(int32_t pos, uint8_t boundary) const {
	return (pos <= m_iUpperX - boundary) && (pos >= m_iLowerX + boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in the 'y' direction. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsPointInY(float pos, float boundary) const {
	return (pos <= m_iUpperY - boundary) && (pos >= m_iLowerY + boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in the 'y' direction. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsPointInY(int32_t pos, uint8_t boundary) const {
	return (pos <= m_iUpperY - boundary) && (pos >= m_iLowerY + boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in the 'z' direction. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsPointInZ(float pos, float boundary) const {
	return (pos <= m_iUpperZ - boundary) && (pos >= m_iLowerZ + boundary);
}

/**
 * The boundary value can be used to ensure a position is only considered to be inside
 * the Region if it is that far in in the 'z' direction. Also, the test is inclusive such
 * that positions lying exactly on the edge of the Region are considered to be inside it.
 * @param pos The position to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsPointInZ(int32_t pos, uint8_t boundary) const {
	return (pos <= m_iUpperZ - boundary) && (pos >= m_iLowerZ + boundary);
}

/**
 * The boundary value can be used to ensure a region is only considered to be inside
 * another Region if it is that far in in all directions. Also, the test is inclusive such
 * that a region is considered to be inside of itself.
 * @param reg The region to test.
 * @param boundary The desired boundary value.
 */
inline bool Region::containsRegion(const Region& reg, uint8_t boundary) const {
	return (reg.m_iUpperX <= m_iUpperX - boundary) && (reg.m_iUpperY <= m_iUpperY - boundary) && (reg.m_iUpperZ <= m_iUpperZ - boundary) && (reg.m_iLowerX >= m_iLowerX + boundary)
			&& (reg.m_iLowerY >= m_iLowerY + boundary) && (reg.m_iLowerZ >= m_iLowerZ + boundary);
}

/**
 * After calling this functions, the extents of this Region are given by the intersection
 * of this Region and the one it was cropped to.
 * @param other The Region to crop to.
 */
inline void Region::cropTo(const Region& other) {
	m_iLowerX = glm::max(m_iLowerX, other.m_iLowerX);
	m_iLowerY = glm::max(m_iLowerY, other.m_iLowerY);
	m_iLowerZ = glm::max(m_iLowerZ, other.m_iLowerZ);
	m_iUpperX = glm::min(m_iUpperX, other.m_iUpperX);
	m_iUpperY = glm::min(m_iUpperY, other.m_iUpperY);
	m_iUpperZ = glm::min(m_iUpperZ, other.m_iUpperZ);
}

/**
 * The same amount of growth is applied in all directions. Negative growth
 * is possible but you should prefer the shrink() function for clarity.
 * @param iAmount The amount to grow by.
 */
inline void Region::grow(int32_t iAmount) {
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
inline void Region::grow(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ) {
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
inline void Region::grow(const glm::ivec3& v3dAmount) {
	grow(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

inline bool Region::isValid() const {
	return m_iUpperX >= m_iLowerX && m_iUpperY >= m_iLowerY && m_iUpperZ >= m_iLowerZ;
}

/**
 * @param iAmountX The amount to move the Region by in 'x'.
 * @param iAmountY The amount to move the Region by in 'y'.
 * @param iAmountZ The amount to move the Region by in 'z'.
 */
inline void Region::shift(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ) {
	shiftLowerCorner(iAmountX, iAmountY, iAmountZ);
	shiftUpperCorner(iAmountX, iAmountY, iAmountZ);
}

/**
 * @param v3dAmount The amount to move the Region by.
 */
inline void Region::shift(const glm::ivec3& v3dAmount) {
	shiftLowerCorner(v3dAmount);
	shiftUpperCorner(v3dAmount);
}

/**
 * @param iAmountX The amount to move the lower corner by in 'x'.
 * @param iAmountY The amount to move the lower corner by in 'y'.
 * @param iAmountZ The amount to move the lower corner by in 'z'.
 */
inline void Region::shiftLowerCorner(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ) {
	m_iLowerX += iAmountX;
	m_iLowerY += iAmountY;
	m_iLowerZ += iAmountZ;
}

/**
 * @param v3dAmount The amount to move the lower corner by.
 */
inline void Region::shiftLowerCorner(const glm::ivec3& v3dAmount) {
	shiftLowerCorner(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

/**
 * @param x The amount to move the upper corner by in 'x'.
 * @param y The amount to move the upper corner by in 'y'.
 * @param z The amount to move the upper corner by in 'z'.
 */
inline void Region::shiftUpperCorner(int32_t x, int32_t y, int32_t z) {
	m_iUpperX += x;
	m_iUpperY += y;
	m_iUpperZ += z;
}

/**
 * @param v3dAmount The amount to move the upper corner by.
 */
inline void Region::shiftUpperCorner(const glm::ivec3& v3dAmount) {
	shiftUpperCorner(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

/**
 * The same amount of shrinkage is applied in all directions. Negative shrinkage
 * is possible but you should prefer the grow() function for clarity.
 * @param iAmount The amount to shrink by.
 */
inline void Region::shrink(int32_t iAmount) {
	m_iLowerX += iAmount;
	m_iLowerY += iAmount;
	m_iLowerZ += iAmount;

	m_iUpperX -= iAmount;
	m_iUpperY -= iAmount;
	m_iUpperZ -= iAmount;
}

/**
 * The amount can be specified seperatly for each direction. Negative shrinkage
 * is possible but you should prefer the grow() function for clarity.
 * @param iAmountX The amount to shrink by in 'x'.
 * @param iAmountY The amount to shrink by in 'y'.
 * @param iAmountZ The amount to shrink by in 'z'.
 */
inline void Region::shrink(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ) {
	m_iLowerX += iAmountX;
	m_iLowerY += iAmountY;
	m_iLowerZ += iAmountZ;

	m_iUpperX -= iAmountX;
	m_iUpperY -= iAmountY;
	m_iUpperZ -= iAmountZ;
}

/**
 * The amount can be specified seperatly for each direction. Negative shrinkage
 * is possible but you should prefer the grow() function for clarity.
 * @param v3dAmount The amount to shrink by (one component for each direction).
 */
inline void Region::shrink(const glm::ivec3& v3dAmount) {
	shrink(v3dAmount.x, v3dAmount.y, v3dAmount.z);
}

/**
 * This function only returns true if the regions are really intersecting and not simply touching.
 */
inline bool intersects(const Region& a, const Region& b) {
	// No intersection if seperated along an axis.
	if (a.getUpperX() < b.getLowerX() || a.getLowerX() > b.getUpperX())
		return false;
	if (a.getUpperY() < b.getLowerY() || a.getLowerY() > b.getUpperY())
		return false;
	if (a.getUpperZ() < b.getLowerZ() || a.getLowerZ() > b.getUpperZ())
		return false;

	// Overlapping on all axes means Regions are intersecting.
	return true;
}

}
