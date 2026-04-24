/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicMap.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "voxelformat/Format.h"
#include <glm/vec3.hpp>

namespace voxelformat {

/**
 * @brief anivoxel format
 *
 * @ingroup Formats
 */
class AniVoxelFormat : public PaletteFormat {
protected:
	struct ChunkHeader {
		uint32_t id;
		uint32_t offset;
		uint32_t size;
		uint32_t position;
	};

	struct BoneData {
		int32_t id = -1;
		int32_t parentId = -1;
		core::String name;
		float length = 0.0f;
		glm::vec3 offset{0.0f};
		glm::vec3 rotation{0.0f};
		color::RGBA color;
		color::RGBA assignmentColor;
		int sceneGraphNodeId = -1;
	};

	struct GraphPoint {
		float frame;
		float value;
		scenegraph::InterpolationType interpolation;
	};

	struct BoneGraphSeries {
		core::DynamicArray<GraphPoint> channels[9]; // tX,tY,tZ,rX,rY,rZ,sX,sY,sZ
	};

	struct AnimationData {
		core::String name;
		int32_t fps = 2;
		int32_t frameLength = 10;
		core::DynamicMap<int32_t, BoneGraphSeries> boneGraphs;
	};

	core::DynamicMap<int32_t, BoneData> _bones;
	core::DynamicArray<AnimationData> _animations;

	ChunkHeader readChunk(io::SeekableReadStream &stream);
	bool skipChunk(io::SeekableReadStream &stream);
	void seek(const ChunkHeader &header, io::SeekableReadStream &stream);
	bool readArmature(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx,
					  int version);
	bool readAnimation(io::SeekableReadStream &stream, const LoadContext &ctx, int version);
	bool readBuffers(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx);
	bool readBone(io::SeekableReadStream &stream, const LoadContext &ctx);
	bool readPalette(io::SeekableReadStream &stream, palette::Palette &palette, const LoadContext &ctx);
	bool readMaterial(io::SeekableReadStream &stream, palette::Palette &palette);
	bool readModel(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
				   const LoadContext &ctx, int version);
	bool createBoneHierarchy(scenegraph::SceneGraph &sceneGraph);
	bool applyAnimations(scenegraph::SceneGraph &sceneGraph);
	static float sampleGraphSeries(const core::DynamicArray<GraphPoint> &points, float frame, int32_t frameLength,
								   float defaultValue);

	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"anivoxel", "", {"voxa"}, {"VOXA"}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED /* | FORMAT_FLAG_SAVE */};
		return f;
	}
};

} // namespace voxelformat
