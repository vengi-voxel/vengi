/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Teardown bin importer
 *
 * Format implementation based on the information from https://github.com/TTFH/Teardown-Converter
 *
 * @ingroup Formats
 */
class TeardownFormat : public Format {
protected:
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override {
		return false;
	}

	enum EntityType : uint8_t {
		Body = 1,
		Shape = 2,
		Light = 3,
		Location = 4,
		Water = 5,
		Joint = 6,
		Vehicle = 7,
		Wheel = 8,
		Screen = 9,
		Trigger = 10,
		Script = 11,
		Animator = 12,

		Max
	};

	struct Header {
		// version read from the file header (e.g. 170 for 1.7.0)
		uint32_t version;
		core::DynamicArray<palette::Palette> palettes;
		core::String levelId;
		core::String levelPath;
		core::String layers;
		core::String mod;
		scenegraph::SceneGraphNodeProperties mods;
		scenegraph::SceneGraphNodeProperties spawnedMods;
	};
	bool readEntity(const Header &header, scenegraph::SceneGraph &sceneGraph, io::ReadStream &s, int parent,
					int &nodeId);

	bool readBody(io::ReadStream &s);
	bool readShape(const Header &header, scenegraph::SceneGraph &sceneGraph, io::ReadStream &s, int parent,
				   int &nodeId);
	bool readLight(io::ReadStream &s);
	bool readLocation(io::ReadStream &s);
	bool readWater(io::ReadStream &s);
	bool readJoint(io::ReadStream &s);
	bool readVehicle(const Header &header, io::ReadStream &s);
	bool readWheel(io::ReadStream &s);
	bool readScreen(io::ReadStream &s);
	bool readTrigger(io::ReadStream &s);
	bool readScript(io::ReadStream &s);
	bool readAnimator(const Header &header, io::ReadStream &s, int parent, int &nodeId);

	bool readVoxels(const Header &header, scenegraph::SceneGraphNode &node, io::ReadStream &s);
	bool readRope(io::ReadStream &s);
	bool readLuaTable(io::ReadStream &s);
	bool readLuaValue(io::ReadStream &s, int typeId);

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Teardown bin", {"bin"}, {}, 0u};
		return f;
	}
};

} // namespace voxelformat
