/**
 * @file
 */
#pragma once

#include "core/String.h"
#include "core/collection/StringMap.h"

namespace ai {

/**
 * @brief Defines some standard names for @c ICharacter attributes. None of these must be used. But if you
 * use them, the remote debugger can make use of known values to render more information into the view.
 */
namespace attributes {
/**
 * @brief Attribute for the name of an entity
 */
const char* const NAME = "Name";
const char* const GROUP = "Group";
const char* const ID = "Id";
const char* const POSITION = "Position";
const char* const SPEED = "Speed";
const char* const ORIENTATION = "Orientation";
}

/**
 * @brief ICharacter attributes for the remote \ref debugger
 */
typedef core::StringMap<core::String> CharacterMetaAttributes;

}
