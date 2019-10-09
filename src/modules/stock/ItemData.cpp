/**
 * @file
 */

#include "ItemData.h"

namespace stock {

ItemData::ItemData(ItemId id, ItemType type) :
	_id(id), _type(type) {
}

void ItemData::setName(const char *name) {
	_name = name;
}

void ItemData::setSize(uint8_t width, uint8_t height) {
	_shape.clear();
	_shape.addRect(0u, 0u, width, height);
}

const char *ItemData::name() const {
	if (_name.empty()) {
		return nullptr;
	}
	return _name.c_str();
}

}
