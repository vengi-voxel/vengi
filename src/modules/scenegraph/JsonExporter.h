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
	JSONEXPORTER_CHILDREN = 1 << 3,

	JSONEXPORTER_ALL = 0xFFFFFFFF
};

struct NodeStats {
	int voxels = 0;
	int vertices = 0;
	int indices = 0;

	NodeStats operator+(const NodeStats &other) const;
	NodeStats &operator+=(const NodeStats &other);
};

/**
 * @brief Export a single node to JSON
 * @param sceneGraph The scene graph containing the node
 * @param nodeId The ID of the node to export
 * @param stream The output stream to write to
 * @param flags Export flags controlling what details to include
 * @return NodeStats containing voxel/vertex/index counts
 */
NodeStats sceneGraphNodeJson(const scenegraph::SceneGraph &sceneGraph, int nodeId, io::WriteStream &stream,
							 uint32_t flags = JSONEXPORTER_ALL);

/**
 * @brief Export node statistics to JSON
 * @param stats The node statistics to export
 * @param stream The output stream to write to
 * @param flags Export flags (used to determine if mesh details should be included)
 */
void sceneGraphNodeStatsJson(const NodeStats &stats, io::WriteStream &stream, uint32_t flags);

void sceneGraphJson(const scenegraph::SceneGraph &sceneGraph, io::WriteStream &stream,
					uint32_t flags = JSONEXPORTER_ALL);

} // namespace scenegraph
