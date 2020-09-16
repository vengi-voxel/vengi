/**
 * @file
 */

#include "OBJFormat.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "voxel/MaterialColor.h"
#include "voxel/VoxelVertex.h"
#include "voxel/Mesh.h"
#include "voxelformat/VoxelVolumes.h"

namespace voxel {

void OBJFormat::writeMtlFile(const core::String& mtlName) const {
	const io::FilePtr& file = io::filesystem()->open(mtlName, io::FileMode::SysWrite);
	if (!file->validHandle()) {
		Log::error("Failed to create mtl file at %s", file->name().c_str());
		return;
	}
	io::FileStream stream(file);
	stream.addStringFormat(false, "newmtl palette\n");
	stream.addStringFormat(false, "map_Ka palette-nippon.png\n");
	stream.addStringFormat(false, "map_Kd palette-nippon.png\n");
}

bool OBJFormat::saveMesh(const voxel::Mesh& mesh, const io::FilePtr &file) {
	bool withColor = true;

	io::FileStream stream(file);

	const int nv = mesh.getNoOfVertices();
	const int ni = mesh.getNoOfIndices();
	if (ni % 3 != 0) {
		Log::error("Unexpected indices amount");
		return false;
	}
	const voxel::VoxelVertex* vertices = mesh.getRawVertexData();
	const voxel::IndexType* indices = mesh.getRawIndexData();
	const MaterialColorArray& colors = getMaterialColors();

	// 1 x 256 is the texture format that we are using for our palette
	const float texcoord = 1.0f / (float)colors.size();
	// it is only 1 pixel high
	const float v1 = 0.0f;
	const float v2 = 1.0f;

	stream.addStringFormat(false, "# github.com/mgerhardy/engine\n");
	stream.addStringFormat(false, "mtllib palette.mtl\n");
	for (int i = 0; i < nv; ++i) {
		const voxel::VoxelVertex& v = vertices[i];
		const float u1 = (float)v.colorIndex * texcoord;
		const float u2 = (float)(v.colorIndex + 1) * texcoord;
		if (withColor) {
			const glm::vec4& color = colors[v.colorIndex];
			stream.addStringFormat(false, "v %i %i %i %f %f %f\n",
				v.position.x, v.position.y, v.position.z, color.r, color.g, color.b);
		} else {
			stream.addStringFormat(false, "v %i %i %i\n",
				v.position.x, v.position.y, v.position.z);
		}
		if (i % 3 == 0) {
			stream.addStringFormat(false, "vt %f %f\n", u1, v1);
		} else if (i % 3 == 1) {
			stream.addStringFormat(false, "vt %f %f\n", u2, v1);
		} else {
			stream.addStringFormat(false, "vt %f %f\n", u2, v2);
		}
	}

	stream.addStringFormat(false, "usemtl palette\n");
	for (int i = 0; i < ni; i += 3) {
		const voxel::IndexType one   = indices[i + 0] + 1;
		const voxel::IndexType two   = indices[i + 1] + 1;
		const voxel::IndexType three = indices[i + 2] + 1;
		stream.addStringFormat(false, "f %i %i %i\n", (int)one, (int)two, (int)three);
	}

	core::String name = file->name();
	core::String mtlname = core::string::stripExtension(name);
	mtlname.append(".mtl");
	writeMtlFile(mtlname);

	return true;
}

}
