/**
 * @file
 */

#pragma once

#include "Types.h"
#include "core/String.h"

namespace util {

/**
 * @brief convert the given input string into lower- or upper-camel-case
 * @param in The string to convert
 * @param firstUpper Convert to upper camel case
 * @return The camel case string
 */
extern core::String convertName(const core::String& in, bool firstUpper);

extern core::String uniformSetterPostfix(const Variable::Type type, int amount);

/**
 * @return @c video::ImageFormat::Max if no mapping was found
 */
extern video::ImageFormat getImageFormat(const core::String& type, int line);

/**
 * @return @c nullptr if no mapping was found
 */
extern const char* getImageFormatGLType(video::ImageFormat format);
extern const char* getImageFormatTypeString(video::ImageFormat format);

extern const char* getPrimitiveTypeString(video::Primitive primitive);

extern int getComponents(const Variable::Type type);

extern Variable::Type getType(const core::String& type, int line);

extern size_t std140Size(const Variable& v);
extern int std140Align(const Variable& v);

extern size_t std430Size(const Variable& v);
extern int std430Align(const Variable& v);

extern const Types& resolveTypes(Variable::Type type);

}
