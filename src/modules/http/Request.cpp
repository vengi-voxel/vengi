/**
 * @file
 */

#include "Request.h"
#include "app/App.h"
#include "core/ArrayLength.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "engine-config.h"

#if __WINDOWS__
#define WIN32_LEAN_AND_MEAN (1)
#include <windows.h>
#include <winhttp.h>
#elif USE_CURL
#include <curl/curl.h>
#endif

namespace http {

#ifdef __WINDOWS__
static void printLastError(const char *ctx) {
	DWORD errnum = ::GetLastError();
	char buffer[512] = "";
	if (::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
						 GetModuleHandleA("winhttp.dll"), errnum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer,
						 sizeof(buffer), NULL) == 0) {
		Log::error("%s: %d - Unknown error", ctx, errnum);
	} else {
		Log::error("%s: %d - %s", ctx, errnum, buffer);
	}
}

static std::wstring s2ws(const std::string &str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

#elif USE_CURL
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	return ((io::WriteStream *)userp)->write(contents, size * nmemb);
}
#endif

bool Request::supported() {
#if __WINDOWS__
	return true;
#elif USE_CURL
	return true;
#else
	return false;
#endif
}

Request::Request(const core::String &url, RequestType type) : _type(type), _url(url) {
	_timeoutSecond = core::Var::get(cfg::HttpTimeout, "5")->intVal();
	_connectTimeoutSecond = core::Var::get(cfg::HttpConnectTimeout, "1")->intVal();
	_userAgent = app::App::getInstance()->fullAppname() + "/" PROJECT_VERSION;
}

bool Request::setBody(const core::String &body) {
	core_assert(_type == RequestType::POST);
	if (_type != RequestType::POST) {
		return false;
	}
	_body = body;
	return true;
}

void Request::addHeader(const core::String &key, const core::String &value) {
	_headers.put(key, value);
}

void Request::noCache() {
	addHeader("Cache-Control", "no-cache");
}

bool Request::execute(io::WriteStream &stream) {
	Log::debug("Starting http request for %s", _url.c_str());
#if __WINDOWS__
	// Initialize WinHTTP and create a session
	HINTERNET hSession =
		WinHttpOpen(nullptr, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (hSession == nullptr) {
		printLastError("Failed to create session for http download");
		return false;
	}

	// Set the connection timeout
	DWORD dwResolveTimeout = _timeoutSecond * 1000;
	DWORD dwConnectTimeout = _connectTimeoutSecond * 1000;
	DWORD dwSendTimeout = dwResolveTimeout;
	DWORD dwReceiveTimeout = dwResolveTimeout;
	if (!WinHttpSetTimeouts(hSession, dwResolveTimeout, dwConnectTimeout, dwSendTimeout, dwReceiveTimeout)) {
		printLastError("Failed to set http timeouts");
		WinHttpCloseHandle(hSession);
		return false;
	}

	std::wstring urlw = s2ws(_url.c_str());
	URL_COMPONENTS url_components = {};
	wchar_t scheme[32];
	wchar_t hostname[128];
	wchar_t url_path[4096];

	/* Convert the URL to its components. */
	url_components.dwStructSize = sizeof(url_components);
	url_components.lpszScheme = scheme;
	url_components.dwSchemeLength = lengthof(scheme);
	url_components.lpszHostName = hostname;
	url_components.dwHostNameLength = lengthof(hostname);
	url_components.lpszUrlPath = url_path;
	url_components.dwUrlPathLength = lengthof(url_path);
	url_components.nPort = INTERNET_DEFAULT_HTTP_PORT;
	if (!WinHttpCrackUrl(urlw.c_str(), 0, 0, &url_components)) {
		printLastError("Failed to parse url");
		WinHttpCloseHandle(hSession);
		return false;
	}

	/* Create the HTTP connection. */
	HINTERNET hConnection = WinHttpConnect(hSession, url_components.lpszHostName, url_components.nPort, 0);
	if (hConnection == nullptr) {
		printLastError("Failed to connect");
		WinHttpCloseHandle(hSession);
		return false;
	}

	const wchar_t *method = _type == RequestType::GET ? L"GET" : L"POST";
	HINTERNET hRequest = WinHttpOpenRequest(hConnection, method, url_components.lpszUrlPath, nullptr,
											WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
											url_components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
	if (hRequest == nullptr) {
		printLastError("Failed to create request");
		WinHttpCloseHandle(hSession);
		WinHttpCloseHandle(hConnection);
		return false;
	}

	// add request headers

	std::wstring reqHeaders;
	reqHeaders += L"User-Agent: ";
	reqHeaders += s2ws(_userAgent.c_str());
	reqHeaders += L"\r\n";
	for (const auto &entry : _headers) {
		reqHeaders += s2ws(entry->first.c_str());
		reqHeaders += L": ";
		reqHeaders += s2ws(entry->second.c_str());
		reqHeaders += L"\r\n";
	}

	SIZE_T len = reqHeaders.size();
	if (!WinHttpAddRequestHeaders(hRequest, reqHeaders.c_str(), DWORD(len),
								  WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) {
		Log::warn("Failed to add request headers to url: %s", _url.c_str());
	}

	// Send the request
	bool requestState = false;
	if (_type == RequestType::GET) {
		while (!requestState) {
			requestState =
				WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
			if (!requestState && GetLastError() == ERROR_WINHTTP_RESEND_REQUEST) {
				continue;
			}
			if (!requestState) {
				Log::error("Failed to send request with error %d", GetLastError());
				break;
			}
		}
	} else {
		std::wstring body = s2ws(_body.c_str());
		while (!requestState) {
			requestState = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)body.c_str(), body.size(),
											  body.size(), 0);
			if (!requestState && GetLastError() == ERROR_WINHTTP_RESEND_REQUEST) {
				continue;
			}
			if (!requestState) {
				Log::error("Failed to send request with error %d", GetLastError());
				break;
			}
		}
	}
	WinHttpReceiveResponse(hRequest, nullptr);
	WinHttpQueryDataAvailable(hRequest, nullptr);

	DWORD dwStatusCode = 0;
	DWORD dwSize = sizeof(dwStatusCode);
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
						&dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
	if (dwStatusCode != HTTP_STATUS_OK) {
		Log::warn("Failed to download url: %s with status code: %d", _url.c_str(), dwStatusCode);
	}

	// Read and save the response data
	DWORD bytesRead;
	BYTE buffer[4096];
	while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead)) {
		// Write the 'bytesRead' bytes from the buffer
		if (bytesRead == 0) {
			break;
		}
		stream.write(buffer, bytesRead);
	}
	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hSession);
	WinHttpCloseHandle(hConnection);
	return true;
#elif USE_CURL
	CURL *curl = curl_easy_init();
	if (curl == nullptr) {
		return false;
	}

	curl_slist *headers = nullptr;
	for (const auto &entry : _headers) {
		const core::String &headerLine = core::string::format("%s: %s", entry->first.c_str(), entry->second.c_str());
		headers = curl_slist_append(headers, headerLine.c_str());
	}
	if (headers) {
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	}
	if (!_body.empty()) {
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, _body.c_str());
	}
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, _type == RequestType::GET ? "GET" : "POST");
	curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _connectTimeoutSecond);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, _timeoutSecond);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, _userAgent.c_str());
	const CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		char *url = nullptr;
		curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
		Log::error("Http request for '%s' failed with error %s", url, curl_easy_strerror(res));
	}
	long statusCode = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
	Log::debug("Got status code %i for %s", (int)statusCode, _url.c_str());
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return res == CURLE_OK;
#endif
	return false;
}

} // namespace http
