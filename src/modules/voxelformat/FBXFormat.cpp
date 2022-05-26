/**
 * @file
 */

#include "FBXFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/String.h"
#include "engine-config.h"
#include "io/StdStreamBuf.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxelformat {

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Failed to write fbx " CORE_STRINGIFY(read));                                                       \
		return false;                                                                                                  \
	}

bool FBXFormat::saveMeshes(const core::Map<int, int> &, const SceneGraph &sceneGraph, const Meshes &meshes,
						   const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale,
						   bool quad, bool withColor, bool withTexCoords) {
	return saveMeshesAscii(meshes, filename, stream, scale, quad, withColor, withTexCoords, sceneGraph);
}

class FBXScopedHeader {
private:
	io::SeekableWriteStream& _stream;
	/**
	 * @brief EndOffset is the distance from the beginning of the file to the end of the node record (i.e. the first
	 * byte of whatever comes next). This can be used to easily skip over unknown or not required records.
	 */
	int64_t _endOffsetPos;

public:
	FBXScopedHeader(io::SeekableWriteStream& stream) : _stream(stream) {
		_endOffsetPos = stream.pos();
		stream.writeUInt32(0u);
	}

	~FBXScopedHeader() {
		const int64_t currentPos = _stream.pos();
		_stream.seek(_endOffsetPos);
		_stream.writeUInt32(currentPos);
		_stream.seek(currentPos);
	}
};

bool FBXFormat::saveMeshesBinary(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords, const SceneGraph &sceneGraph) {
	wrapBool(stream.writeString("Kaydara FBX Binary  ", true))
	stream.writeUInt8(0x1A);  // unknown
	stream.writeUInt8(0x00);  // unknown
	stream.writeUInt32(7300); // version
	// TODO: implement me https://code.blender.org/2013/08/fbx-binary-file-format-specification/
	return false;
}

// https://github.com/blender/blender/blob/00e219d8e97afcf3767a6d2b28a6d05bcc984279/release/io/export_fbx.py
bool FBXFormat::saveMeshesAscii(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords, const SceneGraph &sceneGraph) {
	// TODO: support keyframes (takes)
	stream.writeStringFormat(false, R"(FBXHeaderExtension:  {
	FBXHeaderVersion: 1003
	FBXVersion: 6100
	Creator: "github.com/mgerhardy/vengi %s"
	OtherFlags:  {
		FlagPLE: 0
	}
}

Creator: "%s %s"

Definitions: {
	Version: 100
	Count: 1
	ObjectType: "Model" {
		Count: %i
	}
	ObjectType: "Material" {
		Count: 1
	}
}

Objects: {

)",
							PROJECT_VERSION, app::App::getInstance()->appname().c_str(), PROJECT_VERSION, (int)meshes.size());

	Log::debug("Exporting %i layers", (int)meshes.size());

	// TODO: maybe also export Model: "Model::Camera", "Camera"
	// TODO: are connections and relations needed?
	// https://github.com/libgdx/fbx-conv/blob/master/samples/blender/cube.fbx

	for (const MeshExt &meshExt : meshes) {
		const voxel::Mesh *mesh = meshExt.mesh;
		Log::debug("Exporting layer %s", meshExt.name.c_str());
		const int nv = (int)mesh->getNoOfVertices();
		const int ni = (int)mesh->getNoOfIndices();
		if (ni % 3 != 0) {
			Log::error("Unexpected indices amount");
			return false;
		}
		const SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
		const voxel::Palette &palette = graphNode.palette();
		int frame = 0;
		const SceneGraphTransform &transform = graphNode.transform(frame);
		const voxel::VoxelVertex *vertices = mesh->getRawVertexData();
		const voxel::IndexType *indices = mesh->getRawIndexData();
		const char *objectName = meshExt.name.c_str();
		if (objectName[0] == '\0') {
			objectName = "Noname";
		}

		stream.writeStringFormat(false, "\tModel: \"%s\", \"Mesh\" {\n", objectName);
		wrapBool(stream.writeString("\t\tVersion: 232\n", false))
		wrapBool(stream.writeString("\t\tVertices: ", false))
		for (int i = 0; i < nv; ++i) {
			const voxel::VoxelVertex &v = vertices[i];

			glm::vec3 pos;
			if (meshExt.applyTransform) {
				pos = transform.apply(v.position, meshExt.size);
			} else {
				pos = v.position;
			}
			pos *= scale;
			if (i > 0) {
				wrapBool(stream.writeString(",", false))
			}
			stream.writeStringFormat(false, "%.04f,%.04f,%.04f", pos.x, pos.y, pos.z);
		}
		wrapBool(stream.writeString("\n", false))

		wrapBool(stream.writeString("\t\tPolygonVertexIndex: ", false))

		for (int i = 0; i < ni; i += 3) {
			const uint32_t one = indices[i + 0] + 1;
			const uint32_t two = indices[i + 1] + 1;
			const uint32_t three = indices[i + 2] + 1;
			if (i > 0) {
				wrapBool(stream.writeString(",", false))
			}
			stream.writeStringFormat(false, "%i,%i,%i", (int)one, (int)two, (int)three);
		}
		wrapBool(stream.writeString("\n", false))
		wrapBool(stream.writeString("\t\tGeometryVersion: 124\n", false))

		if (withTexCoords) {
			// 1 x 256 is the texture format that we are using for our palette
			const float texcoord = 1.0f / (float)voxel::PaletteMaxColors;
			// it is only 1 pixel high - sample the middle
			const float v1 = 0.5f;
			wrapBool(stream.writeString("\t\tLayerElementUV: 0 {\n", false))
			wrapBool(stream.writeString("\t\t\tVersion: 101\n", false))
			stream.writeStringFormat(false, "\t\t\tName: \"%sUV\"\n", objectName);
			wrapBool(stream.writeString("\t\t\tMappingInformationType: \"ByPolygonVertex\"\n", false))
			wrapBool(stream.writeString("\t\t\tReferenceInformationType: \"Direct\"\n", false))
			wrapBool(stream.writeString("\t\t\tUV: ", false))

			for (int i = 0; i < ni; i++) {
				const uint32_t index = indices[i];
				const voxel::VoxelVertex &v = vertices[index];
				const float u = ((float)(v.colorIndex) + 0.5f) * texcoord;
				if (i > 0) {
					wrapBool(stream.writeString(",", false))
				}
				stream.writeStringFormat(false, "%f,%f", u, v1);
			}
			wrapBool(stream.writeString("\n\n", false))
			// TODO: UVIndex needed or only for IndexToDirect?

			wrapBool(stream.writeString(
				"\t\tLayerElementTexture: 0 {\n"
				"\t\t\tVersion: 101\n"
				"\t\t\tName: \"\"\n" // TODO
				"\t\t\tMappingInformationType: \"AllSame\"\n"
				"\t\t\tReferenceInformationType: \"Direct\"\n"
				"\t\t\tBlendMode: \"Translucent\"\n"
				"\t\t\tTextureAlpha: 1\n"
				"\t\t\tTextureId: 0\n"
				"\t\t}\n"))
		}

		if (withColor) {
			stream.writeStringFormat(false,
									 "\t\tLayerElementColor: 0 {\n"
									 "\t\t\tVersion: 101\n"
									 "\t\t\tName: \"%sColors\"\n"
									 "\t\t\tMappingInformationType: \"ByPolygonVertex\"\n"
									 "\t\t\tReferenceInformationType: \"Direct\"\n"
									 "\t\t\tColors: ",
									 objectName);
			for (int i = 0; i < ni; i++) {
				const uint32_t index = indices[i];
				const voxel::VoxelVertex &v = vertices[index];
				const glm::vec4 &color = core::Color::fromRGBA(palette.colors[v.colorIndex]);
				if (i > 0) {
					wrapBool(stream.writeString(",", false))
				}
				stream.writeStringFormat(false, "%f,%f,%f,%f", color.r, color.g, color.b, color.a);
			}
			wrapBool(stream.writeString("\n\n", false))
			// TODO: ColorIndex needed or only for IndexToDirect?

			// close LayerElementColor
			wrapBool(stream.writeString("\t\t}\n", false))

			wrapBool(stream.writeString("\t\tLayer: 0 {\n"
							   "\t\t\tVersion: 100\n"
							   "\t\t\tLayerElement: {\n"
							   "\t\t\t\tTypedIndex: 0\n"
							   "\t\t\t\tType: \"LayerElementColor\"\n"
							   "\t\t\t}\n"
							   "\t\t}\n",
							   false))
		}

		// close the model
		wrapBool(stream.writeString("\t}\n}\n\n", false))
	}
	return true;
}

#undef wrapBool

} // namespace voxelformat
