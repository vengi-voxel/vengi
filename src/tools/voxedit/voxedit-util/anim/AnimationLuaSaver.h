/**
 * @file
 */

#pragma once

#include "core/io/File.h"
#include "animation/CharacterSettings.h"

namespace voxedit {

extern bool saveCharacterLua(const animation::CharacterSettings& characterSettings, const io::FilePtr& file);

}
