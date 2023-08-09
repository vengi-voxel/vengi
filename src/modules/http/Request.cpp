/**
 * @file
 */

#include "Request.h"
#include "app/App.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/Var.h"
#include "engine-config.h"
#if USE_CURL
#include <curl/curl.h>
#endif

namespace http {

#if USE_CURL
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	return ((io::WriteStream *)userp)->write(contents, size * nmemb);
}
#endif

bool Request::supported() {
#if USE_CURL
	return true;
#else
	return false;
#endif
}

bool Request::request(const core::String &url, io::WriteStream &stream) {
#if USE_CURL
	CURL *curl = curl_easy_init();
	if (curl == nullptr) {
		return false;
	}

	const core::String userAgent = app::App::getInstance()->appname() + " " PROJECT_VERSION;
	Log::debug("Starting http request for %s", url.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, core::Var::get(cfg::HttpConnectTimeout, "5")->intVal());
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, core::Var::get(cfg::HttpTimeout, "5")->intVal());
	curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
	const CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return res == CURLE_OK;
#endif
	return false;
}

} // namespace http
