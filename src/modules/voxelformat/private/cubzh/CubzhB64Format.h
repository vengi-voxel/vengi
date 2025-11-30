/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * This is loading the b64 file that references multiple 3zh (see @c CubzhFormat class) files with positions, rotations,
 * scale and so on.
 *
 * @note The b64 map is located at @c Voxowl/Particubes/bundle/misc and the 3zh files at @c
 * Voxowl/Particubes/bundle/cache
 *
 * @li v0: lua table serialized
 * @li v1: versionId, map chunk that can be read from cpp to load map, then 3 table serialized as base64 chunks
 * @li v2: versionId, map chunk that can be read from cpp to load map, ambience fields, objects, blocks
 *     ambience, objects and blocks might not be serialized if the value is nil or length is 0
 * @li v3: same as v2 but removed itemDetailsCell and Objects chunk length is now uint32 and not uint16
 */
class CubzhB64Format : public RGBAFormat {
protected:
	struct Ambience {
		color::RGBA skyColor;
		color::RGBA skyHorizonColor;
		color::RGBA skyAbyssColor;
		color::RGBA skyLightColor;
		float skyLightIntensity;

		color::RGBA fogColor;
		float fogNear;
		float fogFar;
		float fogAbsorbtion;

		color::RGBA sunColor;
		float sunIntensity;
		float sunRotation[2];

		float ambientSkyLightFactor;
		float ambientDirLightFactor;

		core::String txt;
	};
	void setAmbienceProperties(scenegraph::SceneGraph &sceneGraph, const Ambience &ambience) const;
	/**
	 * @param[in] archive The io::Archive to load the given file from
	 * @param[in] luaName The file to load from the archive. Can contain directory parts with lua directory separators.
	 * @param[in] modelScene The scene to load the objects into
	 */
	bool loadObject(const io::ArchivePtr &archive, const core::String &luaName, scenegraph::SceneGraph &modelScene,
					const LoadContext &ctx);
	/**
	 * @brief Load all 3zh objects that are placed in the world
	 */
	bool readObjects(const core::String &filename, const io::ArchivePtr &archive, io::ReadStream &stream,
					 scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const LoadContext &ctx,
					 int version);
	/**
	 * @brief Load the map chunk of the file that contains the 3zh map file name and the scaling factor of the world
	 */
	bool readChunkMap(const core::String &filename, const io::ArchivePtr &archive, io::ReadStream &stream,
					  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const LoadContext &ctx);
	bool readAmbience(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					  const LoadContext &ctx, Ambience &ambience);
	bool readBlocks(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					const LoadContext &ctx);
	bool loadVersion1(const core::String &filename, const io::ArchivePtr &archive, io::ReadStream &stream,
					  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const LoadContext &ctx);
	bool loadVersion2(const core::String &filename, const io::ArchivePtr &archive, io::ReadStream &stream,
					  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const LoadContext &ctx);
	bool loadVersion3(const core::String &filename, const io::ArchivePtr &archive, io::ReadStream &stream,
					  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const LoadContext &ctx);
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx);
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) {
		return false;
	}
public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Cubzh World", {"b64"}, {}, 0u};
		return f;
	}
};

} // namespace voxelformat
