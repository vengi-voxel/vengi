/**
 * @file
 * @brief Forward declarations for image types
 *
 * Include this instead of Image.h when you only need ImagePtr
 */

#pragma once

#include "core/SharedPtr.h"

namespace image {

class Image;
typedef core::SharedPtr<Image> ImagePtr;

} // namespace image
