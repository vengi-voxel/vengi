/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"

namespace scenegraph {

void sceneGraphJson(const scenegraph::SceneGraph &sceneGraph, bool printMeshDetails, io::WriteStream &stream);
void sceneGraphJson(const scenegraph::SceneGraph &sceneGraph, bool printMeshDetails);

}
