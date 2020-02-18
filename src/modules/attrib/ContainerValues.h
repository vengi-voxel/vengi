
/**
 * @file
 */

#pragma once

#include "core/collection/Map.h"
#include "AttributeType.h"

namespace attrib {

typedef core::Map<Type, double, 8, network::EnumHash<Type> > Values;
typedef Values::iterator ValuesConstIter;
typedef Values::iterator ValuesIter;

}
