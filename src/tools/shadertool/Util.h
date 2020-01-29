/**
 * @file
 */

#pragma once

#define USE_ALIGN_AS 1
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

extern core::String std140Align(const Variable& v);

extern core::String std140Padding(const Variable& v, int& padding);

extern size_t std140Size(const Variable& v);

extern core::String std430Align(const Variable& v);

extern size_t std430Size(const Variable& v);

extern core::String std430Padding(const Variable& v, int& padding);

extern const Types& resolveTypes(Variable::Type type);

}
