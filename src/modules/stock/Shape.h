/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/GLM.h"
#include <limits.h>

namespace stock {

typedef uint64_t ContainerShapeType;
typedef uint64_t ItemShapeType;

static constexpr int ContainerBitsPerRow = sizeof(ContainerShapeType) * CHAR_BIT;
static constexpr uint8_t ContainerMaxHeight = 32;
static constexpr uint8_t ContainerMaxWidth = ContainerBitsPerRow;
static_assert(ContainerMaxWidth <= ContainerBitsPerRow, "max width exceeds the data type width");

static constexpr int ItemBits = sizeof(ItemShapeType) * CHAR_BIT;
static constexpr uint8_t ItemMaxHeight = 8;
static constexpr uint8_t ItemMaxWidth = 8;
static constexpr ItemShapeType ItemRowLength = 0xff; /* ItemMaxWidth bits */
static_assert(ItemMaxWidth * ItemMaxHeight <= ItemBits, "width and height doesn't fit into the shapetype");

/**
 * @ingroup Stock
 */
class ItemShape {
private:
	ItemShapeType _shape = 0;
public:
	/**
	 * @brief Allows you to define an ItemShapeType.
	 * @note Call this multiple times to define non rectangular shapes.
	 */
	ItemShapeType addRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

	ItemShapeType set(uint8_t x, uint8_t y);

	/**
	 * @brief Returns true if the given coordinates are part of the shape definition, false otherwise
	 */
	bool isInShape(uint8_t x, uint8_t y) const;

	/**
	 * @brief Calculate the amount of valid fields for this shape.
	 */
	int size() const;

	int height() const;

	int width() const;

	void clear();

	operator ItemShapeType() const;
};

inline void ItemShape::clear() {
	_shape = (ItemShapeType)0;
}

inline bool ItemShape::isInShape(uint8_t x, uint8_t y) const {
	core_assert_always(y < ItemMaxHeight && x < ItemMaxWidth);
	return _shape & ((ItemShapeType)1 << (y * ItemMaxWidth + x));
}

inline ItemShape::operator ItemShapeType() const {
	return _shape;
}

/**
 * @ingroup Stock
 */
class ContainerShape {
private:
	ContainerShapeType _containerShape[ContainerMaxHeight] {};
	ContainerShapeType _itemShape[ContainerMaxHeight] {};
public:
	constexpr ContainerShape() {}

	/**
	 * @brief Define the shape by specifying the rect of a valid area.
	 * @note You can call this multiple times if you would like to have a non
	 * rectangular shape.
	 */
	bool addRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

	/**
	 * @brief Define the shape by adding an ItemShapeType at a particular position.
	 */
	void addShape(ItemShapeType shape, uint8_t x, uint8_t y);

	void removeShape(ItemShapeType shape, uint8_t x, uint8_t y);

	/**
	 * @brief Returns true if the given coordinates are part of the shape definition, false otherwise
	 */
	bool isInShape(uint8_t x, uint8_t y) const;

	bool isFree(const ItemShape& shape, uint8_t x, uint8_t y) const;

	bool isFree(uint8_t x, uint8_t y) const;

	int free() const;

	int size() const;
};

inline bool ContainerShape::isInShape(uint8_t x, uint8_t y) const {
	core_assert_always(y < ContainerMaxHeight && x < ContainerMaxWidth);
	return (_containerShape[y] & ((ContainerShapeType)1 << x)) != 0;
}

}
