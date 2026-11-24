/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace util {

/**
 * @brief Escape text for an XML text node or attribute.
 *
 * tinyxml2 only escapes while printing a document. XMLPrinter::PrintString is not a public helper.
 */
core::String xmlEscape(const char *in);

} // namespace util
