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
	stream.addStringFormat(false, "# github.com/mgerhardy/engine\n");
	stream.addStringFormat(false, "\n");
	stream.addStringFormat(false, "newmtl palette\n");
	stream.addStringFormat(false, "Ka 1.000000 1.000000 1.000000\n");
	stream.addStringFormat(false, "Kd 1.000000 1.000000 1.000000\n");
	stream.addStringFormat(false, "Ks 0.000000 0.000000 0.000000\n");
	stream.addStringFormat(false, "Tr 1.000000\n");
	stream.addStringFormat(false, "illum 1\n");
	stream.addStringFormat(false, "Ns 0.000000\n");
	stream.addStringFormat(false, "map_Kd palette-%s.png\n", voxel::getDefaultPaletteName());
}

bool OBJFormat::saveMesh(const voxel::Mesh& mesh, const io::FilePtr &file, float scale, bool quad, bool withColor, bool withTexCoords) {
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
	// it is only 1 pixel high - sample the middle
	const float v1 = 0.5f;

	stream.addStringFormat(false, "# github.com/mgerhardy/engine\n");
	stream.addStringFormat(false, "\n");
	stream.addStringFormat(false, "g Model\n");
	stream.addStringFormat(false, "mtllib palette.mtl\n");
	stream.addStringFormat(false, "usemtl palette\n");

	for (int i = 0; i < nv; ++i) {
		const voxel::VoxelVertex& v = vertices[i];
		stream.addStringFormat(false, "v %.04f %.04f %.04f",
				(float)v.position.x * scale, (float)v.position.y * scale, (float)v.position.z * scale);
		if (withColor) {
			const glm::vec4& color = colors[v.colorIndex];
			stream.addStringFormat(false, " %.03f %.03f %.03f", color.r, color.g, color.b);
		}
		stream.addStringFormat(false, "\n");
	}

	if (quad) {
		if (withTexCoords) {
			for (int i = 0; i < ni; i += 6) {
				const voxel::VoxelVertex& v = vertices[indices[i]];
				const float u = ((float)(v.colorIndex) + 0.5f) * texcoord;
				stream.addStringFormat(false, "vt %f %f\n", u, v1);
				stream.addStringFormat(false, "vt %f %f\n", u, v1);
				stream.addStringFormat(false, "vt %f %f\n", u, v1);
				stream.addStringFormat(false, "vt %f %f\n", u, v1);
			}
		}

		int uvi = 0;
		for (int i = 0; i < ni; i += 6, uvi += 4) {
			const voxel::IndexType one   = indices[i + 0] + 1;
			const voxel::IndexType two   = indices[i + 1] + 1;
			const voxel::IndexType three = indices[i + 2] + 1;
			const voxel::IndexType four  = indices[i + 5] + 1;
			if (withTexCoords) {
				stream.addStringFormat(false, "f %i/%i %i/%i %i/%i %i/%i\n",
					(int)one, uvi + 1, (int)two, uvi + 2, (int)three, uvi + 3, (int)four, uvi + 4);
			} else {
				stream.addStringFormat(false, "f %i %i %i %i\n", (int)one, (int)two, (int)three, (int)four);
			}
		}
	} else {
		if (withTexCoords) {
			for (int i = 0; i < ni; i += 3) {
				const voxel::VoxelVertex& v = vertices[indices[i]];
				const float u = ((float)(v.colorIndex) + 0.5f) * texcoord;
				stream.addStringFormat(false, "vt %f %f\n", u, v1);
				stream.addStringFormat(false, "vt %f %f\n", u, v1);
				stream.addStringFormat(false, "vt %f %f\n", u, v1);
			}
		}

		for (int i = 0; i < ni; i += 3) {
			const voxel::IndexType one   = indices[i + 0] + 1;
			const voxel::IndexType two   = indices[i + 1] + 1;
			const voxel::IndexType three = indices[i + 2] + 1;
			if (withTexCoords) {
				stream.addStringFormat(false, "f %i/%i %i/%i %i/%i\n", (int)one, i + 1, (int)two, i + 2, (int)three, i + 3);
			} else {
				stream.addStringFormat(false, "f %i %i %i\n", (int)one, (int)two, (int)three);
			}
		}
	}

	core::String name = file->name();
	core::String mtlname = core::string::stripExtension(name);
	mtlname.append(".mtl");
	writeMtlFile(mtlname);

	return true;
}

}
