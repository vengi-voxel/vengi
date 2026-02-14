/**
 * @file
 * @defgroup Formats Formats
 * @{
 * File formats.
 * @}
 */

#pragma once

#include "palette/RGBABuffer.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelformat/FormatThumbnail.h"
#include <glm/fwd.hpp>

namespace palette {
class Palette;
}

namespace voxel {
class Mesh;
} // namespace voxel

namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
} // namespace scenegraph

namespace voxelformat {

typedef void (*ProgressMonitor)(const char *name, int cur, int max);

struct LoadContext {
	ProgressMonitor monitor = nullptr;
	inline void progress(const char *name, int cur, int max) const {
		if (monitor == nullptr) {
			return;
		}
		monitor(name, cur, max);
	}
};

struct SaveContext {
	/**
	 * @brief A basic image rendering helper.
	 */
	static image::ImagePtr renderToImageThumbnailCreator(const scenegraph::SceneGraph &sceneGraph,
														 const voxelformat::ThumbnailContext &ctx);
	ProgressMonitor monitor = nullptr;
	inline void progress(const char *name, int cur, int max) const {
		if (monitor == nullptr) {
			return;
		}
		monitor(name, cur, max);
	}
	/**
	 * A callback that are either null or returns an instance of @c image::ImagePtr for the thumbnail of the
	 * given scene graph. Some formats have embedded screenshots.
	 */
	ThumbnailCreator thumbnailCreator = nullptr;
};

// the max amount of voxels - [0-255]
static constexpr int MaxRegionSize = 256;

/**
 * @brief Base class for all voxel formats.
 *
 * @ingroup Formats
 */
class Format {
protected:
	uint8_t _flattenFactor;
	/**
	 * @brief If you have to split the volumes in the scene graph because the format only supports a certain size, you
	 * can return the max size here. If the returned value is not a valid volume size (<= 0) the value is ignored.
	 * @sa singleVolume()
	 * @note @c singleVolume() and @c maxSize() don't work well together as the first would merge everything, and the
	 * latter would split it again if the max size was exceeded.
	 */
	virtual glm::ivec3 maxSize() const;

	/**
	 * @brief Checks whether the given chunk is empty (only contains air).
	 *
	 * @param v The volume
	 * @param maxSize The chunk size
	 * @param x The chunk position
	 * @param y The chunk position
	 * @param z The chunk position
	 */
	bool isEmptyBlock(const voxel::RawVolume *v, const glm::ivec3 &maxSize, int x, int y, int z) const;
	/**
	 * @brief Calculate the boundaries while aligning them to the given @c maxSize. This ensures that the
	 * calculated extends are exactly @c maxSize when iterating over them (and align relative to 0,0,0 and
	 * @c maxSize).
	 *
	 * @param[in] region The region to calculate the aligned mins/maxs for
	 * @param[in] maxSize The size of a single chunk to align with.
	 * @param[out] mins The extends of the aabb aligned with @c maxSize
	 * @param[out] maxs The extends of the aabb aligned with @c maxSize
	 */
	void calcMinsMaxs(const voxel::Region &region, const glm::ivec3 &maxSize, glm::ivec3 &mins, glm::ivec3 &maxs) const;

	color::RGBA flattenRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) const;
	color::RGBA flattenRGB(color::RGBA rgba) const;
	/**
	 * @brief This can be used for rgb color formats to create a palette. Just read
	 * all the colors and add then add them to the palette.
	 *
	 * @sa palette::RGBABuffer
	 * @sa palette::Palette::createPalette()
	 */
	int createPalette(const palette::RGBABuffer &colors, palette::Palette &palette) const;
	int createPalette(const palette::RGBAMaterialMap &colors, palette::Palette &palette) const;

	/**
	 * Some formats are running loop that the user might want to interrupt with CTRL+c or the like. Long lasting loops
	 * should query this boolean and respect the users wish to quit the application.
	 */
	static bool stopExecution();

	static core::String stringProperty(const scenegraph::SceneGraphNode *node, const core::String &name,
									   const core::String &defaultVal = "");
	static bool boolProperty(const scenegraph::SceneGraphNode *node, const core::String &name, bool defaultVal = false);
	static float floatProperty(const scenegraph::SceneGraphNode *node, const core::String &name,
							   float defaultVal = 0.0f);
	static image::ImagePtr createThumbnail(const scenegraph::SceneGraph &sceneGraph, ThumbnailCreator thumbnailCreator,
										   const ThumbnailContext &ctx);
	/**
	 * @param[in] sceneGraph The @c SceneGraph instance to save
	 * @param[in] filename The target file name. Some formats needs this next to the stream to identify or load
	 * additional files.
	 * @param[out] archive The target archive
	 * @param[in] ctx A context object for saving
	 * @todo don't use a stream, but an archive for formats that are split over several files
	 */
	virtual bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							const io::ArchivePtr &archive, const SaveContext &ctx) = 0;
	/**
	 * @brief If the format supports multiple models or groups, this method load them into the scene graph
	 * @todo don't use a stream, but an archive for formats that are split over several files
	 */
	virtual bool loadGroups(const core::String &filename, const io::ArchivePtr &archive,
							scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) = 0;

	/**
	 * @brief Check if the given region is valid for processing
	 */
	bool checkValidRegion(const voxel::Region &region) const;

public:
	Format();
	virtual ~Format() = default;

	/**
	 * @brief If a format only supports a single volume. If this returns true, the @¢ save() method gets a scene graph
	 * with only one model
	 * @sa maxSize()
	 * @note @c singleVolume() and @c maxSize() don't work well together as the first would merge everything, and the
	 * latter would split it again if the max size was exceeded.
	 */
	virtual bool singleVolume() const;

	/**
	 * @brief Whether the format supports saving and restoring model references with their transforms.
	 *
	 * If this returns @c false and the scene graph contains model references, the @c save() method will resolve
	 * the references into actual model nodes before calling @c saveGroups().
	 *
	 * Formats that natively handle references (e.g., VENGI) should override this to return @c true.
	 */
	virtual bool supportsReferences() const;

	/**
	 * Some formats have embedded screenshots of the model. This method doesn't load anything else than that image.
	 * @note Not supported by many formats.
	 * @todo don't use a stream, but an archive for formats that are split over several files
	 */
	virtual image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										   const LoadContext &ctx);

	/**
	 * @brief Only load the palette that is included in the format
	 * @note Not all voxel formats have a palette included - if they do and don't have this method implemented, they
	 * will go the expensive route. They will load all the nodes, all the voxels and just use the palette data. This
	 * means a lot of computation time is wasted and we should consider implementing this for as many as possible
	 * formats.
	 *
	 * @todo don't use a stream, but an archive for formats that are split over several files
	 * @return the amount of colors found in the palette
	 */
	virtual size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							   const LoadContext &ctx);
	/**
	 * @todo don't use a stream, but an archive for formats that are split over several files
	 */
	virtual bool load(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					  const LoadContext &ctx);
	/**
	 * @todo don't use a stream, but an archive for formats that are split over several files
	 */
	virtual bool save(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					  const io::ArchivePtr &archive, const SaveContext &ctx);
};

/**
 * @brief A format with only voxels - but no color attached
 *
 * @ingroup Formats
 */
class NoColorFormat : public Format {};

/**
 * @brief A format with an embedded palette
 *
 * @ingroup Formats
 */
class PaletteFormat : public Format {
private:
	using Super = Format;
protected:
	virtual bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
								   const LoadContext &ctx) = 0;

	/**
	 * @brief This indicates whether the format only supports one palette for the whole scene graph
	 */
	virtual bool onlyOnePalette() const {
		return true;
	}
	/**
	 * A few formats are using a palette index to indicate an empty voxel.
	 * @return A palette index of @c -1 means that the format doesn't support this feature. Otherwise an index between
	 * @c [0,palette::PaletteMaxColors] must be used
	 */
	virtual int emptyPaletteIndex() const;
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override final;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
	bool save(const scenegraph::SceneGraph &sceneGraph, const core::String &filename, const io::ArchivePtr &archive,
			  const SaveContext &ctx) override final;
};

/**
 * @brief A format that stores the voxels with rgba colors
 * @note These color are converted into a palette.
 *
 * @ingroup Formats
 */
class RGBAFormat : public Format {
protected:
	virtual bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
								scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								const LoadContext &ctx) = 0;
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
};

/**
 * @ingroup Formats
 */
class RGBASinglePaletteFormat : public RGBAFormat {
private:
	using Super = RGBAFormat;
protected:
	virtual int emptyPaletteIndex() const;

public:
	bool save(const scenegraph::SceneGraph &sceneGraph, const core::String &filename, const io::ArchivePtr &archive,
			  const SaveContext &ctx) override final;
};

} // namespace voxelformat
