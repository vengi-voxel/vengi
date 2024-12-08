/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "io/Archive.h"

namespace voxelformat {

core::String lookupTexture(const core::String &meshFilename, const core::String &in, const io::ArchivePtr &archive);

}
