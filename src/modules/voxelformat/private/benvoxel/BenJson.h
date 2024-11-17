/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraph.h"

namespace voxelformat {
namespace benv {

bool loadJson(scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const core::String &jsonStr);

} // namespace benv
} // namespace voxelformat
