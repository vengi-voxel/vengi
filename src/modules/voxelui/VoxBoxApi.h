/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"

namespace scenegraph {
class SceneGraph;
}

namespace voxelui {

enum class VoxBoxCategory {
	Character,
	Environment,
	Prop,
	Vegetation,
	Vehicle,
	Random,
	Weapon,
	Architecture,

	Max
};

enum class VoxBoxLicense {
	CC0,
	CCBY,
	CCBYSA,
	CCBYND,
	CCBYNC,
	CCBYNCSA,
	CCBYNCND,

	Max
};

const char *VoxBoxCategoryStr(VoxBoxCategory c);
VoxBoxCategory VoxBoxCategoryFromStr(const core::String &s);

const char *VoxBoxLicenseStr(VoxBoxLicense l);
VoxBoxLicense VoxBoxLicenseFromStr(const core::String &s);

struct VoxBoxModelInfo {
	core::String id;
	core::String userId;
	core::String username;
	core::String name;
	core::String description;
	core::String voxFile;
	VoxBoxCategory category = VoxBoxCategory::Prop;
	VoxBoxLicense license = VoxBoxLicense::CC0;
	int64_t uploadTime = 0;
	int64_t size = 0;
	float rating = 0.0f;
	int totalRatings = 0;
	bool animated = false;
	bool isPublic = true;
};

struct VoxBoxSearchParams {
	core::String nameFilter;
	core::String authorFilter;
	VoxBoxCategory category = VoxBoxCategory::Max;
	VoxBoxLicense license = VoxBoxLicense::Max;
	bool filterAnimated = false;
	bool useAnimatedFilter = false;
	int page = 0;
	int count = 20;
};

struct VoxBoxState {
	core::DynamicArray<VoxBoxModelInfo> info;
	int count = 0;
};

// Root node property keys for VoxBox metadata
static constexpr const char *PropVoxBoxId = "voxbox.store.id";
static constexpr const char *PropVoxBoxCategory = "voxbox.store.category";
static constexpr const char *PropVoxBoxLicense = "voxbox.store.license";
static constexpr const char *PropVoxBoxAnimated = "voxbox.store.animated";
static constexpr const char *PropVoxBoxPublic = "voxbox.store.public";
static constexpr const char *PropVoxBoxUsername = "voxbox.store.username";
static constexpr const char *PropVoxBoxUserId = "voxbox.store.userid";

class VoxBoxApi {
private:
	core::String _refreshToken;
	core::String _accessToken;

public:
	static constexpr const char *BASE_URL = "https://voxbox.store";

	bool login(const core::String &username, const core::String &password);
	bool isLoggedIn() const;
	void logout();
	void setRefreshToken(const core::String &token);
	const core::String &refreshToken() const;
	/**
	 * @brief Extract the user ID (sub claim) from the refresh token JWT.
	 */
	core::String loggedInUserId() const;

	VoxBoxState search(const VoxBoxSearchParams &params = {}) const;

	/**
	 * @brief Download a model, convert from .vox to .vengi, embed metadata as root node properties.
	 * @return The path to the .vengi file (relative to home dir), or empty on failure.
	 */
	core::String download(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info) const;

	static core::String downloadDir();
	static core::String vengiPath(const VoxBoxModelInfo &info);
	static bool isDownloaded(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info);
	static bool removeDownload(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info);

	/**
	 * @brief Write VoxBox metadata into the root node of a scene graph.
	 */
	static void writeMetadata(scenegraph::SceneGraph &sceneGraph, const VoxBoxModelInfo &info);

	/**
	 * @brief Read VoxBox metadata from the root node of a scene graph.
	 */
	static VoxBoxModelInfo readMetadata(const scenegraph::SceneGraph &sceneGraph);

	/**
	 * @brief Export the given scene graph to a temp .vox file for upload.
	 * @return The full path to the temp .vox file, or empty on failure.
	 */
	static core::String exportToVox(const io::FilesystemPtr &filesystem, scenegraph::SceneGraph &sceneGraph);

	bool upload(const io::FilesystemPtr &filesystem, const core::String &voxFilePath, const core::String &coverPath,
				const VoxBoxModelInfo &info) const;
};

} // namespace voxelui
