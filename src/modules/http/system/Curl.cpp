/**
 * @file
 */

#include "Curl.h"
#include "core/Log.h"
#include "http/RequestContext.h"
#include "io/Stream.h"

#include <curl/curl.h>

namespace http {

struct HeaderData {
	Headers *outheaders;
	io::WriteStream *stream;
};

static size_t WriteHeaderData(char *b, size_t size, size_t nitems, void *userdata) {
	HeaderData *headers = (HeaderData *)userdata;
	core::String str;

	size_t total = size * nitems;
	if (total)
		str.append(b, total);

	if (str.last() == '\n')
		str.erase(str.size() - 1);
	if (str.last() == '\r')
		str.erase(str.size() - 1);

	size_t pos = str.find_first_of(':');
	if (pos != core::String::npos) {
		const core::String &key = str.substr(0, pos);
		const core::String &value = str.substr(pos + 2);
		Log::debug("Header: %s: %s", key.c_str(), value.c_str());
		if (headers->outheaders) {
			headers->outheaders->put(key, value);
		}
		if (key == "Content-Length") {
			headers->stream->reserve(value.toInt());
		}
	}
	return total;
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	return ((io::WriteStream *)userp)->write(contents, size * nmemb);
}

bool http_request(io::WriteStream &stream, int *statusCode, Headers *outheaders,
				  RequestContext &ctx) {
	CURL *curl = curl_easy_init();
	if (curl == nullptr) {
		return false;
	}

	curl_slist *headers = nullptr;
	for (const auto &entry : ctx._headers) {
		const core::String &headerLine = core::String::format("%s: %s", entry->first.c_str(), entry->second.c_str());
		headers = curl_slist_append(headers, headerLine.c_str());
	}
	if (headers) {
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	}
	if (!ctx._body.empty()) {
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ctx._body.c_str());
	}
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, ctx._type == RequestType::GET ? "GET" : "POST");
	curl_easy_setopt(curl, CURLOPT_URL, ctx._url.c_str());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
	HeaderData headerdata = {outheaders, &stream};
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteHeaderData);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerdata);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, ctx._connectTimeoutSecond);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, ctx._timeoutSecond);
	// curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3);
	if (!ctx._userAgent.empty()) {
		curl_easy_setopt(curl, CURLOPT_USERAGENT, ctx._userAgent.c_str());
	}
	const CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		char *url = nullptr;
		curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
		Log::error("Http request for '%s' failed with error %s", url, curl_easy_strerror(res));
	}
	long statusCodeCurl = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCodeCurl);
	Log::debug("Got status code %i for %s", (int)statusCodeCurl, ctx._url.c_str());
	if (statusCode) {
		*statusCode = (int)statusCodeCurl;
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return res == CURLE_OK;
}

} // namespace http
