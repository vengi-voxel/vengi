/**
 * @file
 */

#include "Shape.h"
#include "core/Assert.h"

namespace stock {

bool ContainerShape::addRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
	if (x + width >= ContainerMaxWidth) {
		return false;
	}
	if (y + height >= ContainerMaxHeight) {
		return false;
	}
	const ContainerShapeType row = (((ItemShapeType)1 << width) - 1) << x;
	for (height += y; y < height; ++y) {
		_containerShape[y] |= row;
	}
	return true;
}

bool ContainerShape::isFree(uint8_t x, uint8_t y) const {
	if (!isInShape(x, y)) {
		return false;
	}

	const ContainerShapeType itemRow = ((ContainerShapeType)1) & ItemRowLength;
	const ContainerShapeType itemShapeTranslated = itemRow << x;
	if ((itemShapeTranslated & _itemShape[y]) != (ContainerShapeType)0) {
		return false;
	}

	return true;

}

bool ContainerShape::isFree(const ItemShape& itemShape, uint8_t x, uint8_t y) const {
	if (!isInShape(x, y)) {
		return false;
	}

	const ItemShapeType shape = static_cast<ItemShapeType>(itemShape);
	const uint8_t height = itemShape.height();
	for (uint8_t row = 0; row < height; ++row) {
		/* Result has to be limited to ContainerBitsPerRow - theoretically the ItemShapeType
		 * can be smaller than the ContainerShapeType - so use the potentially larger one
		 * here. */
		const ContainerShapeType itemRow = (shape >> (row * ItemMaxWidth)) & ItemRowLength;
		if (itemRow == (ContainerShapeType)0) {
			continue;
		}
		const ContainerShapeType itemShapeTranslated = itemRow << x;

		/* Check if shifting back is out of bounds - that means the item shape is out
		 * of bounds at the given coordinates. */
		if (itemShapeTranslated >> x != itemRow) {
			return false;
		}

		if (y + row >= ContainerMaxHeight && itemRow > 0) {
			return false;
		}

		if ((itemShapeTranslated & ~_containerShape[y + row]) != (ContainerShapeType)0) {
			return false;
		}

		if ((itemShapeTranslated & _itemShape[y + row]) != (ContainerShapeType)0) {
			return false;
		}
	}

	return true;
}

int ContainerShape::free() const {
	int bitCounter = 0;
	for (int row = 0; row < ContainerMaxHeight; ++row) {
		for (uint8_t i = 0; i < ContainerBitsPerRow; ++i) {
			if ((_containerShape[row] & ((ItemShapeType)1 << i)) != 0) {
				if ((_itemShape[row] & ((ItemShapeType)1 << i)) == 0) {
					++bitCounter;
				}
			}
		}
	}
	return bitCounter;
}

int ContainerShape::size() const {
	int bitCounter = 0;
	for (int row = 0; row < ContainerMaxHeight; ++row) {
		for (uint8_t i = 0; i < ContainerBitsPerRow; ++i) {
			if ((_containerShape[row] & ((ItemShapeType)1 << i)) != 0) {
				++bitCounter;
			}
		}
	}
	return bitCounter;
}

void ContainerShape::addShape(ItemShapeType shape, uint8_t x, uint8_t y) {
	core_assert(isInShape(x, y));
	core_assert_always(y < ContainerMaxHeight && y < ContainerMaxWidth);
	for (uint8_t row = 0; row < ItemMaxHeight && y + row < ContainerMaxHeight; ++row) {
		_itemShape[y + row] |= ((shape >> row * ItemMaxWidth) & ItemRowLength) << x;
	}
}

void ContainerShape::removeShape(ItemShapeType shape, uint8_t x, uint8_t y) {
	core_assert(isInShape(x, y));
	core_assert_always(y < ContainerMaxHeight && y < ContainerMaxWidth);
	for (uint8_t row = 0; row < ItemMaxHeight && y + row < ContainerMaxHeight; ++row) {
		_itemShape[y + row] &= ~((shape >> row * ItemMaxWidth) & ItemRowLength) << x;
	}
}

ItemShapeType ItemShape::addRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
	for (height += y; y < height; ++y) {
		_shape |= (((ItemShapeType)1 << width) - 1) << x << (y * ItemMaxWidth);
	}
	return _shape;
}

ItemShapeType ItemShape::set(uint8_t x, uint8_t y) {
	core_assert_always(y < ItemMaxHeight && x < ItemMaxWidth);
	_shape |= (ItemShapeType)1 << (y * ItemMaxWidth + x);
	return _shape;
}

int ItemShape::size() const {
	int bitCounter = 0;
	for (uint8_t i = 0; i < ItemBits; ++i) {
		if ((_shape & ((ItemShapeType)1 << i)) != 0) {
			++bitCounter;
		}
	}
	return bitCounter;
}

static inline constexpr uint64_t calcItemShapeHeightMask() {
	ItemShapeType heightMask = 0;
	for (int i = 0; i < ItemMaxWidth; ++i) {
		heightMask |= (ItemShapeType)1 << (i * CHAR_BIT);
	}
	return heightMask;
}

int ItemShape::height() const {
	int i;
	for (i = ItemMaxWidth - 1; i >= 0; --i) {
		if (_shape & (calcItemShapeHeightMask() << i)) {
			break;
		}
	}
	return i + 1;
}

int ItemShape::width() const {
	int i;
	for (i = ItemMaxHeight - 1; i >= 0; --i) {
		if (_shape & (ItemRowLength << (i * ItemMaxWidth))) {
			break;
		}
	}
	return i + 1;
}

}
