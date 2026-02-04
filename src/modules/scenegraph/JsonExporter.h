/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"

namespace scenegraph {

enum JsonExporterFlags : uint32_t {
	JSONEXPORTER_PALETTE = 1 << 0,
	JSONEXPORTER_MESHDETAILS = 1 << 1,
	JSONEXPORTER_NODEDETAILS = 1 << 2,

	JSONEXPORTER_ALL = 0xFFFFFFFF
};

void sceneGraphJson(const scenegraph::SceneGraph &sceneGraph, bool printMeshDetails, io::WriteStream &stream,
					uint32_t flags = JSONEXPORTER_ALL);
void sceneGraphJson(const scenegraph::SceneGraph &sceneGraph, bool printMeshDetails, uint32_t flags = JSONEXPORTER_ALL);

} // namespace scenegraph
