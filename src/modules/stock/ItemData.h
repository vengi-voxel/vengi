/**
 * @file
 */

#pragma once

#include "Shared_generated.h"
#include "Shape.h"
#include "cooldown/CooldownType.h"
#include <unordered_map>

namespace stock {

using ItemType = network::ItemType;

/**
 * @brief Converts a string into the enum value
 * @ingroup Stock
 */
inline ItemType getItemType(const char* name) {
	return network::getEnum<ItemType>(name, network::EnumNamesItemType());
}

/**
 * @brief Converts a string into the enum value
 * @ingroup Stock
 */
inline ItemType getItemType(const core::String& name) {
	return getItemType(name.c_str());
}

using ItemId = uint32_t;

/**
 * @brief Blueprint that describes a thing that can be managed by the Stock class.
 *
 * @note This is 'static' data - meaning that if you own 100 items of the same type, they
 * share one instance of this class.
 * @ingroup Stock
 */
class ItemData {
protected:
	core::String _name;
	ItemId _id;
	ItemShape _shape;
	ItemType _type;
	std::unordered_map<core::String, core::String> _labels;
	cooldown::Type _construction = cooldown::Type::NONE;
	cooldown::Type _usage = cooldown::Type::NONE;
	cooldown::Type _regenerate = cooldown::Type::NONE;
public:
	ItemData(ItemId id, ItemType type);

	void setName(const char *name);

	void addLabel(const char *key, const char *value);

	/**
	 * @return nullptr if no such key is found
	 */
	const char *label(const char *key) const;

	void setSize(uint8_t width, uint8_t height);

	const ItemType& type() const;

	const ItemShape& shape() const;

	const char *name() const;

	ItemId id() const;

	ItemShape& shape();

	const cooldown::Type& regenerateCooldown() const;

	const cooldown::Type& usageCooldown() const;

	const cooldown::Type& constructionCooldown() const;
};

inline ItemId ItemData::id() const {
	return _id;
}

inline const cooldown::Type& ItemData::regenerateCooldown() const {
	return _regenerate;
}

inline const cooldown::Type& ItemData::usageCooldown() const {
	return _usage;
}

inline const cooldown::Type& ItemData::constructionCooldown() const {
	return _construction;
}

inline const ItemType& ItemData::type() const {
	return _type;
}

inline ItemShape& ItemData::shape() {
	return _shape;
}

inline const ItemShape& ItemData::shape() const {
	return _shape;
}

}
