/**
 * @file
 */

#include "VersionCheck.h"
#include "core/Log.h"
#include "engine-config.h"
#include "http/Client.h"
#include "http/Http.h"
#include "http/Request.h"
#include "json/JSON.h"

namespace util {

static const core::String GitHubURL = "https://api.github.com/repos/vengi-voxel/vengi";

bool isNewerVersion(const core::String &versionLatest, const core::String &vengiVersion) {
	struct VersionData {
		int major = 0;
		int minor = 0;
		int micro = 0;
		int patch = 0;
	};
	VersionData latest;
	if (sscanf(versionLatest.c_str(), "%d.%d.%d.%d", &latest.major, &latest.minor, &latest.micro, &latest.patch) == 0) {
		Log::debug("Failed to parse latest version %s", versionLatest.c_str());
		return false;
	}
	VersionData current;
	if (sscanf(vengiVersion.c_str(), "%d.%d.%d.%d", &current.major, &current.minor, &current.micro, &current.patch) == 0) {
		Log::debug("Failed to parse vengi version %s", vengiVersion.c_str());
		return false;
	}

	// check whether latest version is newer than current version
	if (latest.major > current.major) {
		return true;
	}
	if (latest.major == current.major) {
		if (latest.minor > current.minor) {
			return true;
		}
		if (latest.minor == current.minor) {
			if (latest.micro > current.micro) {
				return true;
			}
			if (latest.micro == current.micro) {
				if (latest.patch > current.patch) {
					return true;
				}
			}
		}
	}

	return false;
}

core::String releaseUrl() {
	return GitHubURL + "/releases/latest";
}

void checkLatestVersion(http::Client &client, core::Function<void(bool)> callback, int timeout) {
	http::Request request(releaseUrl(), http::RequestType::GET);
	if (timeout > 0) {
		request.setTimeoutSecond(timeout);
		request.setConnectTimeoutSecond(timeout);
	}
	if (client.execute(request, [callback](const http::Response &response) {
			callback(response.success && http::isValidStatusCode(response.statusCode) &&
					 isNewVersionAvailable(response.text()));
		}) == 0) {
		callback(false);
	}
}

bool isNewVersionAvailable(const core::String &response) {
	json::Json release = json::Json::parse(response);
	if (!release.contains("tag_name")) {
		Log::warn("github response doesn't contain a tag_name node");
		return false;
	}
	core::String latestVersion = release.get("tag_name").str();
	// our tags usually have a v in front of it
	if (!latestVersion.empty() && latestVersion[0] == 'v') {
		latestVersion = latestVersion.substr(1);
	}
	return isNewerVersion(latestVersion, PROJECT_VERSION);
}

} // namespace util
