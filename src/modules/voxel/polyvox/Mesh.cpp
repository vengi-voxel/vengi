#include "Mesh.h"
#include "CubicSurfaceExtractor.h"
#include "core/Common.h"
#include "core/Trace.h"

namespace voxel {

/// Meshes returned by the surface extractors often have vertices with efficient compressed
/// formats which are hard to interpret directly (see CubicVertex and MarchingCubesVertex).
/// This function creates a new uncompressed mesh containing the much simpler Vertex objects.
Mesh<Vertex> decodeMesh(const Mesh<CubicVertex>& encodedMesh) {
	core_trace_scoped(DecodeMesh);
	Mesh<Vertex> decodedMesh;

	for (size_t ct = 0; ct < encodedMesh.getNoOfVertices(); ct++) {
		decodedMesh.addVertex(decodeVertex(encodedMesh.getVertex(ct)));
	}

	core_assert_msg(encodedMesh.getNoOfIndices() % 3 == 0, "The number of indices must always be a multiple of three.");
	for (uint32_t ct = 0; ct < encodedMesh.getNoOfIndices(); ct += 3) {
		decodedMesh.addTriangle(encodedMesh.getIndex(ct), encodedMesh.getIndex(ct + 1), encodedMesh.getIndex(ct + 2));
	}

	decodedMesh.setOffset(encodedMesh.getOffset());

	return decodedMesh;
}


}
