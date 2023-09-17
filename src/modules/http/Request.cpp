/**
 * @file
 */

#include "Request.h"
#include "app/App.h"
#include "core/ArrayLength.h"
#include "core/GameConfig.h"
#include "core/Log.h"
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

bool Request::request(const core::String &url, io::WriteStream &stream) {
#if __WINDOWS__
	// Initialize WinHTTP and create a session
	HINTERNET hSession =
		WinHttpOpen(nullptr, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (hSession == nullptr) {
		printLastError("Failed to create session for http download");
		return false;
	}

	// Set the connection timeout
	const DWORD httpTimeoutMillis = core::Var::get(cfg::HttpTimeout, "5")->intVal() * 1000;
	DWORD dwResolveTimeout = httpTimeoutMillis;
	DWORD dwConnectTimeout = core::Var::get(cfg::HttpConnectTimeout, "5")->intVal() * 1000;
	DWORD dwSendTimeout = httpTimeoutMillis;
	DWORD dwReceiveTimeout = httpTimeoutMillis;
	if (!WinHttpSetTimeouts(hSession, dwResolveTimeout, dwConnectTimeout, dwSendTimeout, dwReceiveTimeout)) {
		printLastError("Failed to set http timeouts");
		WinHttpCloseHandle(hSession);
		return false;
	}

	std::wstring urlw = s2ws(url.c_str());
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
	WinHttpCrackUrl(urlw.c_str(), 0, 0, &url_components);

	/* Create the HTTP connection. */
	HINTERNET hConnection = WinHttpConnect(hSession, url_components.lpszHostName, url_components.nPort, 0);
	if (hConnection == nullptr) {
		printLastError("Failed to connect");
		WinHttpCloseHandle(hSession);
		return false;
	}

	HINTERNET hRequest = WinHttpOpenRequest(hConnection, L"GET", url_components.lpszUrlPath, nullptr,
											WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
											url_components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
	if (hRequest == nullptr) {
		printLastError("Failed to create request");
		WinHttpCloseHandle(hSession);
		WinHttpCloseHandle(hConnection);
		return false;
	}

	// add request headers
	const wchar_t *reqHeaders = L"User-Agent: Mozilla/5.0\r\n";
	if (!WinHttpAddRequestHeaders(hRequest, reqHeaders, -1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) {
		Log::warn("Failed to add request headers to url: %s", url.c_str());
	}

	// Send the request
	WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	WinHttpReceiveResponse(hRequest, nullptr);
	WinHttpQueryDataAvailable(hRequest, nullptr);

	DWORD dwStatusCode = 0;
	DWORD dwSize = sizeof(dwStatusCode);
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
						&dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
	if (dwStatusCode != HTTP_STATUS_OK) {
		Log::warn("Failed to download url: %s with status code: %d", url.c_str(), dwStatusCode);
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
