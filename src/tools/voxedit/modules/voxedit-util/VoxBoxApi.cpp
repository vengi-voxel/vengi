/**
 * @file
 */

#include "VoxBoxApi.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "http/Http.h"
#include "http/Request.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FilesystemArchive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxelformat/VolumeFormat.h"
#include "json/JSON.h"

namespace voxedit {

static const char *CategoryStrings[] = {"Character", "Environment", "Prop",	  "Vegetation",
										"Vehicle",	 "Random",		"Weapon", "Architecture"};
static_assert(lengthof(CategoryStrings) == (int)VoxBoxCategory::Max, "CategoryStrings size mismatch");

static const char *LicenseStrings[] = {"CC-0",	   "CC-BY",		  "CC-BY-SA",	"CC-BY-ND",
									   "CC-BY-NC", "CC-BY-NC-SA", "CC-BY-NC-ND"};
static_assert(lengthof(LicenseStrings) == (int)VoxBoxLicense::Max, "LicenseStrings size mismatch");

const char *VoxBoxCategoryStr(VoxBoxCategory c) {
	if (c >= VoxBoxCategory::Max) {
		return "";
	}
	return CategoryStrings[(int)c];
}

VoxBoxCategory VoxBoxCategoryFromStr(const core::String &s) {
	for (int i = 0; i < (int)VoxBoxCategory::Max; ++i) {
		if (s == CategoryStrings[i]) {
			return (VoxBoxCategory)i;
		}
	}
	return VoxBoxCategory::Prop;
}

const char *VoxBoxLicenseStr(VoxBoxLicense l) {
	if (l >= VoxBoxLicense::Max) {
		return "";
	}
	return LicenseStrings[(int)l];
}

VoxBoxLicense VoxBoxLicenseFromStr(const core::String &s) {
	for (int i = 0; i < (int)VoxBoxLicense::Max; ++i) {
		if (s == LicenseStrings[i]) {
			return (VoxBoxLicense)i;
		}
	}
	return VoxBoxLicense::CC0;
}

void VoxBoxApi::login(const core::String &username, const core::String &password, core::Function<void(bool)> callback) {
	_client.cancel(_loginRequest);
	const core::String url = _baseUrl + "/api/auth/sign-in";

	json::Json body = json::Json::object();
	body.set("username", username.c_str());
	body.set("password", password.c_str());
	const core::String bodyStr = body.dump();

	http::Request request(url, http::RequestType::POST);
	request.setFollowRedirects(false);
	request.setUserAgent(app::App::getInstance()->fullAppname());
	request.addHeader("Content-Type", "application/json");
	request.setBody(bodyStr);

	_loginRequest = _client.execute(request, [this, callback](const http::Response &response) {
		callback(response.success && response.statusCode == 200 && applyLogin(response.text()));
	});
	if (!_loginRequest) {
		callback(false);
	}
}

bool VoxBoxApi::applyLogin(const core::String &response) {
	json::Json json = json::Json::parse(response);
	if (!json.isValid()) {
		Log::error("VoxBox login: invalid JSON response");
		return false;
	}

	_accessToken = json.strVal("accessToken", "");
	setRefreshToken(json.strVal("refreshToken", ""));

	if (_refreshToken.empty()) {
		Log::error("VoxBox login: no refresh token received");
		return false;
	}

	Log::info("VoxBox login successful");
	return true;
}

bool VoxBoxApi::isLoggedIn() const {
	return !_refreshToken.empty();
}

void VoxBoxApi::logout() {
	_client.cancelAll();
	_refreshToken = "";
	_accessToken = "";
}

void VoxBoxApi::setRefreshToken(const core::String &token) {
	_client.cancel(_loginRequest);
	_refreshToken = token;
	if (token.empty()) {
		_userId = "";
		_username = "";
		return;
	}
	json::Json payload = json::decodeJWTPayload(_refreshToken);
	if (payload.isValid()) {
		_userId = payload.strVal("sub", "");
		_username = payload.strVal("username", "");
	} else {
		_userId = "";
		_username = "";
	}
}

const core::String &VoxBoxApi::refreshToken() const {
	return _refreshToken;
}

const core::String &VoxBoxApi::loggedInUserId() const {
	return _userId;
}

const core::String &VoxBoxApi::loggedInUsername() const {
	return _username;
}

void VoxBoxApi::search(const VoxBoxSearchParams &params, core::Function<void(VoxBoxState)> callback) {
	_client.cancel(_searchRequest);
	core::String url =
		core::String::format("%s/api/model/search?page=%d&count=%d", _baseUrl.c_str(), params.page, params.count);

	if (!params.nameFilter.empty()) {
		url += "&name=" + params.nameFilter;
	}
	if (!params.authorFilter.empty()) {
		url += "&username=" + params.authorFilter;
	}
	if (params.category != VoxBoxCategory::Max) {
		url += core::String::format("&categories[0]=%s", VoxBoxCategoryStr(params.category));
	}
	if (params.license != VoxBoxLicense::Max) {
		url += core::String::format("&licenses[0]=%s", VoxBoxLicenseStr(params.license));
	}
	if (params.useAnimatedFilter) {
		url += params.filterAnimated ? "&animated=true" : "&animated=false";
	}

	http::Request request(url, http::RequestType::GET);
	request.setFollowRedirects(false);
	request.setUserAgent(app::App::getInstance()->fullAppname());
	if (isLoggedIn()) {
		request.addHeader("Cookie", "jwt=" + _refreshToken);
	}

	_searchRequest = _client.execute(request, [callback](const http::Response &response) {
		callback(response.success && response.statusCode == 200 ? parseSearch(response.text()) : VoxBoxState());
	});
	if (!_searchRequest) {
		callback({});
	}
}

VoxBoxState VoxBoxApi::parseSearch(const core::String &response) {
	VoxBoxState state;
	json::Json json = json::Json::parse(response);
	if (!json.isValid()) {
		Log::error("VoxBox search: invalid JSON response");
		return state;
	}

	json::Json data = json.get("data");
	if (!data.isValid() || !data.isArray()) {
		Log::error("VoxBox search: missing data array");
		return state;
	}

	state.count = json.intVal("count", 0);

	for (const auto &entry : data) {
		VoxBoxModelInfo info;
		info.id = entry.strVal("id", "");
		info.userId = entry.strVal("userId", "");
		info.username = entry.strVal("username", "");
		info.name = entry.strVal("name", "");
		info.description = entry.strVal("description", "");
		info.category = VoxBoxCategoryFromStr(entry.strVal("category", ""));
		info.license = VoxBoxLicenseFromStr(entry.strVal("license", ""));
		info.voxFile = entry.strVal("voxFile", "");
		info.uploadTime = (int64_t)entry.intVal("uploadTime", 0);
		info.size = (int64_t)entry.intVal("size", 0);
		info.rating = (float)entry.intVal("rating", 0);
		info.totalRatings = entry.intVal("totalRatings", 0);
		info.animated = entry.boolVal("animated", false);
		info.isPublic = entry.boolVal("public", true);
		if (!info.id.empty()) {
			state.info.push_back(info);
		}
	}

	Log::info("VoxBox search returned %d models", (int)state.info.size());
	return state;
}

// --- Metadata helpers ---

void VoxBoxApi::writeMetadata(scenegraph::SceneGraph &sceneGraph, const VoxBoxModelInfo &info) {
	scenegraph::SceneGraphNode &root = sceneGraph.node(0);
	root.setProperty(scenegraph::PropTitle, info.name);
	root.setProperty(scenegraph::PropDescription, info.description);
	root.setProperty(scenegraph::PropAuthor, info.username);
	root.setProperty(PropVoxBoxId, info.id);
	root.setProperty(PropVoxBoxUserId, info.userId);
	root.setProperty(PropVoxBoxCategory, VoxBoxCategoryStr(info.category));
	root.setProperty(PropVoxBoxLicense, VoxBoxLicenseStr(info.license));
	root.setProperty(PropVoxBoxAnimated, info.animated ? "true" : "false");
	root.setProperty(PropVoxBoxPublic, info.isPublic ? "true" : "false");
	root.setProperty(PropVoxBoxUsername, info.username);
}

VoxBoxModelInfo VoxBoxApi::readMetadata(const scenegraph::SceneGraph &sceneGraph) {
	VoxBoxModelInfo info;
	const scenegraph::SceneGraphNode &root = sceneGraph.root();
	info.id = root.property(PropVoxBoxId);
	info.userId = root.property(PropVoxBoxUserId);
	info.name = root.property(scenegraph::PropTitle);
	info.description = root.property(scenegraph::PropDescription);
	info.username = root.property(PropVoxBoxUsername);
	info.category = VoxBoxCategoryFromStr(root.property(PropVoxBoxCategory));
	info.license = VoxBoxLicenseFromStr(root.property(PropVoxBoxLicense));
	info.animated = root.property(PropVoxBoxAnimated) == "true";
	info.isPublic = root.property(PropVoxBoxPublic) != "false";
	return info;
}

// --- Download / conversion ---

core::String VoxBoxApi::downloadDir() {
	return "voxbox";
}

core::String VoxBoxApi::vengiPath(const VoxBoxModelInfo &info) {
	return core::string::path(downloadDir(), "voxbox.store." + info.id + ".vengi");
}

bool VoxBoxApi::isDownloaded(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info) {
	if (info.id.empty()) {
		return false;
	}
	return io::Filesystem::sysExists(filesystem->homeWritePath(vengiPath(info)));
}

bool VoxBoxApi::removeDownload(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info) {
	return io::Filesystem::sysRemoveFile(filesystem->homeWritePath(vengiPath(info)));
}

void VoxBoxApi::download(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info,
						 core::Function<void(const core::String &)> callback) const {
	if (info.voxFile.empty()) {
		callback("");
		return;
	}
	http::Request request(core::String::format("%s/api/model/blob?id=%s", _baseUrl.c_str(), info.id.c_str()),
						  http::RequestType::GET);
	request.setFollowRedirects(false);
	request.setTimeoutSecond(60);
	if (_client.execute(request, [filesystem, info, callback](const http::Response &response) {
			callback(response.success && http::isValidStatusCode(response.statusCode)
						 ? convertDownload(filesystem, info, response.body)
						 : core::String());
		}) == 0) {
		callback("");
	}
}

core::String VoxBoxApi::convertDownload(const io::FilesystemPtr &filesystem, const VoxBoxModelInfo &info,
										const core::Buffer<uint8_t> &body) {
	// write temp .vox
	const core::String dir = downloadDir();
	filesystem->sysCreateDir(filesystem->homeWritePath(dir));
	const core::String tempVoxPath = core::string::path(dir, "temp_" + info.id + ".vox");
	if (!filesystem->homeWrite(tempVoxPath, body.data(), body.size())) {
		Log::error("Failed to write temp vox file");
		return "";
	}

	// convert .vox -> scene graph
	const io::ArchivePtr &archive = io::openFilesystemArchive(filesystem);
	scenegraph::SceneGraph sceneGraph;
	voxelformat::LoadContext loadCtx;
	io::FileDescription fd;
	fd.set(filesystem->homeWritePath(tempVoxPath));
	if (!voxelformat::loadFormat(fd, archive, sceneGraph, loadCtx)) {
		Log::error("Failed to load downloaded vox file");
		io::Filesystem::sysRemoveFile(filesystem->homeWritePath(tempVoxPath));
		return "";
	}

	// embed metadata
	writeMetadata(sceneGraph, info);

	// save as .vengi
	core::String vengi = vengiPath(info);
	voxelformat::SaveContext saveCtx;
	if (!voxelformat::saveFormat(sceneGraph, filesystem->homeWritePath(vengi), nullptr, archive, saveCtx)) {
		Log::error("Failed to save vengi file");
		io::Filesystem::sysRemoveFile(filesystem->homeWritePath(tempVoxPath));
		return "";
	}

	// clean up temp .vox
	io::Filesystem::sysRemoveFile(filesystem->homeWritePath(tempVoxPath));

	Log::info("Downloaded and converted %s to %s", info.name.c_str(), vengi.c_str());
	return vengi;
}

// --- Export for upload ---

core::String VoxBoxApi::exportToVox(const io::FilesystemPtr &filesystem, scenegraph::SceneGraph &sceneGraph) {
	const core::String dir = downloadDir();
	filesystem->sysCreateDir(filesystem->homeWritePath(dir));
	const core::String tempVox = core::string::path(dir, "upload_temp.vox");
	core::String fullPath = filesystem->homeWritePath(tempVox);

	const io::ArchivePtr &archive = io::openFilesystemArchive(filesystem);
	voxelformat::SaveContext saveCtx;
	if (!voxelformat::saveFormat(sceneGraph, fullPath, nullptr, archive, saveCtx)) {
		Log::error("Failed to export scene to .vox");
		return "";
	}
	return fullPath;
}

// --- Upload ---

void VoxBoxApi::upload(const io::FilesystemPtr &filesystem, const core::String &voxFilePath,
					   const core::String &coverPath, const VoxBoxModelInfo &info,
					   core::Function<void(bool)> callback) const {
	if (!isLoggedIn()) {
		Log::error("VoxBox upload: not logged in");
		callback(false);
		return;
	}

	io::File voxFile(voxFilePath, io::FileMode::SysRead);
	if (!voxFile.validHandle()) {
		Log::error("VoxBox upload: cannot open vox file %s", voxFilePath.c_str());
		callback(false);
		return;
	}
	void *voxBuf = nullptr;
	const int voxSize = voxFile.read(&voxBuf);
	if (voxSize <= 0 || voxBuf == nullptr) {
		Log::error("VoxBox upload: cannot read vox file %s", voxFilePath.c_str());
		callback(false);
		return;
	}

	io::File coverFile(coverPath, io::FileMode::SysRead);
	if (!coverFile.validHandle()) {
		Log::error("VoxBox upload: cannot open cover file %s", coverPath.c_str());
		delete[] (uint8_t *)voxBuf;
		callback(false);
		return;
	}
	void *coverBuf = nullptr;
	const int coverSize = coverFile.read(&coverBuf);
	if (coverSize <= 0 || coverBuf == nullptr) {
		Log::error("VoxBox upload: cannot read cover file %s", coverPath.c_str());
		delete[] (uint8_t *)voxBuf;
		callback(false);
		return;
	}

	const core::String boundary = "----VengiVoxBoxUpload";
	io::BufferedReadWriteStream bodyStream;

	auto addField = [&bodyStream, &boundary](const char *fieldName, const core::String &value) {
		const core::String part =
			core::String::format("--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s\r\n", boundary.c_str(),
								 fieldName, value.c_str());
		bodyStream.write(part.c_str(), part.size());
	};

	addField("name", info.name);
	addField("description", info.description);
	addField("category", VoxBoxCategoryStr(info.category));
	addField("animated", info.animated ? "true" : "false");
	if (!info.id.empty()) {
		addField("id", info.id);
	}

	const core::String coverFilename = core::string::extractFilenameWithExtension(coverPath);
	core::String coverHeader = core::String::format(
		"--%s\r\nContent-Disposition: form-data; name=\"cover\"; filename=\"%s\"\r\nContent-Type: image/png\r\n\r\n",
		boundary.c_str(), coverFilename.c_str());
	bodyStream.write(coverHeader.c_str(), coverHeader.size());
	bodyStream.write(coverBuf, coverSize);
	bodyStream.write("\r\n", 2);

	const core::String voxFilename = core::string::extractFilenameWithExtension(voxFilePath);
	core::String voxHeader = core::String::format("--%s\r\nContent-Disposition: form-data; name=\"vox\"; "
												  "filename=\"%s\"\r\nContent-Type: application/octet-stream\r\n\r\n",
												  boundary.c_str(), voxFilename.c_str());
	bodyStream.write(voxHeader.c_str(), voxHeader.size());
	bodyStream.write(voxBuf, voxSize);
	bodyStream.write("\r\n", 2);

	addField("license", VoxBoxLicenseStr(info.license));
	addField("public", info.isPublic ? "true" : "false");

	const core::String closing = core::String::format("--%s--\r\n", boundary.c_str());
	bodyStream.write(closing.c_str(), closing.size());

	bodyStream.seek(0);
	core::String bodyStr;
	bodyStr.append((const char *)bodyStream.getBuffer(), (int)bodyStream.size());

	delete[] (uint8_t *)voxBuf;
	delete[] (uint8_t *)coverBuf;

	// use upload for new, update for existing
	const bool isUpdate = !info.id.empty();
	const core::String apiPath = isUpdate ? "/api/model/update" : "/api/model/upload";
	const core::String uploadUrl = _baseUrl + apiPath;
	http::Request uploadReq(uploadUrl, isUpdate ? http::RequestType::PATCH : http::RequestType::POST);
	uploadReq.setFollowRedirects(false);
	uploadReq.setUserAgent(app::App::getInstance()->fullAppname());
	uploadReq.addHeader("Cookie", "jwt=" + _refreshToken);
	uploadReq.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
	uploadReq.setBody(bodyStr);
	uploadReq.setTimeoutSecond(120);

	if (_client.execute(uploadReq, [callback](const http::Response &response) {
			callback(response.success && http::isValidStatusCode(response.statusCode));
		}) == 0) {
		callback(false);
	}
}

} // namespace voxedit
