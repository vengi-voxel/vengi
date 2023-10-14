/**
 * @file
 */

#include "VersionCheck.h"
#include "app/App.h"
#include "core/Log.h"
#include "http/Request.h"
#include "io/BufferedReadWriteStream.h"
#include "engine-config.h"
#include <SDL_stdinc.h>
#include <json.hpp>

namespace util {

static const core::String GitHubURL = "https://api.github.com/repos/mgerhardy/vengi";

bool isNewerVersion(const core::String &versionLatest, const core::String &vengiVersion) {
	struct Version {
		int major = 0;
		int minor = 0;
		int micro = 0;
		int patch = 0;
	};
	Version latest;
	if (SDL_sscanf(versionLatest.c_str(), "%d.%d.%d.%d", &latest.major, &latest.minor, &latest.micro, &latest.patch) == 0) {
		Log::debug("Failed to parse latest version %s", versionLatest.c_str());
		return false;
	}
	Version current;
	if (SDL_sscanf(vengiVersion.c_str(), "%d.%d.%d.%d", &current.major, &current.minor, &current.micro, &current.patch) == 0) {
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

bool isNewVersionAvailable() {
	if (!http::Request::supported()) {
		Log::error("Could not check for new version: HTTP requests are not supported");
		return false;
	}
	io::BufferedReadWriteStream stream;
	http::Request request;
	if (!request.request(GitHubURL + "/releases/latest", stream)) {
		Log::error("Could not check for new version: HTTP request failed");
		return false;
	}
	stream.seek(0);
	core::String response;
	if (!stream.readString(stream.size(), response)) {
		Log::error("Failed to read string");
		return false;
	}
	nlohmann::json release = nlohmann::json::parse(response);
	if (!release.contains("tag_name")) {
		Log::warn("github response doesn't contain a tag_name node");
		return false;
	}
	core::String latestVersion = release["tag_name"].get<std::string>().c_str();
	// our tags usually have a v in front of it
	if (latestVersion[0] == 'v') {
		latestVersion = latestVersion.substr(1);
	}
	return isNewerVersion(latestVersion, PROJECT_VERSION);
}

} // namespace util
