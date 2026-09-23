/**
 * @file
 */

#pragma once

#include "core/Function.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"

namespace scenegraph {
class SceneGraph;
}

namespace voxedit {

enum class VoxBoxCategory { Character, Environment, Prop, Vegetation, Vehicle, Random, Weapon, Architecture, Max };

enum class VoxBoxLicense { CC0, CCBY, CCBYSA, CCBYND, CCBYNC, CCBYNCSA, CCBYNCND, Max };

const char *VoxBoxCategoryStr(VoxBoxCategory c);
VoxBoxCategory VoxBoxCategoryFromStr(const core::String &s);
const char *VoxBoxLicenseStr(VoxBoxLicense l);
VoxBoxLicense VoxBoxLicenseFromStr(const core::String &s);

struct VoxBoxModelInfo {
	core::String id, userId, username, name, description, voxFile;
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
	core::String nameFilter, authorFilter;
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

using VoxBoxBoolCallback = core::Function<void(bool)>;
using VoxBoxStateCallback = core::Function<void(VoxBoxState)>;
using VoxBoxStringCallback = core::Function<void(const core::String &)>;

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
	core::String _userId;
	core::String _username;

	bool applyLoginResponse(const core::String &responseBody);
	VoxBoxState parseSearchResponse(const core::String &responseBody) const;
	core::String convertDownloadedVox(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info,
									  const core::String &voxBody) const;

public:
	static constexpr const char *BASE_URL = "https://voxbox.store";

	void loginAsync(const core::String &username, const core::String &password, VoxBoxBoolCallback &&callback);
	bool isLoggedIn() const;
	void logout();
	void setRefreshToken(const core::String &token);
	const core::String &refreshToken() const;
	const core::String &loggedInUserId() const;
	const core::String &loggedInUsername() const;

	void searchAsync(const VoxBoxSearchParams &params, VoxBoxStateCallback &&callback) const;
	void downloadAsync(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info,
					   VoxBoxStringCallback &&callback) const;

	static core::String downloadDir();
	static core::String vengiPath(const VoxBoxModelInfo &info);
	static bool isDownloaded(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info);
	static bool removeDownload(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info);

	static void writeMetadata(scenegraph::SceneGraph &sceneGraph, const VoxBoxModelInfo &info);
	static VoxBoxModelInfo readMetadata(const scenegraph::SceneGraph &sceneGraph);
	static core::String exportToVox(const io::FilesystemPtr &filesystem, scenegraph::SceneGraph &sceneGraph);

	void uploadAsync(const io::FilesystemPtr &filesystem, const core::String &voxFilePath, const core::String &coverPath,
					 const VoxBoxModelInfo &info, VoxBoxBoolCallback &&callback) const;
};

} // namespace voxedit
