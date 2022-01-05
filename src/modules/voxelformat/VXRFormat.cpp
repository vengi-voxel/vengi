/**
 * @file
 */

#include "VXRFormat.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "app/App.h"
#include "VXMFormat.h"
#include "core/StringUtil.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "voxel/RawVolume.h"
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load vmr file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) != true) { \
		Log::error("Could not load vmr file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

bool VXRFormat::saveRecursiveNode(const core::String &name, const voxel::VoxelVolume& volume, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapBool(stream.writeString(volume.name(), true))
	const core::String baseName = core::string::stripExtension(filename);
	const core::String finalName = baseName + name + ".vxm";
	wrapBool(stream.writeString(finalName, true))
	VXMFormat f;
	io::FilePtr outputFile = io::filesystem()->open(finalName, io::FileMode::SysWrite);
	if (!outputFile) {
		Log::error("Failed to open %s for writing", finalName.c_str());
		return false;
	}
	io::FileStream wstream(outputFile.get());
	VoxelVolumes volumes;
	volumes.push_back(voxel::VoxelVolume(volume.volume(), name, volume.visible(), volume.pivot()));
	wrapBool(f.saveGroups(volumes, finalName, wstream))

	wrapBool(stream.writeInt32(0)); // next child count
#if 0
	for (int i = 0; i < childCount; ++i) {
		wrapBool(saveRecursiveNode(i, volumes, filename, stream))
	}
#endif
	return true;
}

bool VXRFormat::saveGroups(const VoxelVolumes& volumes, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapBool(stream.writeUInt32(FourCC('V','X','R','4')));

	int childCount = 0;
	for (const VoxelVolume& v : volumes) {
		if (v.volume() == nullptr) {
			continue;
		}
		++childCount;
	}
	wrapBool(stream.writeInt32(childCount))
	int n = 0;
	for (const VoxelVolume& v : volumes) {
		if (v.volume() == nullptr) {
			continue;
		}
		core::String name = v.name();
		if (name.empty()) {
			name = core::string::format("%i", n);
		}
		wrapBool(saveRecursiveNode(name, v, filename, stream))
		++n;
	}
	return true;
}

bool VXRFormat::loadChildVXM(const core::String& vxmPath, VoxelVolumes& volumes) {
	const io::FilePtr& file = io::filesystem()->open(vxmPath);
	if (!file->validHandle()) {
		return false;
	}
	io::FileStream stream(file.get());
	VXMFormat f;
	return f.loadGroups(vxmPath, stream, volumes);
}

bool VXRFormat::importChildOld(const core::String &filename, io::SeekableReadStream& stream, uint32_t version) {
	if (version <= 2) {
		char id[1024];
		wrapBool(stream.readString(sizeof(id), id, true))
	}

	// TODO: vxm loading is missing here!
	uint32_t dummy;
	wrap(stream.readUInt32(dummy))
	char buf[1024];
	wrapBool(stream.readString(sizeof(buf), buf, true))
	int32_t frameCount;
	wrap(stream.readInt32(frameCount))
	for (int32_t i = 0u; i < frameCount; ++i) {
		uint32_t frame;
		wrap(stream.readUInt32(frame)) // frame index
		wrap(stream.readUInt32(dummy)) // ???
		if (version > 1) {
			stream.readBool();
		}
		glm::vec3 pivot;
		wrap(stream.readFloat(pivot.x))
		wrap(stream.readFloat(pivot.y))
		wrap(stream.readFloat(pivot.z))
		if (version >= 3) {
			glm::vec3 localPivot;
			wrap(stream.readFloat(localPivot.x))
			wrap(stream.readFloat(localPivot.y))
			wrap(stream.readFloat(localPivot.z))
		}
		if (version == 1) {
			float unknown;
			wrap(stream.readFloat(unknown))
			wrap(stream.readFloat(unknown))
			wrap(stream.readFloat(unknown))
			wrap(stream.readFloat(unknown))
			wrap(stream.readFloat(unknown))
			wrap(stream.readFloat(unknown))
		} else {
			glm::quat rot;
			wrap(stream.readFloat(rot.x))
			wrap(stream.readFloat(rot.y))
			wrap(stream.readFloat(rot.z))
			wrap(stream.readFloat(rot.w))
			glm::quat localRot;
			wrap(stream.readFloat(localRot.x))
			wrap(stream.readFloat(localRot.y))
			wrap(stream.readFloat(localRot.z))
			wrap(stream.readFloat(localRot.w))
		}
		float scale;
		wrap(stream.readFloat(scale))
		if (version >= 3) {
			float localScale;
			wrap(stream.readFloat(localScale))
		}
	}
	int32_t children;
	wrap(stream.readInt32(children))
	for (int32_t i = 0u; i < children; ++i) {
		wrapBool(importChildOld(filename, stream, version))
	}
	return true;
}

bool VXRFormat::importChild(const core::String& vxmPath, io::SeekableReadStream& stream, VoxelVolumes& volumes, uint32_t version) {
	char id[1024];
	wrapBool(stream.readString(sizeof(id), id, true))
	char filename[1024];
	wrapBool(stream.readString(sizeof(filename), filename, true))
	if (filename[0] != '\0') {
		core::String modelPath = core::string::extractPath(vxmPath);
		if (!modelPath.empty()) {
			modelPath.append("/");
		}
		modelPath.append(filename);
		if (!loadChildVXM(modelPath, volumes)) {
			Log::warn("Failed to attach model for %s with filename %s", id, filename);
		}
	}
	if (version <= 3) {
		return true;
	}
	if (version == 4) {
		int32_t children = 0;
		wrap(stream.readInt32(children))
		for (int32_t i = 0; i < children; ++i) {
			wrapBool(importChild(vxmPath, stream, volumes, version))
		}
		return true;
	}
	if (version >= 9) {
		stream.readBool(); // collidable
		stream.readBool(); // decorative
	}
	uint32_t dummy;
	if (version >= 6) {
		wrap(stream.readUInt32(dummy)) // color
		stream.readBool(); // favorite
		stream.readBool(); // visible?
	}
	stream.readBool(); // mirror x axis
	stream.readBool(); // mirror y axis
	stream.readBool(); // mirror z axis
	stream.readBool(); // preview mirror x axis
	stream.readBool(); // preview mirror y axis
	stream.readBool(); // preview mirror z axis
	stream.readBool(); // ikAnchor
	float dummyf;
	if (version >= 9) {
		char effectorId[1024];
		wrapBool(stream.readString(sizeof(effectorId), effectorId, true))
		stream.readBool(); // constraints visible
		wrap(stream.readFloat(dummyf)) // rollmin ???
		wrap(stream.readFloat(dummyf)) // rollmax ???
		int32_t constraints;
		wrap(stream.readInt32(constraints))
		for (int32_t i = 0; i < constraints; ++i) {
			wrap(stream.readFloat(dummyf)) // x
			wrap(stream.readFloat(dummyf)) // z
			wrap(stream.readFloat(dummyf)) // radius
		}
	} else {
		stream.readBool(); // ???
		wrap(stream.readFloat(dummyf)) // ???
		wrap(stream.readFloat(dummyf)) // ???
		stream.readBool(); // ???
		stream.readBool(); // ???
		stream.readBool(); // ???
		stream.readBool(); // ???
	}
	int32_t children = 0;
	wrap(stream.readInt32(children))
	for (int32_t i = 0; i < children; ++i) {
		wrapBool(importChild(vxmPath, stream, volumes, version))
	}
	return true;
}

image::ImagePtr VXRFormat::loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) {
	const core::String imageName = filename + ".png";
	return image::loadImage(imageName, false);
}

bool VXRFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, VoxelVolumes& volumes) {
	uint8_t magic[4];
	wrap(stream.readUInt8(magic[0]))
	wrap(stream.readUInt8(magic[1]))
	wrap(stream.readUInt8(magic[2]))
	wrap(stream.readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'R') {
		Log::error("Could not load vxr file: Invalid magic found (%c%c%c%c)",
			magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	int version;
	if (magic[3] >= '0' && magic[3] <= '9') {
		version = magic[3] - '0';
	} else {
		Log::error("Invalid version found");
		return false;
	}

	if (version < 1 || version > 9) {
		Log::error("Could not load vxr file: Unsupported version found (%i)", version);
		return false;
	}

	if (version >= 7) {
		char defaultAnim[1024];
		wrapBool(stream.readString(sizeof(defaultAnim), defaultAnim, true))
	}
	if (version <= 3) {
		uint32_t dummy;
		wrap(stream.readUInt32(dummy))
		uint32_t children = 0;
		wrap(stream.readUInt32(children))
		for (uint32_t i = 0; i < children; ++i) {
			wrapBool(importChildOld(filename, stream, version))
		}
		int32_t modelCount;
		wrap(stream.readInt32(modelCount))
		for (int32_t i = 0; i < modelCount; ++i) {
			char id[1024];
			wrapBool(stream.readString(sizeof(id), id, true))
			char vxmFilename[1024];
			wrapBool(stream.readString(sizeof(vxmFilename), vxmFilename, true))
			if (vxmFilename[0] != '\0') {
				core::String modelPath = core::string::extractPath(filename);
				if (!modelPath.empty()) {
					modelPath.append("/");
				}
				modelPath.append(vxmFilename);
				if (!loadChildVXM(modelPath, volumes)) {
					Log::warn("Failed to attach model for %s with filename %s", id, modelPath.c_str());
				}
			}
		}
		return true;
	}

	int32_t children = 0;
	wrap(stream.readInt32(children))

	if (version >= 8) {
		char baseTemplate[1024];
		wrapBool(stream.readString(sizeof(baseTemplate), baseTemplate, true))
		const bool isStatic = stream.readBool();
		if (isStatic) {
			int32_t lodLevels;
			wrap(stream.readInt32(lodLevels))
			for (int32_t i = 0 ; i < lodLevels; ++i) {
				uint32_t dummy;
				wrap(stream.readUInt32(dummy))
				wrap(stream.readUInt32(dummy))
				uint32_t diffuseTexZipped;
				wrap(stream.readUInt32(diffuseTexZipped))
				stream.skip(diffuseTexZipped);
				const bool hasEmissive = stream.readBool();
				if (hasEmissive) {
					uint32_t emissiveTexZipped;
					wrap(stream.readUInt32(emissiveTexZipped))
					stream.skip(emissiveTexZipped);
				}
				int32_t quadAmount;
				wrap(stream.readInt32(quadAmount))
				for (int32_t quad = 0; quad < quadAmount; ++quad) {
					for (int v = 0; v < 4; ++v) {
						float dummyFloat;
						wrap(stream.readFloat(dummyFloat))
						wrap(stream.readFloat(dummyFloat))
						wrap(stream.readFloat(dummyFloat))
						wrap(stream.readFloat(dummyFloat))
						wrap(stream.readFloat(dummyFloat))
					}
				}
			}
		}
	}

	Log::debug("Found %i children (%i)", children, (int)volumes.size());
	for (int32_t i = 0; i < children; ++i) {
		wrapBool(importChild(filename, stream, volumes, version))
	}

	// some files since version 6 still have stuff here

	return true;
}

#undef wrap
#undef wrapBool

}
